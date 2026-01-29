#ifndef LOOM_CRYPTO_SHA1_H
#define LOOM_CRYPTO_SHA1_H 1

#include "loom/compiler.h"
#include "loom/crypto/crypto.h"
#include "loom/types.h"

#define LOOM_SHA1_DIGEST_SIZE 20

void EXPORT (loom_sha1_hash) (loom_usize_t length, const char *buf,
                              loom_digest_t digest[LOOM_SHA1_DIGEST_SIZE]);

#endif