#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  // append
  char expr[256];
  uint32_t old_val;
} WP;

// append
void init_wp_pool();
WP* new_wp(char *e);
void free_wp(int n);
void display_watchpoints();
bool check_watchpoints();
#endif
