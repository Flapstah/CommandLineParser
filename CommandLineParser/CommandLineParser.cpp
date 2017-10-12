// CommandLineParser.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "parser.h"

int main(int argc, char* argv[])
{
	char* test_argv[] = {
		"test.exe",
		"--test", "123", "456",
		"--other"
	};
	int test_argc = sizeof(test_argv) / sizeof(char*);

	CommandLine::CParser parser(test_argc, test_argv, "Test command", "0.1");
	const CommandLine::CParser::CArgument<int>* arg = parser.AddArgument<int>("test", 't', "test argument", CommandLine::CParser::eF_MULTIPLE_VALUES);
	std::cout << ((parser.Parse()) ? "succeeded" : "failed") << std::endl;
	std::cout << "arg [" << arg->GetValue() << "]" << std::endl;
	return 0;
}

