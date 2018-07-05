/*
 * Base64 encoding/decoding (RFC1341)
 * Copyright (c) 2005-2011, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "base64.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static const unsigned char base64_table[65] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const unsigned char base62_table[] =
	"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

/**
 * base64_encode - Base64 encode
 * @src: Data to be encoded
 * @len: Length of the data to be encoded
 * @out_len: Pointer to output length variable, or %NULL if not used
 * Returns: Allocated buffer of out_len bytes of encoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer. Returned buffer is
 * nul terminated to make it easier to use as a C string. The nul terminator is
 * not included in out_len.
 */

unsigned char * base_encode(const unsigned char *src, size_t len,
			    unsigned char *out, size_t out_capacity,
			    size_t *out_len, const unsigned char *base_table)
{
	unsigned char *pos;
	const unsigned char *end, *in;
	size_t olen;

	olen = len * 4 / 3 + 4; /* 3-byte blocks to 4-byte */
	olen++; /* nul termination */
	if (olen < len)
		return NULL; /* integer overflow */
	if (olen > out_capacity)
		return NULL; /* buffer overflow */
	if (out == NULL)
		return NULL;

	end = src + len;
	in = src;
	pos = out;
	while (end - in >= 3) {
		*pos++ = base_table[in[0] >> 2];
		*pos++ = base_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
		*pos++ = base_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
		*pos++ = base_table[in[2] & 0x3f];
		in += 3;
	}

	if (end - in) {
		*pos++ = base_table[in[0] >> 2];
		if (end - in == 1) {
			*pos++ = base_table[(in[0] & 0x03) << 4];
			*pos++ = '=';
		} else {
			*pos++ = base_table[((in[0] & 0x03) << 4) |
					      (in[1] >> 4)];
			*pos++ = base_table[(in[1] & 0x0f) << 2];
		}
		*pos++ = '=';
	}

	*pos = '\0';
	if (out_len)
		*out_len = pos - out;
	return out;
}

unsigned char *
base64_encode(const unsigned char *src, size_t len, unsigned char *out,
	      size_t out_capacity, size_t *out_len) {
	return base_encode(src, len, out, out_capacity, out_len, base64_table);
}

unsigned char *
base62_encode(const unsigned char *src, size_t len, unsigned char *out,
	      size_t out_capacity, size_t *out_len) {
	return base_encode(src, len, out, out_capacity, out_len, base62_table);
}


/**
 * base64_decode - Base64 decode
 * @src: Data to be decoded
 * @len: Length of the data to be decoded
 * @out: Pointer to output buffer
 * @out_len: Pointer to output length variable
 * Returns: Allocated buffer of out_len bytes of decoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer.
 */
unsigned char * base64_decode(const unsigned char *src, size_t len,
			      unsigned char *out, size_t out_capacity,
			      size_t *out_size)
{
	unsigned char dtable[256], *pos, block[4], tmp;
	size_t i, count, olen;
	int pad = 0;

	memset(dtable, 0x80, 256);
	for (i = 0; i < sizeof(base64_table) - 1; i++)
		dtable[base64_table[i]] = (unsigned char) i;
	dtable['='] = 0;

	count = 0;
	for (i = 0; i < len; i++) {
		if (dtable[src[i]] != 0x80)
			count++;
	}

	if (count == 0 || count % 4)
		return NULL;

	olen = count / 4 * 3;

	if (out_capacity < olen)
		return NULL;

	pos = out;
	if (out == NULL)
		return NULL;

	count = 0;
	for (i = 0; i < len; i++) {
		tmp = dtable[src[i]];
		if (tmp == 0x80)
			continue;

		if (src[i] == '=')
			pad++;
		block[count] = tmp;
		count++;
		if (count == 4) {
			*pos++ = (block[0] << 2) | (block[1] >> 4);
			assert((size_t)(pos - out) < out_capacity);
			*pos++ = (block[1] << 4) | (block[2] >> 2);
			assert((size_t)(pos - out) < out_capacity);
			*pos++ = (block[2] << 6) | block[3];
			assert((size_t)(pos - out) < out_capacity);
			count = 0;
			if (pad) {
				if (pad == 1)
					pos--;
				else if (pad == 2)
					pos -= 2;
				else {
					/* Invalid padding */
					return NULL;
				}
				break;
			}
		}
	}

	*out_size = pos - out;
	return out;
}
