#include "stdafx.h"
#include "resutils.h"
using namespace std;

enum diff_options {
	diffNone = 0x0,
	diffOld = 0x1,
	diffNew = 0x2,
	diffOutput = 0x40000000,
	diffHelp = 0x80000000,
};

const struct { const wchar_t* arg; const wchar_t* arg_alt; const wchar_t* params_desc; const wchar_t* description; const diff_options options; } cmd_options[] = {
	{ L"?",		L"help",			nullptr,		L"show this help",						diffHelp },
	{ L"n",		L"new",				L"<filename>",	L"specify new file(s)",					diffNew },
	{ L"o",		L"old",				L"<filename>",	L"specify old file(s)",					diffOld },
	{ L"O",		L"out",				L"<filename>",	L"output to file",						diffOutput },
};

void print_usage() {
	printf_s("\tUsage: resdiff [options]\n\n");
	for (auto o = std::begin(cmd_options); o != std::end(cmd_options); ++o) {
		if (o->arg != nullptr) printf_s("\t-%S", o->arg); else printf_s("\t");

		int len = 0;
		if (o->arg_alt != nullptr) {
			len = wcslen(o->arg_alt);
			printf_s("\t--%S", o->arg_alt);
		}
		else printf_s("\t");

		if (len < 6) printf_s("\t");

		if (o->params_desc != nullptr) len += printf_s(" %S", o->params_desc);

		if (len < 14) printf_s("\t");

		printf_s("\t: %S\n", o->description);
	}
}

map<wstring, wstring> find_files(const wchar_t* pattern);
template<typename T, typename TFunc> void diff_maps(const T& new_map, const T& old_map, TFunc& func);
void diff_string_maps(FILE* out, const map<wstring, wstring> & new_map, const map<wstring, wstring> & old_map);

void diff_message_table(FILE* out, const wstring& name, const vector<unsigned char> * new_data, const vector<unsigned char> * old_data);

int wmain(int argc, wchar_t* argv[])
{
	int options = diffNone;
	const wchar_t* err_arg = nullptr;
	wstring new_files_pattern, old_files_pattern, output_file;

	printf_s("\n ResDiff v0.1 https://github.com/WalkingCat/ResDiff\n\n");

	for (int i = 1; i < argc; ++i) {
		const wchar_t* arg = argv[i];
		if ((arg[0] == '-') || ((arg[0] == '/'))) {
			diff_options curent_option = diffNone;
			if ((arg[0] == '-') && (arg[1] == '-')) {
				for (auto o = std::begin(cmd_options); o != std::end(cmd_options); ++o) {
					if ((o->arg_alt != nullptr) && (wcscmp(arg + 2, o->arg_alt) == 0)) { curent_option = o->options; }
				}
			}
			else {
				for (auto o = std::begin(cmd_options); o != std::end(cmd_options); ++o) {
					if ((o->arg != nullptr) && (wcscmp(arg + 1, o->arg) == 0)) { curent_option = o->options; }
				}
			}

			bool valid = false;
			if (curent_option != diffNone) {
				valid = true;
				if (curent_option == diffNew) {
					if ((i + 1) < argc) new_files_pattern = argv[++i];
					else valid = false;
				}
				else if (curent_option == diffOld) {
					if ((i + 1) < argc) old_files_pattern = argv[++i];
					else valid = false;
				}
				else if (curent_option == diffOutput) {
					if ((i + 1) < argc) output_file = argv[++i];
					else valid = false;
				} else options = (options | curent_option);
			}
			if (!valid && (err_arg == nullptr)) err_arg = arg;
		}
		else { if (new_files_pattern.empty()) new_files_pattern = arg; else err_arg = arg; }
	}

	if ((new_files_pattern.empty() && old_files_pattern.empty()) || (err_arg != nullptr) || (options & diffHelp)) {
		if (err_arg != nullptr) printf_s("\tError in option: %S\n\n", err_arg);
		print_usage();
		return 0;
	}

	auto out = stdout;
	if (!output_file.empty()) {
		out = nullptr;
		_wfopen_s(&out, output_file.c_str(), L"w, ccs=UTF-8");
	}

	if (out == nullptr) {
		printf_s("can't open %ws for output", output_file.c_str());
		return 0;
	}

	auto new_files = find_files(new_files_pattern.c_str());
	auto old_files = find_files(old_files_pattern.c_str());

	fwprintf_s(out, L" new files: %ws%ws\n", new_files_pattern.c_str(), !new_files.empty() ? L"" : L" (NOT EXISTS!)");
	fwprintf_s(out, L" old files: %ws%ws\n", old_files_pattern.c_str(), !old_files.empty() ? L"" : L" (NOT EXISTS!)");

	fwprintf_s(out, L"\n");

	if (new_files.empty() & old_files.empty()) return 0; // at least one of them must exists

	fwprintf_s(out, L" diff legends: +: added, -: removed, *: changed, $: changed (original)\n");
	fwprintf_s(out, L"\n");

	for (auto& new_pair : new_files) {
		auto&file_name = new_pair.first;
		auto& new_file = new_pair.second;
		auto new_res = load_resource(new_file);

		bool file_is_new = false;
		auto old_file_it = old_files.find(new_pair.first);
		if (old_file_it == old_files.end()) {
			if ((new_files.size() == 1) && (old_files.size() == 1)) {
				old_file_it = old_files.begin(); // allows diff single files with different names
			} else {
				file_is_new = true;
			}
		}
		resource old_res;
		if (old_file_it != old_files.end()) {
			auto& old_file = old_file_it->second;
			old_res = load_resource(old_file);
		}

		bool printed_file_name = false;
		for (auto& new_type_pair : new_res.data) {
			auto& type_name = new_type_pair.first;
			auto& new_type = new_type_pair.second;
			auto& old_type = old_res.data[type_name];
			for (auto& new_data_pair : new_type) {
				auto& name = new_data_pair.first;
				const vector<unsigned char> * new_data = &new_data_pair.second;
				const vector<unsigned char> * old_data = nullptr;
				bool diff = false, res_is_new = false;
				if (old_type.find(name) == old_type.end()) {
					res_is_new = true;
					diff = true;
				} else {
					old_data = &old_type[name];
					if ((new_data->size() != old_data->size()) || (memcmp(new_data->data(), old_data->data(), new_data->size()) != 0)) {
						diff = true;
					}
				}
				if (diff) {
					if (!printed_file_name) {
						fwprintf_s(out, L"\n%ws FILE: %ws\n", file_is_new ? L"+" : L"*", file_name.c_str());
						printed_file_name = true;
					}
					fwprintf_s(out, L" %ws %ws # %ws\n", res_is_new ? L"+" : L"*", type_name.c_str(), name.c_str());

					if (type_name == L"STRING") {
						diff_string_maps(out, parse_strings(name, new_data), parse_strings(name, old_data));
					} else if (type_name == L"MESSAGETABLE") {
						diff_string_maps(out, parse_message_table(new_data), parse_message_table(old_data));
					}
				}
			}
		}

		for (auto& old_type_pair : old_res.data) {
			auto& type_name = old_type_pair.first;
			auto& old_type = old_type_pair.second;
			auto& new_type = new_res.data[type_name];
			for (auto& old_data_pair : old_type) {
				auto& name = old_data_pair.first;
				auto& old_data = old_data_pair.second;
				if (new_type.find(old_data_pair.first) == new_type.end()) {
					fwprintf_s(out, L" - %ws # %ws\n", type_name.c_str(), name.c_str());
				}
			}
		}
	}

	for (auto& old_file_pair : old_files) {
		auto& file_name = old_file_pair.first;
		if (new_files.find(file_name) == new_files.end()) {
			fwprintf_s(out, L"\n- FILE %ws\n", file_name.c_str());
		}
	}

    return 0;
}

