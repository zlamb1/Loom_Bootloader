#ifndef LOOM_CRYPTO_MD5_H
#define LOOM_CRYPTO_MD5_H 1

#include "loom/compiler.h"
#include "loom/crypto/crypto.h"
#include "loom/types.h"

void LOOM_EXPORT (loom_md5_hash) (loom_usize_t length, const char *buf,
                                  loom_digest_t digest[16]);

#endif