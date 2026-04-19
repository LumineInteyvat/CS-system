#include "cpu/exec.h"

make_EHelper(test)
{
  rtl_and(&t2, &id_dest->val, &id_src->val);
  rtl_update_ZFSF(&t2, id_dest->width);
  rtl_li(&t0, 0);
  rtl_set_CF(&t0);
  rtl_set_OF(&t0);

  print_asm_template2(test);
}

make_EHelper(and)
{

  rtl_and(&t2, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t2);

  // 逻辑指令对标志位的固定影响
  rtl_update_ZFSF(&t2, id_dest->width);
  t0 = 0;
  rtl_set_CF(&t0);
  rtl_set_OF(&t0);

  print_asm_template2(and);
}

make_EHelper(xor)
{
  // 1. 执行异或运算：dest = dest ^ src
  rtl_xor(&id_dest->val, &id_dest->val, &id_src->val);

  // 2. 将结果写回
  operand_write(id_dest, &id_dest->val);

  // 3. 更新标志位
  cpu.eflags.CF = 0;                            // i386 手册规定 xor 清除 CF
  cpu.eflags.OF = 0;                            // i386 手册规定 xor 清除 OF
  rtl_update_ZF(&id_dest->val, id_dest->width); // 更新零标志
  rtl_update_SF(&id_dest->val, id_dest->width); // 更新符号标志

  print_asm_template2(xor);
}

make_EHelper(or)
{
  rtl_or(&t2, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);
  rtl_li(&t0, 0);
  rtl_set_CF(&t0);
  rtl_set_OF(&t0);

  print_asm_template2(or);
}

make_EHelper(sar)
{
  rtl_andi(&t0, &id_src->val, 0x1f);
  rtl_sar(&t2, &id_dest->val, &t0);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);
  // unnecessary to update CF and OF in NEMU

  print_asm_template2(sar);
}

make_EHelper(shl)
{
  rtl_andi(&t0, &id_src->val, 0x1f);
  rtl_shl(&t2, &id_dest->val, &t0);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);
  // unnecessary to update CF and OF in NEMU

  print_asm_template2(shl);
}

make_EHelper(shr)
{
  rtl_andi(&t0, &id_src->val, 0x1f);
  rtl_shr(&t2, &id_dest->val, &t0);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);
  // unnecessary to update CF and OF in NEMU

  print_asm_template2(shr);
}

make_EHelper(setcc)
{
  uint8_t subcode = decoding.opcode & 0xf;
  rtl_setcc(&t2, subcode);
  operand_write(id_dest, &t2);

  print_asm("set%s %s", get_cc_name(subcode), id_dest->str);
}

make_EHelper(not)
{
  rtl_mv(&t2, &id_dest->val);
  rtl_not(&t2);
  operand_write(id_dest, &t2);

  print_asm_template1(not);
}
