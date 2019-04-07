#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "astc.h"

static const int BitReverseTable[] = {
	0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
	0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
	0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
	0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
	0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
	0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
	0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
	0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
	0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
	0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
	0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
	0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
	0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
	0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
	0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
	0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

static int WeightPrecTableA[] = { 0, 0, 0, 3, 0, 5, 3, 0, 0, 0, 5, 3, 0, 5, 3, 0 };
static int WeightPrecTableB[] = { 0, 0, 1, 0, 2, 0, 1, 3, 0, 0, 1, 2, 4, 2, 3, 5 };

static int CemTableA[] = { 0, 3, 5, 0, 3, 5, 0, 3, 5, 0, 3, 5, 0, 3, 5, 0, 3, 0, 0 };
static int CemTableB[] = { 8, 6, 5, 7, 5, 4, 6, 4, 3, 5, 3, 2, 4, 2, 1, 3, 1, 2, 1 };

static inline uint_fast32_t color(uint_fast32_t r, uint_fast32_t g, uint_fast32_t b, uint_fast32_t a) {
	return r | g << 8 | b << 16 | a << 24;
}

static inline uint_fast8_t bit_reverse_u8(const uint_fast8_t c, const int bits) {
	return BitReverseTable[c] >> (8 - bits);
}

static inline uint_fast64_t bit_reverse_u64(const uint_fast64_t d, const int bits) {
	uint_fast64_t ret =
		(uint_fast64_t)BitReverseTable[d & 0xff] << 56 |
		(uint_fast64_t)BitReverseTable[d >> 8 & 0xff] << 48 |
		(uint_fast64_t)BitReverseTable[d >> 16 & 0xff] << 40 |
		(uint_fast64_t)BitReverseTable[d >> 24 & 0xff] << 32 |
		(uint_fast32_t)BitReverseTable[d >> 32 & 0xff] << 24 |
		(uint_fast32_t)BitReverseTable[d >> 40 & 0xff] << 16 |
		(uint_fast16_t)BitReverseTable[d >> 48 & 0xff] << 8 | BitReverseTable[d >> 56 & 0xff];
	return ret >> (64 - bits);
}

static inline int getbits(const uint8_t *buf, const int bit, const int len) {
	return (*(int*)(buf + bit / 8) >> (bit % 8)) & ((1 << len) - 1);
}

static inline uint_fast64_t getbits64(const uint8_t *buf, const int bit, const int len) {
	uint_fast64_t mask = len == 64 ? 0xffffffffffffffff : (1ull << len) - 1;
	if (len < 1)
		return 0;
	else if (bit >= 64)
		return (*(uint_fast64_t*)(buf + 8)) >> (bit - 64) & mask;
	else if (bit <= 0)
		return (*(uint_fast64_t*)buf) << -bit & mask;
	else if (bit + len <= 64)
		return (*(uint_fast64_t*)buf) >> bit & mask;
	else
		return ((*(uint_fast64_t*)buf) >> bit | *(uint_fast64_t*)(buf + 8) << (64 - bit)) & mask;
}

static inline uint_fast8_t clamp(const int n) {
	return n < 0 ? 0 : n > 255 ? 255 : n;
}

static inline void bit_transfer_signed(int *a, int *b) {
	*b = (*b >> 1) | (*a & 0x80);
	*a = (*a >> 1) & 0x3f;
	if (*a & 0x20)
		*a -= 0x40;
}

static inline void set_endpoint(int endpoint[8], int r1, int g1, int b1, int a1, int r2, int g2, int b2, int a2) {
	endpoint[0] = r1;
	endpoint[1] = g1;
	endpoint[2] = b1;
	endpoint[3] = a1;
	endpoint[4] = r2;
	endpoint[5] = g2;
	endpoint[6] = b2;
	endpoint[7] = a2;
}

static inline void set_endpoint_clamp(int endpoint[8], int r1, int g1, int b1, int a1, int r2, int g2, int b2, int a2) {
	endpoint[0] = clamp(r1);
	endpoint[1] = clamp(g1);
	endpoint[2] = clamp(b1);
	endpoint[3] = clamp(a1);
	endpoint[4] = clamp(r2);
	endpoint[5] = clamp(g2);
	endpoint[6] = clamp(b2);
	endpoint[7] = clamp(a2);
}

static inline void set_endpoint_blue(int endpoint[8], int r1, int g1, int b1, int a1, int r2, int g2, int b2, int a2) {
	endpoint[0] = (r1 + b1) >> 1;
	endpoint[1] = (g1 + b1) >> 1;
	endpoint[2] = b1;
	endpoint[3] = a1;
	endpoint[4] = (r2 + b2) >> 1;
	endpoint[5] = (g2 + b2) >> 1;
	endpoint[6] = b2;
	endpoint[7] = a2;
}

static inline void set_endpoint_blue_clamp(int endpoint[8], int r1, int g1, int b1, int a1, int r2, int g2, int b2, int a2) {
	endpoint[0] = clamp((r1 + b1) >> 1);
	endpoint[1] = clamp((g1 + b1) >> 1);
	endpoint[2] = clamp(b1);
	endpoint[3] = clamp(a1);
	endpoint[4] = clamp((r2 + b2) >> 1);
	endpoint[5] = clamp((g2 + b2) >> 1);
	endpoint[6] = clamp(b2);
	endpoint[7] = clamp(a2);
}

static inline uint_fast8_t select_color(int v0, int v1, int weight) {
	return ((((v0 << 8 | v0) * (64 - weight) + (v1 << 8 | v1) * weight + 32) >> 6) * 255 + 32768) / 65536;
}

typedef struct {
	int bw;
	int bh;
	int width;
	int height;
	int part_num;
	int dual_plane;
	int plane_selector;
	int weight_range;
	int weight_num; // max: 120
	int cem[4];
	int cem_range;
	int endpoint_value_num; // max: 32
	int endpoints[4][8];
	int weights[144][2];
	int partition[144];
} BlockData;

typedef struct {
	int bits;
	int nonbits;
} IntSeqData;

void decode_intseq(const uint8_t *buf, int offset, const int a, const int b, const int count, const int reverse, IntSeqData *out) {
	static int mt[] = { 0, 2, 4, 5, 7 };
	static int mq[] = { 0, 3, 5 };
	static int TritsTable[5][256] = {
		{0, 1, 2, 0, 0, 1, 2, 1, 0, 1, 2, 2, 0, 1, 2, 2, 0, 1, 2, 0, 0, 1, 2, 1, 0, 1, 2, 2, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 1, 0, 1, 2, 2, 0, 1, 2, 2, 0, 1, 2, 0, 0, 1, 2, 1, 0, 1, 2, 2, 0, 1, 2, 1, 0, 1, 2, 0, 0, 1, 2, 1, 0, 1, 2, 2, 0, 1, 2, 2, 0, 1, 2, 0, 0, 1, 2, 1, 0, 1, 2, 2, 0, 1, 2, 2, 0, 1, 2, 0, 0, 1, 2, 1, 0, 1, 2, 2, 0, 1, 2, 2, 0, 1, 2, 0, 0, 1, 2, 1, 0, 1, 2, 2, 0, 1, 2, 2, 0, 1, 2, 0, 0, 1, 2, 1, 0, 1, 2, 2, 0, 1, 2, 2, 0, 1, 2, 0, 0, 1, 2, 1, 0, 1, 2, 2, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 1, 0, 1, 2, 2, 0, 1, 2, 2, 0, 1, 2, 0, 0, 1, 2, 1, 0, 1, 2, 2, 0, 1, 2, 1, 0, 1, 2, 0, 0, 1, 2, 1, 0, 1, 2, 2, 0, 1, 2, 2, 0, 1, 2, 0, 0, 1, 2, 1, 0, 1, 2, 2, 0, 1, 2, 2, 0, 1, 2, 0, 0, 1, 2, 1, 0, 1, 2, 2, 0, 1, 2, 2, 0, 1, 2, 0, 0, 1, 2, 1, 0, 1, 2, 2, 0, 1, 2, 2},
		{0, 0, 0, 0, 1, 1, 1, 0, 2, 2, 2, 0, 2, 2, 2, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 2, 2, 2, 0, 2, 2, 2, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 2, 2, 2, 0, 2, 2, 2, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 1, 2, 2, 2, 0, 0, 0, 0, 0, 1, 1, 1, 0, 2, 2, 2, 0, 2, 2, 2, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 1, 2, 2, 2, 0, 0, 0, 0, 0, 1, 1, 1, 0, 2, 2, 2, 0, 2, 2, 2, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0, 2, 2, 2, 0, 2, 2, 2, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 2, 2, 2, 0, 2, 2, 2, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 1, 2, 2, 2, 1, 0, 0, 0, 0, 1, 1, 1, 0, 2, 2, 2, 0, 2, 2, 2, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 1, 2, 2, 2, 1},
		{0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2, 2, 2, 2, 2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2, 2, 2, 2, 2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2, 2, 2, 2, 2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2, 2, 2, 2, 2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 2, 2, 2, 2, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2, 2, 2, 2, 2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2, 2, 2, 2, 2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2, 2, 2, 2, 2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2, 2, 2, 2, 2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 2, 2, 2, 2, 2},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2}
	};
	static int QuintsTable[3][128] = {
		{0, 1, 2, 3, 4, 0, 4, 4, 0, 1, 2, 3, 4, 1, 4, 4, 0, 1, 2, 3, 4, 2, 4, 4, 0, 1, 2, 3, 4, 3, 4, 4, 0, 1, 2, 3, 4, 0, 4, 0, 0, 1, 2, 3, 4, 1, 4, 1, 0, 1, 2, 3, 4, 2, 4, 2, 0, 1, 2, 3, 4, 3, 4, 3, 0, 1, 2, 3, 4, 0, 2, 3, 0, 1, 2, 3, 4, 1, 2, 3, 0, 1, 2, 3, 4, 2, 2, 3, 0, 1, 2, 3, 4, 3, 2, 3, 0, 1, 2, 3, 4, 0, 0, 1, 0, 1, 2, 3, 4, 1, 0, 1, 0, 1, 2, 3, 4, 2, 0, 1, 0, 1, 2, 3, 4, 3, 0, 1},
		{0, 0, 0, 0, 0, 4, 4, 4, 1, 1, 1, 1, 1, 4, 4, 4, 2, 2, 2, 2, 2, 4, 4, 4, 3, 3, 3, 3, 3, 4, 4, 4, 0, 0, 0, 0, 0, 4, 0, 4, 1, 1, 1, 1, 1, 4, 1, 4, 2, 2, 2, 2, 2, 4, 2, 4, 3, 3, 3, 3, 3, 4, 3, 4, 0, 0, 0, 0, 0, 4, 0, 0, 1, 1, 1, 1, 1, 4, 1, 1, 2, 2, 2, 2, 2, 4, 2, 2, 3, 3, 3, 3, 3, 4, 3, 3, 0, 0, 0, 0, 0, 4, 0, 0, 1, 1, 1, 1, 1, 4, 1, 1, 2, 2, 2, 2, 2, 4, 2, 2, 3, 3, 3, 3, 3, 4, 3, 3},
		{0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 1, 4, 0, 0, 0, 0, 0, 0, 2, 4, 0, 0, 0, 0, 0, 0, 3, 4, 1, 1, 1, 1, 1, 1, 4, 4, 1, 1, 1, 1, 1, 1, 4, 4, 1, 1, 1, 1, 1, 1, 4, 4, 1, 1, 1, 1, 1, 1, 4, 4, 2, 2, 2, 2, 2, 2, 4, 4, 2, 2, 2, 2, 2, 2, 4, 4, 2, 2, 2, 2, 2, 2, 4, 4, 2, 2, 2, 2, 2, 2, 4, 4, 3, 3, 3, 3, 3, 3, 4, 4, 3, 3, 3, 3, 3, 3, 4, 4, 3, 3, 3, 3, 3, 3, 4, 4, 3, 3, 3, 3, 3, 3, 4, 4}
	};

	if (count <= 0)
		return;

	int n = 0;

	if (a == 3) {
		int mask = (1 << b) - 1;
		int block_count = (count + 4) / 5;
		int last_block_count = (count + 4) % 5 + 1;
		int block_size = 8 + 5 * b;
		int last_block_size = (block_size * last_block_count + 4) / 5;

		if (reverse) {
			for (int i = 0, p = offset; i < block_count; i++, p -= block_size) {
				int now_size = (i < block_count - 1) ? block_size : last_block_size;
				uint_fast64_t d = bit_reverse_u64(getbits64(buf, p - now_size, now_size), now_size);
				int x = (d >> b & 3) | (d >> b * 2 & 0xc) | (d >> b * 3 & 0x10) | (d >> b * 4 & 0x60) | (d >> b * 5 & 0x80);
				for (int j = 0; j < 5 && n < count; j++, n++)
				{
					IntSeqData data;
					data.bits = (int)(d >> (mt[j] + b * j) & mask);
					data.nonbits = TritsTable[j][x];
					out[n] = data;
				}
			}
		}
		else {
			for (int i = 0, p = offset; i < block_count; i++, p += block_size) {
				uint_fast64_t d = getbits64(buf, p, (i < block_count - 1) ? block_size : last_block_size);
				int x = (d >> b & 3) | (d >> b * 2 & 0xc) | (d >> b * 3 & 0x10) | (d >> b * 4 & 0x60) | (d >> b * 5 & 0x80);
				for (int j = 0; j < 5 && n < count; j++, n++)
				{
					IntSeqData data;
					data.bits = (int)(d >> (mt[j] + b * j) & mask);
					data.nonbits = TritsTable[j][x];
					out[n] = data;
				}
			}
		}
	}
	else if (a == 5) {
		int mask = (1 << b) - 1;
		int block_count = (count + 2) / 3;
		int last_block_count = (count + 2) % 3 + 1;
		int block_size = 7 + 3 * b;
		int last_block_size = (block_size * last_block_count + 2) / 3;

		if (reverse) {
			for (int i = 0, p = offset; i < block_count; i++, p -= block_size) {
				int now_size = (i < block_count - 1) ? block_size : last_block_size;
				uint_fast64_t d = bit_reverse_u64(getbits64(buf, p - now_size, now_size), now_size);
				int x = (d >> b & 7) | (d >> b * 2 & 0x18) | (d >> b * 3 & 0x60);
				for (int j = 0; j < 3 && n < count; j++, n++)
				{
					IntSeqData data;
					data.bits = (int)(d >> (mq[j] + b * j) & mask);
					data.nonbits = QuintsTable[j][x];
					out[n] = data;
				}
			}
		}
		else {
			for (int i = 0, p = offset; i < block_count; i++, p += block_size) {
				uint_fast64_t d = getbits64(buf, p, (i < block_count - 1) ? block_size : last_block_size);
				int x = (d >> b & 7) | (d >> b * 2 & 0x18) | (d >> b * 3 & 0x60);
				for (int j = 0; j < 3 && n < count; j++, n++)
				{
					IntSeqData data;
					data.bits = (int)(d >> (mq[j] + b * j) & mask);
					data.nonbits = QuintsTable[j][x];
					out[n] = data;
				}
			}
		}
	}
	else {
		if (reverse)
			for (int p = offset - b; n < count; n++, p -= b)
			{
				IntSeqData data;
				data.bits = (int)bit_reverse_u8(getbits(buf, p, b), b);
				data.nonbits = 0;
				out[n] = data;
			}
		else
			for (int p = offset; n < count; n++, p += b)
			{
				IntSeqData data;
				data.bits = (int)getbits(buf, p, b);
				data.nonbits = 0;
				out[n] = data;
			}
	}
}

