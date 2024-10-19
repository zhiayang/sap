/*
    zarg.h
    Copyright 2023, yuki / zhiayang

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/


/*
    Version 0.1.1
    =============
*/

#if !defined(__ZARG_H)
#define __ZARG_H

#include <cstddef>
#include <cstdint>

#include <deque>
#include <vector>
#include <string>
#include <optional>
#include <string_view>
#include <unordered_set>
#include <unordered_map>

namespace zarg
{
	struct Arg
	{
		char short_name;
		std::string long_name;
		std::optional<std::string> value;
	};

	struct ArgumentSet
	{
		std::unordered_set<char> flags;
		std::unordered_map<std::string, Arg> options;

		std::vector<std::string> positional;

		bool has_option(const std::string& opt) const;
		std::optional<std::string> get_option(const std::string& opt) const;
	};

	struct ArgumentList
	{
		std::vector<char> flags;
		std::vector<Arg> options;
		std::vector<std::string> positional;

		ArgumentSet set() const&;
		ArgumentSet set() &&;
	};

	struct Parser
	{
		ArgumentList parse(int argc, char** argv);

		Parser& add_flag(char flag, std::string description = "");
		Parser& add_option(char short_opt, bool needs_value, std::string description = "");

		Parser& add_option(std::string long_opt, bool needs_value, std::string description = "");
		Parser& add_option(char short_opt, std::string long_opt, bool needs_value, std::string description = "");

		Parser& allow_options_after_positionals(bool x = true);
		Parser& ignore_unknown_flags(bool x = true);

	private:
		struct Option
		{
			char flag_or_short_opt;
			std::string long_option;

			bool needs_value;
			bool is_flag;

			std::string description;
		};


		std::unordered_set<char> m_flags;

		std::unordered_map<char, const Option*> m_short_options;
		std::unordered_map<std::string, const Option*> m_long_options;

		bool m_allow_options_after_positionals = false;
		bool m_ignore_unknown_flags = false;

		std::deque<Option> m_all_options;
	};
}

#endif



#if defined(ZARG_IMPLEMENTATION)

#include <cstdio>
#include <cstdarg>
#include <cstdlib>

namespace zarg
{
	[[noreturn]] static inline void error_and_exit(const char* fmt, ...)
	{
		va_list ap;
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
		exit(1);
	}

	bool ArgumentSet::has_option(const std::string& opt) const
	{
		return this->options.find(opt) != this->options.end();
	}

	std::optional<std::string> ArgumentSet::get_option(const std::string& opt) const
	{
		if(auto it = this->options.find(opt); it != this->options.end())
			return it->second.value;
		else
			return std::nullopt;
	}

	ArgumentSet ArgumentList::set() const&
	{
		ArgumentSet ret {};

		ret.positional = this->positional;
		ret.flags.insert(this->flags.begin(), this->flags.end());

		for(auto& opt : this->options)
		{
			std::string x {};
			if(not opt.long_name.empty())
				x = opt.long_name;
			else
				x = std::string(&opt.short_name, 1);

			ret.options.emplace(std::move(x), opt);
		}

		return ret;
	}

	ArgumentSet ArgumentList::set() &&
	{
		ArgumentSet ret {};

		ret.positional = std::move(this->positional);
		ret.flags.insert(std::move_iterator(this->flags.begin()), std::move_iterator(this->flags.end()));

		for(auto& opt : this->options)
		{
			std::string x {};
			if(not opt.long_name.empty())
				x = opt.long_name;
			else
				x = std::string(&opt.short_name, 1);

			ret.options.emplace(std::move(x), std::move(opt));
		}

		return ret;
	}

