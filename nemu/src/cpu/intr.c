#include "cpu/exec.h"
#include "memory/mmu.h"
#include "memory/memory.h"

void raise_intr(uint8_t NO, vaddr_t ret_addr) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  /* 1. 保存现场：EFLAGS, CS, EIP
   * i386 进入中断/异常时会把返回现场压栈。
   * PA 不做特权级切换，所以只压这三项。
   */
  cpu.esp -= 4;
  vaddr_write(cpu.esp, 4, cpu.eflags.val);

  cpu.esp -= 4;
  vaddr_write(cpu.esp, 4, cpu.cs);

  cpu.esp -= 4;
  vaddr_write(cpu.esp, 4, ret_addr);

  /* 2. 用中断号 NO 索引 IDT
   * 每个 GateDesc 是 8 字节。
   */
  vaddr_t desc_addr = cpu.idtr.base + NO * 8;

  Assert(NO * 8 + 7 <= cpu.idtr.limit,
         "IDT index out of limit: NO = 0x%x, limit = 0x%x",
         NO, cpu.idtr.limit);

  /* 3. 读取门描述符
   * 必须用 vaddr_read()，不能把 desc_addr 强转成宿主机指针。
   */
  uint32_t low = vaddr_read(desc_addr, 4);
  uint32_t high = vaddr_read(desc_addr + 4, 4);

  /* 4. 检查 P 位
   * 按讲义简化门描述符，P 位在高 32 位的 bit 15。
   */
  Assert(high & (1 << 15), "IDT gate not present: NO = 0x%x", NO);

  /* 5. 拼出 OFFSET
   * low[15:0]      = OFFSET[15:0]
   * high[31:16]    = OFFSET[31:16]
   */
  vaddr_t target = (high & 0xffff0000) | (low & 0x0000ffff);
  cpu.cs = (low >> 16) & 0xffff;

  /* 6. 设置跳转
   * raise_intr() 是 void，因此直接修改 decoding。
   */
  decoding.is_jmp = 1;
  decoding.jmp_eip = target;
}

void dev_raise_intr() {
}
