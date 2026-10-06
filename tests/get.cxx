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

/** Tests of the ypath query language, PC_get and PC_vget
 */
class Get: public ParaconfTest
{
protected:
	PC_tree_t tree;

	void SetUp() override
	{
		ParaconfTest::SetUp();
		tree = parse(R"==(
scalar: value
seq: [10, 11, 12]
map: {first: 20, second: 21}
nested:
  seq_of_maps:
    - {name: a, value: 30}
    - {name: b, value: 31}
  map_of_seqs:
    x: [40, 41]
    y: [42, 43]
  seq_of_seqs: [[50, 51], [52, 53]]
prefix: 60
prefix_longer: 61
empty_seq: []
empty_map: {}
)==");
	}

	long get_int(PC_tree_t from, const char* index)
	{
		long result = -1;
		EXPECT_EQ(PC_OK, PC_int(PC_get(from, index), &result)) << index;
		return result;
	}

	std::string get_string(PC_tree_t from, const char* index)
	{
		char* result = nullptr;
		EXPECT_EQ(PC_OK, PC_string(PC_get(from, index), &result)) << index;
		std::string result_str = result ? result : "";
		free(result);
		return result_str;
	}
};

TEST_F(Get, empty_index_returns_the_tree)
{
	PC_tree_t result = PC_get(tree, "");
	EXPECT_EQ(PC_OK, PC_status(result));
	EXPECT_EQ(tree.node, result.node);
	EXPECT_EQ(tree.pcdoc, result.pcdoc);
}

TEST_F(Get, map_key)
{
	EXPECT_EQ("value", get_string(tree, ".scalar"));
}

TEST_F(Get, seq_index)
{
	EXPECT_EQ(10, get_int(tree, ".seq[0]"));
	EXPECT_EQ(11, get_int(tree, ".seq[1]"));
	EXPECT_EQ(12, get_int(tree, ".seq[2]"));
}

TEST_F(Get, map_key_by_index)
{
	EXPECT_EQ("first", get_string(tree, ".map{0}"));
	EXPECT_EQ("second", get_string(tree, ".map{1}"));
}

TEST_F(Get, map_value_by_index)
{
	EXPECT_EQ(20, get_int(tree, ".map<0>"));
	EXPECT_EQ(21, get_int(tree, ".map<1>"));
}

TEST_F(Get, map_key_and_value_by_index_belong_together)
{
	for (int idx = 0; idx < 2; ++idx) {
		std::string key = get_string(PC_get(tree, ".map{%d}", idx), "");
		EXPECT_EQ(get_int(PC_get(tree, ".map<%d>", idx), ""), get_int(PC_get(tree, ".map"), ("." + key).c_str())) << key;
	}
}

TEST_F(Get, nested)
{
	EXPECT_EQ("b", get_string(tree, ".nested.seq_of_maps[1].name"));
	EXPECT_EQ(31, get_int(tree, ".nested.seq_of_maps[1].value"));
	EXPECT_EQ(43, get_int(tree, ".nested.map_of_seqs.y[1]"));
	EXPECT_EQ(52, get_int(tree, ".nested.seq_of_seqs[1][0]"));
	EXPECT_EQ("y", get_string(tree, ".nested.map_of_seqs{1}"));
	EXPECT_EQ(41, get_int(tree, ".nested.map_of_seqs<0>[1]"));
	EXPECT_EQ("value", get_string(tree, ".nested.seq_of_maps[0]{1}"));
}

TEST_F(Get, chained)
{
	PC_tree_t nested = PC_get(tree, ".nested");
	PC_tree_t seq_of_maps = PC_get(nested, ".seq_of_maps");
	EXPECT_EQ(31, get_int(seq_of_maps, "[1].value"));
	EXPECT_EQ(PC_get(tree, ".nested.seq_of_maps[1]").node, PC_get(seq_of_maps, "[1]").node);
}

TEST_F(Get, from_a_root_sequence)
{
	PC_tree_t seq = parse("[[1, 2], {a: 3}]");
	EXPECT_EQ(2, get_int(seq, "[0][1]"));
	EXPECT_EQ(3, get_int(seq, "[1].a"));
}

TEST_F(Get, key_is_not_matched_by_its_prefix)
{
	EXPECT_EQ(60, get_int(tree, ".prefix"));
	EXPECT_EQ(61, get_int(tree, ".prefix_longer"));
	EXPECT_EQ(PC_NODE_NOT_FOUND, PC_status(PC_get(tree, ".prefix_long")));
	expect_error(PC_NODE_NOT_FOUND, "prefix_long");
}

TEST_F(Get, printf_style_format)
{
	EXPECT_EQ(11, get_int(PC_get(tree, ".seq[%d]", 1), ""));
	EXPECT_EQ("value", get_string(PC_get(tree, ".%s", "scalar"), ""));
	EXPECT_EQ(31, get_int(PC_get(tree, ".%s.%s[%ld].%s", "nested", "seq_of_maps", 1L, "value"), ""));
	EXPECT_EQ(21, get_int(PC_get(tree, ".map<%d>", 1), ""));
}

TEST_F(Get, format_that_fails_to_format)
{
	// in the C locale, which the tests run in, a wide character outside ASCII has no multibyte form, so the formatting fails
	EXPECT_EQ(PC_INVALID_PARAMETER, PC_status(PC_get(tree, ".%ls", L"\u00e9")));
	expect_error(PC_INVALID_PARAMETER, "Invalid formatting in PC_get");
}

/// a key exactly as long as the buffer PC_vget first formats the index in, and one longer than that
class GetLongIndex
	: public ParaconfTest
	, public ::testing::WithParamInterface<size_t>
{};

