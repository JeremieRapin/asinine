/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "asinine/asn1.h"
#include "asinine/macros.h"

#include "asinine/test.h"
#include "asinine/tests/asn1.h"

#define TEST_OID1 ASN1_CONST_OID(1,1,2,4)
#define TEST_OID2 ASN1_CONST_OID(2,999,1)

#define TOKEN_(typ, dat, len) \
	{ .class = ASN1_CLASS_UNIVERSAL, .type = (typ), .data = (dat), \
		.length = (len) }
#define STR_TOKEN(typ, str) TOKEN_(typ, (uint8_t*)(str), strlen(str))
#define TOKEN(typ, data) TOKEN_(typ, data, sizeof(data))

#define RAW(type, ...) type, PP_NARG(__VA_ARGS__), __VA_ARGS__
#define EMPTY_RAW(type) type, 0x00
#define SEQ(...) RAW(0x30, __VA_ARGS__)
#define EMPTY_SEQ() EMPTY_RAW(0x30)
#define INT(...) RAW(0x02, __VA_ARGS__)
#define EMPTY_INT() EMPTY_RAW(0x02)
#define OID(...) RAW(0x06, __VA_ARGS__)
#define EMPTY_OID() EMPTY_RAW(0x06)

#define NUM(x) (sizeof(x) / sizeof(x[0]))

static char*
test_asn1_oid_decode(void)
{
	static const uint8_t raw[] = {
		SEQ(
			OID(0x29, 0x02, 0x04), // TEST_OID1
			OID(0x88, 0x37, 0x01)  // TEST_OID2
		)
	};

	asn1_parser_t parser;
	asn1_token_t token;
	asn1_oid_t oid;

	check(asn1_parser_init(&parser, &token, raw, sizeof(raw)) == ASN1_OK);
	check(asn1_parser_next(&parser) == ASN1_OK);

	check(asn1_parser_next(&parser) == ASN1_OK);
	check(asn1_oid(&token, &oid) == ASN1_OK);
	check(asn1_oid_eq(&oid, TEST_OID1));

	check(asn1_parser_next(&parser) == ASN1_OK);
	check(asn1_oid(&token, &oid) == ASN1_OK);
	check(asn1_oid_eq(&oid, TEST_OID2));

	return 0;
}

static char*
test_asn1_oid_decode_invalid(void)
{
	static const uint8_t invalid_padding[] = {
		SEQ(
			OID(0x01, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7F),
			OID(0x80, 0x01),
			OID(0x80, 0x7F),
			EMPTY_OID()
		)
	};

	asn1_parser_t parser;
	asn1_token_t token;
	asn1_oid_t oid;

	check(asn1_parser_init(&parser, &token, invalid_padding,
		sizeof(invalid_padding)) == ASN1_OK);

	check(asn1_parser_next(&parser) == ASN1_OK);
	check(asn1_oid(&token, &oid) == ASN1_ERROR_INVALID);
	check(asn1_parser_next(&parser) == ASN1_OK);
	check(asn1_oid(&token, &oid) == ASN1_ERROR_INVALID);
	check(asn1_parser_next(&parser) == ASN1_OK);
	check(asn1_oid(&token, &oid) == ASN1_ERROR_INVALID);
	check(asn1_parser_next(&parser) == ASN1_OK);
	check(asn1_oid(&token, &oid) == ASN1_ERROR_INVALID);

	return 0;
}

static char*
test_asn1_oid_to_string(void)
{
	char oid_str[128];
	const asn1_oid_t oid = ASN1_OID(1,2,3);
	const asn1_oid_t invalid_oid = ASN1_OID(1);

	check(asn1_oid_to_string(&oid, oid_str, sizeof(oid_str)) == ASN1_OK);
	check(strncmp("1.2.3", oid_str, 5) == 0);

	check(asn1_oid_to_string(&invalid_oid, oid_str, sizeof(oid_str))
		== ASN1_ERROR_INVALID);

	return 0;
}

