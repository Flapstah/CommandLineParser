#pragma once
#include <vector>
#include <iostream>
#include <string>
#include <sstream>
#include <experimental/filesystem>
#include <functional>
#include <limits>
#include <typeinfo>

#define LOG_NORMAL 0
#define LOG_VERBOSE 1
#define LOG_VERY_VERBOSE 2

#define LOG_VERBOSITY LOG_NORMAL 

// TODO: expose unparsed arguments
// TODO: fix error handling

namespace CommandLine
{
	class CParser
	{
	public:
		enum eFlags
		{
			eF_REQUIRED = 1 << 0,					// Failure to provide this argument/switch is an error
			eF_SWITCH = 1 << 1,						// Switch style argument; no parameters collected.  Non-switch arguments collect one parameter.
			eF_MULTIPLE_VALUES = 1 << 2,	// Greedy collection of parameters until the next switch/argument, or end of command line
			eF_FOUND = 1 << 3,						// Indicates this argument was found on the command line
		};

		enum eConstants
		{
			eC_INVALID_INDEX = std::numeric_limits<uint32>::max()
		};

	protected:
		class CArgumentBase
		{
		public:
			typedef std::function<void(CArgumentBase*)> callback;

			virtual ~CArgumentBase()
			{
#if (LOG_VERBOSITY >= LOG_VERY_VERBOSE)
				std::cout << "CArgumentBase [" << GetName() << "] destructed" << std::endl;
#endif // (LOG_VERBOSITY >= LOG_VERY_VERBOSE)
			};

			inline CParser& GetParser(void) const { return m_parser; }
			inline void SetIndex(uint32 index) { m_index = index; }
			inline uint32 GetIndex(void) const { return m_index; }
			inline const char* GetName(void) const { return m_name; }
			inline char GetAbbr(void) const { return m_abbr; }
			inline const char* GetHelp(void) const { return m_help; }
			inline uint32 GetFlags(void) const { return m_flags; }

			bool ParseName(const char* arg, const uint32 index)
			{
				if (strcmp(GetName(), arg) == 0)
				{
#if (LOG_VERBOSITY >= LOG_VERBOSE)
					std::cout << "Found [" << GetName() << "]" << std::endl;
#endif // (LOG_VERBOSITY >= LOG_VERBOSE)
					Register(index);
					return true;
				}

				return false;
			}

			bool ParseAbbr(char arg, const uint32 index)
			{
				// TODO: utf-8
				if (GetAbbr() == arg)
				{
#if (LOG_VERBOSITY >= LOG_VERBOSE)
					std::cout << "Found [" << GetAbbr() << "] (" << GetName() << ")" << std::endl;
#endif // (LOG_VERBOSITY >= LOG_VERBOSE)
					Register(index);
					return true;
				}

				return false;
			}

		protected:
			CArgumentBase(CParser& parser, const char* name, char abbr, const char* help, uint32 flags, callback function)
				: m_parser(parser)
				, m_function(function)
				, m_recurrence(0)
				, m_name(name)
				, m_help(help)
				, m_flags(flags)
				, m_index(eC_INVALID_INDEX)
				, m_abbr(abbr)
			{
#if (LOG_VERBOSITY >= LOG_VERBOSE)
				std::cout << "CArgumentBase [" << GetName() << "] constructed" << std::endl;
#endif // (LOG_VERBOSITY >= LOG_VERBOSE)
			}

			virtual void Register(const uint32 index)
			{
				SetIndex(index);
				++m_recurrence;
				m_flags |= CParser::eF_FOUND;
				if (m_function)
				{
					m_function(this);
				}
			}

		protected:
			callback m_function;
			uint32 m_recurrence;

		private:
			CParser& m_parser;
			const char* m_name;
			const char* m_help;
			uint32 m_flags;
			uint32 m_index; // index into original command line
			const char m_abbr;
		};

	public:
		template<typename T>
		class CArgument : public CArgumentBase
		{
		public:
			CArgument(CParser& parser, const char* name, char abbr, const char* help, uint32 flags, callback function)
				: CArgumentBase(parser, name, abbr, help, flags, function)
			{
#if (LOG_VERBOSITY >= LOG_VERBOSE)
				std::cout << "CParameter<" << typeid(T).name() << "> [" << GetName() << "] constructed" << std::endl;
#endif // (LOG_VERBOSITY >= LOG_VERBOSE)
			}

			virtual ~CArgument()
			{
#if (LOG_VERBOSITY >= LOG_VERBOSE)
				std::cout << "CParameter<" << typeid(T).name() << "> [" << GetName() << "] destructed" << std::endl;
#endif // (LOG_VERBOSITY >= LOG_VERBOSE)
			}

			size_t GetNumValues(void) const { return m_values.size(); }
			const T GetValue(size_t index = 0) const { return (index < GetNumValues()) ? m_values[index] : T(); }