void decode_block_params(const uint8_t *buf, BlockData *block_data) {
	block_data->dual_plane = (buf[1] & 4) >> 2;
	block_data->weight_range = (buf[0] >> 4 & 1) | (buf[1] << 2 & 8);

	if (buf[0] & 3) {
		block_data->weight_range |= buf[0] << 1 & 6;
		switch (buf[0] & 0xc) {
		case 0:
			block_data->width = (*(int*)buf >> 7 & 3) + 4;
			block_data->height = (buf[0] >> 5 & 3) + 2;
			break;
		case 4:
			block_data->width = (*(int*)buf >> 7 & 3) + 8;
			block_data->height = (buf[0] >> 5 & 3) + 2;
			break;
		case 8:
			block_data->width = (buf[0] >> 5 & 3) + 2;
			block_data->height = (*(int*)buf >> 7 & 3) + 8;
			break;
		case 12:
			if (buf[1] & 1) {
				block_data->width = (buf[0] >> 7 & 1) + 2;
				block_data->height = (buf[0] >> 5 & 3) + 2;
			}
			else {
				block_data->width = (buf[0] >> 5 & 3) + 2;
				block_data->height = (buf[0] >> 7 & 1) + 6;
			}
			break;
		}
	}
	else {
		block_data->weight_range |= buf[0] >> 1 & 6;
		switch ((*(int*)buf) & 0x180) {
		case 0:
			block_data->width = 12;
			block_data->height = (buf[0] >> 5 & 3) + 2;
			break;
		case 0x80:
			block_data->width = (buf[0] >> 5 & 3) + 2;
			block_data->height = 12;
			break;
		case 0x100:
			block_data->width = (buf[0] >> 5 & 3) + 6;
			block_data->height = (buf[1] >> 1 & 3) + 6;
			block_data->dual_plane = 0;
			block_data->weight_range &= 7;
			break;
		case 0x180:
			block_data->width = (buf[0] & 0x20) ? 10 : 6;
			block_data->height = (buf[0] & 0x20) ? 6 : 10;
			break;
		}
	}

	block_data->part_num = (buf[1] >> 3 & 3) + 1;

	block_data->weight_num = block_data->width * block_data->height;
	if (block_data->dual_plane)
		block_data->weight_num *= 2;

	int weight_bits, config_bits, cem_base = 0;

	switch (WeightPrecTableA[block_data->weight_range]) {
	case 3:
		weight_bits = block_data->weight_num * WeightPrecTableB[block_data->weight_range] + (block_data->weight_num * 8 + 4) / 5;
		break;
	case 5:
		weight_bits = block_data->weight_num * WeightPrecTableB[block_data->weight_range] + (block_data->weight_num * 7 + 2) / 3;
		break;
	default:
		weight_bits = block_data->weight_num * WeightPrecTableB[block_data->weight_range];
	}

	if (block_data->part_num == 1) {
		block_data->cem[0] = *(int*)(buf + 1) >> 5 & 0xf;
		config_bits = 17;
	}
	else {
		cem_base = *(int*)(buf + 2) >> 7 & 3;
		if (cem_base == 0) {
			int cem = buf[3] >> 1 & 0xf;
			for (int i = 0; i < block_data->part_num; i++)
				block_data->cem[i] = cem;
			config_bits = 29;
		}
		else {
			for (int i = 0; i < block_data->part_num; i++)
				block_data->cem[i] = ((buf[3] >> (i + 1) & 1) + cem_base - 1) << 2;
			switch (block_data->part_num) {
			case 2:
				block_data->cem[0] |= buf[3] >> 3 & 3;
				block_data->cem[1] |= getbits(buf, 126 - weight_bits, 2);
				break;
			case 3:
				block_data->cem[0] |= buf[3] >> 4 & 1;
				block_data->cem[0] |= getbits(buf, 122 - weight_bits, 2) & 2;
				block_data->cem[1] |= getbits(buf, 124 - weight_bits, 2);
				block_data->cem[2] |= getbits(buf, 126 - weight_bits, 2);
				break;
			case 4:
				for (int i = 0; i < 4; i++)
					block_data->cem[i] |= getbits(buf, 120 + i * 2 - weight_bits, 2);
				break;
			}
			config_bits = 25 + block_data->part_num * 3;
		}
	}

	if (block_data->dual_plane) {
		config_bits += 2;
		block_data->plane_selector = getbits(buf, cem_base ? 130 - weight_bits - block_data->part_num * 3 : 126 - weight_bits, 2);
	}

	int remain_bits = 128 - config_bits - weight_bits;

	block_data->endpoint_value_num = 0;
	for (int i = 0; i < block_data->part_num; i++)
		block_data->endpoint_value_num += (block_data->cem[i] >> 1 & 6) + 2;

	for (int i = 0, endpoint_bits; i < (int)(sizeof(CemTableA) / sizeof(int)); i++) {
		switch (CemTableA[i]) {
		case 3:
			endpoint_bits = block_data->endpoint_value_num * CemTableB[i] + (block_data->endpoint_value_num * 8 + 4) / 5;
			break;
		case 5:
			endpoint_bits = block_data->endpoint_value_num * CemTableB[i] + (block_data->endpoint_value_num * 7 + 2) / 3;
			break;
		default:
			endpoint_bits = block_data->endpoint_value_num * CemTableB[i];
		}

		if (endpoint_bits <= remain_bits) {
			block_data->cem_range = i;
			break;
		}
	}
}