static char*
test_asn1_oid_comparison(void)
{
	const asn1_oid_t a = ASN1_OID_FROM_CONST(TEST_OID1);
	const asn1_oid_t b = ASN1_OID(1,2,3);
	const asn1_oid_t c = ASN1_OID_FROM_CONST(TEST_OID1);

	check(asn1_oid_eq(&a, TEST_OID1));
	check(!asn1_oid_eq(&b, TEST_OID1));

	check(asn1_oid_cmp(&a, &b) < 0);
	check(asn1_oid_cmp(&b, &a) > 0);
	check(asn1_oid_cmp(&a, &c) == 0);

	return 0;
}

static char*
test_asn1_parse(void)
{
	// TODO: Add empty sequence
	static const uint8_t raw[] = {
		SEQ( // 0
			SEQ( // 1
				INT(0x01), // 2
				INT(0x02)  // 3
			),
			// TODO: Should this int encoding be invalid?
			INT(0x80, 0x10), // 4
			SEQ(INT(0x11)), // 5 (6)
			SEQ( // 7
				INT(0x01), // 8
				SEQ( // 9
					SEQ(INT(0x02)), INT(0x03) // 10 (11) 12
				)
			)
		)
	};

	asn1_parser_t parser;
	asn1_token_t token;
	int value;

	check(asn1_parser_init(&parser, &token, raw, sizeof(raw)) == ASN1_OK);

	check(asn1_parser_next(&parser) == ASN1_OK);
	check(asn1_is_sequence(&token));

	check(asn1_parser_next(&parser) == ASN1_OK);
	check(asn1_is_sequence(&token));

	check(asn1_parser_next(&parser) == ASN1_OK);
	check(asn1_is_int(&token));
	check(asn1_integer(&token, &value) == ASN1_OK);
	check(value == 0x01);

	check(asn1_parser_next(&parser) == ASN1_OK);
	check(asn1_is_int(&token));
	check(asn1_integer(&token, &value) == ASN1_OK);
	check(value == 0x02);

	check(asn1_parser_next(&parser) == ASN1_OK);
	check(asn1_is_int(&token));
	check(asn1_integer(&token, &value) == ASN1_OK);
	check(value == -0x10);

	check(asn1_parser_next(&parser) == ASN1_OK);
	check(asn1_is_sequence(&token));

	check(asn1_parser_next(&parser) == ASN1_OK);
	check(asn1_is_int(&token));
	check(asn1_integer(&token, &value) == ASN1_OK);
	check(value == 0x11);

	check(asn1_parser_next(&parser) == ASN1_OK);
	check(asn1_is_sequence(&token));

	check(asn1_parser_next(&parser) == ASN1_OK);
	check(asn1_is_int(&token));
	check(asn1_integer(&token, &value) == ASN1_OK);
	check(value == 0x01);

	check(asn1_parser_next(&parser) == ASN1_OK);
	check(asn1_is_sequence(&token));

	check(asn1_parser_next(&parser) == ASN1_OK);
	check(asn1_is_sequence(&token));

	check(asn1_parser_next(&parser) == ASN1_OK);
	check(asn1_is_int(&token));
	check(asn1_integer(&token, &value) == ASN1_OK);
	check(value == 0x02);

	check(asn1_parser_next(&parser) == ASN1_OK);
	check(asn1_is_int(&token));
	check(asn1_integer(&token, &value) == ASN1_OK);
	check(value == 0x03);

	return 0;
}

static char*
test_asn1_parse_invalid(void)
{
	// Indefinite length
	const uint8_t invalid1[] = {0x06, 0x80};
	// Reserved
	const uint8_t invalid2[] = {0x06, 0xFF};
	// Garbage after root token
	static const uint8_t invalid3[] = {
		SEQ(INT(0x01)),
		0xDE, 0xAD, 0xBE, 0xEF
	};

	asn1_parser_t parser;
	asn1_token_t token;

	check(asn1_parser_init(&parser, &token, invalid1, sizeof(invalid1))
		== ASN1_OK);
	check(asn1_parser_next(&parser) == ASN1_ERROR_INVALID);

	check(asn1_parser_init(&parser, &token, invalid2, sizeof(invalid2))
		== ASN1_OK);
	check(asn1_parser_next(&parser) == ASN1_ERROR_INVALID);

	check(asn1_parser_init(&parser, &token, invalid3, sizeof(invalid3))
		== ASN1_OK);
	check(asn1_parser_next(&parser) == ASN1_ERROR_INVALID);

	check(asn1_parser_init(&parser, &token, NULL, 0) == ASN1_ERROR_INVALID);

	return 0;
}

