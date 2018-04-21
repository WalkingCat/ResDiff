#pragma once
typedef std::map<std::wstring, std::vector<unsigned char>> resource_type_data;
typedef std::map<std::wstring, resource_type_data> resource_data;
resource_data load_resource(const std::wstring& file);
std::map<std::wstring, std::wstring> parse_strings(const std::wstring& name, const std::vector<unsigned char> * data);;
std::vector<std::wstring> parse_message_table(const std::vector<unsigned char> * data);;
