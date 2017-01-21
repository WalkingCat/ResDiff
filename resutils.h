#pragma once
struct resource { std::map<std::wstring, std::map<std::wstring, std::vector<unsigned char>>> data; };
resource load_resource(const std::wstring& file);
std::map<std::wstring, std::wstring> parse_strings(const std::wstring& name, const std::vector<unsigned char> * data);;
std::map<std::wstring, std::wstring> parse_message_table(const std::vector<unsigned char> * data);;
