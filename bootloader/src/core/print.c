#include <limits.h>

#include "loom/console.h"
#include "loom/print.h"

#define FLAG_LEFT_JUSTIFY (1 << 0)
#define FLAG_FORCE_SIGN   (1 << 1)
#define FLAG_BLANK_SIGN   (1 << 2)
#define FLAG_PREFIX       (1 << 3)
#define FLAG_PAD_ZEROES   (1 << 4)

#define LENGTH_NONE 0
#define LENGTH_HH   1
#define LENGTH_H    2
#define LENGTH_L    3
#define LENGTH_LL   4
#define LENGTH_J    5
#define LENGTH_Z    6
#define LENGTH_T    7

#define WBUF(LEN, S) (loom_write_buffer){ .len = (LEN), .s = (S), .splats = 1 }

typedef struct
{
  int valid;
  unsigned int value;
} precision_t;

typedef struct
{
  unsigned int base;
} format_t;

static void
flush (const char *fmt, loom_usize_t *run, loom_usize_t *len)
{
  loom_usize_t tmprun = *run;

  if (tmprun)
    {
      loom_usize_t tmplen = *len;

      if (tmplen > LOOM_USIZE_MAX - tmprun)
        *len = LOOM_USIZE_MAX;
      else
        *len = tmplen + tmprun;

      loom_con_write (*run, fmt - tmprun);

      *run = 0;
    }
}

static const char *
parse_flags (const char *fmt, unsigned int *flags)
{
  char ch;
  unsigned int tmp = 0;

read:
  ch = fmt[0];

  switch (ch)
    {
    case '-':
      tmp |= FLAG_LEFT_JUSTIFY;
      break;
    case '+':
      tmp |= FLAG_FORCE_SIGN;
      break;
    case ' ':
      tmp |= FLAG_BLANK_SIGN;
      break;
    case '#':
      tmp |= FLAG_PREFIX;
      break;
    case '0':
      tmp |= FLAG_PAD_ZEROES;
      break;
    default:
      *flags = tmp;
      return fmt;
    }

  ++fmt;
  goto read;
}

static const char *
parse_width (const char *fmt, unsigned int *flags, unsigned int *width,
             va_list *args)
{
  char ch;
  int tmpwidth = 0;

  ch = fmt[0];

  if (ch == '*')
    {
      int tmp = va_arg (*args, int);

      if (tmp < 0)
        {
          *flags |= FLAG_LEFT_JUSTIFY;

          if (tmp == INT_MIN)
            *width = ((unsigned int) INT_MAX) + 1;
          else
            *width = (unsigned int) (-tmp);
        }
      else
        *width = (unsigned int) tmp;

      return ++fmt;
    }

  while (ch >= '0' && ch <= '9')
    {
      if (tmpwidth < INT_MAX / 10 && (ch - '0') <= INT_MAX - tmpwidth * 10)
        tmpwidth = tmpwidth * 10 + (ch - '0');
      ch = (++fmt)[0];
    }

  *width = (unsigned int) tmpwidth;
  return fmt;
}

static const char *
parse_precision (const char *fmt, precision_t *prec, va_list *args)
{
  char ch;
  unsigned int tmpprec = 0;

  ch = fmt[0];

  if (ch != '.')
    {
      prec->valid = 0;
      return fmt;
    }

  ch = (++fmt)[0];

  if (ch == '*')
    {
      int tmp = va_arg (*args, int);

      if (tmp < 0)
        prec->valid = 0;
      else
        {
          prec->valid = 1;
          prec->value = (unsigned int) tmp;
        }

      return ++fmt;
    }

  while (ch >= '0' && ch <= '9')
    {
      if (tmpprec < INT_MAX / 10
          && (unsigned int) (ch - '0') <= INT_MAX - tmpprec * 10)
        tmpprec = tmpprec * 10 + (unsigned int) (ch - '0');
      ch = (++fmt)[0];
    }

  if (tmpprec > INT_MAX)
    loom_panic ("parse_precision");

  prec->valid = 1;
  prec->value = tmpprec;

  return fmt;
}

static const char *
parse_length (const char *fmt, unsigned int *length)
{
  char ch = fmt[0];

  switch (ch)
    {
    case 'h':
      ch = (++fmt)[0];

      if (ch == 'h')
        {
          *length = LENGTH_HH;
          ++fmt;
        }
      else
        *length = LENGTH_H;

      break;
    case 'l':
      ch = (++fmt)[0];

      if (ch == 'l')
        {
          *length = LENGTH_LL;
          ++fmt;
        }
      else
        *length = LENGTH_L;

      break;
    case 'j':
      *length = LENGTH_J;
      ++fmt;
      break;
    case 'z':
      *length = LENGTH_Z;
      ++fmt;
      break;
    case 't':
      *length = LENGTH_T;
      ++fmt;
      break;
    default:
      *length = LENGTH_NONE;
      break;
    }

  return fmt;
}

