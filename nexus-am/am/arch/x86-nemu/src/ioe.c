#include <am.h>
#include <x86.h>

#define RTC_PORT 0x48   // Note that this is not standard
#define I8042_DATA_PORT 0x60
#define I8042_STATUS_PORT 0x64
#define I8042_STATUS_HASKEY_MASK 0x1
static unsigned long boot_time;

void _ioe_init() {
  boot_time = inl(RTC_PORT);
}

unsigned long _uptime() {
  return inl(RTC_PORT) - boot_time;
}

uint32_t* const fb = (uint32_t *)0x40000;

_Screen _screen = {
  .width  = 400,
  .height = 300,
};

extern void *memcpy(void *, const void *, size_t);

void _draw_rect(const uint32_t *pixels, int x, int y, int w, int h) {
  if (x >= _screen.width || y >= _screen.height) {
    return;
  }

  int draw_w = w;
  int draw_h = h;
  if (x + draw_w > _screen.width) {
    draw_w = _screen.width - x;
  }
  if (y + draw_h > _screen.height) {
    draw_h = _screen.height - y;
  }

  for (int j = 0; j < draw_h; j++) {
    memcpy(&fb[(y + j) * _screen.width + x], pixels + j * w, draw_w * sizeof(uint32_t));
  }
}

void _draw_sync() {
}

int _read_key() {
  if ((inb(I8042_STATUS_PORT) & I8042_STATUS_HASKEY_MASK) == 0) {
    return _KEY_NONE;
  }
  return inl(I8042_DATA_PORT);
}
