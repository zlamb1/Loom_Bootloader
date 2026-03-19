#include <limits.h>

#include "loom/console.h"
#include "loom/math.h"
#include "loom/print.h"

#define FLAG_LEFT_JUSTIFY (1u << 0)
#define FLAG_FORCE_SIGN   (1u << 1)
#define FLAG_BLANK_SIGN   (1u << 2)
#define FLAG_PREFIX       (1u << 3)
#define FLAG_PAD_ZEROES   (1u << 4)

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
} precision;

typedef struct
{
  unsigned int base;
} format;

static void
write (loom_writer w, usize len, const char *buf)
{
  if (w.write != null)
    w.write (
        (loom_write_buffer[]) {
            { .len = len, .splats = 1, .s = buf },
            { 0 },
        },
        w.data);
}

static void
flush (loom_writer w, const char *fmt, usize *run, usize *len)
{
  usize tmprun = *run;

  if (tmprun)
    {
      usize tmplen = *len;

      if (loomAdd (tmplen, tmprun, len))
        *len = USIZE_MAX;

      write (w, *run, fmt - tmprun);

      *run = 0;
    }
}

static const char *
parseFlags (const char *fmt, unsigned int *flags)
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
      if (tmp & FLAG_FORCE_SIGN)
        tmp &= ~FLAG_BLANK_SIGN;

      if (tmp & FLAG_LEFT_JUSTIFY)
        tmp &= ~FLAG_PAD_ZEROES;

      *flags = tmp;
      return fmt;
    }

  ++fmt;
  goto read;
}

static const char *
parseWidth (const char *fmt, unsigned int *flags, unsigned int *width,
            va_list *args)
{
  char ch;
  int tmpwidth = 0;

  ch = fmt[0];

  if (ch == '*')
    {
      tmpwidth = va_arg (*args, int);

      if (tmpwidth < 0)
        {
          *flags |= FLAG_LEFT_JUSTIFY;

          if (loomMul (tmpwidth, -1, &tmpwidth))
            tmpwidth = INT_MAX;

          *width = (unsigned int) tmpwidth;
        }
      else
        *width = (unsigned int) tmpwidth;

      return ++fmt;
    }

  while (ch >= '0' && ch <= '9')
    {
      if (loomMul (tmpwidth, 10, &tmpwidth)
          || loomAdd (tmpwidth, ch - '0', &tmpwidth))
        tmpwidth = INT_MAX;

      ch = (++fmt)[0];
    }

  *width = (unsigned int) tmpwidth;
  return fmt;
}

static const char *
parsePrecision (const char *fmt, precision *prec, va_list *args)
{
  char ch;
  int tmpprec = 0;

  ch = fmt[0];

  if (ch != '.')
    {
      prec->valid = 0;
      return fmt;
    }

  ch = (++fmt)[0];

  if (ch == '*')
    {
      tmpprec = va_arg (*args, int);

      if (tmpprec < 0)
        prec->valid = 0;
      else
        {
          prec->valid = 1;
          prec->value = (uint) tmpprec;
        }

      return ++fmt;
    }

  while (ch >= '0' && ch <= '9')
    {
      if (loomMul (tmpprec, 10, &tmpprec)
          || loomAdd (tmpprec, ch - '0', &tmpprec))
        tmpprec = INT_MAX;

      ch = (++fmt)[0];
    }

  prec->valid = 1;
  prec->value = (uint) tmpprec;

  return fmt;
}

static const char *
parseLength (const char *fmt, unsigned int *length)
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
printfWarn (unsigned int length, char spec)
{
  (void) length;
  (void) spec;
  // This will change so that the bootloader doesn't panic
  // on minor printf errors.
  // For now we panic to catch any issues during development.
  loomPanic ("Invalid format specifier.");
}

