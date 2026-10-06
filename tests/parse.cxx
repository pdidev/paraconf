/* Copyright (C) The Paraconf development team, see COPYRIGHT.md file at the
 *               root of the project or at https://github.com/pdidev/paraconf
 *
 * SPDX-License-Identifier: MIT
 */

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include <unistd.h>

#include <gtest/gtest.h>

#include <paraconf.h>
#include <yaml.h>

#include "paraconf_test.h"

/** Tests of the functions that build a tree from a document and destroy it
 */
class Parse: public ParaconfTest
{
	/// the files created by the test, to remove at its end
	std::vector<std::string> m_files;

protected:
	void TearDown() override
	{
		ParaconfTest::TearDown();
		for (auto&& file: m_files) {
			std::remove(file.c_str());
		}
	}

	/** Creates a file with a given content, removed at the end of the test
	 *
	 * \param content the content of the file
	 * \return the path of the file, unique to this test and process
	 */
	std::string make_file(const std::string& content)
	{
		std::string path = ::testing::TempDir() + "paraconf_" + ::testing::UnitTest::GetInstance()->current_test_info()->name() + "_"
		                 + std::to_string(getpid()) + "_" + std::to_string(m_files.size()) + ".yml";
		std::ofstream(path) << content;
		m_files.push_back(path);
		return path;
	}

	/** Parses a document that is expected to be invalid with PC_parse_string, and consumes the error
	 *
	 * \param document the invalid document
	 * \return the message of the error, the reference for the other parsing functions
	 */
	std::string string_error(const char* document)
	{
		PC_parse_string(document);
		std::string message = PC_errmsg();
		expect_error(PC_INVALID_FORMAT);
		return message;
	}
};

/// invalid YAML whose context and problem have distinct lines and columns: the flow sequence opened on line 2, column 4 is still open on line 3,
/// column 2
static const char* const UNCLOSED_FLOW_SEQUENCE = "a: 1\nb: [1, 2\nc: 3\n";

/** Finds the line of a message that starts with a given prefix
 *
 * \param message the message to look into
 * \param prefix the start of the line to look for
 * \return the position of the line in the message, std::string::npos if there is none
 */
static std::size_t find_line(const std::string& message, const std::string& prefix)
{
	if (message.compare(0, prefix.size(), prefix) == 0) return 0;
	std::size_t position = message.find('\n' + prefix);
	return position == std::string::npos ? position : position + 1;
}

/** Checks that a message reports the error of UNCLOSED_FLOW_SEQUENCE in the format of compilers, that text editors parse
 *
 * The problem comes first, as an error, then its context, as a note, each on a line of its own that starts with `file:line:column: `.
 *
 * \param message the message to check
 * \param file the name the message gives to the document
 */
static void expect_unclosed_flow_sequence(const std::string& message, const std::string& file)
{
	std::size_t error = find_line(message, file + ":3:2: error: ");
	std::size_t note = find_line(message, file + ":2:4: note: ");
	EXPECT_NE(std::string::npos, error) << "no `" << file << ":3:2: error: ' line in \"" << message << '"';
	EXPECT_NE(std::string::npos, note) << "no `" << file << ":2:4: note: ' line in \"" << message << '"';
	EXPECT_LT(error, note) << "the error should come before its note in \"" << message << '"';
}

TEST_F(Parse, string_scalar)
{
	PC_tree_t tree = parse("42");
	long value = -1;
	EXPECT_EQ(PC_OK, PC_int(tree, &value));
	EXPECT_EQ(42, value);
}

TEST_F(Parse, string_has_no_path)
{
	EXPECT_STREQ("<string>", PC_path(parse("a: 1")));
}

TEST_F(Parse, string_invalid_yaml_has_no_path)
{
	PC_tree_t tree = PC_parse_string("a: [1\n");
	expect_error(PC_INVALID_FORMAT);
	// a tree in error has no document to give the path of
	EXPECT_EQ(nullptr, PC_path(tree));
}

TEST_F(Parse, string_invalid_yaml)
{
	PC_tree_t tree = PC_parse_string("a: [1, 2\nb: 3\n");
	EXPECT_EQ(PC_INVALID_FORMAT, PC_status(tree));
	expect_error(PC_INVALID_FORMAT, "1:4");
}

TEST_F(Parse, string_invalid_yaml_position)
{
	// positions count from 1, as in every editor
	expect_unclosed_flow_sequence(string_error(UNCLOSED_FLOW_SEQUENCE), "<string>");
}