void decode_endpoints(const uint8_t *buf, BlockData *data) {
	static int TritsTable[] = { 0, 204, 93, 44, 22, 11, 5 };
	static int QuintsTable[] = { 0, 113, 54, 26, 13, 6 };
	IntSeqData seq[32];
	int ev[32];
	decode_intseq(buf, data->part_num == 1 ? 17 : 29, CemTableA[data->cem_range], CemTableB[data->cem_range], data->endpoint_value_num, 0, seq);

	switch (CemTableA[data->cem_range]) {
	case 3:
		for (int i = 0, b, c = TritsTable[CemTableB[data->cem_range]]; i < data->endpoint_value_num; i++) {
			int a = (seq[i].bits & 1) * 0x1ff;
			int x = seq[i].bits >> 1;
			switch (CemTableB[data->cem_range]) {
			case 1:
				b = 0;
				break;
			case 2:
				b = 0b100010110 * x;
				break;
			case 3:
				b = x << 7 | x << 2 | x;
				break;
			case 4:
				b = x << 6 | x;
				break;
			case 5:
				b = x << 5 | x >> 2;
				break;
			case 6:
				b = x << 4 | x >> 4;
				break;
			}
			ev[i] = (a & 0x80) | ((seq[i].nonbits * c + b) ^ a) >> 2;
		}
		break;
	case 5:
		for (int i = 0, b, c = QuintsTable[CemTableB[data->cem_range]]; i < data->endpoint_value_num; i++) {
			int a = (seq[i].bits & 1) * 0x1ff;
			int x = seq[i].bits >> 1;
			switch (CemTableB[data->cem_range]) {
			case 1:
				b = 0;
				break;
			case 2:
				b = 0b100001100 * x;
				break;
			case 3:
				b = x << 7 | x << 1 | x >> 1;
				break;
			case 4:
				b = x << 6 | x >> 1;
				break;
			case 5:
				b = x << 5 | x >> 3;
				break;
			}
			ev[i] = (a & 0x80) | ((seq[i].nonbits * c + b) ^ a) >> 2;
		}
		break;
	default:
		switch (CemTableB[data->cem_range]) {
		case 1:
			for (int i = 0; i < data->endpoint_value_num; i++)
				ev[i] = seq[i].bits * 0xff;
			break;
		case 2:
			for (int i = 0; i < data->endpoint_value_num; i++)
				ev[i] = seq[i].bits * 0x55;
			break;
		case 3:
			for (int i = 0; i < data->endpoint_value_num; i++)
				ev[i] = seq[i].bits << 5 | seq[i].bits << 2 | seq[i].bits >> 1;
			break;
		case 4:
			for (int i = 0; i < data->endpoint_value_num; i++)
				ev[i] = seq[i].bits << 4 | seq[i].bits;
			break;
		case 5:
			for (int i = 0; i < data->endpoint_value_num; i++)
				ev[i] = seq[i].bits << 3 | seq[i].bits >> 2;
			break;
		case 6:
			for (int i = 0; i < data->endpoint_value_num; i++)
				ev[i] = seq[i].bits << 2 | seq[i].bits >> 4;
			break;
		case 7:
			for (int i = 0; i < data->endpoint_value_num; i++)
				ev[i] = seq[i].bits << 1 | seq[i].bits >> 6;
			break;
		case 8:
			for (int i = 0; i < data->endpoint_value_num; i++)
				ev[i] = seq[i].bits;
			break;
		}
	}

	int *v = ev;
	for (int cem = 0; cem < data->part_num; v += (data->cem[cem] / 4 + 1) * 2, cem++) {
		switch (data->cem[cem]) {
		case 0:
			set_endpoint(data->endpoints[cem], v[0], v[0], v[0], 255, v[1], v[1], v[1], 255);
			break;
		case 1:
		{
			int l0 = (v[0] >> 2) | (v[1] & 0xc0);
			int l1 = clamp(l0 + (v[1] & 0x3f));
			set_endpoint(data->endpoints[cem], l0, l0, l0, 255, l1, l1, l1, 255);
		}
		break;
		case 4:
			set_endpoint(data->endpoints[cem], v[0], v[0], v[0], v[2], v[1], v[1], v[1], v[3]);
			break;
		case 5:
			bit_transfer_signed(&v[1], &v[0]);
			bit_transfer_signed(&v[3], &v[2]);
			v[1] += v[0];
			set_endpoint_clamp(data->endpoints[cem], v[0], v[0], v[0], v[2], v[1], v[1], v[1], v[2] + v[3]);
			break;
		case 6:
			set_endpoint(data->endpoints[cem], v[0] * v[3] >> 8, v[1] * v[3] >> 8, v[2] * v[3] >> 8, 255, v[0], v[1], v[2], 255);
			break;
		case 8:
			if (v[0] + v[2] + v[4] <= v[1] + v[3] + v[5])
				set_endpoint(data->endpoints[cem], v[0], v[2], v[4], 255, v[1], v[3], v[5], 255);
			else
				set_endpoint_blue(data->endpoints[cem], v[1], v[3], v[5], 255, v[0], v[2], v[4], 255);
			break;
		case 9:
			bit_transfer_signed(&v[1], &v[0]);
			bit_transfer_signed(&v[3], &v[2]);
			bit_transfer_signed(&v[5], &v[4]);
			if (v[1] + v[3] + v[5] >= 0)
				set_endpoint_clamp(data->endpoints[cem], v[0], v[2], v[4], 255, v[0] + v[1], v[2] + v[3], v[4] + v[5], 255);
			else
				set_endpoint_blue_clamp(data->endpoints[cem], v[0] + v[1], v[2] + v[3], v[4] + v[5], 255, v[0], v[2], v[4], 255);
			break;
		case 10:
			set_endpoint(data->endpoints[cem], v[0] * v[3] >> 8, v[1] * v[3] >> 8, v[2] * v[3] >> 8, v[4], v[0], v[1], v[2], v[5]);
			break;
		case 12:
			if (v[0] + v[2] + v[4] <= v[1] + v[3] + v[5])
				set_endpoint(data->endpoints[cem], v[0], v[2], v[4], v[6], v[1], v[3], v[5], v[7]);
			else
				set_endpoint_blue(data->endpoints[cem], v[1], v[3], v[5], v[7], v[0], v[2], v[4], v[6]);
			break;
		case 13:
			bit_transfer_signed(&v[1], &v[0]);
			bit_transfer_signed(&v[3], &v[2]);
			bit_transfer_signed(&v[5], &v[4]);
			bit_transfer_signed(&v[7], &v[6]);
			if (v[1] + v[3] + v[5] >= 0)
				set_endpoint_clamp(data->endpoints[cem], v[0], v[2], v[4], v[6], v[0] + v[1], v[2] + v[3], v[4] + v[5], v[6] + v[7]);
			else
				set_endpoint_blue_clamp(data->endpoints[cem], v[0] + v[1], v[2] + v[3], v[4] + v[5], v[6] + v[7], v[0], v[2], v[4], v[6]);
			break;
		default:
		{
			throw "Unsupported ASTC format";
			//rb_raise("Unsupported ASTC format");
		}
		}
	}
}

