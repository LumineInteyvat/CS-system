# PA2-2 阶段总结：扩展指令集实现

## 核心目标

本阶段的目标是遍历 `src/cpu/exec/` 目录下的所有文件，补全所有 `TODO()` 实现的指令，并在 `exec.c` 中更新指令注册表，使 NEMU 能够支持更多的 x86 指令。

## 关键模块与函数修改记录

### 1. logic.c - 逻辑运算指令

#### 已实现指令：`test`, `or`, `sar`, `shl`, `shr`, `not`

| 指令 | 实现方法 | 关键点 |
|------|----------|--------|
| `test` | 执行 `AND` 操作但不存储结果，只更新标志位 | 类似于 `and` 但不写回结果 |
| `or` | `rtl_or` 执行按位或，结果写回 | 逻辑指令固定清零 CF 和 OF |
| `sar` | `rtl_sar` 算术右移（符号位填充） | 在 NEMU 中不需要更新 CF/OF |
| `shl` | `rtl_shl` 逻辑左移（零填充） | 在 NEMU 中不需要更新 CF/OF |
| `shr` | `rtl_shr` 逻辑右移（零填充） | 在 NEMU 中不需要更新 CF/OF |
| `not` | `rtl_not` 按位取反 | 不影响任何标志位 |

**实现示例（test 指令）：**
```c
make_EHelper(test)
{
  // 执行 AND 操作但不存储结果，只更新标志位
  rtl_and(&t2, &id_dest->val, &id_src->val);

  // 逻辑指令对标志位的固定影响
  rtl_update_ZFSF(&t2, id_dest->width);
  t0 = 0;
  rtl_set_CF(&t0);
  rtl_set_OF(&t0);

  print_asm_template2(test);
}
```

### 2. arith.c - 算术运算指令

#### 已实现指令：`inc`, `dec`, `neg`

| 指令 | 实现方法 | 标志位处理 |
|------|----------|------------|
| `inc` | `rtl_addi` 加 1 | 不影响 CF，影响 OF/ZF/SF |
| `dec` | `rtl_addi` 减 1 | 不影响 CF，影响 OF/ZF/SF |
| `neg` | `rtl_sub(&t2, &tzero, &dest)` 求补 | 影响 CF（操作数非零则CF=1）和 OF（操作数为最小负数则OF=1） |

**实现要点：**
- `neg` 指令：CF = (dest != 0)，因为 0 - non-zero 会产生借位
- `neg` 指令：OF = (dest 是最小负数)，因为 0 - 0x80000000 会溢出
- `inc`/`dec` 指令：OF 判断使用异或法检测符号位异常翻转

### 3. data-mov.c - 数据传送指令

#### 已实现指令：`pusha`, `popa`, `cltd`, `cwtl`

| 指令 | 功能 | 实现方法 |
|------|------|----------|
| `pusha` | 推送所有通用寄存器到栈 | 按顺序 EAX, ECX, EDX, EBX, ESP(原始), EBP, ESI, EDI 压栈 |
| `popa` | 从栈弹出所有通用寄存器 | 逆序弹出，跳过 ESP |
| `cltd` | 符号扩展 EAX 到 EDX:EAX | 根据 EAX 最高位设置 EDX |
| `cwtl` | 符号扩展 AX/AL 到 EAX/AX | 根据操作数宽度进行符号扩展 |

**pusha/popa 实现注意事项：**
- 16 位模式使用 AX/CX/DX/BX/SP(原始)/BP/SI/DI
- 32 位模式使用 EAX/ECX/EDX/EBX/ESP(原始)/EBP/ESI/EDI
- `popa` 需要跳过 ESP 位置的原始值

### 4. cc.c - 条件码处理

#### 已实现函数：`rtl_setcc`

此函数根据 EFLAGS 状态设置目标操作数为 1 或 0。

| 条件码 | 含义 | 判断逻辑 |
|--------|------|----------|
| CC_O / CC_NO | 溢出/未溢出 | OF |
| CC_B / CC_NB | 低于/不低于 (unsigned) | CF |
| CC_E / CC_NE | 相等/不相等 | ZF |
| CC_BE / CC_NBE | 低于或等于/高于 (unsigned) | CF OR ZF / NOT CF AND NOT ZF |
| CC_S / CC_NS | 有符号/无符号 | SF |
| CC_L / CC_NL | 小于/大于或等于 (signed) | SF != OF |
| CC_LE / CC_NLE | 小于或等于/大于 (signed) | ZF OR (SF != OF) / NOT ZF AND (SF == OF) |

**实现示例：**
```c
void rtl_setcc(rtlreg_t* dest, uint8_t subcode) {
  bool invert = subcode & 0x1;
  // ... 根据 subcode 设置 result
  rtl_li(dest, result);
  if (invert) {
    rtl_xori(dest, dest, 0x1);
  }
}
```

