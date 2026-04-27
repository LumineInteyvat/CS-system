#include "cpu/exec.h"

void diff_test_skip_qemu();
void diff_test_skip_nemu();
void raise_intr(uint8_t NO, vaddr_t ret_addr);

make_EHelper(lidt) {
  vaddr_t addr = id_dest->addr;

  cpu.idtr.limit = vaddr_read(addr, 2);
  cpu.idtr.base = vaddr_read(addr + 2, 4);

  Log("lidt: addr=0x%x, idtr.base=0x%x, idtr.limit=0x%x",
      addr, cpu.idtr.base, cpu.idtr.limit);

  print_asm_template1(lidt);
}

make_EHelper(mov_r2cr) {
  TODO();

  print_asm("movl %%%s,%%cr%d", reg_name(id_src->reg, 4), id_dest->reg);
}

make_EHelper(mov_cr2r) {
  TODO();

  print_asm("movl %%cr%d,%%%s", id_src->reg, reg_name(id_dest->reg, 4));

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}

make_EHelper(int) {
  raise_intr(id_src->val, decoding.seq_eip);

  print_asm("int %s", id_dest->str);

#ifdef DIFF_TEST
  diff_test_skip_nemu();
#endif
}

make_EHelper(iret) {
  // 因此 iret 弹栈顺序是：
  // pop EIP
  // pop CS
  // pop EFLAGS

  vaddr_t ret_eip = vaddr_read(cpu.esp, 4);
  cpu.esp += 4;

  cpu.cs = vaddr_read(cpu.esp, 4) & 0xffff;
  cpu.esp += 4;

  cpu.eflags.val = vaddr_read(cpu.esp, 4);
  cpu.esp += 4;

  decoding.is_jmp = 1;
  decoding.jmp_eip = ret_eip;

  print_asm("iret");
}

uint32_t pio_read(ioaddr_t, int);
void pio_write(ioaddr_t, int, uint32_t);

make_EHelper(in) {
  rtl_li(&t0, pio_read(id_src->val, id_dest->width));
  operand_write(id_dest, &t0);

  print_asm_in(in);

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}

make_EHelper(out) {
  pio_write(id_dest->val, id_src->width, id_src->val);

  print_asm_out(out);

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}