void decode_weights(const uint8_t *buf, BlockData *data) {
	IntSeqData seq[128];
	int wv[128];
	decode_intseq(buf, 128, WeightPrecTableA[data->weight_range], WeightPrecTableB[data->weight_range], data->weight_num, 1, seq);

	if (WeightPrecTableA[data->weight_range] == 0) {
		switch (WeightPrecTableB[data->weight_range]) {
		case 1:
			for (int i = 0; i < data->weight_num; i++)
				wv[i] = seq[i].bits ? 63 : 0;
			break;
		case 2:
			for (int i = 0; i < data->weight_num; i++)
				wv[i] = seq[i].bits << 4 | seq[i].bits << 2 | seq[i].bits;
			break;
		case 3:
			for (int i = 0; i < data->weight_num; i++)
				wv[i] = seq[i].bits << 3 | seq[i].bits;
			break;
		case 4:
			for (int i = 0; i < data->weight_num; i++)
				wv[i] = seq[i].bits << 2 | seq[i].bits >> 2;
			break;
		case 5:
			for (int i = 0; i < data->weight_num; i++)
				wv[i] = seq[i].bits << 1 | seq[i].bits >> 4;
			break;
		}
		for (int i = 0; i < data->weight_num; i++)
			if (wv[i] > 32)
				++wv[i];
	}
	else if (WeightPrecTableB[data->weight_range] == 0) {
		int s = WeightPrecTableA[data->weight_range] == 3 ? 32 : 16;
		for (int i = 0; i < data->weight_num; i++)
			wv[i] = seq[i].nonbits * s;
	}
	else {
		if (WeightPrecTableA[data->weight_range] == 3) {
			switch (WeightPrecTableB[data->weight_range]) {
			case 1:
				for (int i = 0; i < data->weight_num; i++)
					wv[i] = seq[i].nonbits * 50;
				break;
			case 2:
				for (int i = 0; i < data->weight_num; i++) {
					wv[i] = seq[i].nonbits * 23;
					if (seq[i].bits & 2)
						wv[i] += 0b1000101;
				}
				break;
			case 3:
				for (int i = 0; i < data->weight_num; i++)
					wv[i] = seq[i].nonbits * 11 + ((seq[i].bits << 4 | seq[i].bits >> 1) & 0b1100011);
				break;
			}
		}
		else if (WeightPrecTableA[data->weight_range] == 5) {
			switch (WeightPrecTableB[data->weight_range]) {
			case 1:
				for (int i = 0; i < data->weight_num; i++)
					wv[i] = seq[i].nonbits * 28;
				break;
			case 2:
				for (int i = 0; i < data->weight_num; i++) {
					wv[i] = seq[i].nonbits * 13;
					if (seq[i].bits & 2)
						wv[i] += 0b1000010;
				}
				break;
			}
		}
		for (int i = 0; i < data->weight_num; i++) {
			int a = (seq[i].bits & 1) * 0x7f;
			wv[i] = (a & 0x20) | ((wv[i] ^ a) >> 2);
			if (wv[i] > 32)
				++wv[i];
		}
	}

	int ds = (1024 + data->bw / 2) / (data->bw - 1);
	int dt = (1024 + data->bh / 2) / (data->bh - 1);
	int pn = data->dual_plane ? 2 : 1;

	for (int t = 0, i = 0; t < data->bh; t++) {
		for (int s = 0; s < data->bw; s++, i++) {
			int gs = (ds * s * (data->width - 1) + 32) >> 6;
			int gt = (dt * t * (data->height - 1) + 32) >> 6;
			int fs = gs & 0xf;
			int ft = gt & 0xf;
			int v = (gs >> 4) + (gt >> 4) * data->width;
			int w11 = (fs * ft + 8) >> 4;
			int w10 = ft - w11;
			int w01 = fs - w11;
			int w00 = 16 - fs - ft + w11;

			for (int p = 0; p < pn; p++) {
				int p00 = wv[v * pn + p];
				int p01 = wv[(v + 1) * pn + p];
				int p10 = wv[(v + data->width) * pn + p];
				int p11 = wv[(v + data->width + 1) * pn + p];
				data->weights[i][p] = (p00 * w00 + p01 * w01 + p10 * w10 + p11 * w11 + 8) >> 4;
			}
		}
	}
}