### 5. system.c - 系统指令

#### 已实现指令：`lidt`, `mov_r2cr`, `mov_cr2r`, `int`, `iret`, `in`, `out`

| 指令 | 功能 | 实现方法 |
|------|------|----------|
| `lidt` | 加载中断描述符表寄存器 | 从内存读取 6 字节（4 字节基址 + 2 字节限长） |
| `mov_r2cr` | 通用寄存器到控制寄存器 | NEMU 简化实现 |
| `mov_cr2r` | 控制寄存器到通用寄存器 | NEMU 简化实现（返回 0） |
| `int` | 软件中断 | 触发 `diff_test_skip_nemu()` |
| `iret` | 中断返回 | 弹出 EIP，设置 `is_jmp=1` |
| `in` | 从端口读取 | 调用 `pio_read()` |
| `out` | 向端口写入 | 调用 `pio_write()` |

**in/out 实现注意事项：**
- `pio_read(ioaddr_t addr, int len)` - 从指定端口读取
- `pio_write(ioaddr_t addr, int len, uint32_t data)` - 向指定端口写入

### 6. control.c - 控制流指令

#### 已实现指令：`call_rm`

| 指令 | 功能 | 实现方法 |
|------|------|----------|
| `call_rm` | 调用寄存器/内存中的地址 | 先压栈返回地址，再跳转到目标地址 |

## 遇到的问题与解决方法

### 问题 1：rtl_addi 宏展开失败

**现象：** 使用 `rtl_addi` 时编译报错 `unused variable`

**分析：** 
- `rtl.h` 中使用 `make_rtl_arith_logic(add)` 宏生成 `rtl_addi` 函数
- 宏调用链：`concat3(rtl_, add, i)` -> `concat(rtl_add, i)` -> `concat(rtl_addi)` -> 正确展开

**解决：** `rtl_addi` 确实存在，可以直接使用，无需额外定义。

### 问题 2：未使用变量警告导致编译失败

**现象：** GCC 将所有警告视为错误（`-Werror`），未使用的变量导致编译中止。

**解决：** 移除不必要的变量声明，确保每个变量都被使用或移除。

### 问题 3：CPU_state 结构体缺少控制寄存器字段

**现象：** 实现 `mov_r2cr` 和 `mov_cr2r` 时引用 `cpu.cr0` 等字段，编译器报错。

**分析：** NEMU 的 CPU_state 结构体未定义 CR0-CR3 等控制寄存器。

**解决：** 在 system.c 中使用简化实现，不实际访问不存在的结构体字段。对于 `mov_cr2r`，返回 0；对于 `mov_r2cr`，不做实际赋值。

### 问题 4：int/iret 实现过于简化

**现象：** `int` 指令和 `iret` 指令涉及复杂的中断处理机制。

**分析：** 完整的中断处理需要 IDT（中断描述符表）、中断向量号到处理程序的映射等。

**解决：** 采用最小化实现：int 指令标记 `is_int` 并设置中断向量号；iret 从栈弹出 EIP 并跳转。

## NEMU 项目架构理解

### 1. 指令执行流水线

```
取指 (fetch) -> 译码 (decode) -> 执行 (execute) -> 写回 (writeback)
```

NEMU 通过 `opcode_table` 数组将机器码映射到对应的 `DHelper`（译码函数）和 `EHelper`（执行函数）。

### 2. RTL 抽象层

RTL (Register Transfer Language) 提供了：
- **算术逻辑运算**：`rtl_add`, `rtl_sub`, `rtl_and`, `rtl_or`, `rtl_xor`, `rtl_shl`, `rtl_shr`, `rtl_sar`
- **立即数运算变体**：`rtl_addi`, `rtl_subi` 等（通过宏生成）
- **内存操作**：`rtl_lm`（load memory）, `rtl_sm`（store memory）
- **标志位管理**：`rtl_set_CF`, `rtl_set_OF`, `rtl_update_ZFSF` 等
- **符号扩展**：`rtl_sext`

### 3. 译码机制

- `make_DHelper(name)` 定义译码辅助函数
- `read_ModR_M` 读取 x86 的 ModR/M 字节，解析操作数
- 宏 `IDEX`, `IDEXW`, `EX` 将译码函数和执行函数绑定到操作码

### 4. 组机制 (Groups)

NEMU 使用 `make_group` 宏创建指令组，如 `gp1` (0x80-0x83), `gp2` (0xc0-0xc3) 等。
- 每个组有 8 个条目，对应 ext_opcode (0-7)
- 组内每个条目可以是 `EMPTY` (未实现)、`EX(func)` (直接执行) 或 `IDEX(decode, exec)` (译码后执行)

