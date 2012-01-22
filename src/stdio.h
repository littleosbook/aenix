#ifndef STDIO_H
#define STDIO_H

/** 
 * Prints a formatted string to the framebuffer.
 * The current supported types are:
 *  - \c %u: Prints an \c uint32_t or an \c uint8_t
 *  - \c %X: Prints an \c uint32_t as a hexadecimal number in capital letters
 *  - \c %s: Prints a \c char*
 *  - \c %: Prints the character \c %
 * 
 * @param fmt The format string describing how the argument should be printed.
 * @param ... A variadic argument list with one argument for each description
 */
void printf(char *fmt, ...);

#endif /* STDIO_H */
