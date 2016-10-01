#pragma once

#include "stdafx.h"
#include "Log.h"
#include "macros.h"
#include "Util.h"

#include "Shobjidl.h"

namespace EngineCore
{
	class winDialogFilter
	{
	private:
		struct filterItem
		{
			wstring name;
			wstring val;
		};

	public:
		winDialogFilter(){;}
		~winDialogFilter()
		{
			filter.clear();
		}

		void Add(wstring name, wstring val)
		{
			filterItem f;
			f.name = name;
			f.val = val;
			filter.push_back(f);
		}
		void _Add(string name, string val)
		{
			filterItem f;
			f.name = StringToWstring(name);
			f.val = StringToWstring(val);
			filter.push_back(f);
		}

		COMDLG_FILTERSPEC* BuildFILTERSPEC(UINT* num)
		{
			if(filter.size() == 0)return nullptr;
			COMDLG_FILTERSPEC* res = new COMDLG_FILTERSPEC[filter.size()];
			for(int i=0; i<int(filter.size()); i++)
			{
				res[i].pszName = filter[i].name.c_str();
				res[i].pszSpec = filter[i].val.c_str();
			}
			*num = int(filter.size());
			return res;
		}

	private:
		vector<filterItem> filter;
	};

	static wstring winDialogOpenFile(HWND hwnd, wstring title, winDialogFilter filter, bool folder)
	{
		wstring res = L"";

		if(FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))){
			CoUninitialize(); return res;}
		
		IFileOpenDialog* dialog;
		if(FAILED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&dialog)))){
			CoUninitialize(); return res;}
		
		if(folder)
		{
			dialog->SetOptions(FOS_PICKFOLDERS);
		}
		else
		{
			UINT f_num;
			COMDLG_FILTERSPEC* rgSpec = filter.BuildFILTERSPEC(&f_num);
			if(!rgSpec){
				CoUninitialize();
				_RELEASE(dialog);
				return res;}

			dialog->SetFileTypes(f_num, rgSpec);
			_DELETE_ARRAY(rgSpec);
		}

		dialog->SetTitle(title.c_str());
		
		IShellItem *item;
		if(SUCCEEDED(dialog->Show(hwnd)) && 
			SUCCEEDED(dialog->GetResult(&item)))
		{
			PWSTR file;
			if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &file)))
			{
				res = file;
				CoTaskMemFree(file);
			}
			_RELEASE(item);
		}

		_RELEASE(dialog);
		CoUninitialize();

		return res;
	}

	static string _winDialogOpenFile(luaHWND hwnd, string title, winDialogFilter filter)
	{
		return WstringToString(winDialogOpenFile(hwnd.hwnd, StringToWstring(title), filter, false));
	}

	static string _winDialogOpenFolder(luaHWND hwnd, string title)
	{
		return WstringToString(winDialogOpenFile(hwnd.hwnd, StringToWstring(title), winDialogFilter(), true));
	}

	static vector<wstring> winDialogOpenFilesMultiple(HWND hwnd, wstring title, winDialogFilter filter)
	{
		vector<wstring> res;

		if(FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))) {
			CoUninitialize(); return res; }
		
		IFileOpenDialog* dialog;
		if(FAILED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&dialog)))) {
			CoUninitialize(); return res; }
		
		UINT f_num;
		COMDLG_FILTERSPEC* rgSpec = filter.BuildFILTERSPEC(&f_num);
		if(!rgSpec) {
			_RELEASE(dialog);
			CoUninitialize();
			return res; }
		
		dialog->SetFileTypes(f_num, rgSpec);
		_DELETE_ARRAY(rgSpec);

		dialog->SetTitle(title.c_str());
		dialog->SetOptions(FOS_ALLOWMULTISELECT);

		IShellItemArray *items;
		if(FAILED(dialog->Show(hwnd)) 
			|| FAILED(dialog->GetResults(&items))) {
			_RELEASE(dialog);
			CoUninitialize();
			return res; }

		DWORD count;
		if(SUCCEEDED(items->GetCount(&count)))
		{
			for(DWORD i=0; i<count; i++)
			{
				IShellItem *item;
				if(SUCCEEDED(items->GetItemAt(i, &item)))
				{
					PWSTR file;
					if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &file)))
					{
						res.push_back(wstring(file));
						CoTaskMemFree(file);
					}
					_RELEASE(item);
				}
			}
		}

		_RELEASE(items);
		_RELEASE(dialog);
		CoUninitialize();

		return res;
	}

	static luaStringList _winDialogOpenFilesMultiple(luaHWND hwnd, string title, winDialogFilter filter)
	{
		luaStringList res;
		auto strs = winDialogOpenFilesMultiple(hwnd.hwnd, StringToWstring(title), filter);

		for(int i=0; i<(int)strs.size(); i++)
			res.Add(WstringToString(strs[i]));

		return res;
	}

	static wstring winDialogSaveFile(HWND hwnd, wstring title, winDialogFilter filter)
	{
		wstring res = L"";

		if(FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))){
			CoUninitialize(); return res;}
		
		IFileSaveDialog* dialog;
		if(FAILED(CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void**>(&dialog)))){
			CoUninitialize(); return res;}
				
		UINT f_num;
		COMDLG_FILTERSPEC* rgSpec = filter.BuildFILTERSPEC(&f_num);
		if(!rgSpec) {
			_RELEASE(dialog);
			CoUninitialize();
			return res; }
		
		dialog->SetFileTypes(f_num, rgSpec);
		_DELETE_ARRAY(rgSpec);

		dialog->SetTitle(title.c_str());
		dialog->SetDefaultExtension(L"");
		
		IShellItem *item;
		if(SUCCEEDED(dialog->Show(hwnd)) && 
			SUCCEEDED(dialog->GetResult(&item)))
		{
			PWSTR file;
			if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &file)))
			{
				res = file;
				CoTaskMemFree(file);
			}
			_RELEASE(item);
		}

		_RELEASE(dialog);
		CoUninitialize();

		return res;
	}

	static string _winDialogSaveFile(luaHWND hwnd, string title, winDialogFilter filter)
	{
		return WstringToString(winDialogSaveFile(hwnd.hwnd, StringToWstring(title), filter));
	}
}