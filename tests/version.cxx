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

TEST(Version, matches_the_header)
{
	EXPECT_EQ(PARACONF_VERSION, PC_version());
	EXPECT_EQ(PARACONF_VERSION_MAJOR, PC_version() >> 48);
	EXPECT_EQ(PARACONF_VERSION_MINOR, (PC_version() >> 32) & 0xFFFF);
	EXPECT_EQ(PARACONF_VERSION_PATCH, (PC_version() >> 16) & 0xFFFF);
	EXPECT_EQ(PARACONF_VERSION_VARIANT, PC_version() & 0xFFFF);
}
