#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/RamielLib.h>
#include <openssl/aes.h>
#include <openssl/modes.h>
#include <openssl/rand.h>

#include "CrtLibSupport.h"

#include <openssl/opensslv.h>

#if OPENSSL_VERSION_NUMBER < 0x10100000L
#define OBJ_get0_data(o)  ((o)->data)
#define OBJ_length(o)     ((o)->length)
#endif

// IMPORTANT: use EVP_ for aes-ni because currently encryption is so fucking slow

struct ctr_state {
    unsigned char ivec[16];
    unsigned int num;
    unsigned char ecount[16];
};

static AES_KEY aes_key;
static unsigned char iv[8];
static struct ctr_state state;

void EFIAPI init_iv_ctr(unsigned char *iv_ptr) {
    CopyMem(iv, iv_ptr, 8);
}

void EFIAPI init_rand_iv_ctr() {
    RAND_bytes(iv, 8);
}

void EFIAPI init_state_ctr() {
    state.num = 0;
    SetMem(state.ecount, 0, 16);
    SetMem(state.ivec + 8, 0, 8);
    CopyMem(state.ivec, iv, 8);
}

void EFIAPI init_key_ctr(unsigned char *key) {
    AES_set_encrypt_key(key, 128, &aes_key);
}


void EFIAPI encrypt_block_ctr(unsigned char *in, unsigned char *out, UINTN len) {
    CRYPTO_ctr128_encrypt(in, out, len, &aes_key, state.ivec, state.ecount, &(state.num), (block128_f) AES_encrypt);
}
