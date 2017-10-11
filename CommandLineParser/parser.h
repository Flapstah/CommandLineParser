#pragma once
#include <vector>
#include <iostream>
#include <experimental/filesystem>
#include <functional>
#include <limits>

enum ELogVerbosity
{
	eLB_NORMAL,
	eLB_VERBOSE,
	eLB_VERY_VERBOSE,
};

#define LOG_VERBOSITY eLB_VERY_VERBOSE

namespace CommandLine
{
	class CParser
	{
	protected:
		class CParameterBase
		{
		public:
			typedef std::function<void(CParameterBase*)> callback;

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

			virtual ~CParameterBase()
			{
#if (LOG_VERBOSITY >= eLB_VERY_VERBOSE)
				std::cout << "CParameterBase [" << GetName() << "] destructed" << std::endl;
#endif // (LOG_VERBOSITY >= eLB_VERY_VERBOSE)
			};

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
#if (LOG_VERBOSITY >= eLB_VERBOSE)
					std::cout << "Found [" << GetName() << "]" << std::endl;
#endif // (LOG_VERBOSITY >= eLB_VERBOSE)
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
#if (LOG_VERBOSITY >= eLB_VERBOSE)
					std::cout << "Found [" << GetAbbr() << "] (" << GetName() << ")" << std::endl;
#endif // (LOG_VERBOSITY >= eLB_VERBOSE)
					Register(index);
					return true;
				}

				return false;
			}

		protected:
			CParameterBase(CParser& parser, const char* name, char abbr, const char* help, uint32 flags, callback function)
				: m_parser(parser)
				, m_function(function)
				, m_recurrence(0)
				, m_name(name)
				, m_help(help)
				, m_flags(flags)
				, m_index(eC_INVALID_INDEX)
				, m_abbr(abbr)
			{
#if (LOG_VERBOSITY >= eLB_VERY_VERBOSE)
				std::cout << "CParameterBase [" << GetName() << "] constructed" << std::endl;
#endif // (LOG_VERBOSITY >= eLB_VERY_VERBOSE)
			}

