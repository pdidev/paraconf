/* Copyright (C) The Paraconf development team, see COPYRIGHT.md file at the
 *               root of the project or at https://github.com/pdidev/paraconf
 *
 * SPDX-License-Identifier: MIT
 */

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>

#include <unistd.h>

#include <gtest/gtest.h>

#include <paraconf.h>
#include <yaml.h>

#include "paraconf_test.h"

namespace {

/// the number of allocations paraconf makes in this thread before the one that fails, none fails if negative
thread_local int allocations_before_failure = -1;

/// whether the allocation set to fail was reached in this thread
thread_local bool allocation_reached = false;

/** Makes one allocation paraconf makes in this thread fail
 *
 * \param n the number of allocations that succeed before the one that fails
 */
void fail_allocation(int n)
{
	allocations_before_failure = n;
	allocation_reached = false;
}

/** Stops making allocations fail in this thread
 *
 * \return whether the allocation set to fail was reached
 */
bool stop_failing()
{
	allocations_before_failure = -1;
	return allocation_reached;
}

} // namespace

/** The allocation function paraconf is compiled to call in this executable instead of malloc
 */
extern "C" void* pc_test_malloc(size_t size)
{
	if (allocations_before_failure == 0) {
		allocations_before_failure = -1;
		allocation_reached = true;
		return nullptr;
	}
	if (allocations_before_failure > 0) --allocations_before_failure;
	return malloc(size);
}

/** Tests that paraconf reports a failure to allocate memory as an error, instead of crashing
 */
class AllocFailures: public ParaconfTest
{
protected:
	/** Calls a function once for each allocation paraconf makes in it, with that allocation failing, then once with none failing
	 *
	 * Each call must report the failure with PC_SYSTEM_ERROR, exactly once, and free whatever it allocated.
	 *
	 * \param call the function to call, it returns the status of the paraconf call it makes and frees the result
	 */
	template <class Call>
	void expect_each_failure_reported(Call call)
	{
		for (int n = 0;; ++n) {
			fail_allocation(n);
			PC_status_t status = call();
			if (!stop_failing()) {
				EXPECT_EQ(PC_OK, status) << "with no allocation failing";
				EXPECT_LT(0, n) << "the call allocates nothing";
				return;
			}
			EXPECT_EQ(PC_SYSTEM_ERROR, status) << "allocation #" << n << " failing";
			expect_error(PC_SYSTEM_ERROR, "unable to allocate memory");
		}
	}
};

TEST_F(AllocFailures, parse_string)
{
	expect_each_failure_reported([] {
		PC_tree_t tree = PC_parse_string("a: 1");
		PC_status_t status = PC_status(tree);
		PC_tree_destroy(&tree);
		return status;
	});
}

TEST_F(AllocFailures, parse_file)
{
	expect_each_failure_reported([] {
		FILE* file = tmpfile();
		fputs("a: 1", file);
		rewind(file);
		PC_tree_t tree = PC_parse_file(file);
		fclose(file);
		PC_status_t status = PC_status(tree);
		PC_tree_destroy(&tree);
		return status;
	});
}

TEST_F(AllocFailures, parse_path)
{
	std::string path = ::testing::TempDir() + "paraconf_alloc_failures_" + std::to_string(getpid()) + ".yml";
	std::ofstream(path) << "a: 1";
	expect_each_failure_reported([&] {
		PC_tree_t tree = PC_parse_path(path.c_str());
		PC_status_t status = PC_status(tree);
		PC_tree_destroy(&tree);
		return status;
	});
	std::remove(path.c_str());
}

TEST_F(AllocFailures, root_leaves_the_document_to_the_caller)
{
	expect_each_failure_reported([] {
		const char* document = "a: 1\n";
		yaml_parser_t parser;
		yaml_parser_initialize(&parser);
		yaml_parser_set_input_string(&parser, reinterpret_cast<const unsigned char*>(document), strlen(document));
		yaml_document_t yaml_document;
		yaml_parser_load(&parser, &yaml_document);
		yaml_parser_delete(&parser);

		PC_tree_t tree = PC_root(&yaml_document);
		PC_status_t status = PC_status(tree);
		if (status) {
			yaml_document_delete(&yaml_document);
		} else {
			PC_tree_destroy(&tree);
		}
		return status;
	});
}

TEST_F(AllocFailures, get)
{
	PC_tree_t tree = parse("a: 1");
	expect_each_failure_reported([&] { return PC_status(PC_get(tree, ".%s", "a")); });
}

TEST_F(AllocFailures, string)
{
	PC_tree_t tree = parse("a: text");
	expect_each_failure_reported([&] {
		char* value = nullptr;
		PC_status_t status = PC_string(PC_get(tree, ".a"), &value);
		free(value);
		return status;
	});
}

TEST_F(AllocFailures, error_message)
{
	PC_tree_t not_an_int = PC_get(parse("a: text"), ".a");
	long value;
	fail_allocation(0); // PC_int allocates nothing but the message of its error
	EXPECT_EQ(PC_SYSTEM_ERROR, PC_int(not_an_int, &value));
	EXPECT_TRUE(stop_failing());
	expect_error(PC_SYSTEM_ERROR, "unable to allocate memory");
	EXPECT_STREQ("unable to allocate memory", PC_errmsg());

	// the next error gets its message again
	EXPECT_EQ(PC_INVALID_NODE_TYPE, PC_int(not_an_int, &value));
	expect_error(PC_INVALID_NODE_TYPE, "text");
}

TEST(NoErrorContext, errmsg)
{
	// a fresh thread has no error context yet
	std::thread([] {
		fail_allocation(0);
		EXPECT_STREQ("unable to allocate memory", PC_errmsg());
		EXPECT_TRUE(stop_failing());
	}).join();
}

TEST(NoErrorContextDeathTest, errhandler_aborts)
{
	GTEST_FLAG_SET(death_test_style, "threadsafe");
	// a fresh thread has no error context to record its handler in, so the default handler reports it
	EXPECT_DEATH(
		std::thread([] {
			fail_allocation(0);
			PC_errhandler(PC_NULL_HANDLER);
		}).join(),
		"Error in paraconf: unable to allocate memory"
	);
}

TEST(NoErrorContextDeathTest, error_aborts)
{
	GTEST_FLAG_SET(death_test_style, "threadsafe");
	PC_tree_t tree = PC_parse_string("a: text");
	PC_tree_t not_an_int = PC_get(tree, ".a");
	// a fresh thread has no error context to record the error in, so the default handler reports the failure
	EXPECT_DEATH(
		std::thread([&] {
			long value;
			fail_allocation(0);
			PC_int(not_an_int, &value);
		}).join(),
		"Error in paraconf: unable to allocate memory"
	);
	PC_tree_destroy(&tree);
}
