/* Copyright (C) The Paraconf development team, see COPYRIGHT.md file at the
 *               root of the project or at https://github.com/pdidev/paraconf
 *
 * SPDX-License-Identifier: MIT
 */

#include <cstdlib>
#include <string>

#include <gtest/gtest.h>

#include <paraconf.h>

#include "paraconf_test.h"

constexpr char const * YAML_DATA = R"==(
a_int: 100
# float scalar
a_float: 100.1
# string scalar
a_string: this is a string
# list
a_list: [10, 11]
# map
a_map: { first: 20, second: 21 }
# second syntax for lists
another_list:
    - 30
    - 31
# second syntax for maps
another_map:
    first: 40
    second: 41

# logical
a_y: y
a_Y: Y
a_true: true
a_True: True
a_TRUE: TRUE
a_yes: yes
a_Yes: Yes
a_YES: YES
a_on: on
a_On: On
a_ON: ON
a_n: n
a_N: N
a_false: false
a_False: False
a_FALSE: FALSE
a_no: no
a_No: No
a_NO: NO
a_off: off
a_Off: Off
a_OFF: OFF
a_badlog: toto
)==";

/** Reads every value of test_data.yml, a file that covers each kind of node, the same one the Fortran test reads
 */
class TestData: public ParaconfTest
{
protected:
	PC_tree_t conf;

	void SetUp() override
	{
		ParaconfTest::SetUp();
		conf = destroy_at_end(PC_parse_string(YAML_DATA));
		ASSERT_EQ(PC_OK, PC_status(conf));
	}

	long get_int(const char* index)
	{
		long result = -1;
		EXPECT_EQ(PC_OK, PC_int(PC_get(conf, index), &result)) << index;
		return result;
	}

	std::string get_string(const char* index)
	{
		char* result = nullptr;
		EXPECT_EQ(PC_OK, PC_string(PC_get(conf, index), &result)) << index;
		std::string result_str = result ? result : "";
		free(result);
		return result_str;
	}

	int get_bool(const char* index)
	{
		int result = -1;
		EXPECT_EQ(PC_OK, PC_bool(PC_get(conf, index), &result)) << index;
		return result;
	}

	int get_len(const char* index)
	{
		int result = -1;
		EXPECT_EQ(PC_OK, PC_len(PC_get(conf, index), &result)) << index;
		return result;
	}
};

TEST_F(TestData, int_scalar)
{
	EXPECT_EQ(100, get_int(".a_int"));
}

TEST_F(TestData, float_scalar)
{
	double a_float = -1;
	EXPECT_EQ(PC_OK, PC_double(PC_get(conf, ".a_float"), &a_float));
	EXPECT_DOUBLE_EQ(100.1, a_float);
}

TEST_F(TestData, string_scalar)
{
	EXPECT_EQ("this is a string", get_string(".a_string"));
}

TEST_F(TestData, flow_sequence)
{
	EXPECT_EQ(2, get_len(".a_list"));
	EXPECT_EQ(10, get_int(".a_list[0]"));
	EXPECT_EQ(11, get_int(".a_list[1]"));
}

TEST_F(TestData, flow_mapping)
{
	EXPECT_EQ(2, get_len(".a_map"));
	EXPECT_EQ("first", get_string(".a_map{0}"));
	EXPECT_EQ(20, get_int(".a_map<0>"));
	EXPECT_EQ("second", get_string(".a_map{1}"));
	EXPECT_EQ(21, get_int(".a_map<1>"));
	EXPECT_EQ(20, get_int(".a_map.first"));
	EXPECT_EQ(21, get_int(".a_map.second"));
}

TEST_F(TestData, block_sequence)
{
	EXPECT_EQ(2, get_len(".another_list"));
	EXPECT_EQ(30, get_int(".another_list[0]"));
	EXPECT_EQ(31, get_int(".another_list[1]"));
}

TEST_F(TestData, block_mapping)
{
	EXPECT_EQ(2, get_len(".another_map"));
	EXPECT_EQ(40, get_int(".another_map.first"));
	EXPECT_EQ(41, get_int(".another_map.second"));
}

TEST_F(TestData, true_booleans)
{
	for (auto&& index: {".a_y", ".a_Y", ".a_true", ".a_True", ".a_TRUE", ".a_yes", ".a_Yes", ".a_YES", ".a_on", ".a_On", ".a_ON"}) {
		EXPECT_EQ(1, get_bool(index)) << index;
	}
}

TEST_F(TestData, false_booleans)
{
	for (auto&& index: {".a_n", ".a_N", ".a_false", ".a_False", ".a_FALSE", ".a_no", ".a_No", ".a_NO", ".a_off", ".a_Off", ".a_OFF"}) {
		EXPECT_EQ(0, get_bool(index)) << index;
	}
}

TEST_F(TestData, invalid_boolean)
{
	int a_badlog = -1;
	EXPECT_EQ(PC_INVALID_NODE_TYPE, PC_bool(PC_get(conf, ".a_badlog"), &a_badlog));
	expect_error(PC_INVALID_NODE_TYPE, "`toto'");
	EXPECT_EQ(-1, a_badlog);
}
