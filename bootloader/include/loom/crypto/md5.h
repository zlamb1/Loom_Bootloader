#ifndef LOOM_MD5_H
#define LOOM_MD5_H 1

#include "loom/compiler.h"
#include "loom/types.h"

void EXPORT (loom_md5_hash) (loom_usize_t length, const char *buf,
                             char digest[16]);

#endif