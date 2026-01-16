#ifndef LOOM_CRYPTO_SHA1_H
#define LOOM_CRYPTO_SHA1_H 1

#include "loom/compiler.h"
#include "loom/crypto/crypto.h"
#include "loom/types.h"

void EXPORT (loom_sha1_hash) (loom_usize_t length, const char *buf,
                              loom_digest_t digest[20]);

#endif