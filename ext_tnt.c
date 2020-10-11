#include "msgpuck.h"

size_t
print_decimal(char **buf, size_t buf_size, const char *val, uint32_t val_bytes)
{
	int64_t scale = 0;
	const char *scale_head = val;
	enum mp_type scale_mp_type = mp_typeof(*val);
	if (scale_mp_type == MP_UINT)
		scale = (int64_t)mp_decode_uint(&val);
	else if (scale_mp_type == MP_INT)
		scale = mp_decode_int(&val);

	if (scale_head == val || !val_bytes)
		return 0;         /* "undefined" (is ext mp type 1 misused?) */

	val_bytes -= val - scale_head;

	char *pos = *buf;
	size_t output_length = 0;
	uint8_t sign_nibble = (uint8_t)(val[val_bytes - 1]) & 0xF;
	if (sign_nibble == 0xB || sign_nibble == 0xD) {
		++output_length;
		if (buf_size > 1) /* there is a reason to do something */
			*pos++ = '-';
	}

	uint32_t nibbles_left = val_bytes * 2 - 1;
	if (!(*val & 0xF0))   /* first nibble is 0 */
		--nibbles_left;

	if (!nibbles_left) {  /* e.g. 0e2 */
		scale = 0;
		output_length = 1;
		pos = *buf;       /* in case of negative */
	} else if (scale >= nibbles_left) {
		output_length += (size_t)scale + 2; /* 0. */
	} else if (scale > 0) {
		output_length += nibbles_left + 1;  /*  . */
	} else {
		output_length += nibbles_left + (size_t)(-scale);
	}

	if (output_length > buf_size)
		return output_length;

	char *first_dec_digit = pos;
	*first_dec_digit = 'N';   /* no output mark */

	if (scale > 0 && scale >= nibbles_left) {
		*pos++ = '0';
		*pos++ = '.';
		for (int i = 0; i < scale - nibbles_left; ++i)
			*pos++ = '0';
	}

	uint8_t nz_flag = 0;
	do {
		uint8_t c = (uint8_t)(*(val++));
		if (nibbles_left & 0x1) {
			*pos = (char)('0' + (c >> 4));
			nz_flag |= *pos++;
			--nibbles_left;
			if (scale && nibbles_left == scale)
				*pos++ = '.';
		}

		if (nibbles_left) {
			*pos = (char)('0' + (c & 0xF));
			nz_flag |= *pos++;
			--nibbles_left;
			if (scale && nibbles_left == scale)
				*pos++ = '.';
		}
	} while (nibbles_left);

	if (nz_flag) {                        /* non-zero digit exists */
		while (scale++ < 0)
			*pos++ = '0';
	} else if (*first_dec_digit == 'N') { /* no digits at all */
		*first_dec_digit = '0';
		pos = first_dec_digit + 1;
	}

	size_t len = (size_t)(pos - *buf);
	assert(len == output_length);
	*buf = pos;
	return len;
}

int
mp_fprint_ext_tnt(FILE *file, const char **data, int depth)
{
	int8_t type;
	uint32_t len = mp_decode_extl(data, &type);
	const char *ext = *data;
	*data += len;
	switch(type) {
	case 1: { /* decimal */
		char static_buf[128];
		char *pos = static_buf;
		size_t res_length = print_decimal(&pos,
										  sizeof(static_buf),
										  ext,
										  len);
		if (res_length >= sizeof(static_buf)) {
			char *dynamic_buf = malloc(res_length);
			char *pos = dynamic_buf;
			print_decimal(&pos,
						  res_length,
						  ext,
						  len);
			fprintf(file, "%.*s", (int)res_length, dynamic_buf);
			free(dynamic_buf);
		} else {
			fprintf(file, "%.*s", (int)res_length, static_buf);
		}
		return res_length;
	}
		/* TODO uuid and error */
	}
	return mp_fprint_ext_default(file, &ext, depth);
}

int
mp_snprint_ext_tnt(char *buf, int size, const char **data, int depth)
{
	int8_t type;
	uint32_t len = mp_decode_extl(data, &type);
	const char *ext = *data;
	*data += len;
	switch(type) {
	case 1: { /* decimal */
		size_t res_length = print_decimal(&buf, size, ext, len);
		if (res_length < (size_t)size)
			*buf = '\0';
		return res_length;
	}
		/* TODO uuid and error */
	}
	return mp_snprint_ext_default(buf, size, &ext, depth);
}