		protected:
			virtual void Register(const uint32 index)
			{
				const char* arg = nullptr;
				CParser& parser = GetParser();

				while (arg = parser.GetNextArgument())
				{
					if (parser.IsFlagArgument() || parser.IsNamedArgument())
					{
						parser.GetPreviousArgument();
						break;
					}
					else
					{
						std::istringstream iss(arg);
						// make it locale independent
						iss.imbue(std::locale::classic());
						T value;
						iss >> value;
						if (!iss.fail() && iss.eof())
						{
							m_values.push_back(std::move(value));
						}
						else
						{
#if (LOG_VERBOSITY >= LOG_NORMAL)
							std::cout << "CParameter<" << typeid(T).name() << "> unable to parse [" << arg << "] as [" << typeid(T).name() << "] for input parameter [" << GetName() << "], at index [#" << parser.GetArgumentIndex() << "]" << std::endl;
#endif // (LOG_VERBOSITY >= LOG_NORMAL)
							std::exit(-2); // TODO: some better error codes
						}
					}

					// If a single value, break here
					if ((GetFlags() & CParser::eF_MULTIPLE_VALUES) == 0) break;
				}

				if (m_values.size() == 0)
				{
#if (LOG_VERBOSITY >= LOG_NORMAL)
					std::cout << "CParameter<" << typeid(T).name() << "> [" << GetName() << "] does not appear to have values to parse from the command line when parsing input parameter [#" << parser.GetArgumentIndex() << "]" << std::endl;
#endif // (LOG_VERBOSITY >= LOG_NORMAL)
					std::exit(-1); // TODO: some better error codes
				}

				__super::Register(index);
			}

		private:
			std::vector<T> m_values;
		};

		// template specialisation for bool (switch)
		template<>
		class CArgument<bool> : public CArgumentBase
		{
		public:
			CArgument(CParser& parser, const char* name, char abbr, const char* help, uint32 flags, callback function)
				: CArgumentBase(parser, name, abbr, help, flags | CParser::eF_SWITCH, function)
				, m_value(false)
			{
#if (LOG_VERBOSITY >= LOG_VERBOSE)
				std::cout << "CArgument<bool> [" << GetName() << "] constructed (switch)" << std::endl;
#endif // (LOG_VERBOSITY >= LOG_VERBOSE)
			}

			virtual ~CArgument()
			{
#if (LOG_VERBOSITY >= LOG_VERBOSE)
				std::cout << "CArgument<bool> [" << GetName() << "] destructed (switch)" << std::endl;
#endif // (LOG_VERBOSITY >= LOG_VERBOSE)
			}

			operator bool() const { return m_value; }
			size_t GetNumValues(void) const { return 1; }
			const bool& GetValue(void) const { return m_value; }

		protected:
			virtual void Register(const uint32 index) { m_value = true; __super::Register(index); }

		private:
			bool m_value;
		};

	public:
		CParser(int argc, const char* const* argv, const char* description, const char* version, const char* separator = " ")
			: m_argv(argv)
			, m_argc(argc)
			, m_description(description)
			, m_version(version)
			, m_separator(separator)
			, m_argi(0)
			, m_argf(0)
		{
#if (LOG_VERBOSITY >= LOG_VERBOSE)
			std::cout << "CParser constructed" << std::endl;
#endif // (LOG_VERBOSITY >= LOG_VERBOSE)

			m_parameter.reserve(32);
			m_stopParsing = AddSwitch("ignore-rest", '-', "Stop parsing command line arguments following this flag");
			AddSwitch("help", 'h', "Displays usage information", 0, [this](CArgumentBase* pSwitch) { this->Help(); });
			AddSwitch("version", 'v', "Displays version information", 0, [this](CArgumentBase* pSwitch) { this->Version(); });
		}

		~CParser()
		{
			for (CArgumentBase* pParameter : m_parameter)
			{
				delete pParameter;
			}

#if (LOG_VERBOSITY >= LOG_VERBOSE)
			std::cout << "CParser destructed" << std::endl;
#endif // (LOG_VERBOSITY >= LOG_VERBOSE)
		}

		protected:
		bool IsDuplicate(const char* name, char abbr) const
		{
			bool duplicate = false;
			for (const CArgumentBase* pParameter : m_parameter)
			{
				if (abbr == pParameter->GetAbbr())
				{
#if (LOG_VERBOSITY >= LOG_NORMAL)
					std::cout << "Duplicate abbreviation [-" << abbr << "] detected when trying to add [--" << name << "]:[-" << abbr << "]" << std::endl;
#endif // (LOG_VERBOSITY >= LOG_NORMAL)
					duplicate = true;
					break;
				}
				if (strcmp(name, pParameter->GetName()) == 0)
				{
#if (LOG_VERBOSITY >= LOG_NORMAL)
					std::cout << "Duplicate name [--" << name << "] detected when trying to add [--" << name << "]:[-" << abbr << "]" << std::endl;
#endif // (LOG_VERBOSITY >= LOG_NORMAL)
					duplicate = true;
					break;
				}
			}

			return duplicate;
		}

