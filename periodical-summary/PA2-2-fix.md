# PA2-2 修复记录

## 本次修复目标

本次工作的目标是修复 PA2-2 阶段遗留的 CPU 指令实现问题，重点解决：
- DiffTest 下的 `eip mismatch`
- 分组指令 (`gp2/gp3/gp5`) 译码与执行接线缺失
- 条件跳转、间接跳转、间接调用、移位、乘除法等缺失实现
- `runall.sh` 中剩余失败测试，直到全部 `PASS`

最终结果：在 `nemu` 目录执行 `bash runall.sh`，所有 testcase 均通过。

## 主要修改文件

- `nemu/src/cpu/exec/exec.c`
- `nemu/src/cpu/exec/arith.c`
- `nemu/src/cpu/exec/all-instr.h`

## 指令/命令修复记录

### 1. `gp5` 组指令 (`0xff`) 修复

**涉及功能：** `inc/dec/call_rm/jmp_rm/push`

**实现方式：**
- 在 `exec.c` 中补全 `make_group(gp5, ...)` 的组表项。
- 主表 `0xff` 保持 `IDEX(E, gp5)`，只在组表内部使用 `EX(...)`，避免重复对 `E` 操作数译码。

**修复后的功能：**
- `ff /0` -> `inc r/m32`
- `ff /1` -> `dec r/m32`
- `ff /2` -> `call r/m32`
- `ff /4` -> `jmp r/m32`
- `ff /6` -> `push r/m32`

**解决的问题：**
- `add-longlong` 中 `ff 71 fc` 因重复译码导致 `eip mismatch`
- `recursion` 中 `call *m32` 未实现
- `hello-str` 中跳转表 `jmp *r/m32` 未实现
- 后续递归路径中的 `ff /0` (`inc r/m32`) 未实现

### 2. 短条件跳转与近条件跳转修复

**涉及功能：** `jcc`, `jmp`

**实现方式：**
- 将 `0x70-0x7f` 的短条件跳转改为 `IDEXW(J, jcc, 1)`，显式指定 8 位位移宽度。
- 补全 `0x0f 80-0x0f 8f` 两字节近条件跳转。
- 补全 `0xe9` 近跳转和 `0xeb` 短跳转。

**解决的问题：**
- 短跳转被错误当成 32 位位移读取，导致 `eip` 多前进若干字节。
- `fact`、`matrix-mul` 等循环类测试中的跳转路径失败。

### 3. `setcc`、`movzx`、`movsx` 宽度修复

**涉及功能：** `setcc`, `movzx`, `movsx`

**实现方式：**
- 将 `0x0f 90-0x0f 9f` 的 `setcc` 改为 `IDEXW(setcc_E, setcc, 1)`，使目的宽度固定为 1 字节。
- 补全：
  - `0x0f b6` -> `movzx r16/32, r/m8`
  - `0x0f b7` -> `movzx r16/32, r/m16`
  - `0x0f be` -> `movsx r16/32, r/m8`
  - `0x0f bf` -> `movsx r16/32, r/m16`

**解决的问题：**
- `sete %al` 执行后高位残留，导致 `eax` 与 QEMU 不一致。
- 多个比较类测试在 `setcc + movzx` 链路上失败。

### 4. `sbb` 指令接线修复

**涉及功能：** `sbb`

**实现方式：**
- 在 `exec.c` 主 opcode 表中补全：
  - `0x18-0x1b` 寄存器/内存形式
  - `0x1c-0x1d` 立即数到 `eax` 形式
- `arith.c` 中已有 `make_EHelper(sbb)`，只需注册到主表。

**解决的问题：**
- `sub-longlong` 在 `sbb` 处报 `invalid opcode`。

### 5. `adc` 进位逻辑修复

**涉及功能：** `adc`

**实现方式：**
- 在 `arith.c` 中将 `adc` 的 CF 计算改成两次加法进位的或关系：
  1. `dest + src`
  2. `(dest + src) + old_CF`
- 只要任一阶段产生进位，最终 `CF=1`。

**解决的问题：**
- 高 32 位加法依赖 CF 传播时结果不稳定。
- `add-longlong` 这类 64 位拆分加法测试需要精确的 CF 传递。

### 6. `gp3` 组指令 (`0xf6/0xf7`) 修复

**涉及功能：** `test/not/neg/mul/imul/div/idiv`

**实现方式：**
- 在 `exec.c` 中补全 `make_group(gp3, ...)`：
  - `/0`、`/1` -> `test_I`
  - `/2` -> `not`
  - `/3` -> `neg`
  - `/4` -> `mul`
  - `/5` -> `imul1`
  - `/6` -> `div`
  - `/7` -> `idiv`
- 对 `/0`、`/1` 使用 `IDEX(test_I, test)`，其余使用 `EX(...)`。

**解决的问题：**
- `mul-longlong` 的 `f7 /5` (`imul`) 未实现。
- `prime` 中 `idiv` 路径未实现。

### 7. `gp2` 组移位指令修复

**涉及功能：** `shl`, `shr`, `sar`

**实现方式：**
- 在 `exec.c` 中补全 `make_group(gp2, ...)`：
  - `/4` -> `shl`
  - `/5` -> `shr`
  - `/7` -> `sar`
- 主表中的三类入口配合已有 decode helper 使用：
  - `gp2_1_E`
  - `gp2_cl2E`
  - `gp2_Ib2E`

**解决的问题：**
- `shift` 测试在 `c1 /5` 等移位指令处报 `invalid opcode`。

### 8. 乘法相关指令补全

**涉及功能：** `imul1`, `imul2`, `imul3`, `mul`

