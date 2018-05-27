#include "stdafx.h"
#include "resutils.h"
using namespace std;

enum diff_options {
	diffNone = 0x0,
	diffOld = 0x1,
	diffNew = 0x2,
	diffWcs = 0x8,
	diffOutput = 0x40000000,
	diffHelp = 0x80000000,
};

const struct { const wchar_t* arg; const wchar_t* arg_alt; const wchar_t* params_desc; const wchar_t* description; const diff_options options; } cmd_options[] = {
	{ L"?",		L"help",			nullptr,		L"show this help",						diffHelp },
	{ L"n",		L"new",				L"<filename>",	L"specify new file(s)",					diffNew },
	{ L"o",		L"old",				L"<filename>",	L"specify old file(s)",					diffOld },
	{ nullptr,	L"wcs",				nullptr,		L"folder is Windows Component Store",	diffWcs },
	{ L"O",		L"out",				L"<filename>",	L"output to file",						diffOutput },
};

void print_usage(FILE* out) {
	fwprintf_s(out, L" Usage: resdiff [options]\n\n");
	for (auto o = begin(cmd_options); o != end(cmd_options); ++o) {
		if (o->arg != nullptr) fwprintf_s(out, L" -%ls", o->arg); else fwprintf_s(out, L" ");

		int len = 0;
		if (o->arg_alt != nullptr) {
			len = wcslen(o->arg_alt);
			fwprintf_s(out, L"\t--%ls", o->arg_alt);
		} else fwprintf_s(out, L"\t");

		if (len < 6) fwprintf_s(out, L"\t");

		if (o->params_desc != nullptr) len += fwprintf_s(out, L" %ls", o->params_desc);

		if (len < 14) fwprintf_s(out, L"\t");

		fwprintf_s(out, L"\t: %ls\n", o->description);
	}
	fwprintf_s(out, L"\n");
}

void diff_string_maps(FILE* out, const map<wstring, wstring> & new_map, const map<wstring, wstring> & old_map);

int wmain(int argc, wchar_t* argv[])
{
	auto out = stdout;

	int options = diffNone;
	const wchar_t* err_arg = nullptr;
	wstring new_files_pattern, old_files_pattern, output_file;

	fwprintf_s(out, L"\n ResDiff v0.2 https://github.com/WalkingCat/ResDiff\n\n");

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
		print_usage(out);
		return 0;
	}

	if (!output_file.empty()) {
		out = nullptr;
		_wfopen_s(&out, output_file.c_str(), L"w, ccs=UTF-8");
	}

	if (out == nullptr) {
		wprintf_s(L"can't open %ls for output\n", output_file.c_str());
		return 0;
	}

	auto search_files = [&](bool is_new) -> map<wstring, map<wstring, wstring>> {
		map<wstring, map<wstring, wstring>> ret;
		const auto& files_pattern = is_new ? new_files_pattern : old_files_pattern;
		fwprintf_s(out, L" %ls files: %ls", is_new ? L"new" : L"old", files_pattern.c_str());
		if (((options & diffWcs) == diffWcs)) {
			ret = find_files_wcs_ex(files_pattern);
		} else {
			auto files = find_files(files_pattern.c_str());
			if (!files.empty()) ret[wstring()].swap(files);
		}
		fwprintf_s(out, L"%ls\n", !ret.empty() ? L"" : L" (EMPTY!)");
		return ret;
	};

	map<wstring, map<wstring, wstring>> new_file_groups = search_files(true), old_file_groups = search_files(false);
	fwprintf_s(out, L"\n");
	if (new_file_groups.empty() && old_file_groups.empty()) return 0;

	if (((options & diffWcs) == 0)) {
		auto& new_files = new_file_groups[wstring()], &old_files = old_file_groups[wstring()];
		if ((new_files.size() == 1) && (old_files.size() == 1)) {
			// allows diff single files with different names
			auto& new_file_name = new_files.begin()->first;
			auto& old_file_name = old_files.begin()->first;
			if (new_file_name != old_file_name) {
				auto diff_file_names = new_file_name + L" <=> " + old_file_name;
				auto new_file = new_files.begin()->second;
				new_files.clear();
				new_files[diff_file_names] = new_file;
				auto old_file = old_files.begin()->second;
				old_files.clear();
				old_files[diff_file_names] = old_file;
			}
		}
	}
	
	fwprintf_s(out, L" diff legends: +: added, -: removed, *: changed, $: changed (original)\n");

	const map<wstring, wstring> empty_files;
	diff_maps(new_file_groups, old_file_groups,
		[&](const wstring& group_name, const map<wstring, wstring>* new_files, const map<wstring, wstring>* old_files) {
			bool printed_group_name = false;
			wchar_t printed_group_prefix = L' ';
			auto print_group_name = [&](const wchar_t prefix) {
				if (!printed_group_name) {
					fwprintf_s(out, L"\n %lc %ls (\n", prefix, group_name.c_str());
					printed_group_name = true;
					printed_group_prefix = prefix;
				}
			};
			
			bool printed_previous_file_name = false;
			diff_maps(new_files ? *new_files : empty_files, old_files ? *old_files : empty_files,
				[&](const wstring& file_name, const wstring * new_file, const wstring * old_file) {
					bool printed_file_name = false;
					auto print_file_name = [&](const wchar_t prefix) {
						if (!printed_file_name) {
							print_group_name(new_files ? old_files ? L'*' : L'+' : L'-');
							if (printed_previous_file_name) {
								fwprintf_s(out, L"\n");
							}
							fwprintf_s(out, L"   %lc %ls\n", prefix, file_name.c_str());
							printed_previous_file_name = printed_file_name = true;
						}
					};

					if (new_file == nullptr) {
						print_file_name('-');
						return;
					}

					if (old_file == nullptr) {
						print_file_name('+');
					}

					diff_maps(load_resource(*new_file), (old_file != nullptr) ? load_resource(*old_file) : resource_data(),
						[&](const wstring& type_name, const resource_type_data * new_type, const resource_type_data * old_type) {
							diff_maps(new_type ? *new_type : resource_type_data(), old_type ? *old_type : resource_type_data(),
								[&](const wstring& name, const std::vector<unsigned char> * new_data, const std::vector<unsigned char> * old_data) {
									//if (new_data == nullptr) {
									//	print_file_name('*');
									//	fwprintf_s(out, L"     - %ls # %ls\n", type_name.c_str(), name.c_str());
									//	return;
									//}
									const bool diff = (new_data == nullptr) || (old_data == nullptr) ||
										((new_data->size() != old_data->size()) || (memcmp(new_data->data(), old_data->data(), new_data->size())));
									if (diff) {
										print_file_name('*');
										fwprintf_s(out, L"     %lc %ls # %ls\n", new_data ? old_data ? L'*' : L'+' : L'-', type_name.c_str(), name.c_str());

										if (type_name == L"STRING") {
											diff_string_maps(out, parse_strings(name, new_data), parse_strings(name, old_data));
										} else if (type_name == L"MESSAGETABLE") {
											const auto& new_messages = parse_message_table(new_data);
											const auto& old_messages = parse_message_table(old_data);
											auto get_keys = [](const map<wstring, wstring>& m) {
												set<wstring> ret;
												for (auto& pair : m) { ret.insert(pair.first); }
												return ret;
											};
											auto get_values = [](const map<wstring, wstring>& m) {
												vector<wstring> ret;
												for (auto& pair : m) { ret.push_back(pair.second); }
												return ret;
											};
											if (get_keys(new_messages) == get_keys(old_messages)) {
												diff_string_maps(out, new_messages, old_messages);
											} else {
												diff_sequences(get_values(new_messages), get_values(old_messages),
													[&](const wstring* new_message, const wstring* old_message) {
														if (old_message == nullptr) {
															if (new_message != nullptr) {
																fwprintf_s(out, L"       + %ls", new_message->c_str());
															}
														} else {
															if (new_message == nullptr) {
																fwprintf_s(out, L"       - %ls", old_message->c_str());
															}
														}
													}
												);
											}
										}
									}
								}
							);
						}
					);
				}
			);

			if (printed_group_name)
				fwprintf_s(out, L" %lc )\n", printed_group_prefix);
		}
	);

	fwprintf_s(out, L"\n");

    return 0;
}

