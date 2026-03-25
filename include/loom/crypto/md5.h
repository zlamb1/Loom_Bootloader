#ifndef LOOM_CRYPTO_MD5_H
#define LOOM_CRYPTO_MD5_H 1

#include "loom/compiler.h"
#include "loom/crypto/crypto.h"
#include "loom/types.h"

#define LOOM_MD5_DIGEST_SIZE 16

void export (loomMD5Hash) (usize length, const char *buf,
                           loom_digest digest[LOOM_MD5_DIGEST_SIZE]);

#endif