/* Copyright (C) The Paraconf development team, see COPYRIGHT.md file at the
 *               root of the project or at https://github.com/pdidev/paraconf
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PARACONF_TEST_H__
#define PARACONF_TEST_H__

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <paraconf.h>

/** One call to the paraconf error handler
 */
struct PcError {
	/// the status passed to the handler
	PC_status_t status;

	/// the message passed to the handler
	std::string message;
};

/** A fixture that records the errors paraconf reports instead of aborting, and destroys the trees it parsed
 *
 * A test fails if paraconf reports an error the test does not consume with expect_error().
 */
class ParaconfTest: public ::testing::Test
{
	/// the handler in place before the test
	PC_errhandler_t m_previous_handler;

	/// the trees parsed by the test, to destroy at its end
	std::vector<PC_tree_t> m_trees;

	/// the errors reported to the handler and not consumed yet, oldest first
	std::vector<PcError> m_errors;

	static void record(PC_status_t status, const char* message, void* context)
	{
		static_cast<ParaconfTest*>(context)->m_errors.push_back({status, message ? message : ""});
	}

protected:
	void SetUp() override { m_previous_handler = PC_errhandler({record, this}); }

	void TearDown() override
	{
		for (auto&& error: m_errors) {
			ADD_FAILURE() << "unexpected paraconf error #" << error.status << ": " << error.message;
		}
		for (auto&& tree: m_trees) {
			PC_tree_destroy(&tree);
		}
		PC_errhandler(m_previous_handler);
	}

	/** Checks that paraconf reported exactly one error since the last check, and consumes it
	 *
	 * \param status the expected status of the error
	 * \param excerpt a string the error message is expected to contain
	 */
	void expect_error(PC_status_t status, const std::string& excerpt = "")
	{
		ASSERT_EQ(1, m_errors.size()) << "expected exactly one error reported to the handler";
		EXPECT_EQ(status, m_errors[0].status) << m_errors[0].message;
		EXPECT_NE(std::string::npos, m_errors[0].message.find(excerpt)) << "`" << m_errors[0].message << "' lacks `" << excerpt << "'";
		m_errors.clear();
	}

	/** Parses a YAML document that is expected to be valid, the tree is destroyed at the end of the test
	 */
	PC_tree_t parse(const char* document)
	{
		PC_tree_t tree = PC_parse_string(document);
		EXPECT_EQ(PC_OK, PC_status(tree)) << "parsing:\n" << document;
		return destroy_at_end(tree);
	}

	/** Registers a tree for destruction at the end of the test
	 */
	PC_tree_t destroy_at_end(PC_tree_t tree)
	{
		if (!PC_status(tree)) m_trees.push_back(tree);
		return tree;
	}
};

#endif // PARACONF_TEST_H__