static int
printInt (char ch, usize cap, loom_write_buffer wbufs[], char nbuf[],
          va_list *args, unsigned int flags, unsigned int width,
          precision prec, unsigned int length)
{
  unsigned int base = 10, _signed = 0, capitals = 0;
  int retval = 0;
  usize nlen = 0;
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
          loomWbufsAppend (cap, wbufs, WBUF (5, "(nil)"));
          return 0;
        }

      base = 16;

      if (length != LENGTH_NONE)
        printfWarn (length, 'p');

      loomWbufsAppend (cap, wbufs, WBUF (2, "0x"));
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
        loomPanic ("printInt");
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
        loomPanic ("printInt");
      }

  if (ch == 'b' || ch == 'B')
    {
      base = 2;
      capitals = ch == 'B';

      if (flags & FLAG_PREFIX && n.u)
        loomWbufsAppend (cap, wbufs, WBUF (2, capitals ? "0B" : "0b"));
    }

  if (ch == 'o')
    {
      base = 8;

      // Edge Case: always emit the prefix in the case of octal
      // even if the value is zero.
      if (flags & FLAG_PREFIX)
        loomWbufsAppend (cap, wbufs, WBUF (1, "0"));
    }

  if (ch == 'x' || ch == 'X')
    {
      base = 16;
      capitals = ch == 'X';

      if (flags & FLAG_PREFIX && n.u)
        loomWbufsAppend (cap, wbufs, WBUF (2, capitals ? "0X" : "0x"));
    }

  // Normalize to an unsigned int.
  if (_signed)
    {
      if (n.i < 0)
        {
          loomWbufsAppend (cap, wbufs, WBUF (1, "-"));

          if (n.i == INTMAX_MIN)
            n.u = ((uintmax_t) INTMAX_MAX) + 1;
          else
            n.u = (uintmax_t) -n.i;
        }
      else
        {
          n.u = (uintmax_t) n.i;

          if (flags & FLAG_FORCE_SIGN)
            loomWbufsAppend (cap, wbufs, WBUF (1, "+"));
          else if (flags & FLAG_BLANK_SIGN)
            loomWbufsAppend (cap, wbufs, WBUF (1, " "));
        }
    }

buffer_digits:

  if (base < 2 || base > 16)
    loomPanic ("printInt");

  if (!n.u)
    {
      usize splats = 1;

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
    loomWbufsAppend (cap, wbufs,
                     (loom_write_buffer) {
                         .len = 1, .s = "0", .splats = prec.value - nlen });

  wbuf = WBUF (nlen, nbuf);

pad:
  if (!prec.valid && flags & FLAG_PAD_ZEROES)
    {
      usize tmplen = loomWbufsCharLen (wbufs) + nlen;

      if (width > tmplen)
        loomWbufsAppend (cap, wbufs,
                         (loom_write_buffer) {
                             .len = 1, .s = "0", .splats = width - tmplen });
      retval = 1;
    }

  loomWbufsAppend (cap, wbufs, wbuf);
  return retval;
}

static const char *
basePrint (loom_writer w, const char *fmt, usize *len, va_list *args,
           unsigned int flags, unsigned int width, precision prec,
           unsigned int length)
{
  loom_write_buffer wbufs[8] = { 0 };
  char ch = fmt[0], nbuf[sizeof (uintmax_t) * CHAR_BIT];
  usize speclen = 0, cap = sizeof (wbufs) / sizeof (*wbufs);

  if (ch == 0)
    return fmt;

  if (ch == 'c')
    {
      ch = (char) va_arg (*args, int);

      if (length != LENGTH_NONE)
        printfWarn (length, 'c');

      loomWbufsAppend (cap, wbufs, WBUF (1, &ch));
      goto pad;
    }

  if (ch == 's')
    {
      const char *s = va_arg (*args, const char *);
      usize slen = 0, max_slen = USIZE_MAX;

      if (length != LENGTH_NONE)
        printfWarn (length, 's');

      if (s == NULL)
        {
          loomWbufsAppend (cap, wbufs, WBUF (6, "(null)"));
          goto pad;
        }

      if (prec.valid)
        max_slen = prec.value;

      for (; slen < max_slen && s[slen]; ++slen)
        ;

      loomWbufsAppend (cap, wbufs, WBUF (slen, s));
      goto pad;
    }

  if (ch == 'p' || ch == 'd' || ch == 'i' || ch == 'b' || ch == 'B'
      || ch == 'o' || ch == 'u' || ch == 'x' || ch == 'X')
    {
      if (printInt (ch, cap, wbufs, nbuf, args, flags, width, prec, length))
        {
          speclen = loomWbufsCharLen (wbufs);
          goto write;
        }
      else
        goto pad;
    }

  // Invalid specifier.
  printfWarn (length, ch);

  goto done;

pad:
  speclen = loomWbufsCharLen (wbufs);

  if (width > speclen)
    {
      loom_write_buffer wbuf = (loom_write_buffer) {
        .len = 1, .s = " ", .splats = width - speclen
      };

      speclen = width;

      if (flags & FLAG_LEFT_JUSTIFY)
        loomWbufsAppend (cap, wbufs, wbuf);
      else
        loomWbufsPrepend (cap, wbufs, wbuf);
    }

write:
  if (*len > USIZE_MAX - speclen)
    *len = USIZE_MAX;
  else
    *len = *len + speclen;

  if (w.write != null)
    w.write (wbufs, w.data);

done:
  return ++fmt;
}

