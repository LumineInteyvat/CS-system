# AGENTS.md

本文件为 Codex 在处理本仓库代码时提供指导。

## 项目概览

NEMU (NJU Emulator) 是一个专为教学设计的简易全系统 x86 模拟器。小型 x86 程序可以在 NEMU 上运行。其核心特性包括：

- 支持保护模式下大多数常用 x86 指令的 CPU 核心
- 带有 TLB 的 i386 分页机制
- i386 中断与异常处理
- 4 种设备：串口、定时器、键盘、VGA
- 端口映射 I/O (PIO) 与 内存映射 I/O (MMIO)
- 带有调试器的监控器（支持单步执行、寄存器/内存查看、表达式求值、监视点）
- 与 QEMU 的差异测试 (Differential Testing)

periodical-summary下有功能实现的阶段性记录,file_tree.txt,描述了nemu的各个文件功能

## 构建命令

Bash

```
make              # 编译模拟器
make run          # 运行 NEMU 并进入交互式调试器
make gdb          # 启动 GDB 调试模拟器本身
make clean        # 清理构建生成物
./runall.sh       # 批量运行测试（需要设置 $AM_HOME 环境变量）
```

## 架构设计

### 核心组件

**CPU (`src/cpu/`)**

- 模拟器使用定义在 `include/cpu/rtl.h` 中的 RTL（寄存器传输级）抽象。
- 指令通过 `make_EHelper(name)` 宏实现，这些宏会生成 `exec_<name>` 函数。
- `src/cpu/exec/exec.c` 中的操作码表 (opcode table) 使用 `IDEX` 和 `EX` 宏将操作码映射到相应的译码/执行处理函数。
- 组 (Groups, gp1-gp7) 用于处理多用途操作码，这些操作码使用 ModR/M 字节中的 reg 字段作为扩展。
- 寄存器访问使用 `reg_l()`、`reg_w()`、`reg_b()` 宏，它们映射到 i386 编码中的 8 个通用寄存器 (GPR)。

**指令实现流程：**

1. 在 `include/cpu/decode.h` 中使用 `make_DHelper(name)` 定义译码辅助函数。
2. 在 `src/cpu/decode/decode.c` 中实现译码函数。
3. 在 `src/cpu/exec/all-instr.h` 中使用 `make_EHelper(name)` 注册执行辅助函数。
4. 在 `src/cpu/exec/` 下的相应文件中实现执行函数。
5. 在 `src/cpu/exec/exec.c` 的 `opcode_table` 中添加条目。

**内存 (`src/memory/`, `include/memory/`)**

- `vaddr_read()` / `vaddr_write()` 是主要的内存接口。
- `rtl_lm()` / `rtl_sm()` 是 RTL 级别的内存操作函数。

**监控器/调试器 (`src/monitor/`, `include/monitor/`)**

- `src/monitor/debug/ui.c` 中的 `ui_mainloop()` 实现了交互式调试器。
- 命令：`c` (继续), `si [n]` (单步执行), `info r` (查看寄存器), `info w` (查看监视点), `x N EXPR` (查看内存), `p EXPR` (表达式求值), `w EXPR` (设置监视点), `d N` (删除监视点)。
- 表达式求值位于 `src/monitor/expr.c`。
- 监视点位于 `src/monitor/watchpoint.c`。
- 差异测试位于 `src/monitor/diff-test/`。

**设备 (`src/device/`)**

- 设备实现了 MMIO 和端口 I/O 的处理函数。
- `mmio_read()` / `mmio_write()` 用于内存映射 I/O。
- `pio_read()` / `pio_write()` 用于端口映射 I/O。
- 使用 SDL2 库处理 VGA 图像和键盘输入。

### 关键数据结构

- `CPU_state` (`include/cpu/reg.h`)：包含 8 个通用寄存器，可按 32 位、16 位或 8 位访问。
- `DecodeInfo` (`include/cpu/decode.h`)：持有指令译码状态，包括操作数。
- `Operand` (`include/cpu/decode.h`)：表示译码后的操作数（寄存器、内存或立即数）。
- `ModR_M` 和 `SIB` 联合体：用于 x86 ModR/M 和 SIB 字节的译码。

### 构建标志 (Build Flags)

- `DEBUG`：启用指令日志记录和汇编输出。
- `DIFF_TEST`：启用与 QEMU 的差异测试。
- `HAS_IOE`：启用设备 I/O 支持。

## 测试

`runall.sh` 脚本会编译来自 `$AM_HOME/tests/cputest` 的测试用例，并在 NEMU 中运行。当测试输出 "nemu: HIT GOOD TRAP" 时，表示测试通过。

## 调试技巧

- 使用 `-b` 参数可以使 NEMU 以批处理模式运行。
- `-l` 参数指定日志文件路径。
- GDB 支持：`make gdb` 会启动 GDB 并附加到模拟器进程。
- 进行差异测试时，需确保 `qemu` 已在 PATH 路径中，并在编译时开启 `DIFF_TEST=1`。
- `~/home/LemonadeDream~/ics2017/nemu/include/common.h` 中 选择是否注释 `#define DIFF_TEST` 来开启DIFF_TEST, 开启会大幅降低性能，建议单个测试时开启