void select_partition(const uint8_t *buf, BlockData *data) {
	int small_block = data->bw * data->bh < 31;
	int seed = (*(int*)buf >> 13 & 0x3ff) | (data->part_num - 1) << 10;

	uint32_t rnum = seed;
	rnum ^= rnum >> 15;
	rnum -= rnum << 17;
	rnum += rnum << 7;
	rnum += rnum << 4;
	rnum ^= rnum >> 5;
	rnum += rnum << 16;
	rnum ^= rnum >> 7;
	rnum ^= rnum >> 3;
	rnum ^= rnum << 6;
	rnum ^= rnum >> 17;

	int seeds[8];
	for (int i = 0; i < 8; i++) {
		seeds[i] = (rnum >> (i * 4)) & 0xF;
		seeds[i] *= seeds[i];
	}

	int sh[2] = { seed & 2 ? 4 : 5, data->part_num == 3 ? 6 : 5 };

	if (seed & 1)
		for (int i = 0; i < 8; i++)
			seeds[i] >>= sh[i % 2];
	else
		for (int i = 0; i < 8; i++)
			seeds[i] >>= sh[1 - i % 2];

	if (small_block) {
		for (int t = 0, i = 0; t < data->bh; t++) {
			for (int s = 0; s < data->bw; s++, i++) {
				int x = s << 1;
				int y = t << 1;
				int a = (seeds[0] * x + seeds[1] * y + (rnum >> 14)) & 0x3f;
				int b = (seeds[2] * x + seeds[3] * y + (rnum >> 10)) & 0x3f;
				int c = data->part_num < 3 ? 0 : (seeds[4] * x + seeds[5] * y + (rnum >> 6)) & 0x3f;
				int d = data->part_num < 4 ? 0 : (seeds[6] * x + seeds[7] * y + (rnum >> 2)) & 0x3f;
				data->partition[i] = (a >= b && a >= c && a >= d) ? 0 : (b >= c && b >= d) ? 1 : (c >= d) ? 2 : 3;
			}
		}
	}
	else {
		for (int y = 0, i = 0; y < data->bh; y++) {
			for (int x = 0; x < data->bw; x++, i++) {
				int a = (seeds[0] * x + seeds[1] * y + (rnum >> 14)) & 0x3f;
				int b = (seeds[2] * x + seeds[3] * y + (rnum >> 10)) & 0x3f;
				int c = data->part_num < 3 ? 0 : (seeds[4] * x + seeds[5] * y + (rnum >> 6)) & 0x3f;
				int d = data->part_num < 4 ? 0 : (seeds[6] * x + seeds[7] * y + (rnum >> 2)) & 0x3f;
				data->partition[i] = (a >= b && a >= c && a >= d) ? 0 : (b >= c && b >= d) ? 1 : (c >= d) ? 2 : 3;
			}
		}
	}
}

