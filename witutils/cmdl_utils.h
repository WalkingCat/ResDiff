#pragma once
#include <unordered_map>
#include <string>

struct cmdl_option {
	const wchar_t* arg;
	const wchar_t* arg_alt;
	const wchar_t* data_desc;
	const wchar_t* desc;
};

std::unordered_map<const cmdl_option*, std::wstring> parse_cmdl(int argc, wchar_t* argv[], const cmdl_option* options[], size_t count);

using options_data_t = std::unordered_map<const cmdl_option*, std::wstring>;

template<size_t count>
options_data_t parse_cmdl(int argc, wchar_t* argv[], const cmdl_option* (&options)[count])
{
	return parse_cmdl(argc, argv, options, count);
}

void print_cmdl_usage(const wchar_t* app, const cmdl_option* options[], size_t count);

template<size_t count>
void print_cmdl_usage(const wchar_t* app, const cmdl_option* (&options)[count]) {
	print_cmdl_usage(app, options, count);
}
