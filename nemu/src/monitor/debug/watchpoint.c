#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP - 1; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].NO = NR_WP - 1;
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

WP* new_wp(char *e) {
  assert(free_ != NULL);

  WP *wp = free_;
  free_ = free_->next;

  strncpy(wp->expr, e, sizeof(wp->expr) - 1);
  wp->expr[sizeof(wp->expr) - 1] = '\0';

  bool success = false;
  wp->old_val = expr(wp->expr, &success);

  if (!success) {
    wp->next = free_;
    free_ = wp;
    return NULL;
  }

  wp->next = head;
  head = wp;

  printf("Watchpoint %d: %s = %u (0x%x)\n",
      wp->NO, wp->expr, wp->old_val, wp->old_val);

  return wp;
}

void free_wp(int n) {
  WP *prev = NULL, *cur = head;

  while (cur != NULL) {
    if (cur->NO == n) {
      if (prev == NULL) {
        head = cur->next;
      } else {
        prev->next = cur->next;
      }

      cur->next = free_;
      free_ = cur;
      printf("Watchpoint %d deleted\n", n);
      return;
    }
    prev = cur;
    cur = cur->next;
  }

  printf("No watchpoint number %d\n", n);
}

void display_watchpoints() {
  WP *cur = head;
  if (cur == NULL) {
    printf("No watchpoints.\n");
    return;
  }

  while (cur != NULL) {
    printf("Watchpoint %d: %s = %u (0x%x)\n",
        cur->NO, cur->expr, cur->old_val, cur->old_val);
    cur = cur->next;
  }
}

bool check_watchpoints() {
  WP *cur = head;
  bool changed = false;

  while (cur != NULL) {
    bool success = false;
    uint32_t new_val = expr(cur->expr, &success);
    assert(success);

    if (new_val != cur->old_val) {
      printf("Watchpoint %d triggered: %s\n", cur->NO, cur->expr);
      printf("Old value = %u (0x%x)\n", cur->old_val, cur->old_val);
      printf("New value = %u (0x%x)\n", new_val, new_val);
      cur->old_val = new_val;
      changed = true;
    }

    cur = cur->next;
  }

  return changed;
}
