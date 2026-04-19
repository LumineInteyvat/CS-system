#include <klib.h>

static char *heap_ptr = NULL;

void *kalloc(size_t size) {
  if (heap_ptr == NULL) {
    heap_ptr = (char *)_heap.start;
  }

  size = (size + 7) & ~7u;
  if (heap_ptr + size > (char *)_heap.end) {
    return NULL;
  }

  void *ret = heap_ptr;
  heap_ptr += size;
  return ret;
}

void kfree(void *ptr) {
  (void)ptr;
}

void *memset(void *v, int c, size_t n) {
  unsigned char *p = (unsigned char *)v;
  for (size_t i = 0; i < n; i++) {
    p[i] = (unsigned char)c;
  }
  return v;
}

void *memcpy(void *dst, const void *src, size_t n) {
  unsigned char *d = (unsigned char *)dst;
  const unsigned char *s = (const unsigned char *)src;
  for (size_t i = 0; i < n; i++) {
    d[i] = s[i];
  }
  return dst;
}

void *memmove(void *dst, const void *src, size_t n) {
  unsigned char *d = (unsigned char *)dst;
  const unsigned char *s = (const unsigned char *)src;
  if (d < s) {
    for (size_t i = 0; i < n; i++) {
      d[i] = s[i];
    }
  } else if (d > s) {
    for (size_t i = n; i > 0; i--) {
      d[i - 1] = s[i - 1];
    }
  }
  return dst;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *a = (const unsigned char *)s1;
  const unsigned char *b = (const unsigned char *)s2;
  for (size_t i = 0; i < n; i++) {
    if (a[i] != b[i]) {
      return (int)a[i] - (int)b[i];
    }
  }
  return 0;
}

size_t strlen(const char *s) {
  size_t len = 0;
  while (s[len] != '\0') {
    len++;
  }
  return len;
}

char *strcat(char *dst, const char *src) {
  char *ret = dst;
  dst += strlen(dst);
  while ((*dst++ = *src++) != '\0') {
  }
  return ret;
}

char *strcpy(char *dst, const char *src) {
  char *ret = dst;
  while ((*dst++ = *src++) != '\0') {
  }
  return ret;
}

char *strncpy(char *dst, const char *src, size_t n) {
  size_t i = 0;
  for (; i < n && src[i] != '\0'; i++) {
    dst[i] = src[i];
  }
  for (; i < n; i++) {
    dst[i] = '\0';
  }
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 != '\0' && *s1 == *s2) {
    s1++;
    s2++;
  }
  return (unsigned char)*s1 - (unsigned char)*s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (s1[i] != s2[i] || s1[i] == '\0' || s2[i] == '\0') {
      return (unsigned char)s1[i] - (unsigned char)s2[i];
    }
  }
  return 0;
}

char *strtok(char *s, const char *delim) {
  static char *saved = NULL;
  if (s == NULL) {
    s = saved;
  }
  if (s == NULL) {
    return NULL;
  }

  while (*s != '\0' && strchr(delim, *s) != NULL) {
    s++;
  }
  if (*s == '\0') {
    saved = NULL;
    return NULL;
  }

  char *ret = s;
  while (*s != '\0' && strchr(delim, *s) == NULL) {
    s++;
  }
  if (*s == '\0') {
    saved = NULL;
  } else {
    *s = '\0';
    saved = s + 1;
  }
  return ret;
}

char *strstr(const char *haystack, const char *needle) {
  size_t nlen = strlen(needle);
  if (nlen == 0) {
    return (char *)haystack;
  }

  for (; *haystack != '\0'; haystack++) {
    if (strncmp(haystack, needle, nlen) == 0) {
      return (char *)haystack;
    }
  }
  return NULL;
}

const char *strchr(const char *s, int c) {
  while (*s != '\0') {
    if (*s == c) {
      return s;
    }
    s++;
  }
  return (c == '\0') ? s : NULL;
}

int atoi(const char *nptr) {
  int sign = 1;
  int ret = 0;
  while (*nptr == ' ' || *nptr == '\t' || *nptr == '\n') {
    nptr++;
  }
  if (*nptr == '-') {
    sign = -1;
    nptr++;
  } else if (*nptr == '+') {
    nptr++;
  }
  while (*nptr >= '0' && *nptr <= '9') {
    ret = ret * 10 + (*nptr - '0');
    nptr++;
  }
  return ret * sign;
}

int abs(int x) {
  return x < 0 ? -x : x;
}

