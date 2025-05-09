/*
 * Copyright (c) 2013-2016 MsgPuck Authors
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>

#define UNIT_TAP_COMPATIBLE 1
#include "msgpuck.h"
#include "test.h"

#include <type_traits>

#define BUF_MAXLEN ((1L << 18) - 1)
#define STRBIN_MAXLEN (BUF_MAXLEN - 10)
#define MP_NUMBER_MAX_LEN 16
#define MP_NUMBER_CODEC_COUNT 16

#ifndef lengthof
#define lengthof(array) (sizeof(array) / sizeof((array)[0]))
#endif

#ifndef __has_attribute
#  define __has_attribute(x) 0
#endif

#if __has_attribute(noinline) || defined(__GNUC__)
#  define NOINLINE __attribute__((noinline))
#else
#  define NOINLINE
#endif

static char buf[BUF_MAXLEN + 1];
static char str[STRBIN_MAXLEN];
static char *data = buf + 1; /* use unaligned address to fail early */

#define SCALAR(x) x
#define COMPLEX(x)

#define COMMA ,

#define DEFINE_TEST(_type, _complex, _ext, _v, _r, _rl) ({		       \
	_ext(int8_t ext_type = 0);					       \
	const char *d1 = mp_encode_##_type(data, _ext(ext_type COMMA) (_v));   \
	const char *d2 = data;                                                 \
	_complex(const char *d3 = data);                                       \
	_complex(const char *d4 = data);                                       \
	note(""#_type" "#_v"");                                                \
	is(mp_check_##_type(data, d1), 0, "mp_check_"#_type"("#_v") == 0");    \
	is(mp_decode_##_type(&d2 _ext(COMMA &ext_type)), (_v), "mp_decode(mp_encode("#_v")) == "#_v);\
	_complex(mp_next(&d3));                                                \
	_complex(ok(!mp_check(&d4, d3 + _rl), "mp_check("#_v")"));             \
	is((d1 - data), (_rl), "len(mp_encode_"#_type"("#_v")");               \
	is(d1, d2, "len(mp_decode_"#_type"("#_v"))");                          \
	_complex(is(d1, d3, "len(mp_next_"#_type"("#_v"))"));                  \
	_complex(is(d1, d4, "len(mp_check_"#_type"("#_v"))"));                 \
	is(mp_sizeof_##_type(_v), _rl, "mp_sizeof_"#_type"("#_v")");           \
	is(memcmp(data, (_r), (_rl)), 0, "mp_encode("#_v") == "#_r);           \
	})


#define DEFINE_TEST_STRBINEXT(_type, _not_ext, _ext, _vl) ({		       \
	note(""#_type" len="#_vl"");                                           \
	char *s1 = str;                                                        \
	for (uint32_t i = 0; i < _vl; i++) {                                   \
		s1[i] = 'a' + i % 26;                                          \
	}								       \
	_ext(int8_t ext_type = 0);					       \
	const char *d1 = mp_encode_##_type(data, _ext(ext_type COMMA) s1, _vl);\
	const char *d2;                                                        \
	uint32_t len2;                                                         \
	d2 = data;                                                             \
	const char *s2 = mp_decode_##_type(&d2, _ext(&ext_type COMMA) &len2);  \
	is(_vl, len2, "len(mp_decode_"#_type"(x, %u))", _vl);                  \
	_ext(is(ext_type, 0, "type(mp_decode_"#_type"(x))"));		       \
	d2 = data;                                                             \
	_not_ext((void) mp_decode_strbin(&d2, &len2));                         \
	_ext((void) mp_decode_ext(&d2, &ext_type, &len2));		       \
	is(_vl, len2, "len(mp_decode_strbin(x, %u))", _vl);                    \
	const char *d3 = data;                                                 \
	mp_next(&d3);                                                          \
	const char *d4 = data;                                                 \
	ok(!mp_check(&d4, d3 + _vl),                                           \
		"mp_check_"#_type"(mp_encode_"#_type"(x, "#_vl"))");           \
	is(d1, d2, "len(mp_decode_"#_type"(x, "#_vl")");                       \
	is(d1, d3, "len(mp_next_"#_type"(x, "#_vl")");                         \
	is(d1, d4, "len(mp_check_"#_type"(x, "#_vl")");                        \
	is(mp_sizeof_##_type(_vl), (uint32_t) (d1 - data),                     \
		"mp_sizeof_"#_type"("#_vl")");                                 \
	is(memcmp(s1, s2, _vl), 0, "mp_encode_"#_type"(x, "#_vl") == x");      \
})

#define test_uint(...)   DEFINE_TEST(uint, SCALAR, COMPLEX, __VA_ARGS__)
#define test_int(...)    DEFINE_TEST(int, SCALAR, COMPLEX, __VA_ARGS__)
#define test_bool(...)   DEFINE_TEST(bool, SCALAR, COMPLEX, __VA_ARGS__)
#define test_float(...)  DEFINE_TEST(float, SCALAR, COMPLEX, __VA_ARGS__)
#define test_double(...) DEFINE_TEST(double, SCALAR, COMPLEX, __VA_ARGS__)
#define test_strl(...)   DEFINE_TEST(strl, COMPLEX, COMPLEX, __VA_ARGS__)
#define test_binl(...)   DEFINE_TEST(binl, COMPLEX, COMPLEX, __VA_ARGS__)
#define test_extl(...)	 DEFINE_TEST(extl, COMPLEX, SCALAR, __VA_ARGS__)
#define test_array(...)  DEFINE_TEST(array, COMPLEX, COMPLEX, __VA_ARGS__)
#define test_map(...)    DEFINE_TEST(map, COMPLEX, COMPLEX, __VA_ARGS__)
#define test_str(...)    DEFINE_TEST_STRBINEXT(str, SCALAR, COMPLEX, __VA_ARGS__)
#define test_bin(...)    DEFINE_TEST_STRBINEXT(bin, SCALAR, COMPLEX, __VA_ARGS__)
#define test_ext(...)	 DEFINE_TEST_STRBINEXT(ext, COMPLEX, SCALAR, __VA_ARGS__)

#define DEFINE_TEST_SAFE(_type, _complex, _ext, _v, _r, _rl) ({		       \
	note(""#_type"_safe");						       \
	_ext(int8_t ext_type = 0);					       \
	ptrdiff_t sz;							       \
	const char *d;							       \
	/* Test calculating size. */					       \
	sz = 0;								       \
	d = mp_encode_##_type##_safe(NULL, &sz, _ext(ext_type COMMA) (_v));    \
	is(-sz, (_rl), "size after mp_encode_"#_type"_safe(NULL, &sz)");       \
	is(d, NULL, "mp_encode_"#_type"_safe(NULL, &sz)");		       \
	/* Test encoding with no overflow. */				       \
	sz = _rl;							       \
	d = mp_encode_##_type##_safe(data, &sz, _ext(ext_type COMMA) (_v));    \
	is(sz, 0, "size after mp_encode_"#_type"_safe(buf, &sz)");	       \
	is((d - data), (_rl), "len of mp_encode_"#_type"_safe(buf, &sz)");     \
	is(memcmp(data, (_r), (_rl)), 0, "mp_encode_"#_type"_safe(buf, &sz)"); \
	/* Test encoding with with overflow. */				       \
	sz = _rl - 1;							       \
	d = mp_encode_##_type##_safe(data, &sz, _ext(ext_type COMMA) (_v));    \
	is(sz, -1, "size after mp_encode_"#_type"_safe(buf, &sz) overflow");   \
	is(d, data, "mp_encode_"#_type"_safe(buf, &sz) overflow");	       \
	/* Test encoding with sz == NULL. */				       \
	d = mp_encode_##_type##_safe(data, NULL, _ext(ext_type COMMA) (_v));   \
	is((d - data), (_rl), "len of mp_encode_"#_type"_safe(buf, NULL)");    \
	is(memcmp(data, (_r), (_rl)), 0, "mp_encode_"#_type"_safe(buf, NULL)");\
	})

#define DEFINE_TEST_STRBINEXT_SAFE(_type, _not_ext, _ext, _vl) ({	       \
	note(""#_type"_safe");						       \
	_ext(int8_t ext_type = 0);					       \
	ptrdiff_t sz;							       \
	const ptrdiff_t hl = mp_sizeof_##_type##l(_vl);			       \
	const ptrdiff_t rl = hl + _vl;					       \
	char head[5];							       \
	const char *d;							       \
	for (uint32_t i = 0; i < _vl; i++) {				       \
		str[i] = 'a' + i % 26;					       \
	}								       \
	mp_encode_##_type##l(head,_ext(ext_type COMMA) _vl);		       \
	/* Test calculating size. */					       \
	sz = 0;								       \
	d = mp_encode_##_type##_safe(NULL, &sz, _ext(ext_type COMMA) str, _vl);\
	is(-sz, rl, "size after mp_encode_"#_type"_safe(NULL, &sz)");	       \
	is(d, NULL, "mp_encode_"#_type"_safe(NULL, &sz)");		       \
	/* Test encoding with no overflow. */				       \
	sz = rl;							       \
	d = mp_encode_##_type##_safe(data, &sz, _ext(ext_type COMMA) str, _vl);\
	is(sz, 0, "size after mp_encode_"#_type"_safe(buf, &sz)");	       \
	is((d - data), rl, "len of mp_encode_"#_type"_safe(buf, &sz)");	       \
	is(memcmp(data, head, hl), 0,					       \
	   "head of mp_encode_"#_type"_safe(buf, &sz)");		       \
	is(memcmp(data + hl, str, (_vl)), 0,				       \
	   "payload of mp_encode_"#_type"_safe(buf, &sz)");		       \
	/* Test encoding with with overflow. */				       \
	sz = rl - 1;							       \
	d = mp_encode_##_type##_safe(data, &sz, _ext(ext_type COMMA) str, _vl);\
	is(sz, -1, "size after mp_encode_"#_type"_safe(buf, &sz) overflow");   \
	is(d, data, "mp_encode_"#_type"_safe(buf, &sz) overflow");	       \
	/* Test encoding with sz == NULL. */				       \
	d = mp_encode_##_type##_safe(data, NULL, _ext(ext_type COMMA)str, _vl);\
	is((d - data), rl, "len of mp_encode_"#_type"_safe(buf, NULL)");       \
	is(memcmp(data, head, hl), 0,					       \
	   "head of mp_encode_"#_type"_safe(buf, NULL)");		       \
	is(memcmp(data + hl, str, (_vl)), 0,				       \
	   "payload of mp_encode_"#_type"_safe(buf, NULL)");		       \
	})

#define test_uint_safe(...)   DEFINE_TEST_SAFE(uint, SCALAR, COMPLEX, __VA_ARGS__)
#define test_int_safe(...)    DEFINE_TEST_SAFE(int, SCALAR, COMPLEX, __VA_ARGS__)
#define test_bool_safe(...)   DEFINE_TEST_SAFE(bool, SCALAR, COMPLEX, __VA_ARGS__)
#define test_float_safe(...)  DEFINE_TEST_SAFE(float, SCALAR, COMPLEX, __VA_ARGS__)
#define test_double_safe(...) DEFINE_TEST_SAFE(double, SCALAR, COMPLEX, __VA_ARGS__)
#define test_strl_safe(...)   DEFINE_TEST_SAFE(strl, COMPLEX, COMPLEX, __VA_ARGS__)
#define test_binl_safe(...)   DEFINE_TEST_SAFE(binl, COMPLEX, COMPLEX, __VA_ARGS__)
#define test_extl_safe(...)   DEFINE_TEST_SAFE(extl, COMPLEX, SCALAR, __VA_ARGS__)
#define test_array_safe(...)  DEFINE_TEST_SAFE(array, COMPLEX, COMPLEX, __VA_ARGS__)
#define test_map_safe(...)    DEFINE_TEST_SAFE(map, COMPLEX, COMPLEX, __VA_ARGS__)
#define test_str_safe(...)    DEFINE_TEST_STRBINEXT_SAFE(str, SCALAR, COMPLEX, __VA_ARGS__)
#define test_bin_safe(...)    DEFINE_TEST_STRBINEXT_SAFE(bin, SCALAR, COMPLEX, __VA_ARGS__)
#define test_ext_safe(...)    DEFINE_TEST_STRBINEXT_SAFE(ext, COMPLEX, SCALAR, __VA_ARGS__)

static int
test_uints(void)
{
	plan(15*9 + 9);
	header();

	test_uint(0U, "\x00", 1);
	test_uint(1U, "\x01", 1);
	test_uint(0x7eU, "\x7e", 1);
	test_uint(0x7fU, "\x7f", 1);

	test_uint(0x80U, "\xcc\x80", 2);
	test_uint(0xfeU, "\xcc\xfe", 2);
	test_uint(0xffU, "\xcc\xff", 2);

	test_uint(0xfffeU, "\xcd\xff\xfe", 3);
	test_uint(0xffffU, "\xcd\xff\xff", 3);

	test_uint(0x10000U, "\xce\x00\x01\x00\x00", 5);
	test_uint(0xfffffffeU, "\xce\xff\xff\xff\xfe", 5);
	test_uint(0xffffffffU, "\xce\xff\xff\xff\xff", 5);

	test_uint(0x100000000ULL,
	     "\xcf\x00\x00\x00\x01\x00\x00\x00\x00", 9);
	test_uint(0xfffffffffffffffeULL,
	     "\xcf\xff\xff\xff\xff\xff\xff\xff\xfe", 9);
	test_uint(0xffffffffffffffffULL,
	     "\xcf\xff\xff\xff\xff\xff\xff\xff\xff", 9);

	test_uint_safe(0xfffeU, "\xcd\xff\xfe", 3);

	footer();
	return check_plan();
}

static int
test_ints(void)
{
	plan(17*9 + 9);
	header();

	test_int(-0x01, "\xff", 1);
	test_int(-0x1e, "\xe2", 1);
	test_int(-0x1f, "\xe1", 1);
	test_int(-0x20, "\xe0", 1);
	test_int(-0x21, "\xd0\xdf", 2);

	test_int(-0x7f, "\xd0\x81", 2);
	test_int(-0x80, "\xd0\x80", 2);

	test_int(-0x81, "\xd1\xff\x7f", 3);
	test_int(-0x7fff, "\xd1\x80\x01", 3);
	test_int(-0x8000, "\xd1\x80\x00", 3);

	test_int(-0x8001, "\xd2\xff\xff\x7f\xff", 5);
	test_int(-0x7fffffff, "\xd2\x80\x00\x00\x01", 5);
	test_int(-0x80000000LL, "\xd2\x80\x00\x00\x00", 5);

	test_int(-0x80000001LL,
	     "\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", 9);
	test_int(-0x80000001LL,
	     "\xd3\xff\xff\xff\xff\x7f\xff\xff\xff", 9);
	test_int(-0x7fffffffffffffffLL,
	     "\xd3\x80\x00\x00\x00\x00\x00\x00\x01", 9);
	test_int((int64_t)-0x8000000000000000LL,
	     "\xd3\x80\x00\x00\x00\x00\x00\x00\x00", 9);

	test_int_safe(-0x80000000LL, "\xd2\x80\x00\x00\x00", 5);

	footer();
	return check_plan();
}

static int
test_bools(void)
{
	plan(2*9 + 9);
	header();

	test_bool(true, "\xc3", 1);
	test_bool(false, "\xc2", 1);

	test_bool_safe(true, "\xc3", 1);

	footer();
	return check_plan();
}

static int
test_floats(void)
{
	plan(3*9 + 9);
	header();

	test_float((float) 1.0, "\xca\x3f\x80\x00\x00", 5);
	test_float((float) 3.141593, "\xca\x40\x49\x0f\xdc", 5);
	test_float((float) -1e38f, "\xca\xfe\x96\x76\x99", 5);

	test_float_safe((float) 3.141593, "\xca\x40\x49\x0f\xdc", 5);

	footer();
	return check_plan();
}

static int
test_doubles(void)
{
	plan(3*9 + 9);
	header();

	test_double((double) 1.0,
		    "\xcb\x3f\xf0\x00\x00\x00\x00\x00\x00", 9);
	test_double((double) 3.141592653589793,
		    "\xcb\x40\x09\x21\xfb\x54\x44\x2d\x18", 9);
	test_double((double) -1e99,
		    "\xcb\xd4\x7d\x42\xae\xa2\x87\x9f\x2e", 9);

	test_double_safe((double) 3.141592653589793,
			 "\xcb\x40\x09\x21\xfb\x54\x44\x2d\x18", 9);

	footer();
	return check_plan();
}

static int
test_nils(void)
{
	plan(15);
	header();

	const char *d1 = mp_encode_nil(data);
	const char *d2 = data;
	const char *d3 = data;
	const char *d4 = data;
	note("nil");
	mp_decode_nil(&d2);
	mp_next(&d3);
	ok(!mp_check(&d4, d3 + 1), "mp_check_nil()");
	is((d1 - data), 1, "len(mp_encode_nil() == 1");
	is(d1, d2, "len(mp_decode_nil()) == 1");
	is(d1, d3, "len(mp_next_nil()) == 1");
	is(d1, d4, "len(mp_check_nil()) == 1");
	is(mp_sizeof_nil(), 1, "mp_sizeof_nil() == 1");

	ptrdiff_t sz = 0;
	d1 = mp_encode_nil_safe(NULL, &sz);
	is(-sz, 1, "size after mp_encode_nil_safe(NULL, &sz)");
	is(d1, NULL, "mp_encode_nil_safe(NULL, &sz)");

	sz = 1;
	d1 = mp_encode_nil_safe(data, &sz);
	is(sz, 0, "size after mp_encode_nil_safe(buf, &sz)");
	is(d1 - data, 1, "len of mp_encode_nil_safe(buf, &sz)");
	is((unsigned char)data[0], 0xc0, "mp_encode_nil_safe(buf, &sz)");

	sz = 0;
	d1 = mp_encode_nil_safe(data, &sz);
	is(sz, -1, "size after mp_encode_nil_safe(buf, &sz) overflow");
	is(d1, data, "mp_encode_nil_safe(buf, &sz) overflow");

	d1 = mp_encode_nil_safe(data, NULL);
	is(d1 - data, 1, "len of mp_encode_nil_safe(buf, NULL)");
	is((unsigned char)data[0], 0xc0, "mp_encode_nil_safe(buf, NULL)");

	footer();
	return check_plan();
}

static int
test_arrays(void)
{
	plan(9*6 + 9);
	header();

	test_array(0, "\x90", 1);
	test_array(1, "\x91", 1);
	test_array(15, "\x9f", 1);
	test_array(16, "\xdc\x00\x10", 3);
	test_array(0xfffe, "\xdc\xff\xfe", 3);
	test_array(0xffff, "\xdc\xff\xff", 3);
	test_array(0x10000, "\xdd\x00\x01\x00\x00", 5);
	test_array(0xfffffffeU, "\xdd\xff\xff\xff\xfe", 5);
	test_array(0xffffffffU, "\xdd\xff\xff\xff\xff", 5);

	test_array_safe(0xffff, "\xdc\xff\xff", 3);

	footer();
	return check_plan();
}

static int
test_maps(void)
{
	plan(9*6 + 9);
	header();

	test_map(0, "\x80", 1);
	test_map(1, "\x81", 1);
	test_map(15, "\x8f", 1);
	test_map(16, "\xde\x00\x10", 3);
	test_map(0xfffe, "\xde\xff\xfe", 3);
	test_map(0xffff, "\xde\xff\xff", 3);
	test_map(0x10000, "\xdf\x00\x01\x00\x00", 5);
	test_map(0xfffffffeU, "\xdf\xff\xff\xff\xfe", 5);
	test_map(0xffffffffU, "\xdf\xff\xff\xff\xff", 5);

	test_map_safe(0xfffffffeU, "\xdf\xff\xff\xff\xfe", 5);

	footer();
	return check_plan();
}

static int
test_strls(void)
{
	plan(13*6 + 9);
	header();

	test_strl(0x00U, "\xa0", 1);
	test_strl(0x01U, "\xa1", 1);
	test_strl(0x1eU, "\xbe", 1);
	test_strl(0x1fU, "\xbf", 1);

	test_strl(0x20U, "\xd9\x20", 2);
	test_strl(0xfeU, "\xd9\xfe", 2);
	test_strl(0xffU, "\xd9\xff", 2);

	test_strl(0x0100U, "\xda\x01\x00", 3);
	test_strl(0xfffeU, "\xda\xff\xfe", 3);
	test_strl(0xffffU, "\xda\xff\xff", 3);

	test_strl(0x00010000U, "\xdb\x00\x01\x00\x00", 5);
	test_strl(0xfffffffeU, "\xdb\xff\xff\xff\xfe", 5);
	test_strl(0xffffffffU, "\xdb\xff\xff\xff\xff", 5);

	test_strl_safe(0x20U, "\xd9\x20", 2);

	footer();
	return check_plan();
}

static int
test_binls(void)
{
	plan(13*6 + 9);
	header();

	test_binl(0x00U, "\xc4\x00", 2);
	test_binl(0x01U, "\xc4\x01", 2);
	test_binl(0x1eU, "\xc4\x1e", 2);
	test_binl(0x1fU, "\xc4\x1f", 2);

	test_binl(0x20U, "\xc4\x20", 2);
	test_binl(0xfeU, "\xc4\xfe", 2);
	test_binl(0xffU, "\xc4\xff", 2);

	test_binl(0x0100U, "\xc5\x01\x00", 3);
	test_binl(0xfffeU, "\xc5\xff\xfe", 3);
	test_binl(0xffffU, "\xc5\xff\xff", 3);

	test_binl(0x00010000U, "\xc6\x00\x01\x00\x00", 5);
	test_binl(0xfffffffeU, "\xc6\xff\xff\xff\xfe", 5);
	test_binl(0xffffffffU, "\xc6\xff\xff\xff\xff", 5);

	test_binl_safe(0x00010000U, "\xc6\x00\x01\x00\x00", 5);

	footer();
	return check_plan();
}

static int
test_extls(void)
{
	plan(28*6 + 9);
	header();

	/* fixext 1,2,4,8,16 */
	test_extl(0x01U, "\xd4\x00", 2);
	test_extl(0x02U, "\xd5\x00", 2);
	test_extl(0x04U, "\xd6\x00", 2);
	test_extl(0x08U, "\xd7\x00", 2);
	test_extl(0x10U, "\xd8\x00", 2);

	/* ext 8 */
	test_extl(0x11U, "\xc7\x11\x00", 3);
	test_extl(0xfeU, "\xc7\xfe\x00", 3);
	test_extl(0xffU, "\xc7\xff\x00", 3);

	test_extl(0x00U, "\xc7\x00\x00", 3);
	test_extl(0x03U, "\xc7\x03\x00", 3);
	test_extl(0x05U, "\xc7\x05\x00", 3);
	test_extl(0x06U, "\xc7\x06\x00", 3);
	test_extl(0x07U, "\xc7\x07\x00", 3);
	test_extl(0x09U, "\xc7\x09\x00", 3);
	test_extl(0x0aU, "\xc7\x0a\x00", 3);
	test_extl(0x0bU, "\xc7\x0b\x00", 3);
	test_extl(0x0cU, "\xc7\x0c\x00", 3);
	test_extl(0x0dU, "\xc7\x0d\x00", 3);
	test_extl(0x0eU, "\xc7\x0e\x00", 3);
	test_extl(0x0fU, "\xc7\x0f\x00", 3);

	/* ext 16 */
	test_extl(0x0100U, "\xc8\x01\x00\x00", 4);
	test_extl(0x0101U, "\xc8\x01\x01\x00", 4);
	test_extl(0xfffeU, "\xc8\xff\xfe\x00", 4);
	test_extl(0xffffU, "\xc8\xff\xff\x00", 4);

	/* ext 32 */
	test_extl(0x00010000U, "\xc9\x00\x01\x00\x00\x00", 6);
	test_extl(0x00010001U, "\xc9\x00\x01\x00\x01\x00", 6);
	test_extl(0xfffffffeU, "\xc9\xff\xff\xff\xfe\x00", 6);
	test_extl(0xffffffffU, "\xc9\xff\xff\xff\xff\x00", 6);

	test_extl_safe(0x0aU, "\xc7\x0a\x00", 3);

	footer();
	return check_plan();
}

static int
test_strs(void)
{
	plan(12*8 + 11);
	header();

	test_str(0x01);
	test_str(0x1e);
	test_str(0x1f);
	test_str(0x20);
	test_str(0xfe);
	test_str(0xff);
	test_str(0x100);
	test_str(0x101);
	test_str(0xfffe);
	test_str(0xffff);
	test_str(0x10000);
	test_str(0x10001);

	test_str_safe(0xfffe);

	footer();
	return check_plan();
}

static int
test_bins(void)
{
	plan(12*8 + 11);
	header();

	test_bin(0x01);
	test_bin(0x1e);
	test_bin(0x1f);
	test_bin(0x20);
	test_bin(0xfe);
	test_bin(0xff);
	test_bin(0x100);
	test_bin(0x101);
	test_bin(0xfffe);
	test_bin(0xffff);
	test_bin(0x10000);
	test_bin(0x10001);

	test_bin_safe(0x10001);

	footer();
	return check_plan();
}

static int
test_exts(void)
{
	plan(25*9 + 11);
	header();

	test_ext(0x01);
	test_ext(0x02);
	test_ext(0x03);
	test_ext(0x04);
	test_ext(0x05);
	test_ext(0x06);
	test_ext(0x07);
	test_ext(0x08);
	test_ext(0x09);
	test_ext(0x0a);
	test_ext(0x0b);
	test_ext(0x0c);
	test_ext(0x0d);
	test_ext(0x0e);
	test_ext(0x0f);
	test_ext(0x10);

	test_ext(0x11);
	test_ext(0xfe);
	test_ext(0xff);

	test_ext(0x0100);
	test_ext(0x0101);
	test_ext(0xfffe);
	test_ext(0xffff);

	test_ext(0x00010000);
	test_ext(0x00010001);

	test_ext_safe(0xfe);

	footer();
	return check_plan();
}

static int
test_memcpy()
{
	plan(11);
	header();

	int len = 27;
	char *d;

	for (int i = 0; i < len; i++)
		str[i] = 'a' + i % 26;

	d = mp_memcpy(data, str, len);
	is(d - data, len, "len of mp_memcpy()");
	is(memcmp(data, str, len), 0, "payload of mp_memcpy()");

	ptrdiff_t sz = 0;
	d = mp_memcpy_safe(NULL, &sz, str, len);
	is(-sz, len, "size after mp_memcpy_safe(NULL, &sz)");
	is(d, NULL, "mp_memcpy_safe(NULL, &sz)");

	sz = len;
	d = mp_memcpy_safe(data, &sz, str, len);
	is(sz, 0, "size after mp_memcpy_safe(buf, &sz)");
	is(d - data, len, "len of mp_memcpy_safe(buf, &sz)");
	is(memcmp(data, str, len), 0, "mp_memcpy_safe(buf, &sz)");

	sz = len - 1;
	d = mp_memcpy_safe(data, &sz, str, len);
	is(sz, -1, "size after mp_memcpy_safe(buf, &sz) overflow");
	is(d, data, "mp_memcpy_safe(buf, &sz) overflow");

	d = mp_memcpy_safe(data, NULL, str, len);
	is(d - data, len, "len of (buf, NULL)");
	is(memcmp(data, str, len), 0, "mp_memcpy_safe(buf, NULL)");

	footer();
	return check_plan();
}

static void
test_next_on_array(uint32_t count)
{
	note("next/check on array(%u)", count);
	char *d1 = data;
	d1 = mp_encode_array(d1, count);
	for (uint32_t i = 0; i < count; i++) {
		d1 = mp_encode_uint(d1, i % 0x7f); /* one byte */
	}
	uint32_t len = count + mp_sizeof_array(count);
	const char *d2 = data;
	const char *d3 = data;
	ok(!mp_check(&d2, data + BUF_MAXLEN), "mp_check(array %u))", count);
	is((d1 - data), (ptrdiff_t)len, "len(array %u) == %u", count, len);
	is((d2 - data), (ptrdiff_t)len, "len(mp_check(array %u)) == %u", count, len);
	mp_next(&d3);
	is((d3 - data), (ptrdiff_t)len, "len(mp_next(array %u)) == %u", count, len);
}

static int
test_next_on_arrays(void)
{
	plan(52);
	header();

	test_next_on_array(0x00);
	test_next_on_array(0x01);
	test_next_on_array(0x0f);
	test_next_on_array(0x10);
	test_next_on_array(0x11);
	test_next_on_array(0xfe);
	test_next_on_array(0xff);
	test_next_on_array(0x100);
	test_next_on_array(0x101);
	test_next_on_array(0xfffe);
	test_next_on_array(0xffff);
	test_next_on_array(0x10000);
	test_next_on_array(0x10001);

	footer();
	return check_plan();
}

static void
test_next_on_map(uint32_t count)
{
	note("next/check on map(%u)", count);
	char *d1 = data;
	d1 = mp_encode_map(d1, count);
	for (uint32_t i = 0; i < 2 * count; i++) {
		d1 = mp_encode_uint(d1, i % 0x7f); /* one byte */
	}
	uint32_t len = 2 * count + mp_sizeof_map(count);
	const char *d2 = data;
	const char *d3 = data;
	ok(!mp_check(&d2, data + BUF_MAXLEN), "mp_check(map %u))", count);
	is((d1 - data), (ptrdiff_t)len, "len(map %u) == %u", count, len);
	is((d2 - data), (ptrdiff_t)len, "len(mp_check(map %u)) == %u", count, len);
	mp_next(&d3);
	is((d3 - data), (ptrdiff_t)len, "len(mp_next(map %u)) == %u", count, len);
}

static int
test_next_on_maps(void)
{
	plan(52);
	header();

	test_next_on_map(0x00);
	test_next_on_map(0x01);
	test_next_on_map(0x0f);
	test_next_on_map(0x10);
	test_next_on_map(0x11);
	test_next_on_map(0xfe);
	test_next_on_map(0xff);
	test_next_on_map(0x100);
	test_next_on_map(0x101);
	test_next_on_map(0xfffe);
	test_next_on_map(0xffff);
	test_next_on_map(0x10000);
	test_next_on_map(0x10001);

	footer();
	return check_plan();
}

/**
 * When inlined in Release in clang, this function behaves weird. Looking
 * sometimes like if its call had no effect even though it did encoding.
 */
static NOINLINE bool
test_encode_uint_custom_size(char *buf, uint64_t val, int size)
{
	char *pos;
	switch (size) {
	case 1:
		if (val > 0x7f)
			return false;
		mp_store_u8(buf, val);
		return true;
	case 2:
		if (val > UINT8_MAX)
			return false;
		pos = mp_store_u8(buf, 0xcc);
		mp_store_u8(pos, (uint8_t)val);
		return true;
	case 3:
		if (val > UINT16_MAX)
			return false;
		pos = mp_store_u8(buf, 0xcd);
		mp_store_u16(pos, (uint16_t)val);
		return true;
	case 5:
		if (val > UINT32_MAX)
			return false;
		pos = mp_store_u8(buf, 0xce);
		mp_store_u32(pos, (uint32_t)val);
		return true;
	case 9:
		pos = mp_store_u8(buf, 0xcf);
		mp_store_u64(pos, val);
		return true;
	}
	abort();
	return false;
}

/**
 * Unlike test_encode_uint_custom_size(), this one doesn't seem to behave
 * strange when inlined, but perhaps it just wasn't used in all the same ways as
 * the former.
 */
static NOINLINE bool
test_encode_int_custom_size(char *buf, int64_t val, int size)
{
	char *pos;
	switch (size) {
	case 1:
		if (val > 0x7f || val < -0x20)
			return false;
		if (val >= 0)
			mp_store_u8(buf, val);
		else
			mp_store_u8(buf, (uint8_t)(0xe0 | val));
		return true;
	case 2:
		if (val < INT8_MIN || val > INT8_MAX)
			return false;
		pos = mp_store_u8(buf, 0xd0);
		mp_store_u8(pos, (uint8_t)val);
		return true;
	case 3:
		if (val < INT16_MIN || val > INT16_MAX)
			return false;
		pos = mp_store_u8(buf, 0xd1);
		mp_store_u16(pos, (uint16_t)val);
		return true;
	case 5:
		if (val < INT32_MIN || val > INT32_MAX)
			return false;
		pos = mp_store_u8(buf, 0xd2);
		mp_store_u32(pos, (uint32_t)val);
		return true;
	case 9:
		pos = mp_store_u8(buf, 0xd3);
		mp_store_u64(pos, val);
		return true;
	}
	abort();
	return false;
}

static int
test_encode_uint_all_sizes(char mp_nums[][MP_NUMBER_MAX_LEN], uint64_t val)
{
	int sizes[] = {1, 2, 3, 5, 9};
	int count = lengthof(sizes);
	int used = 0;
	for (int i = 0; i < count; ++i) {
		assert(used < MP_NUMBER_CODEC_COUNT);
		if (test_encode_uint_custom_size(mp_nums[used], val, sizes[i]))
			++used;
	}
	return used;
}

static int
test_encode_int_all_sizes(char mp_nums[][MP_NUMBER_MAX_LEN], int64_t val)
{
	int sizes[] = {1, 2, 3, 5, 9};
	int count = lengthof(sizes);
	int used = 0;
	for (int i = 0; i < count; ++i) {
		assert(used < MP_NUMBER_CODEC_COUNT);
		if (test_encode_int_custom_size(mp_nums[used], val, sizes[i]))
			++used;
	}
	return used;
}

static void
test_compare_uint(uint64_t a, uint64_t b)
{
	char mp_nums_a[MP_NUMBER_CODEC_COUNT][MP_NUMBER_MAX_LEN];
	int count_a = test_encode_uint_all_sizes(mp_nums_a, a);

	char mp_nums_b[MP_NUMBER_CODEC_COUNT][MP_NUMBER_MAX_LEN];
	int count_b = test_encode_uint_all_sizes(mp_nums_b, b);

	for (int ai = 0; ai < count_a; ++ai) {
		for (int bi = 0; bi < count_b; ++bi) {
			int r = mp_compare_uint(mp_nums_a[ai], mp_nums_b[bi]);
			if (a < b) {
				ok(r < 0, "mp_compare_uint(%" PRIu64 ", "
				   "%" PRIu64 ") < 0", a, b);
			} else if (a > b) {
				ok(r > 0, "mp_compare_uint(%" PRIu64 ", "
				   "%" PRIu64 ") > 0", a, b);
			} else {
				ok(r == 0, "mp_compare_uint(%" PRIu64 ", "
				   "%" PRIu64 ") == 0", a, b);
			}
		}
	}
}

static int
test_compare_uints(void)
{
	plan(2209);
	header();

	uint64_t nums[] = {
		0, 1, 0x7eU, 0x7fU, 0x80U, 0xfeU, 0xffU, 0xfffeU, 0xffffU,
		0x10000U, 0xfffffffeU, 0xffffffffU, 0x100000000ULL,
		0xfffffffffffffffeULL, 0xffffffffffffffffULL
	};
	int count = sizeof(nums) / sizeof(*nums);
	for (int i = 0; i < count; i++) {
		for (int j = 0; j < count; j++) {
			test_compare_uint(nums[i], nums[j]);
		}
	}

	footer();
	return check_plan();
}

static bool
fequal(float a, float b)
{
	return a > b ? a - b < 1e-5f : b - a < 1e-5f;
}

static bool
dequal(double a, double b)
{
	return a > b ? a - b < 1e-10 : b - a < 1e-10;
}

static int
test_format(void)
{
	plan(282);
	header();

	const size_t buf_size = 1024;
	char buf[buf_size];
	size_t sz;
	const char *fmt;
	const char *p, *c, *e;
	uint32_t len = 0;

	fmt = "%d %u %i  %ld %lu %li  %lld %llu %lli"
	      "%hd %hu %hi  %hhd %hhu %hhi";
	sz = mp_format(buf, buf_size, fmt, 1, 2, 3,
		       (long)4, (long)5, (long)6,
		       (long long)7, (long long)8, (long long)9,
		       (short)10, (short)11, (short)12,
		       (char)13, (char)14, (char)15);
	p = buf;
	for (unsigned i = 0; i < 15; i++) {
		ok(mp_typeof(*p) == MP_UINT, "Test type on step %d", i);
		ok(mp_decode_uint(&p) == i + 1, "Test value on step %d", i);
	}
	sz = mp_format(buf, buf_size, fmt, -1, -2, -3,
		       (long)-4, (long)-5, (long)-6,
		       (long long)-7, (long long)-8, (long long)-9,
		       (short)-10, (unsigned short)-11, (short)-12,
		       (signed char)-13, (unsigned char)-14, (signed char)-15);
	p = buf;
	for (int i = 0; i < 15; i++) {
		uint64_t expects[5] = { UINT_MAX - 1,
					ULONG_MAX - 4,
					ULLONG_MAX - 7,
					USHRT_MAX - 10,
					UCHAR_MAX - 13 };
		if (i % 3 == 1) {
			ok(mp_typeof(*p) == MP_UINT, "Test type on step %d", i);
			ok(mp_decode_uint(&p) == expects[i / 3],
			   "Test value on step %d", i);
		} else {
			ok(mp_typeof(*p) == MP_INT, "Test type on step %d", i);
			ok(mp_decode_int(&p) == - i - 1,
			   "Test value on step %d", i);
		}
	}

	char data1[32];
	char *data1_end = data1;
	data1_end = mp_encode_array(data1_end, 2);
	data1_end = mp_encode_str(data1_end, "ABC", 3);
	data1_end = mp_encode_uint(data1_end, 11);
	size_t data1_len = data1_end - data1;
	assert(data1_len <= sizeof(data1));

	char data2[32];
	char *data2_end = data2;
	data2_end = mp_encode_int(data2_end, -1234567890);
	data2_end = mp_encode_str(data2_end, "DEFGHIJKLMN", 11);
	data2_end = mp_encode_uint(data2_end, 321);
	size_t data2_len = data2_end - data2;
	assert(data2_len <= sizeof(data2));

	fmt = "%d NIL [%d %b %b] this is test"
		"[%d %%%% [[ %d {%s %f %%  %.*s %lf %.*s NIL}"
		"%p %d %.*p ]] %d%d%d]";
#define TEST_PARAMS 0, 1, true, false, -1, 2, \
	"flt", 0.1, 6, "double#ignored", 0.2, 0, "ignore", \
	data1, 3, data2_len, data2, 4, 5, 6
	sz = mp_format(buf, buf_size, fmt, TEST_PARAMS);
	p = buf;
	e = buf + sz;

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_UINT, "type");
	ok(mp_decode_uint(&p) == 0, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_NIL, "type");
	mp_decode_nil(&p);

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_ARRAY, "type");
	ok(mp_decode_array(&p) == 3, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_UINT, "type");
	ok(mp_decode_uint(&p) == 1, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_BOOL, "type");
	ok(mp_decode_bool(&p) == true, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_BOOL, "type");
	ok(mp_decode_bool(&p) == false, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_ARRAY, "type");
	ok(mp_decode_array(&p) == 5, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_INT, "type");
	ok(mp_decode_int(&p) == -1, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_ARRAY, "type");
	ok(mp_decode_array(&p) == 1, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_ARRAY, "type");
	ok(mp_decode_array(&p) == 5, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_UINT, "type");
	ok(mp_decode_uint(&p) == 2, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_MAP, "type");
	ok(mp_decode_map(&p) == 3, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_STR, "type");
	c = mp_decode_str(&p, &len);
	ok(len == 3, "decode");
	ok(memcmp(c, "flt", 3) == 0, "compare");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_FLOAT, "type");
	ok(fequal(mp_decode_float(&p), 0.1), "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_STR, "type");
	c = mp_decode_str(&p, &len);
	ok(len == 6, "decode");
	ok(memcmp(c, "double", 6) == 0, "compare");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_DOUBLE, "type");
	ok(dequal(mp_decode_double(&p), 0.2), "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_STR, "type");
	c = mp_decode_str(&p, &len);
	ok(len == 0, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_NIL, "type");
	mp_decode_nil(&p);

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(((size_t)(c - p) == data1_len) &&
	   memcmp(p, data1, data1_len) == 0, "compare");
	p = c;

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_UINT, "type");
	ok(mp_decode_uint(&p) == 3, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_INT, "type");
	ok(mp_decode_int(&p) == -1234567890, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_STR, "type");
	c = mp_decode_str(&p, &len);
	ok(len == 11, "decode");
	ok(memcmp(c, "DEFGHIJKLMN", 11) == 0, "compare");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_UINT, "type");
	ok(mp_decode_uint(&p) == 321, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_UINT, "type");
	ok(mp_decode_uint(&p) == 4, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_UINT, "type");
	ok(mp_decode_uint(&p) == 5, "decode");

	c = p;
	ok(mp_check(&c, e) == 0, "check");
	ok(mp_typeof(*p) == MP_UINT, "type");
	ok(mp_decode_uint(&p) == 6, "decode");

	ok(p == e, "nothing more");

	ok(sz < 70, "no magic detected");

	for (size_t lim = 0; lim <= 70; lim++) {
		memset(buf, 0, buf_size);
		size_t test_sz = mp_format(buf, lim, fmt, TEST_PARAMS);
		ok(test_sz == sz, "return value on step %d", (int)lim);
		bool all_zero = true;
		for(size_t z = lim; z < buf_size; z++)
			all_zero = all_zero && (buf[z] == 0);
		ok(all_zero, "buffer overflow on step %d", (int)lim);

	}

#undef TEST_PARAMS

	footer();
	return check_plan();
}

int
test_mp_print()
{
	plan(12);
	header();

	char msgpack[128];
	char *d = msgpack;
	d = mp_encode_array(d, 8);
	d = mp_encode_int(d, -5);
	d = mp_encode_uint(d, 42);
	d = mp_encode_str(d, "kill/bill", 9);
	d = mp_encode_array(d, 0);
	d = mp_encode_map(d, 6);
	d = mp_encode_str(d, "bool true", 9);
	d = mp_encode_bool(d, true);
	d = mp_encode_str(d, "bool false", 10);
	d = mp_encode_bool(d, false);
	d = mp_encode_str(d, "null", 4);
	d = mp_encode_nil(d);
	d = mp_encode_str(d, "float", 5);
	d = mp_encode_float(d, 3.14);
	d = mp_encode_str(d, "double", 6);
	d = mp_encode_double(d, 3.14);
	d = mp_encode_uint(d, 100);
	d = mp_encode_uint(d, 500);
	/* MP_EXT with type 123 and of size 3 bytes. */
	d = mp_encode_extl(d, 123, 3);
	memcpy(d, "str", 3);
	d += 3;
	char bin[] = "\x12test\x34\b\t\n\"bla\\-bla\"\f\r";
	d = mp_encode_bin(d, bin, sizeof(bin));
	d = mp_encode_map(d, 0);
	assert(d <= msgpack + sizeof(msgpack));

	const char *expected =
		"[-5, 42, \"kill/bill\", [], "
		"{\"bool true\": true, \"bool false\": false, \"null\": null, "
		"\"float\": 3.14, \"double\": 3.14, 100: 500}, "
		"(extension: type 123, len 3), "
		"\"\\u0012test4\\b\\t\\n\\\"bla\\\\-bla\\\"\\f\\r\\u0000\", {}]";
	int esize = strlen(expected);

	char result[256];

	int fsize = mp_snprint(result, sizeof(result), msgpack);
	ok(fsize == esize, "mp_snprint return value");
	ok(strcmp(result, expected) == 0, "mp_snprint result");

	fsize = mp_snprint(NULL, 0, msgpack);
	ok(fsize == esize, "mp_snprint limit = 0");

	fsize = mp_snprint(result, 1, msgpack);
	ok(fsize == esize && result[0] == '\0', "mp_snprint limit = 1");

	fsize = mp_snprint(result, 2, msgpack);
	ok(fsize == esize && result[1] == '\0', "mp_snprint limit = 2");

	fsize = mp_snprint(result, esize, msgpack);
	ok(fsize == esize && result[esize - 1] == '\0',
	   "mp_snprint limit = expected");

	fsize = mp_snprint(result, esize + 1, msgpack);
	ok(fsize == esize && result[esize] == '\0',
	   "mp_snprint limit = expected + 1");

	FILE *tmpf = tmpfile();
	if (tmpf != NULL) {
		int fsize = mp_fprint(tmpf, msgpack);
		ok(fsize == esize, "mp_fprint return value");
		(void) rewind(tmpf);
		int rsize = fread(result, 1, sizeof(result), tmpf);
		ok(rsize == esize && memcmp(result, expected, esize) == 0,
		   "mp_fprint result");
		fclose(tmpf);
	}

	/* stdin is read-only */
	int rc = mp_fprint(stdin, msgpack);
	is(rc, -1, "mp_fprint I/O error");

	/* Test mp_snprint max nesting depth. */
	int mp_buff_sz = (MP_PRINT_MAX_DEPTH + 1) * mp_sizeof_array(1) +
			 mp_sizeof_uint(1);
	int exp_str_sz = 2 * (MP_PRINT_MAX_DEPTH + 1) + 3 + 1;
	char *mp_buff = (char *)malloc(mp_buff_sz);
	char *exp_str = (char *)malloc(exp_str_sz);
	char *decoded = (char *)malloc(exp_str_sz);
	char *buff_wptr = mp_buff;
	char *exp_str_wptr = exp_str;
	for (int i = 0; i <= 2 * (MP_PRINT_MAX_DEPTH + 1); i++) {
		int exp_str_rest = exp_str_sz - (exp_str_wptr - exp_str);
		assert(exp_str_rest > 0);
		if (i < MP_PRINT_MAX_DEPTH + 1) {
			buff_wptr = mp_encode_array(buff_wptr, 1);
			rc = snprintf(exp_str_wptr, exp_str_rest, "[");
		} else if (i == MP_PRINT_MAX_DEPTH + 1) {
			buff_wptr = mp_encode_uint(buff_wptr, 1);
			rc = snprintf(exp_str_wptr, exp_str_rest, "...");
		} else {
			rc = snprintf(exp_str_wptr, exp_str_rest, "]");
		}
		exp_str_wptr += rc;
	}
	assert(exp_str_wptr + 1 == exp_str + exp_str_sz);
	rc = mp_snprint(decoded, exp_str_sz, mp_buff);
	ok(rc == exp_str_sz - 1, "mp_snprint max nesting depth return value");
	ok(strcmp(decoded, exp_str) == 0, "mp_snprint max nesting depth result");
	free(decoded);
	free(exp_str);
	free(mp_buff);
	footer();
	return check_plan();
}

enum mp_ext_test_type {
	MP_EXT_TEST_PLAIN,
	MP_EXT_TEST_MSGPACK,
};

static int
mp_fprint_ext_test(FILE *file, const char **data, int depth)
{
	int8_t type;
	uint32_t len = mp_decode_extl(data, &type);
	const char *ext = *data;
	*data += len;
	switch(type) {
	case MP_EXT_TEST_PLAIN:
		return fprintf(file, "%.*s", len, ext);
	case MP_EXT_TEST_MSGPACK:
		return mp_fprint_recursion(file, &ext, depth);
	}
	return fprintf(file, "undefined");
}

static int
mp_snprint_ext_test(char *buf, int size, const char **data, int depth)
{
	int8_t type;
	uint32_t len = mp_decode_extl(data, &type);
	const char *ext = *data;
	*data += len;
	switch(type) {
	case MP_EXT_TEST_PLAIN:
		return snprintf(buf, size, "%.*s", len, ext);
	case MP_EXT_TEST_MSGPACK:
		return mp_snprint_recursion(buf, size, &ext, depth);
	}
	return snprintf(buf, size, "undefined");
}

static int
test_mp_print_ext(void)
{
	plan(5);
	header();
	mp_snprint_ext = mp_snprint_ext_test;
	mp_fprint_ext = mp_fprint_ext_test;

	char *pos = buf;
	const char *plain = "plain-str";
	size_t plain_len = strlen(plain);
	pos = mp_encode_array(pos, 4);
	pos = mp_encode_uint(pos, 100);
	pos = mp_encode_ext(pos, MP_EXT_TEST_PLAIN, plain, plain_len);
	pos = mp_encode_extl(pos, MP_EXT_TEST_MSGPACK,
			     mp_sizeof_str(plain_len));
	pos = mp_encode_str(pos, plain, plain_len);
	pos = mp_encode_uint(pos, 200);

	int size = mp_snprint(NULL, 0, buf);
	int real_size = mp_snprint(str, sizeof(str), buf);
	is(size, real_size, "mp_snrpint size match");
	const char *expected = "[100, plain-str, \"plain-str\", 200]";
	is(strcmp(str, expected), 0, "str is correct");

	FILE *tmpf = tmpfile();
	if (tmpf == NULL)
		abort();
	real_size = mp_fprint(tmpf, buf);
	is(size, real_size, "mp_fprint size match");
	rewind(tmpf);
	real_size = (int) fread(str, 1, sizeof(str), tmpf);
	is(real_size, size, "mp_fprint written correct number of bytes");
	str[real_size] = 0;
	is(strcmp(str, expected), 0, "str is correct");
	fclose(tmpf);

	mp_snprint_ext = mp_snprint_ext_default;
	mp_fprint_ext = mp_fprint_ext_default;
	footer();
	return check_plan();
}

int
test_mp_check()
{
	plan(71);
	header();

#define invalid(data, fmt, ...) ({ \
	const char *p = data; \
	isnt(mp_check(&p, p + sizeof(data) - 1), 0, fmt, ## __VA_ARGS__); \
})

	/* fixmap */
	invalid("\x81", "invalid fixmap 1");
	invalid("\x81\x01", "invalid fixmap 2");
	invalid("\x8f\x01", "invalid fixmap 3");

	/* fixarray */
	invalid("\x91", "invalid fixarray 1");
	invalid("\x92\x01", "invalid fixarray 2");
	invalid("\x9f\x01", "invalid fixarray 3");

	/* fixstr */
	invalid("\xa1", "invalid fixstr 1");
	invalid("\xa2\x00", "invalid fixstr 2");
	invalid("\xbf\x00", "invalid fixstr 3");

	/* bin8 */
	invalid("\xc4", "invalid bin8 1");
	invalid("\xc4\x01", "invalid bin8 2");

	/* bin16 */
	invalid("\xc5", "invalid bin16 1");
	invalid("\xc5\x00\x01", "invalid bin16 2");

	/* bin32 */
	invalid("\xc6", "invalid bin32 1");
	invalid("\xc6\x00\x00\x00\x01", "invalid bin32 2");

	/* ext8 */
	invalid("\xc7", "invalid ext8 1");
	invalid("\xc7\x00", "invalid ext8 2");
	invalid("\xc7\x01\xff", "invalid ext8 3");
	invalid("\xc7\x02\xff\x00", "invalid ext8 4");

	/* ext16 */
	invalid("\xc8", "invalid ext16 1");
	invalid("\xc8\x00\x00", "invalid ext16 2");
	invalid("\xc8\x00\x01\xff", "invalid ext16 3");
	invalid("\xc8\x00\x02\xff\x00", "invalid ext16 4");

	/* ext32 */
	invalid("\xc9", "invalid ext32 1");
	invalid("\xc9\x00\x00\x00\x00", "invalid ext32 2");
	invalid("\xc9\x00\x00\x00\x01\xff", "invalid ext32 3");
	invalid("\xc9\x00\x00\x00\x02\xff\x00", "invalid ext32 4");

	/* float32 */
	invalid("\xca", "invalid float32 1");
	invalid("\xca\x00\x00\x00", "invalid float32 2");

	/* float64 */
	invalid("\xcb", "invalid float64 1");
	invalid("\xcb\x00\x00\x00\x00\x00\x00\x00", "invalid float64 2");

	/* uint8 */
	invalid("\xcc", "invalid uint8 1");

	/* uint16 */
	invalid("\xcd\x00", "invalid uint16 1");

	/* uint32 */
	invalid("\xce\x00\x00\x00", "invalid uint32 1");

	/* uint64 */
	invalid("\xcf\x00\x00\x00\x00\x00\x00\x00", "invalid uint64 1");

	/* int8 */
	invalid("\xd0", "invalid int8 1");

	/* int16 */
	invalid("\xd1\x00", "invalid int16 1");

	/* int32 */
	invalid("\xd2\x00\x00\x00", "invalid int32 1");

	/* int64 */
	invalid("\xd3\x00\x00\x00\x00\x00\x00\x00", "invalid int64 1");

	/* fixext8 */
	invalid("\xd4", "invalid fixext8 1");
	invalid("\xd4\x05", "invalid fixext8 2");

	/* fixext16 */
	invalid("\xd5", "invalid fixext16 1");
	invalid("\xd5\x05\x05", "invalid fixext16 2");

	/* fixext32 */
	invalid("\xd6", "invalid fixext32 1");
	invalid("\xd6\x00\x00\x05\x05", "invalid fixext32 2");

	/* fixext64 */
	invalid("\xd7", "invalid fixext64 1");
	invalid("\xd7\x00\x00\x00\x00\x00\x00\x05\x05", "invalid fixext64 2");

	/* fixext128 */
	invalid("\xd8", "invalid fixext128 1");
	invalid("\xd8\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x05\x05", "invalid fixext128 2");

	/* str8 */
	invalid("\xd9", "invalid str8 1");
	invalid("\xd9\x01", "invalid str8 2");

	/* str16 */
	invalid("\xda", "invalid str16 1");
	invalid("\xda\x00\x01", "invalid str16 2");

	/* str32 */
	invalid("\xdb", "invalid str32 1");
	invalid("\xdb\x00\x00\x00\x01", "invalid str32 2");

	/* array16 */
	invalid("\xdc", "invalid array16 1");
	invalid("\xdc\x00\x01", "invalid array16 2");

	/* array32 */
	invalid("\xdd", "invalid array32 1");
	invalid("\xdd\x00\x00\x00\x01", "invalid array32 2");

	/* map16 */
	invalid("\xde", "invalid map16 1");
	invalid("\xde\x00\x01", "invalid map16 2");
	invalid("\xde\x00\x01\x5", "invalid map16 3");
	invalid("\xde\x80\x00", "invalid map16 4");

	/* map32 */
	invalid("\xdf", "invalid map32 1");
	invalid("\xdf\x00\x00\x00\x01", "invalid map32 2");
	invalid("\xdf\x00\x00\x00\x01\x5", "invalid map32 3");
	invalid("\xdf\x80\x00\x00\x00", "invalid map32 4");

	/* 0xc1 is never used */
	invalid("\xc1", "invalid header 1");
	invalid("\x91\xc1", "invalid header 2");
	invalid("\x93\xff\xc1\xff", "invalid header 3");
	invalid("\x82\xff\xc1\xff\xff", "invalid header 4");

#undef invalid

	footer();

	return check_plan();
}

static int
test_mp_check_exact(void)
{
	plan(11);
	header();

#define invalid(data, fmt, ...) ({ \
	const char *p = data; \
	isnt(mp_check_exact(&p, p + sizeof(data) - 1), 0, fmt, ## __VA_ARGS__); \
})
#define valid(data, fmt, ...) ({ \
	const char *p = data; \
	is(mp_check_exact(&p, p + sizeof(data) - 1), 0, fmt, ## __VA_ARGS__); \
})

	invalid("", "empty");
	invalid("\x92\xc0\xc1", "ill");

	invalid("\x92", "trunc 1");
	invalid("\x92\xc0", "trunc 2");
	invalid("\x93\xc0\xc0", "trunc 3");

	invalid("\xc0\xc0", "junk 1");
	invalid("\x92\xc0\xc0\xc0", "junk 2");
	invalid("\x92\xc0\x91\xc0\xc0", "junk 3");

	valid("\xc0", "valid 1");
	valid("\x91\xc0", "valid 2");
	valid("\x92\xc0\x91\xc0", "valid 3");

#undef valid
#undef invalid

	footer();
	return check_plan();
}

/**
 * Check validity of a simple extension type.
 * Its type is 0x42 and payload is a byte string where i-th byte value is its
 * position from the end of the string.
 */
int
mp_check_ext_data_test(int8_t type, const char *data, uint32_t len)
{
	if (type != 0x42)
		return 1;
	for (uint32_t i = 0; i < len; i++) {
		if ((unsigned char)data[i] != len - i)
			return 1;
	}
	return 0;
}

int
test_mp_check_ext_data()
{
	plan(24);
	header();

#define invalid(data, fmt, ...) ({ \
	const char *p = data; \
	isnt(mp_check(&p, p + sizeof(data) - 1), 0, fmt, ## __VA_ARGS__); \
})
#define valid(data, fmt, ...) ({ \
	const char *p = data; \
	is(mp_check(&p, p + sizeof(data) - 1), 0, fmt, ## __VA_ARGS__); \
})

	mp_check_ext_data_f mp_check_ext_data_svp = mp_check_ext_data;
	mp_check_ext_data = mp_check_ext_data_test;

	/* ext8 */
	invalid("\xc7\x00\x13", "invalid ext8 - bad type");
	invalid("\xc7\x01\x42\x13", "invalid ext8 - bad data");
	valid("\xc7\x01\x42\x01", "valid ext8");

	/* ext16 */
	invalid("\xc8\x00\x00\x13", "invalid ext16 - bad type");
	invalid("\xc8\x00\x01\x42\x13", "invalid ext16 - bad data");
	valid("\xc8\x00\x01\x42\x01", "valid ext16");

	/* ext32 */
	invalid("\xc9\x00\x00\x00\x00\x13", "invalid ext32 - bad type");
	invalid("\xc9\x00\x00\x00\x01\x42\x13", "invalid ext32 - bad data");
	valid("\xc9\x00\x00\x00\x01\x42\x01", "valid ext32");

	/* fixext8 */
	invalid("\xd4\x13\x01", "invalid fixext8 - bad type");
	invalid("\xd4\x42\x13", "invalid fixext8 - bad data");
	valid("\xd4\x42\x01", "valid fixext8");

	/* fixext16 */
	invalid("\xd5\x13\x02\x01", "invalid fixext16 - bad type");
	invalid("\xd5\x42\x13\x13", "invalid fixext16 - bad data");
	valid("\xd5\x42\x02\x01", "valid fixext16");

	/* fixext32 */
	invalid("\xd6\x13\x04\x03\x02\x01", "invalid fixext32 - bad type");
	invalid("\xd6\x42\x13\x13\x13\x13", "invalid fixext32 - bad data");
	valid("\xd6\x42\x04\x03\x02\x01", "valid fixext32");

	/* fixext64 */
	invalid("\xd7\x13\x08\x07\x06\x05\x04\x03\x02\x01",
		"invalid fixext64 - bad type");
	invalid("\xd7\x42\x13\x13\x13\x13\x13\x13\x13\x13",
		"invalid fixext64 - bad data");
	valid("\xd7\x42\x08\x07\x06\x05\x04\x03\x02\x01",
	      "valid fixext64");

	/* fixext128 */
	invalid("\xd8\x13\x10\x0f\x0e\x0d\x0c\x0b\x0a\x09\x08\x07\x06\x05\x04"
		"\x03\x02\x01", "invalid fixext128 - bad type");
	invalid("\xd8\x42\x13\x13\x13\x13\x13\x13\x13\x13\x13\x13\x13\x13\x13"
		"\x13\x13\x13", "invalid fixext128 - bad data");
	valid("\xd8\x42\x10\x0f\x0e\x0d\x0c\x0b\x0a\x09\x08\x07\x06\x05\x04"
	      "\x03\x02\x01", "valid fixext128");

	mp_check_ext_data = mp_check_ext_data_svp;

#undef valid
#undef invalid

	footer();
	return check_plan();
}

static struct mp_check_error last_error;

static void
mp_check_on_error_test(const struct mp_check_error *err)
{
	last_error = *err;
}

static int
test_mp_check_error(void)
{
	const int trunc_error_count = 30;
	const int ill_error_count = 3;
	const int ext_error_count = 4;
	const int junk_error_count = 3;
	plan(6 * trunc_error_count +
	     5 * ill_error_count +
	     7 * ext_error_count +
	     5 * junk_error_count);
	header();

	mp_check_ext_data_f mp_check_ext_data_svp = mp_check_ext_data;
	mp_check_ext_data = mp_check_ext_data_test;
	mp_check_on_error_f mp_check_on_error_svp = mp_check_on_error;
	mp_check_on_error = mp_check_on_error_test;

#define check_error(check_, data_, offset_, type_, trunc_count_,	\
		    ext_type_, ext_len_, msg_)				\
	do {			\
		const char *data = data_;				\
		const char *end = data + sizeof(data_) - 1;		\
		const char *p = data;					\
		isnt(check_(&p, end), 0, msg_);				\
		is(last_error.type, type_, msg_ " - error type");	\
		is(last_error.data, data, msg_ " - error data");	\
		is(last_error.end, end, msg_ " - error data end");	\
		is(last_error.pos - last_error.data, (ptrdiff_t)offset_,\
		   msg_ " - error data pos");				\
		if (last_error.type == MP_CHECK_ERROR_TRUNC) {		\
			is(last_error.trunc_count, trunc_count_,	\
			   msg_ " - error trunc count");		\
		}							\
		if (last_error.type == MP_CHECK_ERROR_EXT) {		\
			is(last_error.ext_type, ext_type_,		\
			   msg_ " - error ext type");			\
			is(last_error.ext_len, ext_len_,		\
			   msg_ " - error ext len");			\
		}							\
	} while (0)

#define check_error_trunc(data_, offset_, trunc_count_, msg_)		\
	check_error(mp_check, data_, offset_, MP_CHECK_ERROR_TRUNC,	\
		    trunc_count_, 0, 0, msg_)

#define check_error_ill(data_, offset_, msg_)				\
	check_error(mp_check, data_, offset_, MP_CHECK_ERROR_ILL,	\
		    0, 0, 0, msg_)

#define check_error_ext(data_, offset_, ext_type_, ext_len_, msg_)	\
	check_error(mp_check, data_, offset_, MP_CHECK_ERROR_EXT,	\
		    0, ext_type_, ext_len_, msg_)

#define check_error_junk(data_, offset_, msg_)				\
	check_error(mp_check_exact, data_, offset_, MP_CHECK_ERROR_JUNK,\
		    0, 0, 0, msg_)

	check_error_trunc("", 0, 1, "empty");

	check_error_trunc("\xa2", 0, 1, "trunc fixstr");
	check_error_trunc("\xd9", 0, 1, "trunc str8 header");
	check_error_trunc("\xd9\x10", 0, 1, "trunc str8 data");
	check_error_trunc("\xda\x00", 0, 1, "trunc str16 header");
	check_error_trunc("\xda\x00\x10", 0, 1, "trunc str16 data");
	check_error_trunc("\xdb\x00\x00\x00", 0, 1, "trunc str32 header");
	check_error_trunc("\xdb\x00\x00\x00\x10", 0, 1, "trunc str32 data");

	check_error_trunc("\x92", 1, 2, "trunc fixarray");
	check_error_trunc("\xdc\x00", 0, 1, "trunc array16 header");
	check_error_trunc("\xdc\x00\x10", 3, 16, "trunc array16 data");
	check_error_trunc("\xdd\x00\x00\x00", 0, 1, "trunc array32 header");
	check_error_trunc("\xdd\x00\x00\x00\x10", 5, 16, "trunc array32 data");

	check_error_trunc("\x82", 1, 4, "trunc fixmap");
	check_error_trunc("\xde\x00", 0, 1, "trunc map16 header");
	check_error_trunc("\xde\x00\x10", 3, 32, "trunc map16 data");
	check_error_trunc("\xdf\x00\x00\x00", 0, 1, "trunc map32 header");
	check_error_trunc("\xdf\x00\x00\x00\x10", 5, 32, "trunc map32 data");

	check_error_trunc("\xd4", 0, 1, "trunc fixext header");
	check_error_trunc("\xd4\x42", 0, 1, "trunc fixext data");
	check_error_trunc("\xc7\x10", 0, 1, "trunc ext8 header");
	check_error_trunc("\xc7\x10\x42", 0, 1, "trunc ext8 data");
	check_error_trunc("\xc8\x00\x10", 0, 1, "trunc ext16 header");
	check_error_trunc("\xc8\x00\x10\x42", 0, 1, "trunc ext16 data");
	check_error_trunc("\xc9\x00\x00\x00\x10", 0, 1, "trunc ext32 header");
	check_error_trunc("\xc9\x00\x00\x00\x10\x42", 0, 1, "trunc ext32 data");

	check_error_trunc("\x92\x82", 2, 5, "trunc nested 1");
	check_error_trunc("\x92\x82\xc0", 3, 4, "trunc nested 2");
	check_error_trunc("\x92\x82\xc0\x92", 4, 5, "trunc nested 3");
	check_error_trunc("\x92\x82\xc0\x92\x82", 5, 8, "trunc nested 4");

	check_error_ill("\xc1", 0, "ill 1");
	check_error_ill("\x92\xc1", 1, "ill 2");
	check_error_ill("\x92\xc0\xc1", 2, "ill 3");

	check_error_ext("\xd4\x42\x00", 2, 0x42, 1, "bad fixext");
	check_error_ext("\xc7\x01\x42\x00", 3, 0x42, 1, "bad ext8");
	check_error_ext("\xc8\x00\x01\x42\x00", 4, 0x42, 1, "bad ext16");
	check_error_ext("\xc9\x00\x00\x00\x01\x42\x00", 6, 0x42, 1, "bad ext32");

	check_error_junk("\xc0\xc0", 1, "junk 1");
	check_error_junk("\xc0\x91\xc1", 1, "junk 2");
	check_error_junk("\x92\xc0\xc0\xc0", 3, "junk 3");

#undef check_error_junk
#undef check_error_ext
#undef check_error_ill
#undef check_error_trunc
#undef check_error

	mp_check_ext_data = mp_check_ext_data_svp;
	mp_check_on_error = mp_check_on_error_svp;

	footer();
	return check_plan();
}

#define double_eq(a, b) (fabs((a) - (b)) < 1e-15)

template<typename TargetT, typename ValueT, typename ReadF>
static void
test_read_num(ValueT num1, ReadF read_f, bool is_ok)
{
	/*
	 * Build the header message.
	 */
	const int str_cap = 256;
	char str[str_cap];
	char *end = str + str_cap;
	char *pos = str + snprintf(str, str_cap, "typed read of ");
	if (std::is_same<ValueT, float>::value)
		pos += snprintf(pos, end - pos, "%f", (float)num1);
	else if (std::is_same<ValueT, double>::value)
		pos += snprintf(pos, end - pos, "%lf", (double)num1);
	else if (num1 >= 0)
		pos += snprintf(pos, end - pos, "%llu", (long long)num1);
	else
		pos += snprintf(pos, end - pos, "%lld", (long long)num1);
	pos += snprintf(pos, end - pos, " into ");

	static_assert(!std::is_same<TargetT, float>::value,
		      "no float reading");
	if (std::is_integral<TargetT>::value) {
		pos += snprintf(pos, end - pos, "int%zu_t",
				sizeof(TargetT) * 8);
	} else {
		pos += snprintf(pos, end - pos, "double");
	}
	note("%s", str);
	/*
	 * Perform the actual tests.
	 */
	char mp_nums[MP_NUMBER_CODEC_COUNT][MP_NUMBER_MAX_LEN];
	int count = 0;
	if (std::is_integral<ValueT>::value) {
		if (num1 >= 0) {
			count += test_encode_uint_all_sizes(
				&mp_nums[count], num1);
		}
		if (num1 <= INT64_MAX) {
			count += test_encode_int_all_sizes(
				&mp_nums[count], num1);
		}
	} else if (!std::is_integral<TargetT>::value || !is_ok) {
		/*
		 * Only encode floating point types when
		 * 1) expect to also decode them back successfully.
		 * 2) want to fail to decode an integer.
		 *
		 * Encoding integers as floats for decoding them back into
		 * integers won't work.
		 */
		if (std::is_same<ValueT, float>::value)
			mp_encode_float(mp_nums[count++], (float)num1);
		mp_encode_double(mp_nums[count++], (double)num1);
	}
	for (int i = 0; i < count; ++i) {
		const char *mp_num_pos1 = mp_nums[i];
		char code = *mp_num_pos1;
		/* Sanity check of the test encoding. */
		if (mp_typeof(*mp_num_pos1) == MP_INT) {
			fail_unless(mp_decode_int(&mp_num_pos1) ==
				    (int64_t)num1);
		} else if (mp_typeof(*mp_num_pos1) == MP_UINT) {
			fail_unless(mp_decode_uint(&mp_num_pos1) ==
				    (uint64_t)num1);
		} else if (mp_typeof(*mp_num_pos1) == MP_FLOAT) {
			fail_unless(mp_decode_float(&mp_num_pos1) ==
				    (float)num1);
		} else {
			fail_unless(mp_decode_double(&mp_num_pos1) ==
				    (double)num1);
		}

		const char *mp_num_pos2 = mp_nums[i];
		TargetT num2 = 0;
		int rc = read_f(&mp_num_pos2, &num2);
		if (!is_ok) {
			is(rc, -1, "check failure for code 0x%02X", code);
			is(mp_num_pos2, mp_nums[i], "check position");
			continue;
		}
		is(rc, 0, "check success for code 0x%02X", code);
		is(mp_num_pos1, mp_num_pos2, "check position");
		if (!std::is_integral<TargetT>::value) {
			ok(double_eq(num1, num2), "check float number");
			continue;
		}
		if (num1 >= 0) {
			fail_unless(num2 >= 0);
			is((uint64_t)num1, (uint64_t)num2, "check int number");
		} else {
			fail_unless(num2 < 0);
			is((int64_t)num1, (int64_t)num2, "check int number");
		}
	}
}

template<typename TargetT, typename ReadF>
static void
test_read_num_from_non_numeric_mp(ReadF read_f)
{
	const int str_cap = 256;
	char str[str_cap];
	char *end = str + str_cap;
	char *pos = str + snprintf(str, str_cap, "ensure failure of reading ");
	if (std::is_integral<TargetT>::value) {
		pos += snprintf(pos, end - pos, "int%zu_t",
				sizeof(TargetT) * 8);
	} else if (read_f == (void *)mp_read_double) {
		pos += snprintf(pos, end - pos, "double");
	} else if (read_f == (void *)mp_read_double_lossy) {
		pos += snprintf(pos, end - pos, "double with precision loss");
	} else {
		abort();
	}
	note("%s", str);

	char bad_types[16][16];
	int bad_count = 0;
	mp_encode_array(bad_types[bad_count++], 1);
	mp_encode_map(bad_types[bad_count++], 1);
	mp_encode_str0(bad_types[bad_count++], "abc");
	mp_encode_bool(bad_types[bad_count++], true);
	mp_encode_ext(bad_types[bad_count++], 1, "abc", 3);
	mp_encode_bin(bad_types[bad_count++], "abc", 3);
	mp_encode_nil(bad_types[bad_count++]);
	for (int i = 0; i < bad_count; ++i) {
		TargetT val;
		const char *pos = bad_types[i];
		char code = *pos;
		int rc = read_f(&pos, &val);
		is(rc, -1, "check fail for code 0x%02X", code);
		is(pos, bad_types[i], "check position for code 0x%02X", code);
	}
}

#define test_read_int8(num, success)						\
	test_read_num<int8_t>(num, mp_read_int8, success)
#define test_read_int16(num, success)						\
	test_read_num<int16_t>(num, mp_read_int16, success)
#define test_read_int32(num, success)						\
	test_read_num<int32_t>(num, mp_read_int32, success)
#define test_read_int64(num, success)						\
	test_read_num<int64_t>(num, mp_read_int64, success)
#define test_read_double(num, success)						\
	test_read_num<double>(num, mp_read_double, success)
#define test_read_double_lossy(num, success)					\
	test_read_num<double>(num, mp_read_double_lossy, success)

static int
test_mp_read_typed()
{
	plan(716);
	header();

	test_read_int8(12, true);
	test_read_int8(127, true);
	test_read_int8(128, false);
	test_read_int8(-12, true);
	test_read_int8(-128, true);
	test_read_int8(-129, false);
	test_read_int8(-3e-4f, false);
	test_read_int8(123.45, false);
	test_read_num_from_non_numeric_mp<int8_t>(mp_read_int8);

	test_read_int16(123, true);
	test_read_int16(32767, true);
	test_read_int16(32768, false);
	test_read_int16(-123, true);
	test_read_int16(-32768, true);
	test_read_int16(-32769, false);
	test_read_int16(-2e-3f, false);
	test_read_int16(12.345, false);
	test_read_num_from_non_numeric_mp<int16_t>(mp_read_int16);

	test_read_int32(123, true);
	test_read_int32(12345, true);
	test_read_int32(2147483647, true);
	test_read_int32(2147483648, false);
	test_read_int32(-123, true);
	test_read_int32(-12345, true);
	test_read_int32(-2147483648, true);
	test_read_int32(-2147483649LL, false);
	test_read_int32(-1e2f, false);
	test_read_int32(1.2345, false);
	test_read_num_from_non_numeric_mp<int32_t>(mp_read_int32);

	test_read_int64(123, true);
	test_read_int64(12345, true);
	test_read_int64(123456789, true);
	test_read_int64(9223372036854775807ULL, true);
	test_read_int64(9223372036854775808ULL, false);
	test_read_int64(-123, true);
	test_read_int64(-12345, true);
	test_read_int64(-123456789, true);
	test_read_int64(-9223372036854775807LL, true);
	test_read_int64(100.0f, false);
	test_read_int64(-5.4321, false);
	test_read_num_from_non_numeric_mp<int64_t>(mp_read_int64);

	test_read_double(123, true);
	test_read_double(12345, true);
	test_read_double(123456789, true);
	test_read_double(1234567890000ULL, true);
	test_read_double(123456789123456789ULL, false);
	test_read_double(-123, true);
	test_read_double(-12345, true);
	test_read_double(-123456789, true);
	test_read_double(-1234567890000LL, true);
	test_read_double(-123456789123456789LL, false);
	test_read_double(6.565e6f, true);
	test_read_double(-5.555, true);
	test_read_num_from_non_numeric_mp<double>(mp_read_double);

	test_read_double_lossy(123, true);
	test_read_double_lossy(12345, true);
	test_read_double_lossy(123456789, true);
	test_read_double_lossy(1234567890000ULL, true);
	test_read_double_lossy(123456789123456789ULL, true);
	test_read_double_lossy(-123, true);
	test_read_double_lossy(-12345, true);
	test_read_double_lossy(-123456789, true);
	test_read_double_lossy(-1234567890000LL, true);
	test_read_double_lossy(-123456789123456789LL, true);
	test_read_double_lossy(6.565e6f, true);
	test_read_double_lossy(-5.555, true);
	test_read_num_from_non_numeric_mp<double>(mp_read_double_lossy);

	footer();
	return check_plan();
}

static int
test_overflow()
{
	plan(5);
	header();

	const char *chk;
	char *d;
	d = data;
	chk = data;
	d = mp_encode_array(d, 1);
	d = mp_encode_array(d, UINT32_MAX);
	is(mp_check(&chk, d), 1, "mp_check array overflow");

	d = data;
	chk = data;
	d = mp_encode_array(d, 1);
	d = mp_encode_map(d, UINT32_MAX);
	is(mp_check(&chk, d), 1, "mp_check map overflow");

	d = data;
	chk = data;
	d = mp_encode_array(d, 2);
	d = mp_encode_str(d, "", 0);
	d = mp_encode_strl(d, UINT32_MAX);
	is(mp_check(&chk, d), 1, "mp_check str overflow");

	d = data;
	chk = data;
	d = mp_encode_array(d, 2);
	d = mp_encode_bin(d, "", 0);
	d = mp_encode_binl(d, UINT32_MAX);
	is(mp_check(&chk, d), 1, "mp_check bin overflow");

	d = data;
	chk = data;
	d = mp_encode_array(d, 2);
	d = mp_encode_str0(d, "");
	d = mp_encode_strl(d, UINT32_MAX);
	is(mp_check(&chk, d), 1, "mp_check str overflow");

	footer();
	return check_plan();
}


int main()
{
	plan(27);
	header();

	test_uints();
	test_ints();
	test_bools();
	test_floats();
	test_doubles();
	test_nils();
	test_strls();
	test_binls();
	test_extls();
	test_strs();
	test_bins();
	test_exts();
	test_arrays();
	test_maps();
	test_memcpy();
	test_next_on_arrays();
	test_next_on_maps();
	test_compare_uints();
	test_format();
	test_mp_print();
	test_mp_print_ext();
	test_mp_check();
	test_mp_check_exact();
	test_mp_check_ext_data();
	test_mp_check_error();
	test_mp_read_typed();
	test_overflow();

	footer();
	return check_plan();
}
