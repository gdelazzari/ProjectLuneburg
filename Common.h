#ifndef _COMMON_H
#define _COMMON_H


#define ABS(x)                      (((x) > 0) ? (x) : (-(x)))

#define TOLOWER(c)                  ((c >= 'A' && c <= 'Z') ? ('a' + (c - 'A')) : (c))
#define TOUPPER(c)                  ((c >= 'a' && c <= 'z') ? ('A' + (c - 'a')) : (c))

#define ISLOWER(c)                  ((c >= 'a') && (c <= 'z'))
#define ISUPPER(c)                  ((c >= 'A') && (c <= 'Z'))

#define ISFIELD(x, y)               (((x >= 0) && (x < 8)) && ((y >= 0) && (y < 8)))

#define MAT(m, x, y)                (m[y][x + 2])


#endif