		public:
		template <typename T>
		const CArgument<T>* AddArgument(const char* name, char abbr, const char* help = "", uint32 flags = 0, CArgumentBase::callback function = nullptr)
		{
			bool duplicate = IsDuplicate(name, abbr);
			CArgument<T>* pArgument = nullptr;

			if (!duplicate)
			{
				pArgument = new CArgument<T>(*this, name, abbr, help, flags, function);
				m_parameter.push_back(pArgument);
			}

			return pArgument;
		}

		const CArgument<bool>* AddSwitch(const char* name, char abbr, const char* help = "", uint32 flags = 0, CArgumentBase::callback function = nullptr)
		{
			return AddArgument<bool>(name, abbr, help, flags, function);
		}

		bool Parse(void)
		{
			const char* arg = nullptr;
			bool parsed = false;

			while (arg = GetNextArgument())
			{
				if (IsFlagArgument())
				{
					for (m_argf = 1; (!*m_stopParsing) && (m_argf < strlen(arg)); ++m_argf)
					{
						parsed = false;
						for (CArgumentBase* pParameter : m_parameter)
						{
							if (parsed = pParameter->ParseAbbr(arg[m_argf], GetArgumentIndex())) break;
						}

						if (!parsed)
						{
							std::cerr << "Unknown flag [" << arg[m_argf] << "] at index [#" << m_argf << "] in [" << arg << "] at index [#" << GetArgumentIndex() << "]" << std::endl;
							return false;
						}
					}
				}
				else if (IsNamedArgument())
				{
					parsed = false;
					for (CArgumentBase* pParameter : m_parameter)
					{
						if (parsed = pParameter->ParseName(&arg[2], GetArgumentIndex())) break;
					}

					if (!parsed)
					{
						std::cerr << "Unknown argument [" << arg << "] at index [#" << GetArgumentIndex() << "]" << std::endl;
						return false;
					}
				}
				else
				{
					// This is an unnamed parameter
					m_unnamed.push_back(GetArgumentIndex());
				}
			}

			return HaveAllRequiredParameters();
		}

		void Help(void) const
		{
			std::experimental::filesystem::path executable(m_argv[0]);
			std::cout << "Usage:" << std::endl << executable.filename();
			for (const CArgumentBase* pParameter : m_parameter)
			{
				bool optional = (pParameter->GetFlags() & CParser::eF_REQUIRED) == 0;
				std::cout << m_separator << (optional ? "[" : "") << "-" << pParameter->GetAbbr() << (optional ? "]" : "");
			}
			std::cout << std::endl << std::endl << "Where:" << std::endl;
			for (const CArgumentBase* pParameter : m_parameter)
			{
				bool required = (pParameter->GetFlags() & CParser::eF_REQUIRED) != 0;
				std::cout << "-" << pParameter->GetAbbr() << "," << m_separator << "--" << pParameter->GetName() << std::endl;
				std::cout << m_separator << (required ? "(required)" : "") << pParameter->GetHelp() << std::endl << std::endl;
			}
			Version();
			std::cout << std::endl << "Description:" << std::endl << m_description << std::endl;
		}

		inline void Version(void) const { std::cout << "Version" << m_separator << "[" << m_version << "]" << std::endl; }

		protected:
		inline uint32 GetArgumentIndex(void) const { return m_argi; }
		inline const char* GetNextArgument(void) const { return ((m_argi + 1) < m_argc) ? m_argv[++m_argi] : nullptr; }
		inline const char* GetPreviousArgument(void) const { return ((m_argi - 1) >= 0) ? m_argv[--m_argi] : nullptr; }
		inline bool IsFlagArgument(void) const { return (m_argv[m_argi][0] == '-') && ((m_argv[m_argi][1] != '-') || (strlen(m_argv[m_argi]) == 2)); }
		inline bool IsNamedArgument(void) const { return (m_argv[m_argi][0] == '-') && (m_argv[m_argi][1] == '-') && (strlen(m_argv[m_argi]) > 2); }
		bool HaveAllRequiredParameters(void) const
		{
			// Check for required parameters
			for (const CArgumentBase* pParameter : m_parameter)
			{
				if (
					((pParameter->GetFlags() & CParser::eF_REQUIRED) == CParser::eF_REQUIRED) &&
					(pParameter->GetIndex() == CParser::eC_INVALID_INDEX)
					)
				{
					std::cerr << "Required argument [-" << pParameter->GetAbbr() << "," << m_separator << "--" << pParameter->GetName() << "]" << std::endl;
					return false;
				}
			}

			return true;
		}

	private:
		std::vector<uint32> m_unnamed; // stores indices of unnamed arguments
		std::vector<CArgumentBase*> m_parameter;
		const CArgument<bool>* m_stopParsing;
		const char* const* m_argv; // stores arguments as passed on the command line to the program
		const char* m_description; // stores a description of this command
		const char* m_version; // stores the version of this command
		const char* m_separator; // stores the separator used in formatting help and version
		const uint32 m_argc; // stores the argument count as passed to the program
		mutable uint32 m_argi; // stores the index into m_argv of the argument being processed
		uint32 m_argf; // stores the index into m_argv[m_argi] of the flag being processed
	};
}