usize
loomWriterFormatV (loom_writer w, const char *fmt, va_list args)
{
  usize len = 0, run = 0;
  char ch;

  unsigned int flags, width, length;
  precision prec = { 0 };

read:
  ch = fmt[0];

  if (ch != '%')
    {
      if (ch == 0)
        goto done;

      if (run < USIZE_MAX)
        ++run;
      else
        flush (w, fmt, &run, &len);

      ++fmt;
      goto read;
    }

  ch = (++fmt)[0];

  if (ch == '%')
    {
      if (run < USIZE_MAX)
        {
          ++run;
          flush (w, fmt - 1, &run, &len);
        }
      else
        {
          flush (w, fmt - 1, &run, &len);
          if (len < USIZE_MAX)
            ++len;
          write (w, 1, "%");
        }

      ++fmt;
      goto read;
    }

  flush (w, fmt - 1, &run, &len);

  fmt = parseFlags (fmt, &flags);
  fmt = parseWidth (fmt, &flags, &width, &args);
  fmt = parsePrecision (fmt, &prec, &args);
  fmt = parseLength (fmt, &length);
  fmt = basePrint (w, fmt, &len, &args, flags, width, prec, length);

  goto read;

done:
  flush (w, fmt, &run, &len);
  return len;
}

usize
loomWriterFormat (loom_writer w, const char *fmt, ...)
{
  usize length;
  va_list args;
  va_start (args, fmt);
  length = loomWriterFormatV (w, fmt, args);
  va_end (args);
  return length;
}

typedef struct
{
  usize length;
  char *s;
} write_buffer_ctx;

static void
writeBuffer (loom_write_buffer wbufs[], void *p)
{
  loom_write_buffer wbuf;
  write_buffer_ctx *ctx = (write_buffer_ctx *) p;

  for (int i = 0; wbufs[i].s != NULL; ++i)
    {
      wbuf = wbufs[i];

      for (usize j = 0; j < wbuf.splats; ++j)
        for (usize k = 0; k < wbuf.len; ++k)
          {
            if (!ctx->length)
              return;

            *ctx->s++ = wbuf.s[k];
            --ctx->length;
          }
    }
}

usize
loomBufferFormatV (char *s, usize n, const char *fmt, va_list args)
{
  usize length;
  write_buffer_ctx ctx = {
    .length = n ? n - 1 : 0,
    .s = s,
  };

  loom_writer w = { .write = writeBuffer, .data = &ctx };

  length = loomWriterFormatV (w, fmt, args);

  if (ctx.length >= n)
    loomPanic ("loomWriterFormat");

  if (n)
    s[n - ctx.length - 1] = 0;

  return length;
}

usize
loomBufferFormat (char *s, usize n, const char *fmt, ...)
{
  va_list args;
  usize length;
  va_start (args, fmt);
  length = loomBufferFormatV (s, n, fmt, args);
  va_end (args);
  return length;
}

static void
writeConsoles (loom_write_buffer wbufs[], unused void *data)
{
  loomConsolesWriteAll (wbufs);
}

usize
loomLogV (const char *fmt, va_list args)
{
  loom_writer w = { .write = writeConsoles };
  return loomWriterFormatV (w, fmt, args);
}

usize
loomLog (const char *fmt, ...)
{
  va_list args;
  usize len;
  va_start (args, fmt);
  len = loomLogV (fmt, args);
  va_end (args);
  return len;
}

usize
loomLogLnV (const char *fmt, va_list args)
{
  loom_writer w = { .write = writeConsoles };
  return loomWriterFormatV (w, fmt, args) + loomWriterFormat (w, "\n");
}

usize
loomLogLn (const char *fmt, ...)
{
  va_list args;
  usize len;
  va_start (args, fmt);
  len = loomLogLnV (fmt, args);
  va_end (args);
  return len;
}