static void
printf_warn (unsigned int length, char spec)
{
  (void) length;
  (void) spec;
  // This will change so that the bootloader doesn't panic
  // on minor printf errors.
  // For now we panic to catch any issues during development.
  loom_panic ("Invalid format specifier.");
}

static int
printint (char ch, loom_usize_t cap, loom_write_buffer wbufs[], char nbuf[],
          va_list *args, unsigned int flags, unsigned int width,
          precision_t prec, unsigned int length)
{
  unsigned int base = 10, _signed = 0, capitals = 0;
  int retval = 0;
  loom_usize_t nlen = 0;
  loom_write_buffer wbuf;

  union
  {
    intmax_t i;
    uintmax_t u;
  } n;

  if (ch == 'd' || ch == 'i')
    _signed = 1;

  // Handle special casing for pointers.
  if (ch == 'p')
    {
      void *p = va_arg (*args, void *);
      n.u = (uintptr_t) p;

      if (p == NULL)
        {
          loom_wbufs_append (cap, wbufs, WBUF (5, "(nil)"));
          return 0;
        }

      base = 16;

      if (length != LENGTH_NONE)
        printf_warn (length, 'p');

      loom_wbufs_append (cap, wbufs, WBUF (2, "0x"));
      goto buffer_digits;
    }

  if (_signed)
    switch (length)
      {
      case LENGTH_NONE:
        n.i = va_arg (*args, int);
        break;
      case LENGTH_HH:
        n.i = (signed char) va_arg (*args, int);
        break;
      case LENGTH_H:
        n.i = (short int) va_arg (*args, int);
        break;
      case LENGTH_L:
        n.i = va_arg (*args, long int);
        break;
      case LENGTH_LL:
        n.i = va_arg (*args, long long int);
        break;
      case LENGTH_J:
        n.i = va_arg (*args, intmax_t);
        break;
      case LENGTH_Z:
        n.i = va_arg (*args, size_t);
        break;
      case LENGTH_T:
        n.i = va_arg (*args, ptrdiff_t);
        break;
      default:
        loom_panic ("printint");
      }
  else
    switch (length)
      {
      case LENGTH_NONE:
        n.u = va_arg (*args, unsigned int);
        break;
      case LENGTH_HH:
        n.u = (unsigned char) va_arg (*args, unsigned int);
        break;
      case LENGTH_H:
        n.u = (unsigned short int) va_arg (*args, unsigned int);
        break;
      case LENGTH_L:
        n.u = va_arg (*args, unsigned long int);
        break;
      case LENGTH_LL:
        n.u = va_arg (*args, unsigned long long int);
        break;
      case LENGTH_J:
        n.u = va_arg (*args, uintmax_t);
        break;
      case LENGTH_Z:
        n.u = va_arg (*args, size_t);
        break;
      case LENGTH_T:
        n.u = (uintmax_t) va_arg (*args, ptrdiff_t);
        break;
      default:
        loom_panic ("printint");
      }

  if (ch == 'b' || ch == 'B')
    {
      base = 2;
      capitals = ch == 'B';

      if (flags & FLAG_PREFIX && n.u)
        loom_wbufs_append (cap, wbufs, WBUF (2, capitals ? "0B" : "0b"));
    }

  if (ch == 'o')
    {
      base = 8;

      // Edge Case: always emit the prefix in the case of octal
      // even if the value is zero.
      if (flags & FLAG_PREFIX)
        loom_wbufs_append (cap, wbufs, WBUF (1, "0"));
    }

  if (ch == 'x' || ch == 'X')
    {
      base = 16;
      capitals = ch == 'X';

      if (flags & FLAG_PREFIX && n.u)
        loom_wbufs_append (cap, wbufs, WBUF (2, capitals ? "0X" : "0x"));
    }

  // Normalize to an unsigned int.
  if (_signed)
    {
      if (n.i < 0)
        {
          loom_wbufs_append (cap, wbufs, WBUF (1, "-"));

          if (n.i == INTMAX_MIN)
            n.u = ((uintmax_t) INTMAX_MAX) + 1;
          else
            n.u = (uintmax_t) -n.i;
        }
      else
        {
          n.u = (uintmax_t) n.i;

          if (flags & FLAG_FORCE_SIGN)
            loom_wbufs_append (cap, wbufs, WBUF (1, "+"));
          else if (flags & FLAG_BLANK_SIGN)
            loom_wbufs_append (cap, wbufs, WBUF (1, " "));
        }
    }

buffer_digits:

  if (base < 2 || base > 16)
    loom_panic ("printint");

  if (!n.u)
    {
      loom_usize_t splats = 1;

      if (prec.valid)
        {
          if (!prec.value)
            return 0;
          else
            splats = prec.value;
        }

      nlen = splats;
      wbuf = (loom_write_buffer) { .len = 1, .s = "0", .splats = splats };
      goto pad;
    }

  while (n.u)
    {
      uintmax_t rem = n.u % base;
      n.u /= base;
      nbuf[nlen++] = (capitals ? "0123456789ABCDEF" : "0123456789abcdef")[rem];
    }

  // Reverse the buffer to get forward ordering of the digits.
  for (unsigned int i = 0; i < nlen / 2; ++i)
    {
      unsigned int index = (nlen - 1) - i;
      char tmp = nbuf[i];
      nbuf[i] = nbuf[index];
      nbuf[index] = tmp;
    }

  if (prec.valid && prec.value > nlen)
    // Leading zeroes for precision go before number.
    loom_wbufs_append (cap, wbufs,
                       (loom_write_buffer) {
                           .len = 1, .s = "0", .splats = prec.value - nlen });

  wbuf = WBUF (nlen, nbuf);

pad:
  if (!prec.valid && flags & FLAG_PAD_ZEROES)
    {
      loom_usize_t tmplen = loom_wbufs_char_len (wbufs) + nlen;

      if (width > tmplen)
        loom_wbufs_append (cap, wbufs,
                           (loom_write_buffer) {
                               .len = 1, .s = "0", .splats = width - tmplen });
      retval = 1;
    }

  loom_wbufs_append (cap, wbufs, wbuf);
  return retval;
}

