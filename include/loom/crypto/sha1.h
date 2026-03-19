#ifndef LOOM_CRYPTO_SHA1_H
#define LOOM_CRYPTO_SHA1_H 1

#include "loom/compiler.h"
#include "loom/crypto/crypto.h"
#include "loom/types.h"

#define LOOM_SHA1_DIGEST_SIZE 20

void export (loomSHA1Hash) (usize length, const char *buf,
                            loom_digest digest[LOOM_SHA1_DIGEST_SIZE]);

#endif