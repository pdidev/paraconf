/* Copyright (C) The Paraconf development team, see COPYRIGHT.md file at the
 *               root of the project or at https://github.com/pdidev/paraconf
 *
 * SPDX-License-Identifier: MIT
 */

#include <climits>
#include <cstdlib>
#include <ostream>
#include <string>

#include <gtest/gtest.h>

#include <paraconf.h>

#include "paraconf_test.h"

/** Tests of the functions that read the content of a node: PC_len, PC_int, PC_double, PC_string and PC_bool
 */
using Scalars = ParaconfTest;

// PC_len

TEST_F(Scalars, len_of_a_sequence)
{
	int len = -1;
	EXPECT_EQ(PC_OK, PC_len(parse("[1, 2, 3]"), &len));
	EXPECT_EQ(3, len);
	EXPECT_EQ(PC_OK, PC_len(parse("[]"), &len));
	EXPECT_EQ(0, len);
}

TEST_F(Scalars, len_of_a_mapping)
{
	int len = -1;
	EXPECT_EQ(PC_OK, PC_len(parse("{a: 1, b: 2}"), &len));
	EXPECT_EQ(2, len);
	EXPECT_EQ(PC_OK, PC_len(parse("{}"), &len));
	EXPECT_EQ(0, len);
}

TEST_F(Scalars, len_of_a_scalar_is_its_length_in_bytes)
{
	int len = -1;
	EXPECT_EQ(PC_OK, PC_len(parse("hello"), &len));
	EXPECT_EQ(5, len);
	EXPECT_EQ(PC_OK, PC_len(parse("\"\""), &len));
	EXPECT_EQ(0, len);
	EXPECT_EQ(PC_OK, PC_len(parse("h\xC3\xA9llo"), &len));
	EXPECT_EQ(6, len);
}

TEST_F(Scalars, len_of_a_tree_in_error)
{
	PC_tree_t missing = PC_get(parse("{}"), ".missing");
	expect_error(PC_NODE_NOT_FOUND);
	int len = -1;
	EXPECT_EQ(PC_NODE_NOT_FOUND, PC_len(missing, &len));
	EXPECT_EQ(-1, len);
}

// PC_int

/// a YAML value and the integer it holds
struct IntCase {
	const char* yaml;
	long value;
};

void PrintTo(const IntCase& param, std::ostream* os)
{
	*os << ::testing::PrintToString(param.yaml);
}

class Int
	: public ParaconfTest
	, public ::testing::WithParamInterface<IntCase>
{};

TEST_P(Int, is_read)
{
	long value = -1;
	EXPECT_EQ(PC_OK, PC_int(parse(GetParam().yaml), &value));
	EXPECT_EQ(GetParam().value, value);
}

INSTANTIATE_TEST_SUITE_P(
	Value,
	Int,
	::testing::Values(IntCase{"0", 0}, IntCase{"42", 42}, IntCase{"-42", -42}, IntCase{"+42", 42}, IntCase{"0x1F", 31}, IntCase{"\"42\"", 42})
);

class InvalidInt
	: public ParaconfTest
	, public ::testing::WithParamInterface<const char*>
{};

TEST_P(InvalidInt, is_an_error)
{
	long value = -1;
	PC_status_t status = PC_int(parse(GetParam()), &value);
	EXPECT_NE(PC_OK, status);
	expect_error(status);
	EXPECT_EQ(-1, value);
}

INSTANTIATE_TEST_SUITE_P(Value, InvalidInt, ::testing::Values("abc", "1.5", "12abc", "1e3", "\"\"", ""));

TEST_F(Scalars, int_limits)
{
	long value = -1;
	EXPECT_EQ(PC_OK, PC_int(parse(std::to_string(LONG_MAX).c_str()), &value));
	EXPECT_EQ(LONG_MAX, value);
	EXPECT_EQ(PC_OK, PC_int(parse(std::to_string(LONG_MIN).c_str()), &value));
	EXPECT_EQ(LONG_MIN, value);
}

TEST_F(Scalars, int_overflow)
{
	// both limits end with a digit lower than 9 whatever the size of long, so incrementing it in the string overflows
	std::string above_max = std::to_string(LONG_MAX);
	++above_max.back();
	std::string below_min = std::to_string(LONG_MIN);
	++below_min.back();
	for (auto&& yaml: {above_max, below_min, std::string("99999999999999999999999999")}) {
		long value = -1;
		PC_status_t status = PC_int(parse(yaml.c_str()), &value);
		EXPECT_NE(PC_OK, status) << yaml << " read as " << value;
		expect_error(status, yaml);
		EXPECT_EQ(-1, value);
	}
}

