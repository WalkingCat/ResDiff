#include "stdafx.h"
#include "resutils.h"

using namespace std;

void diff_strings(FILE* out, const wstring& id, const wstring* new_string, const wstring* old_string);
void diff_string_maps(FILE* out, const map<wstring, wstring> & new_map, const map<wstring, wstring> & old_map);

int wmain(int argc, wchar_t* argv[])
{
	wprintf_s(L"\n ResDiff v0.2 https://github.com/WalkingCat/ResDiff\n\n");

	const auto& options_data = parse_cmdl(argc, argv, diff_cmdl::options);
	const auto& params = init_diff_params(options_data);

	if (params.show_help || (!params.error.empty()) || (params.new_files_pattern.empty() && params.old_files_pattern.empty())) {
		if (!params.error.empty()) {
			printf_s("\t%ls\n\n", params.error.c_str());
		}
		if (params.show_help) print_cmdl_usage(L"resdiff", diff_cmdl::options);
		return 0;
	}

	auto out = params.output_file;
	fwprintf_s(out, L"\n diff legends: +: added, -: removed, *: changed, $: changed (original)\n");

	const map<wstring, wstring> empty_files;
	diff_maps(params.new_file_groups, params.old_file_groups,
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
											auto cleanup = [](map<wstring, wstring>&& messages) {
												for (auto& pair : messages) {
													auto& message = pair.second;
													auto pos = message.find(L"\r\n", 0);
													if ((pos != wstring::npos) && (pos == message.size() - 2)) {
														// remove trailing line-ending if its the only one
														message.erase(pos, 2);
													}
												}
												return move(messages);
											};
											const auto& new_messages = cleanup(parse_message_table(new_data));
											const auto& old_messages = cleanup(parse_message_table(old_data));
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
														diff_strings(out, wstring(), new_message, old_message);
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
	if (start_pos < str.size()) ret.emplace_back(str.cbegin() + start_pos, str.cend());
	return ret;
}

void diff_strings(FILE* out, const wstring& id, const wstring* new_string, const wstring* old_string) {
	if (new_string != nullptr) {
		if (old_string != nullptr) {
			if ((*new_string) != (*old_string)) {
				if (is_multiline(*new_string) || (is_multiline(*old_string))) {
					fwprintf_s(out, L"       * %ls\n", id.c_str());
					diff_sequences(get_lines(*new_string), get_lines(*old_string),
						[&](const wstring* new_line, const wstring* old_line) {
							if (new_line && old_line) {
								if (wcscmp(new_line->c_str(), old_line->c_str()) == 0) {
									fwprintf_s(out, L"           %ls\n", new_line->c_str());
								} else {
									fwprintf_s(out, L"         * %ls\n", new_line->c_str());
									fwprintf_s(out, L"         $ %ls\n", old_line->c_str());
								}
							} else if (new_line) {
								fwprintf_s(out, L"         + %ls\n", new_line->c_str());
							} else if (old_line) {
								fwprintf_s(out, L"         - %ls\n", old_line->c_str());
							}
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
}

void diff_string_maps(FILE* out, const map<wstring, wstring> & new_map, const map<wstring, wstring> & old_map) {
	diff_maps(new_map, old_map, [&](const wstring& id, const wstring* new_string, const wstring* old_string) {
		diff_strings(out, id, new_string, old_string);
		});
}