**实现方式：**
- 补全主表：
  - `0x69` -> `imul r16/32, r/m16/32, imm16/32`
  - `0x6b` -> `imul r16/32, r/m16/32, imm8`
  - `0x0f af` -> `imul r16/32, r/m16/32`
- 这些 helper 在 `arith.c` 中已有实现，修复重点是注册与宽度配置。

**解决的问题：**
- `matrix-mul` 在 `0x0f af` 上失败。
- 乘法长整型相关测试依赖三种 `imul` 形式。

### 9. 立即数压栈修复

**涉及功能：** `push imm32`, `push imm8`

**实现方式：**
- 在主表补全：
  - `0x68` -> `IDEX(I, push)`
  - `0x6a` -> `IDEXW(push_SI, push, 1)`
- `0x6a` 必须显式指定宽度 1，否则会把后续指令字节误读进立即数。

**解决的问题：**
- `add`、`mul-longlong` 中的 `push 0x1` 路径失败。
- 修复前 `0x6a` 被错误解码成 4 字节立即数，直接吞掉后续 `call` 字节，造成 `eip mismatch`。

### 10. `cwtl/cltd` 修复

**涉及功能：** `0x98`, `0x99`

**实现方式：**
- 在主表补全：
  - `0x98` -> `cwtl`
  - `0x99` -> `cltd`
- 对应执行函数位于 `data-mov.c`。

**解决的问题：**
- `prime` 中 `cltd` + `idiv` 的除法前准备路径失败。

### 11. `inc/dec` 寄存器版修复

**涉及功能：** `0x40-0x4f`

**实现方式：**
- 在主表补全：
  - `0x40-0x47` -> `inc r32`
  - `0x48-0x4f` -> `dec r32`
- 使用 `IDEXW(r, inc, 0)` / `IDEXW(r, dec, 0)`。

**解决的问题：**
- 多个循环测试在 `incl %ebx` 等基础自增/自减指令上失败。

### 12. 声明补全

**涉及文件：** `all-instr.h`

**实现方式：**
- 为所有已接入主表或组表但未声明的 helper 补全 `make_EHelper(...)` 声明。
- 包括但不限于：
  - `sbb`
  - `not`, `neg`
  - `mul`, `imul1`, `imul2`, `imul3`
  - `div`, `idiv`
  - `shl`, `shr`, `sar`
  - `cwtl`, `cltd`
  - `call_rm`
  - `movsx`

**解决的问题：**
- 避免 `exec.c` 注册后编译阶段出现未声明符号错误。

## 遇到的问题与解决方法

### 问题 1：`ff` 组指令重复译码导致 `eip mismatch`

**现象：**
- `add-longlong` 在 `ff 71 fc` 处报 `eip mismatch`
- NEMU 认为指令长度比 QEMU 多 2 字节

**原因：**
- 若主表使用 `IDEX(E, gp5)`，组表内再写 `IDEX(E, push)`，就会二次调用 `decode_E`。

**解决方法：**
- 保持主表只译码一次 `E`
- 组表内部统一使用 `EX(...)`

### 问题 2：短跳转宽度错误

**现象：**
- `jcc`、`jmp short` 路径大量 `eip mismatch`

**原因：**
- 8 位位移被当成默认操作数宽度读取，错误地读取了 4 字节。

**解决方法：**
- 对短跳转使用显式宽度：`IDEXW(..., 1)`

### 问题 3：`setcc` 宽度错误导致高位脏数据残留

**现象：**
- `sete %al` 后，NEMU 的 `eax` 与 QEMU 不一致，高 24 位保留旧值。

**原因：**
- `setcc` 被错误按默认 32 位操作数宽度处理。

**解决方法：**
- 将 `setcc` 目的操作数宽度强制固定为 1 字节。

### 问题 4：立即数压栈 `0x6a` 吞掉后续指令字节

**现象：**
- `push 0x1` 后直接出现异常，日志中能看到后一条 `call` 的字节也被拼进当前指令。

**原因：**
- `push imm8` 未显式设置为 1 字节宽度。

**解决方法：**
- 使用 `IDEXW(push_SI, push, 1)`。

### 问题 5：并行回归导致 DiffTest 端口冲突

**现象：**
- 同时跑多个 `build/nemu -b ...` 会报：
  - `qemu-system-i386: -s: Failed to find an available port`
- 继而出现“第一条指令就 mismatch”的假象。

**原因：**
- DiffTest 会启动 QEMU gdbstub，多个实例并发运行时会抢占同一调试端口。

**解决方法：**
- 测试阶段改为严格串行回归。
- 对并行回归产生的首指令 mismatch 结果不直接采信，只作为端口冲突迹象处理。

## 验证过程

### 单项验证
在修复每个问题后，使用如下方式串行验证：

```bash
build/nemu -b -l build/nemu-log.txt /home/LemonadeDream/ics2017/nexus-am/tests/cputest/build/<test>-x86-nemu.bin
```

通过标准：
- 输出 `nemu: HIT GOOD TRAP`
- DiffTest 不再报寄存器或 `eip mismatch`

### 全量验证
最终在 `nemu` 目录执行：

```bash
bash runall.sh
```

最终结果：全部 testcase `PASS`。

## 本次工作的整体结论

本轮修复的核心不是新增复杂执行逻辑，而是系统性补全：
- opcode 到 helper 的注册关系
- 分组指令的组表项
- 立即数/目的操作数的正确宽度
- 避免组指令中的重复译码

在这些基础设施补齐后，原来分散表现为：
- `invalid opcode`
- `eip mismatch`
- 寄存器 diff mismatch

的问题，最终都被收敛并修复，测试集已全部通过。
