#ifndef EXT_TNT_H_INCLUDED
#define EXT_TNT_H_INCLUDED

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

int
mp_fprint_ext_tnt(FILE *file, const char **data, int depth);
int
mp_snprint_ext_tnt(char *buf, int size, const char **data, int depth);

#endif /* EXT_TNT_H_INCLUDED */
