/*
 *  FIPS-180-1 compliant SHA-1 algorithm
 *
 *  - optyx
 */
#include <windows.h>
#include <winsock.h>
#include "sha1.h"

#define rol(x,n) ((x << n) | ((x) >> (32 - n)))

#define V(n) (					\
	tmp = W[(n - 3) & 0xf] ^ W[(n - 8) & 0xf] \
           ^ W[(n - 14) & 0xf] ^ W[(n) & 0xf],	\
	(W[n & 0xf] = rol(tmp,1))		\
)

#define R(a,b,c,d,e,f,k,x) {			\
	e += rol(a,5) + f(b,c,d) + k + x;	\
	b = rol(b,30);				\
}

#define K1 0x5a827999
#define K2 0x6ed9eba1
#define K3 0x8f1bbcdc
#define K4 0xca62c1d6

#define F1(x,y,z) (z ^ (x & (y ^ z)))
#define F2(x,y,z) (x ^ y ^ z)
#define F3(x,y,z) ((x & y) | (z & (x | y)))
#define F4(x,y,z) (x ^ y ^ z)

VOID SHA1_Init(SHA1_CTX * ctx)
{
	ctx->A = 0x67452301;
	ctx->B = 0xefcdab89;
	ctx->C = 0x98badcfe;
	ctx->D = 0x10325476;
	ctx->E = 0xc3d2e1f0;

	ctx->bits0 = 0;
	ctx->bits1 = 0;
	return;
}

VOID SHA1_Transform(SHA1_CTX * ctx, UCHAR data[64])
{
	ULONG tmp, W[16], A, B, C, D, E;

	W[ 0] = htonl(*(ULONG *) &data[ 0]);
	W[ 1] = htonl(*(ULONG *) &data[ 4]);
	W[ 2] = htonl(*(ULONG *) &data[ 8]);
	W[ 3] = htonl(*(ULONG *) &data[12]);
	W[ 4] = htonl(*(ULONG *) &data[16]);
	W[ 5] = htonl(*(ULONG *) &data[20]);
	W[ 6] = htonl(*(ULONG *) &data[24]);
	W[ 7] = htonl(*(ULONG *) &data[28]);
	W[ 8] = htonl(*(ULONG *) &data[32]);
	W[ 9] = htonl(*(ULONG *) &data[36]);
	W[10] = htonl(*(ULONG *) &data[40]);
	W[11] = htonl(*(ULONG *) &data[44]);
	W[12] = htonl(*(ULONG *) &data[48]);
	W[13] = htonl(*(ULONG *) &data[52]);
	W[14] = htonl(*(ULONG *) &data[56]);
	W[15] = htonl(*(ULONG *) &data[60]);

	A = ctx->A;
	B = ctx->B;
	C = ctx->C;
	D = ctx->D;
	E = ctx->E;

	/* round 1 */
	R(A, B, C, D, E, F1, K1, W[ 0]);
	R(E, A, B, C, D, F1, K1, W[ 1]);
	R(D, E, A, B, C, F1, K1, W[ 2]);
	R(C, D, E, A, B, F1, K1, W[ 3]);
	R(B, C, D, E, A, F1, K1, W[ 4]);
	R(A, B, C, D, E, F1, K1, W[ 5]);
	R(E, A, B, C, D, F1, K1, W[ 6]);
	R(D, E, A, B, C, F1, K1, W[ 7]);
	R(C, D, E, A, B, F1, K1, W[ 8]);
	R(B, C, D, E, A, F1, K1, W[ 9]);
	R(A, B, C, D, E, F1, K1, W[10]);
	R(E, A, B, C, D, F1, K1, W[11]);
	R(D, E, A, B, C, F1, K1, W[12]);
	R(C, D, E, A, B, F1, K1, W[13]);
	R(B, C, D, E, A, F1, K1, W[14]);
	R(A, B, C, D, E, F1, K1, W[15]);
	R(E, A, B, C, D, F1, K1, V(16));
	R(D, E, A, B, C, F1, K1, V(17));
	R(C, D, E, A, B, F1, K1, V(18));
	R(B, C, D, E, A, F1, K1, V(19));

	/* round 2 */
	R(A, B, C, D, E, F2, K2, V(20));
	R(E, A, B, C, D, F2, K2, V(21));
	R(D, E, A, B, C, F2, K2, V(22));
	R(C, D, E, A, B, F2, K2, V(23));
	R(B, C, D, E, A, F2, K2, V(24));
	R(A, B, C, D, E, F2, K2, V(25));
	R(E, A, B, C, D, F2, K2, V(26));
	R(D, E, A, B, C, F2, K2, V(27));
	R(C, D, E, A, B, F2, K2, V(28));
	R(B, C, D, E, A, F2, K2, V(29));
	R(A, B, C, D, E, F2, K2, V(30));
	R(E, A, B, C, D, F2, K2, V(31));
	R(D, E, A, B, C, F2, K2, V(32));
	R(C, D, E, A, B, F2, K2, V(33));
	R(B, C, D, E, A, F2, K2, V(34));
	R(A, B, C, D, E, F2, K2, V(35));
	R(E, A, B, C, D, F2, K2, V(36));
	R(D, E, A, B, C, F2, K2, V(37));
	R(C, D, E, A, B, F2, K2, V(38));
	R(B, C, D, E, A, F2, K2, V(39));

	/* round 3 */
	R(A, B, C, D, E, F3, K3, V(40));
	R(E, A, B, C, D, F3, K3, V(41));
	R(D, E, A, B, C, F3, K3, V(42));
	R(C, D, E, A, B, F3, K3, V(43));
	R(B, C, D, E, A, F3, K3, V(44));
	R(A, B, C, D, E, F3, K3, V(45));
	R(E, A, B, C, D, F3, K3, V(46));
	R(D, E, A, B, C, F3, K3, V(47));
	R(C, D, E, A, B, F3, K3, V(48));
	R(B, C, D, E, A, F3, K3, V(49));
	R(A, B, C, D, E, F3, K3, V(50));
	R(E, A, B, C, D, F3, K3, V(51));
	R(D, E, A, B, C, F3, K3, V(52));
	R(C, D, E, A, B, F3, K3, V(53));
	R(B, C, D, E, A, F3, K3, V(54));
	R(A, B, C, D, E, F3, K3, V(55));
	R(E, A, B, C, D, F3, K3, V(56));
	R(D, E, A, B, C, F3, K3, V(57));
	R(C, D, E, A, B, F3, K3, V(58));
	R(B, C, D, E, A, F3, K3, V(59));

	/* round 4 */
	R(A, B, C, D, E, F4, K4, V(60));
	R(E, A, B, C, D, F4, K4, V(61));
	R(D, E, A, B, C, F4, K4, V(62));
	R(C, D, E, A, B, F4, K4, V(63));
	R(B, C, D, E, A, F4, K4, V(64));
	R(A, B, C, D, E, F4, K4, V(65));
	R(E, A, B, C, D, F4, K4, V(66));
	R(D, E, A, B, C, F4, K4, V(67));
	R(C, D, E, A, B, F4, K4, V(68));
	R(B, C, D, E, A, F4, K4, V(69));
	R(A, B, C, D, E, F4, K4, V(70));
	R(E, A, B, C, D, F4, K4, V(71));
	R(D, E, A, B, C, F4, K4, V(72));
	R(C, D, E, A, B, F4, K4, V(73));
	R(B, C, D, E, A, F4, K4, V(74));
	R(A, B, C, D, E, F4, K4, V(75));
	R(E, A, B, C, D, F4, K4, V(76));
	R(D, E, A, B, C, F4, K4, V(77));
	R(C, D, E, A, B, F4, K4, V(78));
	R(B, C, D, E, A, F4, K4, V(79));

	ctx->A += A;
	ctx->B += B;
	ctx->C += C;
	ctx->D += D;
	ctx->E += E;
	return;
}

