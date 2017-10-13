// CommandLineParser.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "parser.h"

int main(int argc, char* argv[])
{
	char* test_argv[] = {
		"test.exe",
		"file1", "file2", "file3",
		"destination"
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

	uint32 count = parser.GetUnparsedArgumentCount();
	for (uint32 index = 0; index <= count; ++index)
	{
		std::cout << "unparsed #" << index << ": [" << parser.GetUnparsedArgument(index) << "]" << std::endl;
	}

	return 0;
}

