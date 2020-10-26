#ifndef EXT_TNT_H_INCLUDED
#define EXT_TNT_H_INCLUDED

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#if defined(__cplusplus)
extern "C" {
#endif /* defined(__cplusplus) */

int
mp_fprint_ext_tnt(FILE *file, const char **data, int depth);
int
mp_snprint_ext_tnt(char *buf, int size, const char **data, int depth);

#if defined(__cplusplus)
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* EXT_TNT_H_INCLUDED */
