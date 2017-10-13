// CommandLineParser.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "parser.h"

int main(int argc, char* argv[])
{
	char* test_argv[] = {
		"test.exe",
		"--test", "123", "456",
		"-v",
		"--test", "789"
	};
	int test_argc = sizeof(test_argv) / sizeof(char*);

	CommandLine::CParser parser(test_argc, test_argv, "Test command", "0.1");
	const CommandLine::CParser::CArgument<char*>* arg = parser.AddArgument<char*>("test", 't', "test argument", CommandLine::CParser::eF_MULTIPLE_VALUES);
	std::cout << ((parser.Parse()) ? "succeeded" : "failed") << std::endl;
	for (uint32 i = 0; i < arg->GetNumValues(); ++i)
	{
		std::cout << "#" << i << " : [" << arg->GetValue(i) << "]" << std::endl;
	}
	std::cout << "times occurred: " << arg->GetTimesOccured() << std::endl;
	return 0;
}

