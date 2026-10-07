<!--
 * @Author: Asephyr 1558825085@qq.com
 * @Date: 2026-10-07 21:02:17
 * @LastEditors: Asephyr 1558825085@qq.com
 * @LastEditTime: 2026-10-07 21:03:48
 * @FilePath: \examination1_26.10.5\README.md
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
-->
# GM6020 正弦速度/位置跟随

基于 STM32F407IGH6 的 RoboMaster 考核工程：用 GM6020 电机跟随自定义正弦曲线，并启用独立看门狗保护。

## 功能

- **速度环**：跟随正弦速度曲线 `A·sin(2πft)`（默认幅值 2π rad/s ≈ 60 rpm，频率 0.25 Hz）
- **位置环**：跟随正弦位置曲线（默认幅值 π/2 rad，频率 0.2 Hz），与速度环级联
- **模式切换**：`ControlTaskSetMode(0)` 速度模式，`ControlTaskSetMode(1)` 位置模式//main.c第107行
- **看门狗保护**：IWDG 超时约 0.8s，主循环定期喂狗

## 目录结构

```
.
├── CMakeLists.txt          # 战队 CMake 模板
├── CMakePresets.json       # CMake 预设
├── C_project.ioc           # STM32CubeMX 工程配置
├── Core/                   # CubeMX 生成：外设初始化、main、中断
├── Drivers/                # CMSIS + HAL 驱动
├── Tasks/                  # 用户代码
│   ├── inc/                # 头文件 (ControlTask.h, Gm6020.h, Pid.h)
│   └── src/                # 源文件 (ControlTask.cpp, Gm6020.cpp, Pid.cpp)
├── startup_stm32f407xx.s   # 启动文件
└── STM32F407xx_FLASH.ld    # 链接脚本
```

## 外设配置

| 外设 | 配置 | 用途 |
|---|---|---|
| CAN1 | PD0/PD1, 1Mbps | GM6020 电机通信 |
| TIM6 | 1kHz | 控制定时器（`MainTask`） |
| IWDG | PSC64/RLR400 (~0.8s) | 看门狗保护 |

## 编译与烧录

```shell
cmake --preset Debug
cmake --build --preset Debug
```

产物：`build/C_project.elf`（`build/Debug/` 下另有 `.hex`/`.bin`）。

使用 Daplink + Ozone 烧录调试。