TEST_F(Parse, string_invalid_yaml_without_context_position)
{
	// libyaml reports an undefined alias with the position of the problem only, there is no context to give a position for
	std::string message = string_error("a: 1\nb: 2\nc: *undefined\n");
	EXPECT_EQ(0u, find_line(message, "<string>:3:4: error: ")) << "\"" << message << '"';
	EXPECT_EQ(std::string::npos, message.find('\n')) << "no note should be given for the missing context in \"" << message << '"';
}

TEST_F(Parse, string_empty_document)
{
	PC_tree_t tree = parse("");
	int len = -1;
	EXPECT_EQ(PC_INVALID_NODE_TYPE, PC_len(tree, &len));
	expect_error(PC_INVALID_NODE_TYPE, "empty tree");
	EXPECT_EQ(-1, len);
	EXPECT_EQ(PC_INVALID_NODE_TYPE, PC_status(PC_get(tree, ".a")));
	expect_error(PC_INVALID_NODE_TYPE, "empty tree");
}

TEST_F(Parse, string_comment_only)
{
	PC_tree_t tree = parse("# nothing but a comment\n");
	EXPECT_EQ(nullptr, tree.node);
}

TEST_F(Parse, string_with_anchor_and_alias)
{
	PC_tree_t tree = parse("a: &anchor {x: 1}\nb: *anchor\n");
	long value = -1;
	EXPECT_EQ(PC_OK, PC_int(PC_get(tree, ".b.x"), &value));
	EXPECT_EQ(1, value);
}

TEST_F(Parse, string_independent_trees)
{
	PC_tree_t tree1 = parse("value: 1");
	PC_tree_t tree2 = parse("value: 2");
	long value1 = -1, value2 = -1;
	EXPECT_EQ(PC_OK, PC_int(PC_get(tree1, ".value"), &value1));
	EXPECT_EQ(PC_OK, PC_int(PC_get(tree2, ".value"), &value2));
	EXPECT_EQ(1, value1);
	EXPECT_EQ(2, value2);
}

TEST_F(Parse, file)
{
	FILE* file = std::tmpfile();
	ASSERT_NE(nullptr, file);
	std::fputs("a: 1\nb: [2, 3]\n", file);
	std::rewind(file);
	PC_tree_t tree = destroy_at_end(PC_parse_file(file));
	std::fclose(file);
	ASSERT_EQ(PC_OK, PC_status(tree));
	long value = -1;
	EXPECT_EQ(PC_OK, PC_int(PC_get(tree, ".b[1]"), &value));
	EXPECT_EQ(3, value);
}

TEST_F(Parse, file_has_no_path)
{
	FILE* file = std::tmpfile();
	ASSERT_NE(nullptr, file);
	std::fputs("a: 1\n", file);
	std::rewind(file);
	PC_tree_t tree = destroy_at_end(PC_parse_file(file));
	std::fclose(file);
	ASSERT_EQ(PC_OK, PC_status(tree));
	// a FILE* has no name
	EXPECT_STREQ("<file>", PC_path(tree));
	EXPECT_STREQ("<file>", PC_path(PC_get(tree, ".a")));
}

TEST_F(Parse, file_invalid_yaml_has_no_path)
{
	FILE* file = std::tmpfile();
	ASSERT_NE(nullptr, file);
	std::fputs("a: [1\n", file);
	std::rewind(file);
	PC_tree_t tree = PC_parse_file(file);
	std::fclose(file);
	expect_error(PC_INVALID_FORMAT);
	EXPECT_EQ(nullptr, PC_path(tree));
}

TEST_F(Parse, file_invalid_yaml)
{
	FILE* file = std::tmpfile();
	ASSERT_NE(nullptr, file);
	std::fputs("a: [1, 2\nb: 3\n", file);
	std::rewind(file);
	PC_tree_t tree = PC_parse_file(file);
	std::fclose(file);
	EXPECT_EQ(PC_INVALID_FORMAT, PC_status(tree));
	expect_error(PC_INVALID_FORMAT);
}

TEST_F(Parse, file_invalid_yaml_position)
{
	FILE* file = std::tmpfile();
	ASSERT_NE(nullptr, file);
	std::fputs(UNCLOSED_FLOW_SEQUENCE, file);
	std::rewind(file);
	PC_parse_file(file);
	std::fclose(file);
	// a FILE* has no name
	expect_unclosed_flow_sequence(PC_errmsg(), "<file>");
	expect_error(PC_INVALID_FORMAT);
}

