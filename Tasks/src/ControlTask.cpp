
/*
 * @Author: Asephyr 1558825085@qq.com
 * @Date: 2026-10-06 22:25:55
 * @LastEditors: Asephyr 1558825085@qq.com
 * @LastEditTime: 2026-10-07 20:37:34
 * @FilePath: \examination1_26.10.5\Tasks\inc\ControlTask.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "ControlTask.h"
#include "Gm6020.h"
#include "Pid.h"
#include "can.h"
#include "tim.h"
#include "iwdg.h"
#include <cmath>


namespace {
    constexpr float kPi = 3.14159265358979323846f;
    constexpr float kTwoPi = 2.0f * kPi;
    constexpr float kDt = 0.001f; // 1kHz 控制周期

    constexpr float kVelAmp = kTwoPi;
    constexpr float kVelFreq = 0.25f; // 0.25Hz 正弦速度参考
    constexpr float kPosAmp = kPi / 2.0f;
    constexpr float kPosFreq = 0.2f; // 0.2Hz 正弦位置参考

    constexpr float kVoltMax = 25000.0f; // 最大电压值, 单位 mV
    constexpr float kVelRefMax = 20.0f; // 最大速度参考值, 单位 rad/s
    

    Gm6020 g_motor(1);

    Pid g_velPid(1000.0f, 200.0f, 0.0f, kVoltMax, -kVoltMax);
    Pid g_posPid(20.0f, 0.0f, 0.0f, kVelRefMax, -kVelRefMax);

    enum class Mode {Velocity, Position};
    Mode g_mode = Mode::Velocity;
    uint32_t g_tick = 0;
   
    float g_posCenter = 0.0f;
    float g_posPhase = 0.0f;

    void sendControl(){
        uint8_t data[8] = {0};
        g_motor.encode(data);

        CAN_TxHeaderTypeDef tx = {0};
        tx.StdId = g_motor.txId();
        tx.IDE = CAN_ID_STD;
        tx.RTR = CAN_RTR_DATA;
        tx.DLC = 8;

        uint32_t mailbox;
        if(HAL_CAN_GetTxMailboxesFreeLevel(&hcan1)>0){
            HAL_CAN_AddTxMessage(&hcan1, &tx, data, &mailbox);
        }
    }

}    

extern "C" void ControlTaskInit(void) 
{
    
    CAN_FilterTypeDef filter = {0};
    filter.FilterBank = 0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh = 0x0000;
    filter.FilterIdLow = 0x0000;
    filter.FilterMaskIdHigh = 0x0000;
    filter.FilterMaskIdLow = 0x0000;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation = ENABLE;
    filter.SlaveStartFilterBank = 14;
    HAL_CAN_ConfigFilter(&hcan1, &filter);

    HAL_CAN_Start(&hcan1);
    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);

    HAL_TIM_Base_Start_IT(&htim6);

}

extern "C" void CanFeedbackCallback(uint32_t std_id, const uint8_t *data){

    if (std_id == g_motor.rxId()){
        g_motor.decode(data);
    }
}

extern "C" void MainTask(void){
    ++g_tick;


    float voltage;

    if (g_mode == Mode::Velocity){

        float t = g_tick * kDt;
        float refVel = kVelAmp * std::sin(kTwoPi * kVelFreq * t);// 正弦速度参考
        voltage = g_velPid.calc(refVel, g_motor.vel());// 速度环 PID 控制

    }else{

        g_posPhase += kTwoPi * kPosFreq * kDt;// 位置环正弦参考相位累加
        if(g_posPhase > kTwoPi) g_posPhase -= kTwoPi;

        float refPos = g_posCenter + kPosAmp * std::sin(g_posPhase);// 正弦位置参考
        float refVel = g_posPid.calc(refPos, g_motor.angle());// 位置环 PID 控制, 输出速度参考
        voltage = g_velPid.calc(refVel, g_motor.vel());
    }

    g_motor.setVoltage(static_cast<int16_t>(voltage));
    sendControl();

}

extern "C" void ControlTaskSetMode(uint8_t mode){
    Mode m;
    if(mode==0){
        m = Mode::Velocity;
    }else{
        m = Mode::Position;
    }
    if(m == g_mode) return;

    g_mode = m;

    if(g_mode == Mode::Position){
        g_posCenter = g_motor.angle();
        g_posPhase = 0.0f;
        g_posPid.reset();
    }
    g_velPid.reset();
}

extern "C" void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan){

    CAN_RxHeaderTypeDef rx;//存报文头（ID，长度，帧类型等）
    uint8_t  data[8];

    if(HAL_CAN_GetRxMessage(hcan,CAN_RX_FIFO0,&rx,data)!=HAL_OK){
        return;
    }
    if(rx.IDE != CAN_ID_STD){
        return;
    }
    CanFeedbackCallback(rx.StdId, data);
}

extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM6) {
        MainTask();
    }
}