	ArgumentList Parser::parse(int argc, char** argv)
	{
		ArgumentList ret {};

		bool no_more_opts = false;
		for(int arg_num = 1; arg_num < argc; arg_num++)
		{
			auto arg = std::string_view(argv[arg_num]);
			if(arg.empty())
				continue;

			if(arg == "--")
			{
				no_more_opts = true;
				continue;
			}

			if(no_more_opts || (not arg.starts_with("-")))
			{
				ret.positional.emplace_back(argv[arg_num]);
				if(not m_allow_options_after_positionals)
					no_more_opts = true;

				continue;
			}


			if(arg[0] == '-' && not arg.starts_with("--"))
			{
				bool stop = false;
				for(size_t i = 1; not stop && i < arg.size(); i++)
				{
					if(m_flags.contains(arg[i]))
					{
						ret.flags.push_back(arg[i]);
					}
					else if(auto it = m_short_options.find(arg[i]); it != m_short_options.end())
					{
						auto& opt = *it->second;
						if(opt.needs_value)
						{
							// once we need a value, we cannot keep parsing the "clumped" options.
							stop = true;

							auto rest = arg.substr(i + 1);
							if(not rest.empty())
							{
								// get the option as the remaining string
								ret.options.push_back(Arg {
								    .short_name = arg[i],
								    .long_name = opt.long_option,
								    .value = std::string(rest),
								});
							}
							else
							{
								// get the option as the next option.
								if(arg_num + 1 == argc)
									error_and_exit("missing required value for option '-%c'\n", arg[i]);

								ret.options.push_back(Arg {
								    .short_name = arg[i],
								    .long_name = opt.long_option,
								    .value = std::string(argv[arg_num + 1]),
								});

								// skip that arg.
								arg_num += 1;
							}
						}
						else
						{
							ret.options.push_back(Arg {
							    .short_name = arg[i],
							    .long_name = opt.long_option,
							    .value = std::nullopt,
							});
						}
					}
					else
					{
						if(not m_ignore_unknown_flags)
							error_and_exit("unrecognised option '-%c'\n", arg[i]);
					}
				}
			}
			else
			{
				auto [option_name, option_value] = [&arg]() -> auto
				{
					auto tmp = arg.substr(2);

					std::string name {};
					std::string_view value {};

					auto i = tmp.find('=');
					if(i != std::string::npos)
					{
						name = std::string(tmp.substr(0, i));
						value = tmp.substr(i + 1);
					}
					else
					{
						name = std::string(tmp);
					}

					struct F
					{
						std::string name;
						std::string_view value;
					};

					return F {
						.name = std::move(name),
						.value = value,
					};
				}
				();

				if(auto it = m_long_options.find(option_name); it != m_long_options.end())
				{
					auto& opt = *it->second;
					if(opt.needs_value)
					{
						std::string opt_value {};
						if(not option_value.empty())
						{
							opt_value = std::string(option_value);
						}
						else
						{
							if(arg_num + 1 == argc)
								error_and_exit("missing required value for option '--%s'\n", option_name.c_str());

							opt_value = std::string(argv[arg_num + 1]);
							arg_num += 1;
						}

						ret.options.push_back(Arg {
						    .short_name = opt.flag_or_short_opt,
						    .long_name = opt.long_option,
						    .value = std::move(opt_value),
						});
					}
					else
					{
						if(not option_value.empty())
							error_and_exit("option '--%s' does not accept a value\n", option_name.c_str());

						ret.options.push_back(Arg {
						    .short_name = opt.flag_or_short_opt,
						    .long_name = std::move(option_name),
						    .value = std::nullopt,
						});
					}
				}
				else
				{
					if(not m_ignore_unknown_flags)
						error_and_exit("unrecognised option '--%s'\n", option_name.c_str());
				}
			}
		}

		return ret;
	}

	Parser& Parser::allow_options_after_positionals(bool x)
	{
		m_allow_options_after_positionals = x;
		return *this;
	}

	Parser& Parser::ignore_unknown_flags(bool x)
	{
		m_ignore_unknown_flags = x;
		return *this;
	}

	Parser& Parser::add_flag(char flag, std::string description)
	{
		if(m_flags.contains(flag))
			error_and_exit("flag '-%c' already exists\n", flag);
		else if(m_short_options.contains(flag))
			error_and_exit("'-%c' was already defined as a short option\n", flag);

		Option opt {
			.flag_or_short_opt = flag,
			.long_option = "",
			.needs_value = false,
			.is_flag = true,
			.description = std::move(description),
		};

		m_all_options.push_back(std::move(opt));
		m_flags.insert(flag);

		return *this;
	}

	Parser& Parser::add_option(char short_opt, bool needs_value, std::string description)
	{
		if(m_short_options.contains(short_opt))
			error_and_exit("short option '-%c' already exists\n", short_opt);
		else if(m_flags.contains(short_opt))
			error_and_exit("'-%c' was already defined as a flag\n", short_opt);

		Option opt {
			.flag_or_short_opt = short_opt,
			.long_option = "",
			.needs_value = needs_value,
			.is_flag = false,
			.description = std::move(description),
		};

		const auto* tmp = &m_all_options.emplace_back(std::move(opt));
		m_short_options[short_opt] = tmp;

		return *this;
	}

	Parser& Parser::add_option(std::string long_opt, bool needs_value, std::string description)
	{
		if(long_opt.size() < 2)
			error_and_exit("long option must be at least 2 characters long ('%s')\n", long_opt.c_str());
		else if(m_long_options.contains(long_opt))
			error_and_exit("long option '--%s' already exists\n", long_opt.c_str());

		Option opt {
			.flag_or_short_opt = 0,
			.long_option = long_opt,
			.needs_value = needs_value,
			.is_flag = false,
			.description = std::move(description),
		};

		const auto* tmp = &m_all_options.emplace_back(std::move(opt));
		m_long_options[long_opt] = tmp;

		return *this;
	}

	Parser& Parser::add_option(char short_opt, std::string long_opt, bool needs_value, std::string description)
	{
		if(long_opt.size() < 2)
			error_and_exit("long option must be at least 2 characters long ('%s')\n", long_opt.c_str());
		else if(m_long_options.contains(long_opt))
			error_and_exit("long option '--%s' already exists\n", long_opt.c_str());
		else if(m_short_options.contains(short_opt))
			error_and_exit("short option '-%c' already exists\n", short_opt);
		else if(m_flags.contains(short_opt))
			error_and_exit("'-%c' was already defined as a flag\n", short_opt);

		Option opt {
			.flag_or_short_opt = short_opt,
			.long_option = long_opt,
			.needs_value = needs_value,
			.is_flag = false,
			.description = std::move(description),
		};

		const auto* tmp = &m_all_options.emplace_back(std::move(opt));

		m_short_options.emplace(short_opt, tmp);
		m_long_options.emplace(std::move(long_opt), tmp);

		return *this;
	}
}
#endif
