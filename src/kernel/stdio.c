#include "stdio.h"
#include "stdint.h"
#include "stdarg.h"

#include "fb.h"

void printf(char *s, ...)
{
    va_list ap;
    char *p;
    uint32_t uival;
    char *sval;

    va_start(ap, s);
    for (p = s; *p != '\0'; ++p) {
        if (*p != '%') {
            fb_put_b(*p);
            continue;
        }

        switch (*++p) {
            case 'c':
                uival = va_arg(ap, uint32_t);
                fb_put_b((uint8_t) uival);
                break;
            case 'u':
                uival = va_arg(ap, uint32_t);
                fb_put_ui(uival);
                break;
            case 'X':
                uival = va_arg(ap, uint32_t);
                fb_put_ui_hex(uival);
                break;
            case 's':
                sval = va_arg(ap, char*);
                fb_put_s(sval);
                break;
            case '%':
                fb_put_b('%');
                break;
        }
    }

    va_end(ap);
}