VOID SHA1_Update(SHA1_CTX *ctx, UCHAR *input, ULONG length)
{
	ULONG pos, left;

	if(length == 0)
		return;

	pos = ctx->bits0 & 0x3F;
	left = 64 - pos;

	ctx->bits0 += length;

	if(ctx->bits0 < length)
		ctx->bits1++;

	if(left)
	{
		if(left > length)
		{
			memcpy(&ctx->buffer[pos], input, length);
			return;
		}
		memcpy(&ctx->buffer[pos], input, left);
		length -= left;
		input += left;
		SHA1_Transform(ctx, ctx->buffer);
	}

	while(length >= 64)
	{
		SHA1_Transform(ctx, input);
		length -= 64;
		input += 64;
	}

	if(length)
		memcpy(ctx->buffer, input, length);
	return;
}

static UCHAR sha1_padding[64] =
	"\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

VOID SHA1_Final(SHA1_CTX * ctx, UCHAR digest[20])
{
	ULONG last, pad;
	UCHAR msglen[8];

	*(ULONG *) &msglen[0] = ntohl(((ctx->bits0 >> 29)
		| (ctx->bits1 << 3)));
	*(ULONG *) &msglen[4] = ntohl((ctx->bits0 << 3));

	last = ctx->bits0 & 0x3F;
	pad = (last < 56) ? (56 - last) : (120 - last);

	SHA1_Update(ctx, sha1_padding, pad);
	SHA1_Update(ctx, msglen, 8);

	*(ULONG *) &digest[ 0] = ntohl(ctx->A);
	*(ULONG *) &digest[ 4] = ntohl(ctx->B);
	*(ULONG *) &digest[ 8] = ntohl(ctx->C);
	*(ULONG *) &digest[12] = ntohl(ctx->D);
	*(ULONG *) &digest[16] = ntohl(ctx->E);
	return;
}

