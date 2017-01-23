#include "stdafx.h"
#include "resutils.h"
#include <codecvt>

using namespace std;

resource_data load_resource(const wstring& file) {
	resource_data ret;
	auto module = LoadLibraryExW(file.c_str(), NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE);
	if (module != NULL) {
		EnumResourceTypesW(module, [](HMODULE hModule, LPTSTR lpszType, LONG_PTR lParam) -> BOOL {
			auto& rsrc = *(resource_data*)lParam;
			bool ignore = false;
			if (IS_INTRESOURCE(lpszType)) {
				switch ((int)lpszType) {
				case (int)RT_VERSION:		ignore = true; break;
				case (int)RT_MANIFEST:		ignore = true; break;
				}
			} else {
				if (wcscmp(lpszType, L"MUI") == 0) ignore = true;
			}
			if (!ignore) {
				EnumResourceNamesW(hModule, lpszType, [](HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR lParam) -> BOOL {
					wstring type_name, name;

					if (IS_INTRESOURCE(lpszType)) {
						switch ((int)lpszType) {
						case (int)RT_CURSOR:		type_name = L"CURSOR"; break;
						case (int)RT_BITMAP:		type_name = L"BITMAP"; break;
						case (int)RT_ICON:			type_name = L"ICON"; break;
						case (int)RT_MENU:			type_name = L"MENU"; break;
						case (int)RT_DIALOG:		type_name = L"DIALOG"; break;
						case (int)RT_STRING:		type_name = L"STRING"; break;
						case (int)RT_FONTDIR:		type_name = L"FONTDIR"; break;
						case (int)RT_FONT:			type_name = L"FONT"; break;
						case (int)RT_ACCELERATOR:	type_name = L"ACCELERATOR"; break;
						case (int)RT_RCDATA:		type_name = L"RCDATA"; break;
						case (int)RT_MESSAGETABLE:	type_name = L"MESSAGETABLE"; break;
						case (int)RT_GROUP_CURSOR:	type_name = L"GROUP_CURSOR"; break;
						case (int)RT_GROUP_ICON:	type_name = L"GROUP_ICON"; break;
						case (int)RT_HTML:			type_name = L"HTML"; break;
						default: {
								wchar_t buf[16] = {};
								_itow_s((int)lpszType, buf, 10);
								type_name = buf;
								break;
							}
						}
					} else {
						type_name = wstring(L"\"") + lpszType + L"\"";
					}

					if (IS_INTRESOURCE(lpszName)) {
						wchar_t buf[16] = {};
						_itow_s((int)lpszName, buf, 10);
						name = buf;
					} else name = wstring(L"\"") + lpszName + L"\"";
					auto hrsrc = FindResourceW(hModule, lpszName, lpszType);
					if (hrsrc != NULL) {
						auto res = LockResource(LoadResource(hModule, hrsrc));
						auto size = SizeofResource(hModule, hrsrc);
						auto& rsrc = *(resource_data*)lParam;
						auto& data = rsrc[type_name][name];
						data.resize(size);
						memcpy_s(data.data(), data.size(), res, size);
					}
					return TRUE;
				}, (LONG_PTR)&rsrc);
			}
			return TRUE;
		}, (LONG_PTR)&ret);
		FreeLibrary(module);
	}
	return ret;
}

std::map<std::wstring, std::wstring> parse_strings(const std::wstring & name, const vector<unsigned char>* data) {
	const WORD id_hi = (WORD)wcstoul(name.c_str(), nullptr, 10) - 1;

	map<wstring, wstring> ret;
	wchar_t buf[8] = {};
	if (data != nullptr) {
		const size_t size = data->size();
		size_t pos = 0;
		for (WORD n = 0; n < 16; ++n) {
			const WORD id = (id_hi << 4) + n;
			_ultow_s(id, buf, 10);

			if ((pos + sizeof(WORD)) > size) break;
			const WORD len = *(WORD*)(data->data() + pos);
			pos += sizeof(WORD);
			if (len == 0) continue;

			if ((pos + len * sizeof(wchar_t)) > size) break;
			ret.emplace(make_pair(buf, wstring((wchar_t*)(data->data() + pos), len)));
			pos += len * sizeof(wchar_t);
		}
	}
	return ret;
}

std::map<std::wstring, std::wstring> parse_message_table(const std::vector<unsigned char>* data)
{
	map<wstring, wstring> ret;
	wchar_t buf[16] = L"0x";
	if (data != nullptr) {
		auto msg_data = (PMESSAGE_RESOURCE_DATA)data->data();
		for (DWORD i = 0; i < msg_data->NumberOfBlocks; ++i) {
			PMESSAGE_RESOURCE_BLOCK msg_block = msg_data->Blocks + i;
			auto msg_entry = (PMESSAGE_RESOURCE_ENTRY)(data->data() + msg_block->OffsetToEntries);
			for (auto id = msg_block->LowId; id <= msg_block->HighId; ++id) {
				_ultow_s(id, buf + 2, _countof(buf) - 2, 16);
				if (msg_entry->Flags == 1) {
					ret[buf] = wstring((wchar_t*)msg_entry->Text, (msg_entry->Length - (msg_entry->Text - (PBYTE)msg_entry)) / sizeof(wchar_t));
				} else {
					printf_s("TODO: MESSAGE_RESOURCE_ENTRY.Flags = %d, .Length = %d\n", msg_entry->Flags, msg_entry->Length);
					//static std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>> conv;
					//ret[buf] = conv.from_bytes((const char*)&msg_entry->Text[0], (const char*)msg_entry->Text[msg_entry->Length - 1]);
				}
				msg_entry = (PMESSAGE_RESOURCE_ENTRY) ((PBYTE)msg_entry + msg_entry->Length);
			}
		}
	}
	return ret;

}

