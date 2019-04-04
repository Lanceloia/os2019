#include "klib.h"
#include <stdarg.h>

#include <stdbool.h>

#define ZEROPAD 1
#define SIGN 2

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

char digit[16]={
  '0','1','2','3',
  '4','5','6','7',
  '8','9','a','b',
  'c','d','e','f'};

char tmp[66];

char *number(char *str, long num, int base, int flags, int length){

  assert(base == 10 || base == 16);
  int i;

  char sign = '\0';  /* default */

  if (flags & SIGN){
    if (num < 0){
      num = -num;
      sign = '-';
    }
  }

  i = 0;

  if(num == 0){
    tmp[i++] = '0';
  }
  else{
    while(num != 0){
      tmp[i++] = digit[((unsigned long) num) % (unsigned) base];
      num = ((unsigned long) num) / (unsigned) base;
    }
  }

  if(sign != '\0')
    *str++ = sign;

  while(length > i){
    if(flags & ZEROPAD)
      *str++ = '0';
    else
      *str++ = ' ';
    length--;
  }
  while(i-- > 0)
    *str++ = tmp[i];

  return str;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  char *str;

  for(str = out; *fmt; fmt++){
    unsigned long num ;
    int base;
    int flags;
    int length;

    bool bFmt = true;
    if(*fmt != '%'){
      *str++ = *fmt;
      continue;
    }

    bFmt = true;
    while(bFmt){
      fmt++;  /* To skips first '%' */
      switch(*fmt){
        default: bFmt = false;
      }
    }

    /* Default base */
    base = 10;

    /* Mode */
    flags = 0;
Repeat_Mode:
    switch(*fmt) {
      case '0':
        flags |= ZEROPAD;
        fmt ++;
        goto Repeat_Mode;
    } 

    /* Length */
    length = 0;
Repeat_Length:
    switch(*fmt) {
      case'0':case'1':case'2':case'3':
      case'4':case'5':case'6':case'7':
      case'8':case'9':
        length = length * 10 + (*fmt-'0');
        fmt ++;
        goto Repeat_Length;
    }


    /* Type */
    switch(*fmt) {
      case 's':{
        char *s = va_arg(ap, char *);
        int str_len = strlen(s);
        int out_len = length > str_len ? length : str_len; 
        for(int i = 0; i < str_len; i ++)
          *str++ = *s++;
        for(int i = str_len; i < out_len; i ++)
          if(flags & ZEROPAD)
            *str++ = '0';
          else
            *str ++ = ' ';
        continue;
      }
      case 'x':{
        base = 16;
        break;
      }
      case 'u':{
        break;
      }
      case 'd':{
        flags |= SIGN;
        break;
      }
    }


    /* end switch*/

    /* number */
    num = va_arg(ap,int);

    str = number(str, num, base, flags, length);

  }  /* end for(str = out; *fmt; fmt++) */

  *str='\0';
  return str-out;
}

#define PRINTF_BUF_SIZE 512
static char printf_buf[PRINTF_BUF_SIZE];

int printf(const char *fmt, ...) {
  int n = 0;

  va_list ap;
  va_start(ap,fmt);
  n = vsprintf(printf_buf, fmt, ap);
  va_end(ap);

  assert(n<PRINTF_BUF_SIZE);

  for(int i = 0; i < PRINTF_BUF_SIZE && printf_buf[i]!='\0'; i ++)
    _putc(printf_buf[i]);

  return n;
}

int sprintf(char *out, const char *fmt, ...) {
  int n = 0;

  va_list ap;
  va_start(ap,fmt);
  n = vsprintf(out, fmt, ap);
  va_end(ap);

  return n;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  return 0;
}

#endif