			inline void Register(const uint32 index)
			{
				SetIndex(index);
				++m_recurrence;
				m_flags |= CParameterBase::eF_FOUND;
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
		class CParameter : public CParameterBase
		{
		public:
			CParameter(CParser& parser, const char* name, char abbr, const char* help, uint32 flags, callback function)
				: CParameterBase(parser, name, abbr, help, flags, function)
			{
#if (LOG_VERBOSITY >= eLB_VERY_VERBOSE)
				std::cout << "CParameter [" << GetName() << "] constructed" << std::endl;
#endif // (LOG_VERBOSITY >= eLB_VERY_VERBOSE)
			}

			virtual ~CParameter()
			{
#if (LOG_VERBOSITY >= eLB_VERY_VERBOSE)
				std::cout << "CParameter [" << GetName() << "] destructed" << std::endl;
#endif // (LOG_VERBOSITY >= eLB_VERY_VERBOSE)
			}

			size_t GetNumValues(void) const { return m_values.size(); }
			const T& GetValue(size_t index = 0) const { return (index < GetNumValues()) ? m_values[index] : T(); }

		protected:
			void Register(const uint32 index)
			{
				const char* arg = nullptr;

				while (arg = PeekNextArgument())
				{
					if (IsFlagArgument() || IsNamedArgument())
					{
						break;
					}
					else
					{
						std::istringstream iss(arg);
						T value;
						iss >> value;
						if (!iss.fail() && ss.eof())
						{
							m_values.push_back(std::move(value));
						}
						else
						{
#if (LOG_VERBOSITY >= eLB_NORMAL)
							std::cout << "CParameter<T> [" << GetName() << "] unable to parse [" << arg << "] from input parameter [" << GetArgumentIndex() + 1 << "]" << std::endl;
#endif // (LOG_VERBOSITY >= eLB_NORMAL)
							std::exit(-2); // TODO: some better error codes
						}
					}

					// This argument has now been processed, so skip it
					GetNextArgument();

					// If a single value, break here
					if ((GetFlags() & eF_MULTIPLE_VALUES) == 0) break;
				}

				if (m_values.size() == 0)
				{
#if (LOG_VERBOSITY >= eLB_NORMAL)
					std::cout << "CParameter<T> [" << GetName() << "] does not appear to have values to parse from the command line when parsing input parameter [" << GetArgumentIndex() + 1 << "]" << std::endl;
#endif // (LOG_VERBOSITY >= eLB_NORMAL)
					std::exit(-1); // TODO: some better error codes
				}

				__super::Register(index);
			}

		private:
			std::vector<T> m_values;
		};

		// template specialisation for bool (switch)
		template<>
		class CParameter<bool> : public CParameterBase
		{
		public:
			CParameter(CParser& parser, const char* name, char abbr, const char* help, uint32 flags, callback function)
				: CParameterBase(parser, name, abbr, help, flags | CParameterBase::eF_SWITCH, function)
				, m_value(false)
			{
#if (LOG_VERBOSITY >= eLB_VERY_VERBOSE)
				std::cout << "CParameter<bool> [" << GetName() << "] constructed (switch)" << std::endl;
#endif // (LOG_VERBOSITY >= eLB_VERY_VERBOSE)
			}

			virtual ~CParameter()
			{
#if (LOG_VERBOSITY >= eLB_VERY_VERBOSE)
				std::cout << "CParameter<bool> [" << GetName() << "] destructed (switch)" << std::endl;
#endif // (LOG_VERBOSITY >= eLB_VERY_VERBOSE)
			}

			operator bool() const { return m_value; }
			size_t GetNumValues(void) const { return 1; }
			const bool& GetValue(void) const { return m_value; }

		protected:
			void Register(const uint32 index) { m_value = true; __super::Register(index); }

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
#if (LOG_VERBOSITY >= eLB_VERY_VERBOSE)
			std::cout << "CParser constructed" << std::endl;
#endif // (LOG_VERBOSITY >= eLB_VERY_VERBOSE)

			m_parameter.reserve(32);
			// N.B. ignore-rest *must* be the first parameter added (see GetStopParsingSwitch() below)
			AddSwitch("ignore-rest", '-', "Stop parsing command line arguments following this flag");
			AddSwitch("help", 'h', "Displays usage information", 0, [this](CParameterBase* pSwitch) { this->Help(); });
			AddSwitch("version", 'v', "Displays version information", 0, [this](CParameterBase* pSwitch) { this->Version(); });
		}

		~CParser()
		{
			for (CParameterBase* pParameter : m_parameter)
			{
				delete pParameter;
			}

#if (LOG_VERBOSITY >= eLB_VERY_VERBOSE)
			std::cout << "CParser destructed" << std::endl;
#endif // (LOG_VERBOSITY >= eLB_VERY_VERBOSE)
		}

		bool IsDuplicate(const char* name, char abbr) const
		{
			bool duplicate = false;
			for (const CParameterBase* pParameter : m_parameter)
			{
				if (abbr == pParameter->GetAbbr())
				{
#if (LOG_VERBOSITY >= eLB_NORMAL)
					std::cout << "Duplicate abbreviation [-" << abbr << "] detected when trying to add [--" << name << "]:[-" << abbr << "]" << std::endl;
#endif // (LOG_VERBOSITY >= eLB_NORMAL)
					duplicate = true;
					break;
				}
				if (strcmp(name, pParameter->GetName()) == 0)
				{
#if (LOG_VERBOSITY >= eLB_NORMAL)
					std::cout << "Duplicate name [--" << name << "] detected when trying to add [--" << name << "]:[-" << abbr << "]" << std::endl;
#endif // (LOG_VERBOSITY >= eLB_NORMAL)
					duplicate = true;
					break;
				}
			}

			return duplicate;
		}

		const CParameter<bool>* AddSwitch(const char* name, char abbr, const char* help = "", uint32 flags = 0, CParameterBase::callback function = nullptr)
		{
			bool duplicate = IsDuplicate(name, abbr);
			CParameter<bool>* pSwitch = nullptr;

			if (!duplicate)
			{
				pSwitch = new CParameter<bool>(*this, name, abbr, help, flags, function);
				m_parameter.push_back(pSwitch);
			}

			return pSwitch;
		}

		bool Parse(void)
		{
			const char* arg = nullptr;
			bool parsed = false;

			const CParameter<bool>* pStopParsing = GetStopParsingSwitch();
			while (arg = GetNextArgument())
			{
				if (IsFlagArgument())
				{
					for (m_argf = 1; (!*pStopParsing) && (m_argf < strlen(arg)); ++m_argf)
					{
						parsed = false;
						for (CParameterBase* pParameter : m_parameter)
						{
							if (parsed = pParameter->ParseAbbr(arg[m_argf], GetArgumentIndex())) break;
						}

						if (!parsed)
						{
							std::cerr << "Unknown argument encountered [" << arg[m_argf] << "] [#" << m_argf << "] in [#" << GetArgumentIndex() << "]:[" << arg << "]" << std::endl;
							return false;
						}
					}
				}
				else if (IsNamedArgument())
				{
					parsed = false;
					for (CParameterBase* pParameter : m_parameter)
					{
						if (parsed = pParameter->ParseName(&arg[2], GetArgumentIndex())) break;
					}

					if (!parsed)
					{
						std::cerr << "Unknown argument encountered [#" << GetArgumentIndex() << "]:[" << arg << "]" << std::endl;
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
			for (const CParameterBase* pParameter : m_parameter)
			{
				bool optional = (pParameter->GetFlags() & CParameterBase::eF_REQUIRED) == 0;
				std::cout << m_separator << (optional ? "[" : "") << "-" << pParameter->GetAbbr() << (optional ? "]" : "");
			}
			std::cout << std::endl << std::endl << "Where:" << std::endl;
			for (const CParameterBase* pParameter : m_parameter)
			{
				bool required = (pParameter->GetFlags() & CParameterBase::eF_REQUIRED) != 0;
				std::cout << "-" << pParameter->GetAbbr() << "," << m_separator << "--" << pParameter->GetName() << std::endl;
				std::cout << m_separator << (required ? "(required)" : "") << pParameter->GetHelp() << std::endl << std::endl;
			}
			Version();
			std::cout << std::endl << "Description:" << std::endl << m_description << std::endl;
		}

		inline void Version(void) const { std::cout << "Version" << m_separator << "[" << m_version << "]" << std::endl; }
		inline uint32 GetArgumentIndex(void) const { return m_argi; }
		inline const char* GetNextArgument(void) const { return ((m_argi + 1) < m_argc) ? m_argv[++m_argi] : nullptr; }
		inline const char* PeekNextArgument(void) const { return ((m_argi + 1) < m_argc) ? m_argv[m_argi + 1] : nullptr; }
		inline bool IsFlagArgument(void) const { return (m_argv[m_argi][0] == '-') && ((m_argv[m_argi][1] != '-') || (strlen(m_argv[m_argi]) == 2)); }
		inline bool IsNamedArgument(void) const { return (m_argv[m_argi][0] == '-') && (m_argv[m_argi][1] == '-') && (strlen(m_argv[m_argi]) > 2); }

	protected:
		bool HaveAllRequiredParameters(void) const
		{
			// Check for required parameters
			for (const CParameterBase* pParameter : m_parameter)
			{
				if (
					((pParameter->GetFlags() & CParameterBase::eF_REQUIRED) == CParameterBase::eF_REQUIRED) &&
					(pParameter->GetIndex() == CParameterBase::eC_INVALID_INDEX)
					)
				{
					std::cerr << "Required argument [-" << pParameter->GetAbbr() << "," << m_separator << "--" << pParameter->GetName() << "]" << std::endl;
					return false;
				}
			}

			return true;
		}

		const CParameter<bool>* GetStopParsingSwitch(void) const { return static_cast<CParameter<bool>*>(m_parameter[0]); }

	private:
		std::vector<uint32> m_unnamed; // stores indices of unnamed arguments
		std::vector<CParameterBase*> m_parameter;
		const char* const* m_argv; // stores arguments as passed on the command line to the program
		const char* m_description; // stores a description of this command
		const char* m_version; // stores the version of this command
		const char* m_separator; // stores the separator used in formatting help and version
		const uint32 m_argc; // stores the argument count as passed to the program
		mutable uint32 m_argi; // stores the index into m_argv of the argument being processed
		uint32 m_argf; // stores the index into m_argv[m_argi] of the flag being processed
	};
}