TEST_F(Scalars, int_of_a_non_scalar)
{
	long value = -1;
	EXPECT_EQ(PC_INVALID_NODE_TYPE, PC_int(parse("[1]"), &value));
	expect_error(PC_INVALID_NODE_TYPE, "Expected a scalar, found sequence");
	EXPECT_EQ(PC_INVALID_NODE_TYPE, PC_int(parse("{a: 1}"), &value));
	expect_error(PC_INVALID_NODE_TYPE, "Expected a scalar, found mapping");
	EXPECT_EQ(-1, value);
}

TEST_F(Scalars, int_of_a_tree_in_error)
{
	PC_tree_t missing = PC_get(parse("{}"), ".missing");
	expect_error(PC_NODE_NOT_FOUND);
	long value = -1;
	EXPECT_EQ(PC_NODE_NOT_FOUND, PC_int(missing, &value));
	EXPECT_EQ(-1, value);
}

// PC_double

/// a YAML value and the floating point number it holds
struct DoubleCase {
	const char* yaml;
	double value;
};

void PrintTo(const DoubleCase& param, std::ostream* os)
{
	*os << ::testing::PrintToString(param.yaml);
}

class Double
	: public ParaconfTest
	, public ::testing::WithParamInterface<DoubleCase>
{};

TEST_P(Double, is_read)
{
	double value = -1;
	EXPECT_EQ(PC_OK, PC_double(parse(GetParam().yaml), &value));
	EXPECT_DOUBLE_EQ(GetParam().value, value);
}

INSTANTIATE_TEST_SUITE_P(
	Value,
	Double,
	::testing::Values(
		DoubleCase{"0", 0.},
		DoubleCase{"1.5", 1.5},
		DoubleCase{"-2.25", -2.25},
		DoubleCase{"+2.25", 2.25},
		DoubleCase{"3", 3.},
		DoubleCase{"1e3", 1e3},
		DoubleCase{"1.5E-3", 1.5e-3},
		DoubleCase{".5", .5},
		DoubleCase{"\"1.5\"", 1.5}
	)
);

class InvalidDouble
	: public ParaconfTest
	, public ::testing::WithParamInterface<const char*>
{};

TEST_P(InvalidDouble, is_an_error)
{
	double value = -1;
	PC_status_t status = PC_double(parse(GetParam()), &value);
	EXPECT_NE(PC_OK, status);
	expect_error(status);
}

INSTANTIATE_TEST_SUITE_P(Value, InvalidDouble, ::testing::Values("abc", "1.5x", "1.2.3", "\"\"", "1e999"));

TEST_F(Scalars, double_of_a_non_scalar)
{
	double value = -1;
	EXPECT_EQ(PC_INVALID_NODE_TYPE, PC_double(parse("[1.5]"), &value));
	expect_error(PC_INVALID_NODE_TYPE, "Expected a scalar, found sequence");
	EXPECT_EQ(-1, value);
}

TEST_F(Scalars, double_of_a_tree_in_error)
{
	PC_tree_t missing = PC_get(parse("{}"), ".missing");
	expect_error(PC_NODE_NOT_FOUND);
	double value = -1;
	EXPECT_EQ(PC_NODE_NOT_FOUND, PC_double(missing, &value));
	EXPECT_EQ(-1, value);
}

// PC_string

/// a YAML value and the string it holds
struct StringCase {
	const char* yaml;
	const char* value;
};

void PrintTo(const StringCase& param, std::ostream* os)
{
	*os << ::testing::PrintToString(param.yaml);
}

class String
	: public ParaconfTest
	, public ::testing::WithParamInterface<StringCase>
{};

TEST_P(String, is_read)
{
	char* value = nullptr;
	EXPECT_EQ(PC_OK, PC_string(parse(GetParam().yaml), &value));
	ASSERT_NE(nullptr, value);
	EXPECT_STREQ(GetParam().value, value);
	free(value);
}

INSTANTIATE_TEST_SUITE_P(
	Value,
	String,
	::testing::Values(
		StringCase{"plain text", "plain text"},
		StringCase{"\"double quoted: with \\\"escapes\\\"\\n\"", "double quoted: with \"escapes\"\n"},
		StringCase{"'single quoted: ''it'''", "single quoted: 'it'"},
		StringCase{"\"\"", ""},
		StringCase{"42", "42"},
		StringCase{"true", "true"},
		StringCase{"|\n  line 1\n  line 2\n", "line 1\nline 2\n"},
		StringCase{">\n  folded\n  text\n", "folded text\n"}
	)
);

