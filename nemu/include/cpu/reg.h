#ifndef __REG_H__
#define __REG_H__

#include "common.h"

enum { R_EAX, R_ECX, R_EDX, R_EBX, R_ESP, R_EBP, R_ESI, R_EDI };
enum { R_AX, R_CX, R_DX, R_BX, R_SP, R_BP, R_SI, R_DI };
enum { R_AL, R_CL, R_DL, R_BL, R_AH, R_CH, R_DH, R_BH };

/* TODO: Re-organize the `CPU_state' structure to match the register
 * encoding scheme in i386 instruction format. For example, if we
 * access cpu.gpr[3]._16, we will get the `bx' register; if we access
 * cpu.gpr[1]._8[1], we will get the 'ch' register. Hint: Use `union'.
 * For more details about the register encoding scheme, see i386 manual.
 */
void isa_reg_display();
uint32_t isa_reg_str2val(const char *s, bool *success);

typedef struct {
  union {
    union {
      uint32_t _32;
      uint16_t _16;
      uint8_t _8[2];
    } gpr[8];

    struct {
      rtlreg_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
    };
  };

  vaddr_t eip;

// 新增：EFLAGS 寄存器
  union {
    uint32_t val; // 允许直接作为一个 32 位整体进行访问
    struct {
      uint32_t CF : 1; // Bit 0
      uint32_t _1 : 1; // Bit 1 (固定为1)
      uint32_t PF : 1; // Bit 2
      uint32_t _3 : 1; // Bit 3 (固定为0)
      uint32_t AF : 1; // Bit 4
      uint32_t _5 : 1; // Bit 5 (固定为0)
      uint32_t ZF : 1; // Bit 6
      uint32_t SF : 1; // Bit 7
      uint32_t TF : 1; // Bit 8
      uint32_t IF : 1; // Bit 9
      uint32_t DF : 1; // Bit 10
      uint32_t OF : 1; // Bit 11
      uint32_t _rest: 20; // 其余高位
    };
  } eflags;

} CPU_state;

extern CPU_state cpu;

static inline int check_reg_index(int index) {
  assert(index >= 0 && index < 8);
  return index;
}

#define reg_l(index) (cpu.gpr[check_reg_index(index)]._32)
#define reg_w(index) (cpu.gpr[check_reg_index(index)]._16)
#define reg_b(index) (cpu.gpr[check_reg_index(index) & 0x3]._8[index >> 2])

extern const char* regsl[];
extern const char* regsw[];
extern const char* regsb[];

static inline const char* reg_name(int index, int width) {
  assert(index >= 0 && index < 8);
  switch (width) {
    case 4: return regsl[index];
    case 1: return regsb[index];
    case 2: return regsw[index];
    default: assert(0);
  }
}

#endif