TEST_F(Parse, path)
{
	std::string path = make_file("a: 1\n");
	PC_tree_t tree = destroy_at_end(PC_parse_path(path.c_str()));
	ASSERT_EQ(PC_OK, PC_status(tree));
	long value = -1;
	EXPECT_EQ(PC_OK, PC_int(PC_get(tree, ".a"), &value));
	EXPECT_EQ(1, value);
}

TEST_F(Parse, path_is_recorded)
{
	std::string path = make_file("a: 1\n");
	PC_tree_t tree = destroy_at_end(PC_parse_path(path.c_str()));
	ASSERT_EQ(PC_OK, PC_status(tree));
	EXPECT_EQ(path, PC_path(tree));
	EXPECT_EQ(path, PC_path(PC_get(tree, ".a")));
}

TEST_F(Parse, path_of_a_subtree_in_error)
{
	std::string path = make_file("a: 1\n");
	PC_tree_t tree = destroy_at_end(PC_parse_path(path.c_str()));
	ASSERT_EQ(PC_OK, PC_status(tree));
	PC_tree_t missing = PC_get(tree, ".missing");
	expect_error(PC_NODE_NOT_FOUND);
	// it still refers to the document, but is in error all the same
	EXPECT_EQ(nullptr, PC_path(missing));
}

TEST_F(Parse, path_missing_file)
{
	PC_tree_t tree = PC_parse_path("/nonexistent/paraconf/file.yml");
	EXPECT_EQ(PC_SYSTEM_ERROR, PC_status(tree));
	expect_error(PC_SYSTEM_ERROR);
}

TEST_F(Parse, path_missing_file_has_no_path)
{
	PC_tree_t tree = PC_parse_path("/nonexistent/paraconf/file.yml");
	expect_error(PC_SYSTEM_ERROR);
	EXPECT_EQ(nullptr, PC_path(tree));
}

TEST_F(Parse, path_missing_file_message)
{
	const char* path = "/nonexistent/paraconf/file.yml";
	PC_parse_path(path);
	// the message names the file, and keeps the reason the system gives
	EXPECT_THAT(PC_errmsg(), ::testing::HasSubstr(std::strerror(ENOENT)));
	expect_error(PC_SYSTEM_ERROR, path);
}

TEST_F(Parse, path_invalid_yaml)
{
	std::string path = make_file("a: [1, 2\nb: 3\n");
	PC_tree_t tree = PC_parse_path(path.c_str());
	EXPECT_EQ(PC_INVALID_FORMAT, PC_status(tree));
	expect_error(PC_INVALID_FORMAT, path);
}

TEST_F(Parse, path_invalid_yaml_position)
{
	std::string path = make_file(UNCLOSED_FLOW_SEQUENCE);
	PC_parse_path(path.c_str());
	expect_unclosed_flow_sequence(PC_errmsg(), path);
	expect_error(PC_INVALID_FORMAT);
}

TEST_F(Parse, root_of_a_libyaml_document)
{
	const char* document = "a: 1\n";
	yaml_parser_t parser;
	ASSERT_TRUE(yaml_parser_initialize(&parser));
	yaml_parser_set_input_string(&parser, reinterpret_cast<const unsigned char*>(document), strlen(document));
	yaml_document_t yaml_document;
	ASSERT_TRUE(yaml_parser_load(&parser, &yaml_document));
	yaml_parser_delete(&parser);

	// the tree takes ownership of the document content
	PC_tree_t tree = destroy_at_end(PC_root(&yaml_document));
	ASSERT_EQ(PC_OK, PC_status(tree));
	long value = -1;
	EXPECT_EQ(PC_OK, PC_int(PC_get(tree, ".a"), &value));
	EXPECT_EQ(1, value);
	EXPECT_STREQ("<string>", PC_path(tree));
}

TEST_F(Parse, destroy_resets_the_tree)
{
	PC_tree_t tree = PC_parse_string("a: 1");
	ASSERT_EQ(PC_OK, PC_status(tree));
	EXPECT_EQ(PC_OK, PC_tree_destroy(&tree));
	EXPECT_EQ(nullptr, tree.node);
	EXPECT_EQ(nullptr, tree.pcdoc);
}

TEST_F(Parse, destroy_a_tree_in_error)
{
	PC_tree_t tree = PC_parse_path("/nonexistent/paraconf/file.yml");
	expect_error(PC_SYSTEM_ERROR);
	EXPECT_EQ(PC_OK, PC_tree_destroy(&tree));
}
