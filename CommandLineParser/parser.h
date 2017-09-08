#pragma once
#include <vector>
#include <iostream>
#include <experimental/filesystem>
#include <functional>
#include <limits>

// TODO: check for abbreviation and name collisions when adding parameters
// TODO: fix parameter storage to own the parameter memory (std::move) and return a pointer to the stored parameter (for things like m_stopProcessing)
namespace CommandLine
{
	class CParser
	{
	public:
		class CParameter
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

			virtual ~CParameter()
			{
				//std::cout << "CParameter [" << GetName() << "] destructed" << std::endl;
			};

			inline void SetIndex(uint32 index) { m_index = index; }
			inline uint32 GetIndex(void) { return m_index; }
			inline const char* GetName(void) { return m_name; }
			inline const char* GetAbbr(void) { return m_abbr; }
			inline const char* GetHelp(void) { return m_help; }
			inline uint32 GetFlags(void) { return m_flags; }

			virtual bool ParseName(const char* arg, const uint32 index) = 0;
			virtual bool ParseAbbr(const char arg, const uint32 index) = 0;

		protected:
			CParameter(const char* name, const char* abbr, const char* help, uint32 flags)
				: m_name(name)
				, m_abbr(abbr)
				, m_help(help)
				, m_flags(flags)
				, m_index(eC_INVALID_INDEX)
			{
				//std::cout << "CParameter [" << GetName() << "] constructed" << std::endl;
			}

		private:
			const char* m_name;
			const char* m_abbr;
			const char* m_help;
			uint32 m_flags;
			uint32 m_index; // index into original command line
		};

		class CSwitch : public CParameter
		{
		public:
			typedef std::function<void(CSwitch*)> callback;

			CSwitch(const char* name, const char* abbr, const char* help = "", uint32 flags = 0, callback function = nullptr)
				: CParameter(name, abbr, help, flags | CParameter::eF_SWITCH)
				, m_function(function)
				, m_recurrence(0)
				, m_value(false)
			{
				//std::cout << "CSwitch [" << GetName() << "] constructed" << std::endl;
			}

			virtual ~CSwitch()
			{
				//std::cout << "CSwitch [" << GetName() << "] destructed" << std::endl;
			}

			inline operator bool()
			{
				return m_value;
			}

			virtual bool ParseName(const char* arg, const uint32 index) override
			{
				if (strcmp(GetName(), arg) == 0)
				{
					std::cout << "Found [" << GetName() << "]" << std::endl;
					Register(index);
					return true;
				}

				return false;
			}

			virtual bool ParseAbbr(const char arg, const uint32 index) override
			{
				// TODO: utf-8
				if (GetAbbr()[0] == arg)
				{
					std::cout << "Found [" << GetAbbr() << "] (" << GetName() << ")" << std::endl;
					Register(index);
					return true;
				}

				return false;
			}

		protected:
		private:
			callback m_function;
			uint32 m_recurrence;
			bool m_value;

			inline void Register(const uint32 index)
			{
				++m_recurrence;
				SetIndex(index);
				m_value = true;
				if (m_function)
				{
					m_function(this);
				}
			}
		};

		CParser(int argc, const char* const* argv, const char* description, const char* version, const char* separator = " ")
			: m_argv(argv)
			, m_argc(argc)
			, m_description(description)
			, m_version(version)
			, m_separator(separator)
			, m_stopParsing("ignore-rest", "-", "Stop parsing command line arguments following this flag")
			, m_displayHelp("help", "h", "Displays usage information", 0, [this](CSwitch* pSwitch) { this->Help(); })
			, m_displayVersion("version", "v", "Displays version information", 0, [this](CSwitch* pSwitch) { this->Version(); })
			, m_argi(0)
			, m_argf(0)
		{
			//std::cout << "CParser constructed" << std::endl;

			m_parameter.reserve(32);
			m_parameter.push_back(&m_stopParsing);
			m_parameter.push_back(&m_displayHelp);
			m_parameter.push_back(&m_displayVersion);
		}

		~CParser()
		{
			//std::cout << "CParser destructed" << std::endl;
		}

		bool Parse(void)
		{
			const char* arg = nullptr;
			bool parsed = false;

			while (arg = GetNextArgument())
			{
				if (IsFlagArgument())
				{
					for (m_argf = 1; (!m_stopParsing) && (m_argf < strlen(arg)); ++m_argf)
					{
						parsed = false;
						for (CParameter* pParameter : m_parameter)
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
					for (CParameter* pParameter : m_parameter)
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
			for (CParameter* pArg : m_parameter)
			{
				bool optional = (pArg->GetFlags() & CParameter::eF_REQUIRED) == 0;
				std::cout << m_separator << (optional ? "[" : "") << "-" << pArg->GetAbbr() << (optional ? "]" : "");
			}
			std::cout << std::endl << std::endl << "Where:" << std::endl;
			for (CParameter* pArg : m_parameter)
			{
				bool required = (pArg->GetFlags() & CParameter::eF_REQUIRED) != 0;
				std::cout << "-" << pArg->GetAbbr() << "," << m_separator << "--" << pArg->GetName() << std::endl;
				std::cout << m_separator << (required ? "(required)" : "") << pArg->GetHelp() << std::endl << std::endl;
			}
			Version();
			std::cout << std::endl << "Description:" << std::endl << m_description << std::endl;
		}

		inline void Version(void) const { std::cout << "Version" << m_separator << "[" << m_version << "]" << std::endl; }
		inline uint32 GetArgumentIndex(void) const { return m_argi; }
		inline const char* GetNextArgument(void) const { return ((m_argi + 1) < m_argc) ? m_argv[++m_argi] : nullptr; }
		inline bool IsFlagArgument(void) const { return (m_argv[m_argi][0] == '-') && ((m_argv[m_argi][1] != '-') || (strlen(m_argv[m_argi]) == 2)); }
		inline bool IsNamedArgument(void) const { return (m_argv[m_argi][0] == '-') && (m_argv[m_argi][1] == '-') && (strlen(m_argv[m_argi]) > 2); }

	protected:
		bool HaveAllRequiredParameters(void) const
		{
			// Check for required parameters
			for (CParameter* pParameter : m_parameter)
			{
				if (
					((pParameter->GetFlags() & CParameter::eF_REQUIRED) == CParameter::eF_REQUIRED) &&
					(pParameter->GetIndex() == CParameter::eC_INVALID_INDEX)
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
		std::vector<CParameter*> m_parameter;
		const char* const* m_argv; // stores arguments as passed on the command line to the program
		const char* m_description; // stores a description of this command
		const char* m_version; // stores the version of this command
		const char* m_separator; // stores the separator used in formatting help and version
		const uint32 m_argc; // stores the argument count as passed to the program
		CSwitch m_stopParsing;
		CSwitch m_displayHelp;
		CSwitch m_displayVersion;
		mutable uint32 m_argi; // stores the index into m_argv of the argument being processed
		uint32 m_argf; // stores the index into m_argv[m_argi] of the flag being processed
	};
}
