/* Copyright (C) The Paraconf development team, see COPYRIGHT.md file at the
 *               root of the project or at https://github.com/pdidev/paraconf
 *
 * SPDX-License-Identifier: MIT
 */

#include <cstdio>
#include <fstream>
#include <string>

#include <unistd.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <paraconf.h>

#include "paraconf_test.h"

/** Tests that the errors about a node of a document say where the node is, as compilers do: `file:line:column: error: message'
 */
class Location: public ParaconfTest
{
protected:
	/// the path of the file read by the test
	std::string path;

	/// the document read from it: a value of the wrong type, as typed by mistake, in a nested mapping
	PC_tree_t tree;

	void SetUp() override
	{
		ParaconfTest::SetUp();
		path = ::testing::TempDir() + "paraconf_" + ::testing::UnitTest::GetInstance()->current_test_info()->name() + "_" + std::to_string(getpid())
		     + ".yml";
		std::ofstream(path) << "mesh:\n  nx: 100\n  ny: 1OO\n  steps: [1, 2]\n";
		tree = destroy_at_end(PC_parse_path(path.c_str()));
		ASSERT_EQ(PC_OK, PC_status(tree));
	}

	void TearDown() override
	{
		ParaconfTest::TearDown();
		std::remove(path.c_str());
	}

	/** Checks the message of the last error starts with a position, and consumes the error
	 *
	 * \param status the expected status of the error
	 * \param position the expected `file:line:column' the message starts with
	 * \param excerpt a string the rest of the message is expected to contain
	 */
	void expect_located_error(PC_status_t status, const std::string& position, const std::string& excerpt)
	{
		std::string message = PC_errmsg() ? PC_errmsg() : "";
		EXPECT_THAT(message, ::testing::StartsWith(position + ": error: "));
		expect_error(status, excerpt);
	}
};

TEST_F(Location, int_conversion)
{
	long value = -1;
	EXPECT_EQ(PC_INVALID_NODE_TYPE, PC_int(PC_get(tree, ".mesh.ny"), &value));
	expect_located_error(PC_INVALID_NODE_TYPE, path + ":3:7", "1OO");
}

TEST_F(Location, double_conversion)
{
	double value = -1;
	EXPECT_EQ(PC_INVALID_NODE_TYPE, PC_double(PC_get(tree, ".mesh.ny"), &value));
	expect_located_error(PC_INVALID_NODE_TYPE, path + ":3:7", "1OO");
}

TEST_F(Location, bool_conversion)
{
	int value = -1;
	EXPECT_EQ(PC_INVALID_NODE_TYPE, PC_bool(PC_get(tree, ".mesh.nx"), &value));
	expect_located_error(PC_INVALID_NODE_TYPE, path + ":2:7", "100");
}

TEST_F(Location, scalar_expected)
{
	// a value that is not a scalar is located where it starts
	char* value = nullptr;
	EXPECT_EQ(PC_INVALID_NODE_TYPE, PC_string(PC_get(tree, ".mesh"), &value));
	expect_located_error(PC_INVALID_NODE_TYPE, path + ":2:3", "Expected a scalar");
	EXPECT_EQ(nullptr, value);
}

TEST_F(Location, key_not_found)
{
	// a missing key is located at the mapping searched
	EXPECT_EQ(PC_NODE_NOT_FOUND, PC_status(PC_get(tree, ".mesh.nz")));
	expect_located_error(PC_NODE_NOT_FOUND, path + ":2:3", "Key `nz' not found");
}

TEST_F(Location, index_out_of_range)
{
	EXPECT_EQ(PC_NODE_NOT_FOUND, PC_status(PC_get(tree, ".mesh.steps[2]")));
	expect_located_error(PC_NODE_NOT_FOUND, path + ":4:10", "Index 2 out of range");
	EXPECT_EQ(PC_NODE_NOT_FOUND, PC_status(PC_get(tree, ".mesh<3>")));
	expect_located_error(PC_NODE_NOT_FOUND, path + ":2:3", "Index 3 out of range");
}

TEST_F(Location, sequence_or_mapping_expected)
{
	EXPECT_EQ(PC_INVALID_NODE_TYPE, PC_status(PC_get(tree, ".mesh.nx[0]")));
	expect_located_error(PC_INVALID_NODE_TYPE, path + ":2:7", "Expected a sequence");
	EXPECT_EQ(PC_INVALID_NODE_TYPE, PC_status(PC_get(tree, ".mesh.nx.key")));
	expect_located_error(PC_INVALID_NODE_TYPE, path + ":2:7", "Expected a mapping");
	EXPECT_EQ(PC_INVALID_NODE_TYPE, PC_status(PC_get(tree, ".mesh.nx{0}")));
	expect_located_error(PC_INVALID_NODE_TYPE, path + ":2:7", "Expected a mapping");
}

TEST_F(Location, string_document)
{
	// a document not read from a file is named as PC_path names it
	long value = -1;
	EXPECT_EQ(PC_INVALID_NODE_TYPE, PC_int(PC_get(parse("a: abc"), ".a"), &value));
	expect_located_error(PC_INVALID_NODE_TYPE, "<string>:1:4", "abc");
}

TEST_F(Location, request_syntax_error_is_not_located)
{
	// a syntax error is in the request, not in the document
	EXPECT_EQ(PC_INVALID_PARAMETER, PC_status(PC_get(tree, ".mesh.steps[x]")));
	EXPECT_THAT(PC_errmsg(), ::testing::StartsWith("Expected integer"));
	expect_error(PC_INVALID_PARAMETER);
}