const size_t find_linebreak(const wstring& str, size_t* pos = nullptr) {
	auto ret = str.find_first_of(L"\r\n", pos ? *pos : 0);
	if (pos) {
		*pos = ret + 1;
		if ((ret != wstring::npos) && (str[ret] == L'\r') && ((ret + 1) < str.size()) && (str[ret + 1] == L'\n')) {
			*pos += 1; // treat \r\n as a whole
		}
	}
	return ret;
}

bool is_multiline(const wstring& str) { return find_linebreak(str) != wstring::npos; }

vector<wstring> get_lines(const wstring& str) {
	vector<wstring> ret;
	size_t start_pos = 0, next_start_pos = start_pos;
	for (auto pos = find_linebreak(str, &next_start_pos); pos != wstring::npos; pos = find_linebreak(str, &next_start_pos)) {
		ret.emplace_back(str.cbegin() + start_pos, str.cbegin() + pos);
		start_pos = next_start_pos;
	}
	if(start_pos < str.size()) ret.emplace_back(str.cbegin() + start_pos, str.cend());
	return ret;
}

void diff_string_maps(FILE* out, const map<wstring, wstring> & new_map, const map<wstring, wstring> & old_map) {
	diff_maps(new_map, old_map, [&](const wstring& id, const wstring* new_string, const wstring* old_string) {
		if (new_string != nullptr) {
			if (old_string != nullptr) {
				if ((*new_string) != (*old_string)) {
					if (is_multiline(*new_string) || (is_multiline(*old_string))) {
						fwprintf_s(out, L"       * %ls\n", id.c_str());
						diff_sequences(get_lines(*new_string), get_lines(*old_string),
							[&](const wstring* new_line, const wstring* old_line) {
								fwprintf_s(out, L"         %lc %ls\n", new_line ? old_line ? L' ' : L'+' : L'-',
									new_line ? new_line->c_str() : old_line->c_str());
							}
						);
					} else {
						fwprintf_s(out, L"       * %ls %ls\n", id.c_str(), new_string->c_str());
						fwprintf_s(out, L"       $ %ls %ls\n", id.c_str(), old_string->c_str());
					}
				}
			} else {
				if (is_multiline(*new_string)) {
					fwprintf_s(out, L"       + %ls\n", id.c_str());
					for (const auto& line : get_lines(*new_string)) {
						fwprintf_s(out, L"         + %ls\n", line.c_str());
					}
				} else {
					fwprintf_s(out, L"       + %ls %ls\n", id.c_str(), new_string->c_str());
				}
			}
		} else if (old_string != nullptr) {
			if (is_multiline(*old_string)) {
				fwprintf_s(out, L"       - %ls\n", id.c_str());
				for (const auto& line : get_lines(*old_string)) {
					fwprintf_s(out, L"         - %ls\n", line.c_str());
				}

			} else {
				fwprintf_s(out, L"       - %ls %ls\n", id.c_str(), old_string->c_str());
			}
		}
	});
}
