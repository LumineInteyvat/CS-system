#include "common.h"

#define KEYDOWN_MASK 0x8000

#define NAME(key) \
  [_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

size_t events_read(void *buf, size_t len) {
  char event[32];
  int key = _read_key();
  int n;

  if (key != _KEY_NONE) {
    bool is_keydown = (key & KEYDOWN_MASK) != 0;
    int keycode = key & ~KEYDOWN_MASK;
    n = snprintf(event, sizeof(event), "%s %s\n", is_keydown ? "kd" : "ku", keyname[keycode]);
  }
  else {
    n = snprintf(event, sizeof(event), "t %u\n", (unsigned int)_uptime());
  }

  size_t read_len = len < (size_t)n ? len : (size_t)n;
  memcpy(buf, event, read_len);
  return read_len;
}

static char dispinfo[128] __attribute__((used));

size_t dispinfo_read(void *buf, off_t offset, size_t len) {
  size_t size = strlen(dispinfo);
  if (offset >= size) {
    return 0;
  }

  size_t remain = size - offset;
  size_t read_len = len < remain ? len : remain;
  memcpy(buf, dispinfo + offset, read_len);
  return read_len;
}

size_t fb_write(const void *buf, off_t offset, size_t len) {
  size_t fb_size = _screen.width * _screen.height * sizeof(uint32_t);
  if (offset >= fb_size) {
    return 0;
  }

  size_t write_len = len < fb_size - offset ? len : fb_size - offset;
  write_len -= write_len % sizeof(uint32_t);

  const uint32_t *pixels = buf;
  size_t pixel_offset = offset / sizeof(uint32_t);
  int x = pixel_offset % _screen.width;
  int y = pixel_offset / _screen.width;
  size_t pixels_left = write_len / sizeof(uint32_t);
  size_t done = 0;

  while (pixels_left > 0 && y < _screen.height) {
    int row_pixels = _screen.width - x;
    if ((size_t)row_pixels > pixels_left) {
      row_pixels = pixels_left;
    }

    _draw_rect(pixels + done, x, y, row_pixels, 1);
    done += row_pixels;
    pixels_left -= row_pixels;
    x = 0;
    y ++;
  }

  return done * sizeof(uint32_t);
}

void init_device() {
  _ioe_init();
  snprintf(dispinfo, sizeof(dispinfo), "WIDTH:%d\nHEIGHT:%d\n", _screen.width, _screen.height);
}
