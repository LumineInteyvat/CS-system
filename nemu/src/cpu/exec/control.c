#include "cpu/exec.h"

make_EHelper(jmp)
{
  // the target address is calculated at the decode stage
  decoding.is_jmp = 1;

  print_asm("jmp %x", decoding.jmp_eip);
}

make_EHelper(jcc)
{
  // the target address is calculated at the decode stage
  uint8_t subcode = decoding.opcode & 0xf;
  rtl_setcc(&t2, subcode);
  decoding.is_jmp = t2;

  print_asm("j%s %x", get_cc_name(subcode), decoding.jmp_eip);
}

make_EHelper(jmp_rm)
{
  decoding.jmp_eip = id_dest->val;
  decoding.is_jmp = 1;

  print_asm("jmp *%s", id_dest->str);
}

make_EHelper(call)
{
  // 1. 将下一条指令的地址 (返回地址) 压栈
  rtl_push(&decoding.seq_eip);

  // 2. 发生跳转，告诉执行框架我们要改变 EIP
  decoding.is_jmp = 1;

  print_asm("call %x", decoding.jmp_eip);
}

make_EHelper(ret)
{
  // 1. 从栈中弹出返回地址
  rtl_pop(&decoding.jmp_eip);

  // 2. 告诉框架发生跳转
  decoding.is_jmp = 1;

  print_asm("ret");
}

make_EHelper(call_rm)
{
  rtl_push(&decoding.seq_eip);
  decoding.jmp_eip = id_dest->val;
  decoding.is_jmp = 1;

  print_asm("call *%s", id_dest->str);
}
