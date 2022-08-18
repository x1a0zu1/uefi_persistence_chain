#ifndef __RAMIEL_LIB_H__
#define __RAMIEL_LIB_H__

void EFIAPI init_iv_ctr(unsigned char *iv_ptr);
void EFIAPI init_rand_iv_ctr();
void EFIAPI init_state_ctr();
void EFIAPI init_key_ctr(unsigned char *key);
void EFIAPI encrypt_block_ctr(unsigned char *in, unsigned char *out, UINTN len);
#endif