static const char *
print (const char *fmt, loom_usize_t *len, va_list *args, unsigned int flags,
       unsigned int width, precision_t prec, unsigned int length)
{
  loom_write_buffer wbufs[8] = { 0 };
  char ch = fmt[0], nbuf[sizeof (uintmax_t) * CHAR_BIT];
  loom_usize_t speclen = 0, cap = sizeof (wbufs) / sizeof (*wbufs);

  if (ch == 0)
    return fmt;

  if (ch == 'c')
    {
      ch = (char) va_arg (*args, int);

      if (length != LENGTH_NONE)
        printf_warn (length, 'c');

      loom_wbufs_append (cap, wbufs, WBUF (1, &ch));
      goto pad;
    }

  if (ch == 's')
    {
      const char *s = va_arg (*args, const char *);
      loom_usize_t slen = 0, max_slen = LOOM_USIZE_MAX;

      if (length != LENGTH_NONE)
        printf_warn (length, 's');

      if (prec.valid)
        max_slen = prec.value;

      for (; slen < max_slen && s[slen]; ++slen)
        ;

      loom_wbufs_append (cap, wbufs, WBUF (slen, s));
      goto pad;
    }

  if (ch == 'p' || ch == 'd' || ch == 'i' || ch == 'b' || ch == 'B'
      || ch == 'o' || ch == 'u' || ch == 'x' || ch == 'X')
    {
      if (printint (ch, cap, wbufs, nbuf, args, flags, width, prec, length))
        {
          speclen = loom_wbufs_char_len (wbufs);
          goto write;
        }
      else
        goto pad;
    }

  // Invalid specifier.
  printf_warn (length, ch);

  goto done;

pad:
  speclen = loom_wbufs_char_len (wbufs);

  if (width > speclen)
    {
      loom_write_buffer wbuf = (loom_write_buffer) {
        .len = 1, .s = " ", .splats = width - speclen
      };

      speclen = width;

      if (flags & FLAG_LEFT_JUSTIFY)
        loom_wbufs_append (cap, wbufs, wbuf);
      else
        loom_wbufs_prepend (cap, wbufs, wbuf);
    }

write:
  if (*len > LOOM_USIZE_MAX - speclen)
    *len = LOOM_USIZE_MAX;
  else
    *len = *len + speclen;

  loom_con_write_all (wbufs);

done:
  return ++fmt;
}

loom_usize_t
loom_vprintf (const char *fmt, va_list args)
{
  loom_usize_t len = 0, run = 0;
  char ch;

  unsigned int flags, width, length;
  precision_t prec = { 0 };

read:
  ch = fmt[0];

  if (ch != '%')
    {
      if (ch == 0)
        goto done;

      if (run < LOOM_USIZE_MAX)
        ++run;
      else
        flush (fmt, &run, &len);

      ++fmt;
      goto read;
    }

  ch = (++fmt)[0];

  if (ch == '%')
    {
      if (run < LOOM_USIZE_MAX)
        {
          ++run;
          flush (fmt - 1, &run, &len);
        }
      else
        {
          flush (fmt - 1, &run, &len);
          if (len < LOOM_USIZE_MAX)
            ++len;
          loom_con_write (1, "%");
        }

      ++fmt;
      goto read;
    }

  fmt = parse_flags (fmt, &flags);
  fmt = parse_width (fmt, &flags, &width, &args);
  fmt = parse_precision (fmt, &prec, &args);
  fmt = parse_length (fmt, &length);
  fmt = print (fmt, &len, &args, flags, width, prec, length);

  goto read;

done:
  flush (fmt, &run, &len);
  return len;
}

loom_usize_t
loom_printf (const char *fmt, ...)
{
  va_list args;
  loom_usize_t len;

  va_start (args, fmt);
  len = loom_vprintf (fmt, args);
  va_end (args);

  return len;
}
