// tester.cpp
// Copyright (c) 2024, yuki
// SPDX-License-Identifier: Apache-2.0

#include "tester.h"

int main(int argc, char** argv)
{
	if(argc != 2)
	{
		zpr::println("usage: sap-test <test dir>");
		return 0;
	}

	const auto test_dir = argv[1];

	test::Context context {};

	test::test_parser(context, test_dir);
}