unsigned long time() {
  return _uptime();
}

static unsigned int rand_seed = 1;

void srand(unsigned int seed) {
  rand_seed = seed;
}

int rand() {
  rand_seed = rand_seed * 1103515245u + 12345u;
  return (int)((rand_seed >> 16) & 0x7fff);
}

typedef struct {
  char *buf;
  size_t size;
  size_t len;
} out_ctx_t;

static void out_char(out_ctx_t *ctx, char ch) {
  if (ctx->buf != NULL && ctx->len + 1 < ctx->size) {
    ctx->buf[ctx->len] = ch;
  }
  ctx->len++;
}

static void out_string(out_ctx_t *ctx, const char *s) {
  if (s == NULL) {
    s = "(null)";
  }
  while (*s != '\0') {
    out_char(ctx, *s++);
  }
}

static void out_uint(out_ctx_t *ctx, unsigned int value, unsigned int base, int upper) {
  char tmp[16];
  int n = 0;
  do {
    unsigned int digit = value % base;
    if (digit < 10) {
      tmp[n++] = '0' + digit;
    } else {
      tmp[n++] = (upper ? 'A' : 'a') + digit - 10;
    }
    value /= base;
  } while (value != 0);

  while (n > 0) {
    out_char(ctx, tmp[--n]);
  }
}

static void out_int(out_ctx_t *ctx, int value) {
  if (value < 0) {
    out_char(ctx, '-');
    out_uint(ctx, (unsigned int)(-(value + 1)) + 1, 10, 0);
  } else {
    out_uint(ctx, (unsigned int)value, 10, 0);
  }
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
  out_ctx_t ctx = {.buf = str, .size = size, .len = 0};

  for (const char *p = format; *p != '\0'; p++) {
    if (*p != '%') {
      out_char(&ctx, *p);
      continue;
    }

    p++;
    if (*p == '\0') {
      break;
    }

    switch (*p) {
      case '%':
        out_char(&ctx, '%');
        break;
      case 'c':
        out_char(&ctx, (char)va_arg(ap, int));
        break;
      case 's':
        out_string(&ctx, va_arg(ap, const char *));
        break;
      case 'd':
        out_int(&ctx, va_arg(ap, int));
        break;
      case 'u':
        out_uint(&ctx, va_arg(ap, unsigned int), 10, 0);
        break;
      case 'x':
        out_uint(&ctx, va_arg(ap, unsigned int), 16, 0);
        break;
      case 'X':
        out_uint(&ctx, va_arg(ap, unsigned int), 16, 1);
        break;
      case 'p':
        out_string(&ctx, "0x");
        out_uint(&ctx, (unsigned int)(uintptr_t)va_arg(ap, void *), 16, 0);
        break;
      default:
        out_char(&ctx, '%');
        out_char(&ctx, *p);
        break;
    }
  }

  if (ctx.buf != NULL && ctx.size > 0) {
    size_t pos = (ctx.len < ctx.size - 1) ? ctx.len : ctx.size - 1;
    ctx.buf[pos] = '\0';
  }

  return (int)ctx.len;
}

int vsprintf(char *str, const char *format, va_list ap) {
  return vsnprintf(str, (size_t)-1, format, ap);
}

int snprintf(char *s, size_t n, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  int ret = vsnprintf(s, n, format, ap);
  va_end(ap);
  return ret;
}

int sprintf(char *out, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  int ret = vsnprintf(out, (size_t)-1, format, ap);
  va_end(ap);
  return ret;
}

int printf(const char *fmt, ...) {
  char buf[1024];
  va_list ap;
  va_start(ap, fmt);
  int ret = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  for (int i = 0; i < ret && buf[i] != '\0'; i++) {
    _putc(buf[i]);
  }
  return ret;
}

int sscanf(const char *str, const char *format, ...) {
  (void)str;
  (void)format;
  return 0;
}

void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *)) {
  unsigned char *arr = (unsigned char *)base;
  unsigned char tmp[256];

  if (size == 0 || size > sizeof(tmp)) {
    return;
  }

  for (size_t i = 0; i < nmemb; i++) {
    for (size_t j = i + 1; j < nmemb; j++) {
      unsigned char *a = arr + i * size;
      unsigned char *b = arr + j * size;
      if (compar(a, b) > 0) {
        memcpy(tmp, a, size);
        memcpy(a, b, size);
        memcpy(b, tmp, size);
      }
    }
  }
}
