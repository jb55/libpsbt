
#define _DEFAULT_SOURCE

#include <limits.h>
#include <endian.h>
#include <string.h>

#include "compactsize.h"

u32 compactsize_peek_length(u8 chsize) {
	if (chsize < 253)
		return sizeof(u8);
	else if (chsize == 253)
		return sizeof(u8) + sizeof(u16);
	else if (chsize == 254)
		return sizeof(u8) + sizeof(u32);
	else
		return sizeof(u8) + sizeof(u64);
}

u32 compactsize_length(u64 data) {
	if (data < 253)
		return sizeof(u8);
	else if (data <= USHRT_MAX)
		return sizeof(u8) + sizeof(u16);
	else if (data <= UINT_MAX)
		return sizeof(u8) + sizeof(u32);
	else
		return sizeof(u8) + sizeof(u64);
}

inline static void serialize_u8(u8 *dest, u8 data) {
	*dest = data;
}

inline static void serialize_u16(u8 *dest, u16 data) {
	data = htole16(data);
	memcpy(dest, &data, sizeof(data));
}

inline static void serialize_u32(u8 *dest, u32 data) {
	data = htole32(data);
	memcpy(dest, &data, sizeof(data));
}

inline static void serialize_u64(u8 *dest, u64 data) {
	data = htole64(data);
	memcpy(dest, &data, sizeof(data));
}

inline static u8 deserialize_u8(u8 *src) {
	return *src;
}

inline static u16 deserialize_u16(u8 *src) {
	u16 data;
	memcpy(&data, src, sizeof(data));
	return le16toh(data);
}

inline static u32 deserialize_u32(u8 *src) {
	u32 data;
	memcpy(&data, src, sizeof(data));
	return le32toh(data);
}

inline static u64 deserialize_u64(u8 *src) {
	u64 data;
	memcpy(&data, src, sizeof(data));
	return le64toh(data);
}

void compactsize_write(u8 *dest, u64 size)
{
	if (size < 253) {
		serialize_u8(dest, size);
	}
	else if (size <= USHRT_MAX) {
		serialize_u8(dest, 253);
		serialize_u16(dest, size);
	}
	else if (size <= UINT_MAX) {
		serialize_u8(dest, 254);
		serialize_u32(dest, size);
	}
	else {
		serialize_u8(dest, 255);
		serialize_u64(dest, size);
	}
}



#define READERR(msg) \
	{if (err)				\
		*err = PSBT_COMPACT_READ_ERROR; \
	psbt_errmsg = msg; \
	return -1;}

u64 compactsize_read(u8 *data, enum psbt_result *err) {
	u8 *p = data;
	u8 chsize = deserialize_u8(p++);
	u64 ret_size = 0;

	if (chsize < 253) {
		ret_size = chsize;
	}

	else if (chsize == 253) {
		ret_size = deserialize_u16(p);
		if (ret_size < 253)
			READERR("non-canonical compactsize_read()");

	}
	else if (chsize == 254) {
		ret_size = deserialize_u32(p);

		if (ret_size < 0x10000u)
			READERR("non-canonical compactsize_read()");
	}
	else {
		ret_size = deserialize_u64(p);
		if (ret_size < 0x100000000ULL)
			READERR("non-canonical compactsize_read()");
	}

	if (ret_size > (u64)MAX_SERIALIZE_SIZE)
		READERR("non-canonical compactsize_read()");

	return ret_size;
}