TEST_P(GetLongIndex, through_a_format_argument)
{
	std::string key(GetParam(), 'k');
	PC_tree_t tree = parse((key + ": 1").c_str());
	long value = -1;
	EXPECT_EQ(PC_OK, PC_int(PC_get(tree, ".%s", key.c_str()), &value));
	EXPECT_EQ(1, value);
}

TEST_P(GetLongIndex, without_format_argument)
{
	std::string key(GetParam(), 'k');
	PC_tree_t tree = parse((key + ": 1").c_str());
	long value = -1;
	EXPECT_EQ(PC_OK, PC_int(PC_get(tree, ("." + key).c_str()), &value));
	EXPECT_EQ(1, value);
}

INSTANTIATE_TEST_SUITE_P(KeyLength, GetLongIndex, ::testing::Values(254, 255, 256, 1000));

TEST_F(Get, missing_key)
{
	PC_tree_t result = PC_get(tree, ".map.third");
	EXPECT_EQ(PC_NODE_NOT_FOUND, PC_status(result));
	expect_error(PC_NODE_NOT_FOUND, "Key `third' not found in mapping (request was: `$tree.map.third')");
}

TEST_F(Get, seq_index_out_of_range)
{
	EXPECT_EQ(PC_NODE_NOT_FOUND, PC_status(PC_get(tree, ".seq[3]")));
	expect_error(PC_NODE_NOT_FOUND, "Index 3 out of range");
	EXPECT_EQ(PC_NODE_NOT_FOUND, PC_status(PC_get(tree, ".seq[-1]")));
	expect_error(PC_NODE_NOT_FOUND, "Index -1 out of range");
	EXPECT_EQ(PC_NODE_NOT_FOUND, PC_status(PC_get(tree, ".empty_seq[0]")));
	expect_error(PC_NODE_NOT_FOUND, "Index 0 out of range");
}

TEST_F(Get, map_index_out_of_range)
{
	EXPECT_EQ(PC_NODE_NOT_FOUND, PC_status(PC_get(tree, ".map{2}")));
	expect_error(PC_NODE_NOT_FOUND, "Index 2 out of range");
	EXPECT_EQ(PC_NODE_NOT_FOUND, PC_status(PC_get(tree, ".map<2>")));
	expect_error(PC_NODE_NOT_FOUND, "Index 2 out of range");
	EXPECT_EQ(PC_NODE_NOT_FOUND, PC_status(PC_get(tree, ".map<-1>")));
	expect_error(PC_NODE_NOT_FOUND, "Index -1 out of range");
	EXPECT_EQ(PC_NODE_NOT_FOUND, PC_status(PC_get(tree, ".empty_map{0}")));
	expect_error(PC_NODE_NOT_FOUND, "Index 0 out of range");
}

TEST_F(Get, key_in_a_non_mapping)
{
	EXPECT_EQ(PC_INVALID_NODE_TYPE, PC_status(PC_get(tree, ".seq.key")));
	expect_error(PC_INVALID_NODE_TYPE, "Expected a mapping, found a sequence");
	EXPECT_EQ(PC_INVALID_NODE_TYPE, PC_status(PC_get(tree, ".scalar.key")));
	expect_error(PC_INVALID_NODE_TYPE, "Expected a mapping, found a scalar");
}

TEST_F(Get, index_in_a_non_sequence)
{
	EXPECT_EQ(PC_INVALID_NODE_TYPE, PC_status(PC_get(tree, ".map[0]")));
	expect_error(PC_INVALID_NODE_TYPE, "Expected a sequence, found a mapping");
	EXPECT_EQ(PC_INVALID_NODE_TYPE, PC_status(PC_get(tree, ".scalar[0]")));
	expect_error(PC_INVALID_NODE_TYPE, "Expected a sequence, found a scalar");
}

TEST_F(Get, map_index_in_a_non_mapping)
{
	EXPECT_EQ(PC_INVALID_NODE_TYPE, PC_status(PC_get(tree, ".seq{0}")));
	expect_error(PC_INVALID_NODE_TYPE, "Expected a mapping, found a sequence");
	EXPECT_EQ(PC_INVALID_NODE_TYPE, PC_status(PC_get(tree, ".seq<0>")));
	expect_error(PC_INVALID_NODE_TYPE, "Expected a mapping, found a sequence");
}

/// an ill-formed ypath expression
class GetSyntaxError
	: public ParaconfTest
	, public ::testing::WithParamInterface<const char*>
{};

TEST_P(GetSyntaxError, is_an_invalid_parameter)
{
	PC_tree_t tree = parse("{map: {key: 1}, seq: [1, 2]}");
	EXPECT_EQ(PC_INVALID_PARAMETER, PC_status(PC_get(tree, GetParam())));
	expect_error(PC_INVALID_PARAMETER);
}

INSTANTIATE_TEST_SUITE_P(
	Index,
	GetSyntaxError,
	::testing::Values("map", ".seq[0]x", ".seq[]", ".seq[x]", ".seq[0", ".seq[0}", ".map{}", ".map{x}", ".map{0", ".map<>", ".map<x>", ".map<0")
);

TEST_F(Get, tree_in_error_is_propagated)
{
	PC_tree_t missing = PC_get(tree, ".missing");
	expect_error(PC_NODE_NOT_FOUND);

	// a tree in error goes through unchanged, without reporting the error again
	PC_tree_t result = PC_get(missing, ".scalar");
	EXPECT_EQ(PC_NODE_NOT_FOUND, PC_status(result));
	result = PC_get(missing, "[0]");
	EXPECT_EQ(PC_NODE_NOT_FOUND, PC_status(result));
	result = PC_get(missing, "invalid syntax");
	EXPECT_EQ(PC_NODE_NOT_FOUND, PC_status(result));
}
