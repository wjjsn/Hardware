# Hardware 开发约束

## 代码风格

- 定宽整数统一包含 `<stdint.h>`，使用 `uint8_t`、`uint16_t`、`uint32_t` 等全局类型。
- 不使用 `<cstdint>` 和 `std::uint*_t`。
- 位操作统一使用 `bits_operation.hpp` 中的 `BIT::SET`、`BIT::CLR`、`BIT::TGL`、`BIT::READ`。
- 不使用 `SET_BIT`、`CLR_BIT`、`READ_BIT` 等位操作宏。
- 头文件使用 `#pragma once`。
- 修改现有驱动时保持原有接口和结构，不顺带重命名、重构或改变行为。

## HAL 接口

- Hardware 通过模板参数使用 HAL，不直接访问 MCU 寄存器。
- GPIO、TIM、PWM、I2C、UART、SPI 等接口以 `subprojects/HAL/AGENTS.md` 为准。
- 新驱动沿用仓库现有的静态模板接口风格，不创建同一功能的第二套接口。