void applicate_color(const BlockData *data, uint32_t *outbuf) {
	if (data->dual_plane) {
		int ps[] = { 0, 0, 0, 0 };
		ps[data->plane_selector] = 1;
		if (data->part_num > 1) {
			for (int i = 0; i < data->bw * data->bh; i++) {
				int p = data->partition[i];
				uint_fast8_t r = select_color(data->endpoints[p][0], data->endpoints[p][4], data->weights[i][ps[0]]);
				uint_fast8_t g = select_color(data->endpoints[p][1], data->endpoints[p][5], data->weights[i][ps[1]]);
				uint_fast8_t b = select_color(data->endpoints[p][2], data->endpoints[p][6], data->weights[i][ps[2]]);
				uint_fast8_t a = select_color(data->endpoints[p][3], data->endpoints[p][7], data->weights[i][ps[3]]);
				outbuf[i] = color(r, g, b, a);
			}
		}
		else {
			for (int i = 0; i < data->bw * data->bh; i++) {
				uint_fast8_t r = select_color(data->endpoints[0][0], data->endpoints[0][4], data->weights[i][ps[0]]);
				uint_fast8_t g = select_color(data->endpoints[0][1], data->endpoints[0][5], data->weights[i][ps[1]]);
				uint_fast8_t b = select_color(data->endpoints[0][2], data->endpoints[0][6], data->weights[i][ps[2]]);
				uint_fast8_t a = select_color(data->endpoints[0][3], data->endpoints[0][7], data->weights[i][ps[3]]);
				outbuf[i] = color(r, g, b, a);
			}
		}
	}
	else if (data->part_num > 1) {
		for (int i = 0; i < data->bw * data->bh; i++) {
			int p = data->partition[i];
			uint_fast8_t r = select_color(data->endpoints[p][0], data->endpoints[p][4], data->weights[i][0]);
			uint_fast8_t g = select_color(data->endpoints[p][1], data->endpoints[p][5], data->weights[i][0]);
			uint_fast8_t b = select_color(data->endpoints[p][2], data->endpoints[p][6], data->weights[i][0]);
			uint_fast8_t a = select_color(data->endpoints[p][3], data->endpoints[p][7], data->weights[i][0]);
			outbuf[i] = color(r, g, b, a);
		}
	}
	else {
		for (int i = 0; i < data->bw * data->bh; i++) {
			uint_fast8_t r = select_color(data->endpoints[0][0], data->endpoints[0][4], data->weights[i][0]);
			uint_fast8_t g = select_color(data->endpoints[0][1], data->endpoints[0][5], data->weights[i][0]);
			uint_fast8_t b = select_color(data->endpoints[0][2], data->endpoints[0][6], data->weights[i][0]);
			uint_fast8_t a = select_color(data->endpoints[0][3], data->endpoints[0][7], data->weights[i][0]);
			outbuf[i] = color(r, g, b, a);
		}
	}
}

