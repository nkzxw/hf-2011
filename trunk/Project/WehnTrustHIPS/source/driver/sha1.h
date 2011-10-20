#ifndef __SHA1_H
#define __SHA1_H

#define SHA1_HASH_SIZE	20

typedef struct sha1_ctx_s {
	ULONG	A;
	ULONG	B;
	ULONG	C;
	ULONG	D;
	ULONG	E;
	ULONG	bits0;
	ULONG	bits1;
	UCHAR	buffer[64];
} SHA1_CTX;

VOID SHA1_Init(SHA1_CTX *ctx);
VOID SHA1_Update(SHA1_CTX *ctx, UCHAR * input, ULONG length);
VOID SHA1_Final(SHA1_CTX *ctx, UCHAR digest[SHA1_HASH_SIZE]);

#endif /* __SHA1_H */