map<wstring, wstring> find_files(const wchar_t * pattern)
{
	map<wstring, wstring> ret;
	wchar_t path[MAX_PATH] = {};
	wcscpy_s(path, pattern);
	WIN32_FIND_DATA fd;
	HANDLE find = ::FindFirstFile(pattern, &fd);
	if (find != INVALID_HANDLE_VALUE) {
		do {
			if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
				PathRemoveFileSpec(path);
				PathCombine(path, path, fd.cFileName);
				ret[fd.cFileName] = path;
			}
		} while (::FindNextFile(find, &fd));
		::FindClose(find);
	}
	return ret;
}

void diff_string_maps(FILE* out, const map<wstring, wstring> & new_map, const map<wstring, wstring> & old_map) {
	diff_maps(new_map, old_map, [&](const wstring& id, const wstring* new_string, const wstring* old_string) {
		if (new_string != nullptr) {
			if (old_string != nullptr) {
				if ((*new_string) != (*old_string)) {
					fwprintf_s(out, L"  * %ws %ws\n", id.c_str(), new_string->c_str());
					fwprintf_s(out, L"  $ %ws %ws\n", id.c_str(), old_string->c_str());
				}
			} else {
				fwprintf_s(out, L"  + %ws %ws\n", id.c_str(), new_string->c_str());
			}
		} else if (old_string != nullptr) {
			fwprintf_s(out, L"  - %ws %ws\n", id.c_str(), old_string->c_str());
		}
	});
}

template<typename T, typename TFunc>
void diff_maps(const T & new_map, const T & old_map, TFunc & func)
{
	for (auto& new_pair : new_map) {
		auto& old_it = old_map.find(new_pair.first);
		func(new_pair.first, &new_pair.second, old_it == old_map.end() ? nullptr : &old_it->second);
	}
	for (auto& old_pair : old_map) {
		if (new_map.find(old_pair.first) == new_map.end()) {
			func(old_pair.first, nullptr, &old_pair.second);
		}
	}
}
