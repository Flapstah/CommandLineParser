// CommandLineParser.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "parser.h"

int main(int argc, char* argv[])
{
	char* test_argv[] = {
		"test.exe",
		"--help"
	};
	int test_argc = sizeof(test_argv) / sizeof(char*);

	CommandLine::CParser parser(test_argc, test_argv, "Test command", "0.1");
	std::cout << ((parser.Parse()) ? "succeeded" : "failed") << std::endl;
	return 0;
}

