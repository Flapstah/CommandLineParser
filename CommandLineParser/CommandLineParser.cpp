// CommandLineParser.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "parser.h"

int main(int argc, char* argv[])
{
	char* test_argv[] = {
		"test.exe",
		"--test", "file1", "file2", "file3",
		"destination"
	};
	int test_argc = sizeof(test_argv) / sizeof(char*);

	CommandLine::CParser parser(test_argc, test_argv, "Test command", "0.1");
	const CommandLine::CParser::CArgument<char*>* arg = parser.AddArgument<char*>("test", 't', "test argument");
	std::cout << ((parser.Parse()) ? "succeeded" : "failed") << std::endl;
	std::cout << "Values of [" << arg->GetName() << "]" << std::endl;
	for (size_t i = 0; i < arg->GetNumValues(); ++i)
	{
		std::cout << "#" << i << " : [" << arg->GetValue(i) << "]" << std::endl;
	}
	std::cout << "times occurred: " << arg->GetTimesOccured() << std::endl;

	size_t count = parser.GetUnparsedArgumentCount();
	std::cout << "There were [" << count << "] unparsed arguments" << std::endl;
	for (size_t index = 0; index < count; ++index)
	{
		std::cout << "unparsed #" << index << ": [" << parser.GetUnparsedArgument(index) << "]" << std::endl;
	}

	return 0;
}