static char*
test_asn1_parse_time(void)
{
	// Unix epoch
	const char epoch_raw[] = "700101000000Z";
	const asn1_token_t epoch_token = STR_TOKEN(ASN1_TYPE_UTCTIME, epoch_raw);

	// Y2K
	const char y2k_raw[] = "000101000000Z";
	const asn1_token_t y2k_token = STR_TOKEN(ASN1_TYPE_UTCTIME, y2k_raw);

	// February has 29 days in leap years
	const char leap_feb_raw[] = "000229000000Z";
	const asn1_token_t leap_feb_token = STR_TOKEN(ASN1_TYPE_UTCTIME,
		leap_feb_raw);

	// Y2K38
	const char y2k38_raw[] = "380119031408Z";
	const asn1_token_t y2k38_token = STR_TOKEN(ASN1_TYPE_UTCTIME, y2k38_raw);

	asn1_time_t time;

	check(asn1_time(&epoch_token, &time) == ASN1_OK);
	check(time == 0);

	check(asn1_time(&y2k_token, &time) == ASN1_OK);
	check(time == 946684800);

	check(asn1_time(&leap_feb_token, &time) == ASN1_OK);
	check(time == 951782400);

	check(asn1_time(&y2k38_token, &time) == ASN1_OK);
	check(time == 2147483648);

	return 0;
}

static char*
test_asn1_parse_invalid_time(void)
{
	// Garbage
	const char garbage_raw[] = "ZYMMDDHHMMSS0";
	const asn1_token_t garbage_token = STR_TOKEN(ASN1_TYPE_UTCTIME,
		garbage_raw);

	// Incomplete time
	const char incomplete_raw[] = "01010";
	const asn1_token_t incomplete_token = STR_TOKEN(ASN1_TYPE_UTCTIME,
		incomplete_raw);

	// Timezone needs to be specified
	const char missing_tz_raw[] = "010101010101";
	const asn1_token_t missing_tz_token = STR_TOKEN(ASN1_TYPE_UTCTIME,
		missing_tz_raw);

	// Midnight is encoded as 000000 (HHMMSS)
	const char midnight_raw[] = "100101240000Z";
	const asn1_token_t midnight_token = STR_TOKEN(ASN1_TYPE_UTCTIME,
		midnight_raw);

	// February only has 29 days in leap years (% 4 == 0)
	const char leap_year_raw[] = "010229000000Z";
	const asn1_token_t leap_year_token = STR_TOKEN(ASN1_TYPE_UTCTIME,
		leap_year_raw);

	// April only has 30 days
	const char days_raw[] = "010431000000Z";
	const asn1_token_t days_token = STR_TOKEN(ASN1_TYPE_UTCTIME,
		days_raw);

	asn1_time_t time;

	check(asn1_time(&garbage_token, &time) == ASN1_ERROR_INVALID);
	check(asn1_time(&incomplete_token, &time) == ASN1_ERROR_INVALID);
	check(asn1_time(&missing_tz_token, &time) == ASN1_ERROR_INVALID);
	check(asn1_time(&midnight_token, &time) == ASN1_ERROR_INVALID);
	check(asn1_time(&leap_year_token, &time) == ASN1_ERROR_INVALID);
	check(asn1_time(&days_token, &time) == ASN1_ERROR_INVALID);

	return 0;
}

int
test_asn1_all(int *tests_run)
{
	declare_set;

	run_test(test_asn1_oid_decode);
	run_test(test_asn1_oid_decode_invalid);
	run_test(test_asn1_oid_to_string);
	run_test(test_asn1_oid_comparison);
	run_test(test_asn1_parse);
	run_test(test_asn1_parse_invalid);
	run_test(test_asn1_parse_time);
	run_test(test_asn1_parse_invalid_time);

	end_set;
}