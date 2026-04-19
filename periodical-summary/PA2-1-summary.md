# PA2-1 阶段总结：基础设施构建与首个 C 程序运行

## 核心目标

本阶段的目标是搭建 NEMU 的基础指令执行环境，完善 RTL (Register Transfer Language) 抽象层，并实现最基本的 6 条指令，最终使得 `dummy` 客户程序（一个直接返回的空 C 程序）能够在 NEMU 中成功运行并触发 `HIT GOOD TRAP`。

## 关键模块与函数修改记录

### 1. 状态寄存器扩展 (`include/cpu/reg.h`, `src/monitor/monitor.c`)

- **修改内容**：在 `CPU_state` 结构体中通过 C 语言的 **匿名联合体 (Union)** 和 **位域 (Bit-field)** 实现了 32 位的 `EFLAGS` 寄存器。精确定义了 CF (Bit 0)、ZF (Bit 6)、SF (Bit 7)、OF (Bit 11) 等关键标志位。
- **功能要点**：在 `monitor.c` 的 `restart()` 函数中，将 `cpu.eflags.val` 初始化为 `0x2`，符合 i386 手册规范。这是后续所有条件判断和算术运算的基础。

### 2. RTL 伪指令补全 (`include/cpu/rtl.h`)

- **修改内容**：清除了大量 `TODO()`，实现了基础的 RTL 伪指令。
- **功能要点**：
    - **标志位管理**：实现了 `rtl_set/get_eflags`，以及自动根据运算结果更新标志位的 `rtl_update_ZF` 和 `rtl_update_SF`。
    - **堆栈机制**：实现了向下生长的栈操作 `rtl_push` (`esp -= 4`, 写入内存) 和 `rtl_pop` (读取内存, `esp += 4`)。
    - **逻辑运算**：实现了基于宽度的符号扩展 (`rtl_sext`) 和最高位提取 (`rtl_msb`)。

### 3. 指令逻辑实现 (`src/cpu/exec/*.c`)

- **`data-mov.c` (数据传送)**：封装了 `push` 和 `pop`。
- **`control.c` (控制流)**：实现了 `call` 和 `ret`。掌握了通过设置 `decoding.is_jmp = 1` 并修改 `decoding.jmp_eip` 来打破顺序执行流的机制。
- **`arith.c` (算术)**：实现了 `sub` 指令。深刻理解了减法对 CF（借位）和 OF（溢出，符号位异常翻转）的复杂影响。
- **`logic.c` (逻辑)**：实现了 `xor` 指令。验证了逻辑指令必须主动清零 CF 和 OF 的架构强制要求。

### 4. 表驱动译码与执行 (`src/cpu/exec/exec.c`)

- **修改内容**：在 `opcode_table` 和 `gp1` 组中注册了上述 6 条指令。
- **功能要点**：理解了 NEMU “取首字节 -> 查表 -> 分发对应 `decode` 和 `execute` 函数”的流水线架构。宏 `IDEX` 和 `EX` 成功将硬件机器码与 C 语言函数指针绑定。