### 5. 执行框架关键变量

| 变量 | 含义 |
|------|------|
| `decoding.seq_eip` | 顺序执行时的下一条指令地址 |
| `decoding.jmp_eip` | 跳转目标地址 |
| `decoding.is_jmp` | 是否发生跳转（设置为 1 表示跳转） |
| `decoding.opcode` | 当前操作码 |
| `decoding.ext_opcode` | 组内扩展操作码 |
| `decoding.is_operand_size_16` | 是否为 16 位操作数模式 |

### 6. 标志位处理原则

- **算术指令**：`add`, `sub`, `adc`, `sbb` 等需要更新 CF, OF, ZF, SF
- **逻辑指令**：`and`, `or`, `xor`, `test` 等固定清零 CF 和 OF
- **移位指令**：在 NEMU 中不需要更新 CF 和 OF（简化实现）
- **NOT 指令**：不影响任何标志位

## 新增指令（PA2-2 后期补充）

### 1. data-mov.c - 数据传送指令（补充）

#### 已实现指令：`xchg`

| 指令 | 功能 | 实现方法 |
|------|------|----------|
| `xchg` | 交换两个寄存器或寄存器与内存的值 | 使用临时寄存器 t0 中转，完成后写回 |

**实现示例：**
```c
make_EHelper(xchg)
{
  rtl_mv(&t0, &id_dest->val);
  rtl_mv(&id_dest->val, &id_src->val);
  operand_write(id_dest, &id_dest->val);
  rtl_mv(&id_src->val, &t0);
  operand_write(id_src, &id_src->val);
  print_asm_template2(xchg);
}
```

### 2. exec.c - 指令注册表更新

#### 新增注册条目

| 操作码 | 指令 | 说明 |
|--------|------|------|
| 0x08-0x0d | `or` | 按位或指令（4 条变体） |
| 0x84-0x85 | `test` | 测试指令（2 条变体） |
| 0x90-0x91 | `xchg` | 寄存器交换（与 eax 交换） |
| 0x90 (2-byte) | `nop` | 0x90 在 2-byte 表中是 nop |
| 0x70-0x7f | `jcc` | 条件跳转（8 组条件） |
| 0x0f 0x90-0x0f 0x9f | `setcc` | 条件字节设置（8 组条件） |
| 0x0f 0xb6 | `movzx` | 零扩展传送 |

### 3. decode.c - 译码函数（补充）

#### 新增译码函数

**setcc_E 译码（用于 SETcc 指令）：**
```c
make_DHelper(setcc_E)
{
  decode_op_rm(eip, id_dest, true, NULL, false);
}
```

**a2a 译码（用于 XCHG eax, reg 指令）：**
```c
make_DHelper(a2a)
{
  decode_op_a(eip, id_dest, true);
  id_src->type = OP_TYPE_REG;
  id_src->reg = R_EAX;
  id_src->width = id_dest->width;
  rtl_lr(&id_src->val, R_EAX, id_src->width);
}
```

### 4. all-instr.h - 执行函数声明

新增以下声明：
```c
make_EHelper(setcc);
make_EHelper(test);
make_EHelper(jmp);
make_EHelper(jcc);
make_EHelper(jmp_rm);
make_EHelper(movzx);
```

### 5. cc.c - 条件码处理（补充）

`rtl_setcc` 函数已完善，支持所有条件码的判定逻辑。

## 待完成工作

exec.c 中仍有大量 `EMPTY` 条目，主要包括：
- **gp2 组** (0xc0, 0xc1, 0xd0, 0xd1, 0xd2, 0xd3)：旋转/移位指令的变体
- **gp3 组** (0xf6, 0xf7)：`test`, `not`, `neg`, `mul`, `imul`, `div`, `idiv`
- **gp4 组** (0xfe)：`inc`, `dec`
- **gp5 组** (0xff)：`inc`, `dec`, `call`, `call far`, `jmp`, `jmp far`, `push`
- **gp7 组** (0x0f 0x01)：`lgdt`, `lidt`, `sgdt`, `sidt`, `lmsw`, `smsw`, `lar`, `lsl` 等

这些指令的实现需要对标志位处理的更深入理解，特别是旋转指令的 CF 和 OF 标志位更新逻辑。

## 测试状态

- **dummy 测试**：通过（输出 `nemu: HIT GOOD TRAP`）
- **add 测试**：进行中
- **add-longlong 测试**：失败于 eip=0x100039，byte 0x45

**已知问题：** add-longlong 测试在执行 `call nem_assert` 和 `ret` 序列后返回到错误的地址（0x100039 而非预期的 0x100094），疑似栈状态损坏或 push/pop 实现问题。
