#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#include <stdarg.h>

static char digits[] = "0123456789ABCDEF";

static void
flushbuf(int fd, char *buf, int *p)
{
  if(*p > 0){
    write(fd, buf, *p);
    *p = 0;
  }
}

static void
bufputc(int fd, char *buf, int *p, char c)
{
  buf[*p] = c;
  *p += 1;
  if(*p >= 1024)
    flushbuf(fd, buf, p);
}

static void
printint(int fd, char *buf, int *p, long long xx, int base, int sgn)
{
  char num[20];
  int i, neg;
  unsigned long long x;

  neg = 0;
  if(sgn && xx < 0){
    neg = 1;
    x = -xx;
  } else {
    x = xx;
  }

  i = 0;
  do{
    num[i++] = digits[x % base];
  }while((x /= base) != 0);
  if(neg)
    num[i++] = '-';

  while(--i >= 0)
    bufputc(fd, buf, p, num[i]);
}

static void
printptr(int fd, char *buf, int *p, uint64 x) {
  int i;
  bufputc(fd, buf, p, '0');
  bufputc(fd, buf, p, 'x');
  for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
    bufputc(fd, buf, p, digits[x >> (sizeof(uint64) * 8 - 4)]);
}

// Print to the given fd. Only understands %d, %x, %p, %c, %s.
void
vprintf(int fd, const char *fmt, va_list ap)
{
  char *s;
  int c0, c1, c2, i, state;
  char buf[1024];
  int p = 0;

  state = 0;
  for(i = 0; fmt[i]; i++){
    c0 = fmt[i] & 0xff;
    if(state == 0){
      if(c0 == '%'){
        state = '%';
      } else {
        bufputc(fd, buf, &p, c0);
      }
    } else if(state == '%'){
      c1 = c2 = 0;
      if(c0) c1 = fmt[i+1] & 0xff;
      if(c1) c2 = fmt[i+2] & 0xff;
      if(c0 == 'd'){
        printint(fd, buf, &p, va_arg(ap, int), 10, 1);
      } else if(c0 == 'l' && c1 == 'd'){
        printint(fd, buf, &p, va_arg(ap, uint64), 10, 1);
        i += 1;
      } else if(c0 == 'l' && c1 == 'l' && c2 == 'd'){
        printint(fd, buf, &p, va_arg(ap, uint64), 10, 1);
        i += 2;
      } else if(c0 == 'u'){
        printint(fd, buf, &p, va_arg(ap, uint32), 10, 0);
      } else if(c0 == 'l' && c1 == 'u'){
        printint(fd, buf, &p, va_arg(ap, uint64), 10, 0);
        i += 1;
      } else if(c0 == 'l' && c1 == 'l' && c2 == 'u'){
        printint(fd, buf, &p, va_arg(ap, uint64), 10, 0);
        i += 2;
      } else if(c0 == 'x'){
        printint(fd, buf, &p, va_arg(ap, uint32), 16, 0);
      } else if(c0 == 'l' && c1 == 'x'){
        printint(fd, buf, &p, va_arg(ap, uint64), 16, 0);
        i += 1;
      } else if(c0 == 'l' && c1 == 'l' && c2 == 'x'){
        printint(fd, buf, &p, va_arg(ap, uint64), 16, 0);
        i += 2;
      } else if(c0 == 'p'){
        printptr(fd, buf, &p, va_arg(ap, uint64));
      } else if(c0 == 'c'){
        bufputc(fd, buf, &p, va_arg(ap, uint32));
      } else if(c0 == 's'){
        if((s = va_arg(ap, char*)) == 0)
          s = "(null)";
        for(; *s; s++)
          bufputc(fd, buf, &p, *s);
      } else if(c0 == '%'){
        bufputc(fd, buf, &p, '%');
      } else {
        // Unknown % sequence.  Print it to draw attention.
        bufputc(fd, buf, &p, '%');
        bufputc(fd, buf, &p, c0);
      }

      state = 0;
    }
  }

  flushbuf(fd, buf, &p);
}

void
fprintf(int fd, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vprintf(fd, fmt, ap);
}

void
printf(const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vprintf(1, fmt, ap);
}
