#pragma once
#include "cmdl_utils.h"

struct diff_cmdl {
	const static cmdl_option show_help;
	const static cmdl_option new_files;
	const static cmdl_option old_files;
	const static cmdl_option recursive;
	const static cmdl_option wcs_folder;
	const static cmdl_option out_file;
	const static cmdl_option * options[6];
};

struct diff_params {
	bool show_help;
	std::wstring error;
	std::wstring new_files_pattern;
	std::wstring old_files_pattern;
	std::wstring output_file_name;
	bool is_wcs;
	bool is_rec;
};

diff_params init_diff_params(const options_data_t& options_data);