TEST_F(Scalars, string_in_utf8)
{
	char* value = nullptr;
	EXPECT_EQ(PC_OK, PC_string(parse("h\xC3\xA9llo"), &value));
	EXPECT_STREQ("h\xC3\xA9llo", value);
	free(value);
}

TEST_F(Scalars, string_is_a_copy)
{
	PC_tree_t tree = parse("original");
	char* value = nullptr;
	ASSERT_EQ(PC_OK, PC_string(tree, &value));
	value[0] = 'X';
	free(value);
	ASSERT_EQ(PC_OK, PC_string(tree, &value));
	EXPECT_STREQ("original", value);
	free(value);
}

TEST_F(Scalars, string_of_a_non_scalar)
{
	char* value = nullptr;
	EXPECT_EQ(PC_INVALID_NODE_TYPE, PC_string(parse("[a]"), &value));
	expect_error(PC_INVALID_NODE_TYPE, "Expected a scalar, found sequence");
	EXPECT_EQ(PC_INVALID_NODE_TYPE, PC_string(parse("{a: b}"), &value));
	expect_error(PC_INVALID_NODE_TYPE, "Expected a scalar, found mapping");
	EXPECT_EQ(nullptr, value);
}

TEST_F(Scalars, string_of_a_tree_in_error)
{
	PC_tree_t missing = PC_get(parse("{}"), ".missing");
	expect_error(PC_NODE_NOT_FOUND);
	char* value = nullptr;
	EXPECT_EQ(PC_NODE_NOT_FOUND, PC_string(missing, &value));
	EXPECT_EQ(nullptr, value);
}

// PC_bool, YAML 1.1 booleans: https://yaml.org/type/bool.html

class TrueBool
	: public ParaconfTest
	, public ::testing::WithParamInterface<const char*>
{};

TEST_P(TrueBool, is_true)
{
	int value = -1;
	EXPECT_EQ(PC_OK, PC_bool(parse(GetParam()), &value));
	EXPECT_EQ(1, value);
}

INSTANTIATE_TEST_SUITE_P(Value, TrueBool, ::testing::Values("y", "Y", "yes", "Yes", "YES", "true", "True", "TRUE", "on", "On", "ON"));

class FalseBool
	: public ParaconfTest
	, public ::testing::WithParamInterface<const char*>
{};

TEST_P(FalseBool, is_false)
{
	int value = -1;
	EXPECT_EQ(PC_OK, PC_bool(parse(GetParam()), &value));
	EXPECT_EQ(0, value);
}

INSTANTIATE_TEST_SUITE_P(Value, FalseBool, ::testing::Values("n", "N", "no", "No", "NO", "false", "False", "FALSE", "off", "Off", "OFF"));

class InvalidBool
	: public ParaconfTest
	, public ::testing::WithParamInterface<const char*>
{};

TEST_P(InvalidBool, is_an_error)
{
	int value = -1;
	EXPECT_EQ(PC_INVALID_NODE_TYPE, PC_bool(parse(GetParam()), &value));
	expect_error(PC_INVALID_NODE_TYPE);
	EXPECT_EQ(-1, value);
}

// close misses of every valid spelling, and values other languages read as booleans
INSTANTIATE_TEST_SUITE_P(
	Value,
	InvalidBool,
	::testing::Values(
		"\"\"",
		"ye",
		"yess",
		"yeah",
		"yES",
		"YeS",
		"YEs",
		"nO",
		"noo",
		"nope",
		"t",
		"tru",
		"truE",
		"tRUE",
		"TRue",
		"trueish",
		"f",
		"fals",
		"fALSE",
		"FAlse",
		"falsey",
		"o",
		"oN",
		"onn",
		"ONN",
		"Only you",
		"of",
		"oFF",
		"OfF",
		"offf",
		"0",
		"1",
		"maybe"
	)
);

TEST_F(Scalars, bool_of_a_non_scalar)
{
	int value = -1;
	EXPECT_EQ(PC_INVALID_NODE_TYPE, PC_bool(parse("[true]"), &value));
	expect_error(PC_INVALID_NODE_TYPE, "Expected a scalar, found sequence");
	EXPECT_EQ(-1, value);
}

TEST_F(Scalars, bool_of_a_tree_in_error)
{
	PC_tree_t missing = PC_get(parse("{}"), ".missing");
	expect_error(PC_NODE_NOT_FOUND);
	int value = -1;
	EXPECT_EQ(PC_NODE_NOT_FOUND, PC_bool(missing, &value));
	EXPECT_EQ(-1, value);
}
