#include "stdafx.h"
#include "diff_commons.h"

const cmdl_option diff_cmdl::show_help { L"?",		L"help",		nullptr,		L"show this help" };
const cmdl_option diff_cmdl::new_files { L"n",		L"new",			L"<filename>",	L"specify new file(s)" };
const cmdl_option diff_cmdl::old_files { L"o",		L"old",			L"<filename>",	L"specify old file(s)" };
const cmdl_option diff_cmdl::recursive { L"r",		L"recursive",	nullptr,		L"search folder recursively" };
const cmdl_option diff_cmdl::wcs_folder { nullptr,	L"wcs",			nullptr,		L"folder is Windows Component Store" };
const cmdl_option diff_cmdl::out_file { L"O",		L"out",			L"<filename>",	L"output to file" };
const cmdl_option * diff_cmdl::options[6] = { &show_help, &new_files, &old_files, &recursive, &wcs_folder, &out_file };

diff_params init_diff_params(const options_data_t& options_data)
{
	diff_params ret = {};

	ret.show_help = options_data.find(&diff_cmdl::show_help) != options_data.end();
	auto opt_it = options_data.find(nullptr);
	if (opt_it != options_data.end()) {
		ret.error = opt_it->second;
	}
	opt_it = options_data.find(&diff_cmdl::new_files);
	if (opt_it != options_data.end()) {
		ret.new_files_pattern = opt_it->second;
	}
	opt_it = options_data.find(&diff_cmdl::old_files);
	if (opt_it != options_data.end()) {
		ret.old_files_pattern = opt_it->second;
	}
	opt_it = options_data.find(&diff_cmdl::out_file);
	if (opt_it != options_data.end()) {
		ret.output_file_name = opt_it->second;
	}
	ret.is_wcs = options_data.find(&diff_cmdl::wcs_folder) != options_data.end();
	ret.is_rec = options_data.find(&diff_cmdl::recursive) != options_data.end();

	return ret;
}