void decode_block(const uint8_t *buf, const int bw, const int bh, uint32_t *outbuf) {
	if (buf[0] == 0xfc && (buf[1] & 1) == 1) {
		uint_fast32_t c = color(buf[9], buf[11], buf[13], buf[15]);
		for (int i = 0; i < bw * bh; i++)
			outbuf[i] = c;
	}
	else {
		BlockData block_data;
		block_data.bw = bw;
		block_data.bh = bh;
		decode_block_params(buf, &block_data);
		decode_endpoints(buf, &block_data);
		decode_weights(buf, &block_data);
		if (block_data.part_num > 1)
			select_partition(buf, &block_data);
		applicate_color(&block_data, outbuf);
	}
}

void decode_astc(const uint8_t *data, const int w, const int h, const int bw, const int bh, uint32_t *image) {
	int bcw = (w + bw - 1) / bw;
	int bch = (h + bh - 1) / bh;
	int clen_last = (w + bw - 1) % bw + 1;
	uint32_t *buf = (uint32_t*)calloc(bw * bh, sizeof(uint32_t));
	const uint8_t *ptr = data;
	for (int t = 0; t < bch; t++) {
		for (int s = 0; s < bcw; s++, ptr += 16) {
			decode_block(ptr, bw, bh, buf);
			int clen = (s < bcw - 1 ? bw : clen_last) * 4;
			for (int i = 0, y = h - t * bh - 1; i < bh && y >= 0; i++, y--)
				memcpy(image + y * w + s * bw, buf + i * bw, clen);
		}
	}
	free(buf);
}
