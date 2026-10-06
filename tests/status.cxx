/* Copyright (C) The Paraconf development team, see COPYRIGHT.md file at the
 *               root of the project or at https://github.com/pdidev/paraconf
 *
 * SPDX-License-Identifier: MIT
 */

#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <paraconf.h>

namespace {

/// A handler context that records every call
struct Recorder {
	std::vector<PC_status_t> statuses;
	std::vector<std::string> messages;

	static void handle(PC_status_t status, const char* message, void* context)
	{
		static_cast<Recorder*>(context)->statuses.push_back(status);
		static_cast<Recorder*>(context)->messages.push_back(message);
	}

	PC_errhandler_t handler() { return {handle, this}; }
};

/// Sets an error handler for the duration of a scope
class HandlerGuard
{
	PC_errhandler_t m_previous;

public:
	explicit HandlerGuard(PC_errhandler_t handler)
		: m_previous(PC_errhandler(handler))
	{}

	~HandlerGuard() { PC_errhandler(m_previous); }
};

/// A document with one key, `a', the tests look a missing key up in
PC_tree_t parse_doc()
{
	return PC_parse_string("a: 1");
}

} // namespace

TEST(Status, default_is_assert)
{
	// a fresh thread has the default handler
	PC_errhandler_t previous = PC_NULL_HANDLER;
	std::thread([&] { previous = PC_errhandler(PC_NULL_HANDLER); }).join();
	EXPECT_EQ(PC_ASSERT_HANDLER.func, previous.func);
	EXPECT_EQ(PC_ASSERT_HANDLER.context, previous.context);
}

TEST(ErrorHandlerDeathTest, assert_aborts_with_the_message)
{
	GTEST_FLAG_SET(death_test_style, "threadsafe");
	PC_tree_t tree = parse_doc();
	EXPECT_DEATH(
		{
			PC_errhandler(PC_ASSERT_HANDLER);
			PC_get(tree, ".missing");
		},
		"Error in paraconf: Key `missing' not found"
	);
	PC_tree_destroy(&tree);
}

TEST(Status, set_returns_the_previous_one)
{
	Recorder recorder1, recorder2;
	HandlerGuard guard(recorder1.handler());
	PC_errhandler_t previous = PC_errhandler(recorder2.handler());
	EXPECT_EQ(&Recorder::handle, previous.func);
	EXPECT_EQ(&recorder1, previous.context);
	previous = PC_errhandler(PC_NULL_HANDLER);
	EXPECT_EQ(&recorder2, previous.context);
}

TEST(Status, receives_status_message_and_context)
{
	Recorder recorder;
	HandlerGuard guard(recorder.handler());
	PC_tree_t tree = parse_doc();
	PC_get(tree, ".missing");
	ASSERT_EQ(1u, recorder.statuses.size());
	EXPECT_EQ(PC_NODE_NOT_FOUND, recorder.statuses[0]);
	EXPECT_EQ("Key `missing' not found in mapping (request was: `$tree.missing')\n", recorder.messages[0]);
	PC_tree_destroy(&tree);
}

TEST(Status, not_called_on_success)
{
	Recorder recorder;
	HandlerGuard guard(recorder.handler());
	PC_tree_t tree = parse_doc();
	long value;
	PC_int(PC_get(tree, ".a"), &value);
	EXPECT_TRUE(recorder.statuses.empty());
	PC_tree_destroy(&tree);
}

TEST(Status, null_handler_still_returns_the_status)
{
	HandlerGuard guard(PC_NULL_HANDLER);
	PC_tree_t tree = parse_doc();
	EXPECT_EQ(PC_NODE_NOT_FOUND, PC_status(PC_get(tree, ".missing")));
	long value;
	EXPECT_EQ(PC_NODE_NOT_FOUND, PC_int(PC_get(tree, ".missing"), &value));
	PC_tree_destroy(&tree);
}

TEST(Status, errmsg_is_the_last_message)
{
	HandlerGuard guard(PC_NULL_HANDLER);
	PC_tree_t tree = parse_doc();
	PC_get(tree, ".first");
	EXPECT_STREQ("Key `first' not found in mapping (request was: `$tree.first')\n", PC_errmsg());
	PC_get(tree, ".second");
	EXPECT_STREQ("Key `second' not found in mapping (request was: `$tree.second')\n", PC_errmsg());
	PC_tree_destroy(&tree);
}

TEST(Status, errmsg_is_kept_by_success)
{
	HandlerGuard guard(PC_NULL_HANDLER);
	PC_tree_t tree = parse_doc();
	PC_get(tree, ".missing");
	long value;
	EXPECT_EQ(PC_OK, PC_int(PC_get(tree, ".a"), &value));
	EXPECT_STREQ("Key `missing' not found in mapping (request was: `$tree.missing')\n", PC_errmsg());
	PC_tree_destroy(&tree);
}

TEST(Status, is_per_thread)
{
	Recorder main_recorder, thread_recorder;
	HandlerGuard guard(main_recorder.handler());
	PC_tree_t tree = parse_doc();
	std::thread([&] {
		PC_errhandler(thread_recorder.handler());
		PC_get(tree, ".in_thread");
	}).join();
	EXPECT_TRUE(main_recorder.statuses.empty());
	ASSERT_EQ(1u, thread_recorder.statuses.size());
	EXPECT_NE(std::string::npos, thread_recorder.messages[0].find("in_thread"));
	PC_tree_destroy(&tree);
}

TEST(Status, errmsg_is_per_thread)
{
	HandlerGuard guard(PC_NULL_HANDLER);
	PC_tree_t tree = parse_doc();
	PC_get(tree, ".in_main");
	std::thread([&] {
		PC_errhandler(PC_NULL_HANDLER);
		PC_get(tree, ".in_thread");
		EXPECT_NE(nullptr, strstr(PC_errmsg(), "in_thread"));
	}).join();
	EXPECT_NE(nullptr, strstr(PC_errmsg(), "in_main"));
	PC_tree_destroy(&tree);
}

TEST(Status, concurrent_errors)
{
	HandlerGuard guard(PC_NULL_HANDLER);
	PC_tree_t tree = parse_doc();
	std::vector<std::thread> threads;
	std::vector<int> mismatches(8, 0);
	for (int idx = 0; idx < 8; ++idx) {
		threads.emplace_back([&, idx] {
			PC_errhandler(PC_NULL_HANDLER);
			std::string key = "key" + std::to_string(idx);
			for (int iter = 0; iter < 1000; ++iter) {
				PC_get(tree, ".%s", key.c_str());
				if (!strstr(PC_errmsg(), key.c_str())) ++mismatches[idx];
			}
		});
	}
	for (auto&& thread: threads) {
		thread.join();
	}
	for (int idx = 0; idx < 8; ++idx) {
		EXPECT_EQ(0, mismatches[idx]) << "thread #" << idx;
	}
	PC_tree_destroy(&tree);
}
