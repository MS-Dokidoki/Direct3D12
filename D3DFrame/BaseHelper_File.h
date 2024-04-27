#pragma once
#include "Base.h"

typedef BASE_HANDLE FILE_HANDLE;
typedef LPCWSTR PATH;

namespace BaseHelper
{
	namespace File
	{
		enum FileMethods
		{
			// 始终创建新文件。若指定文件存在且可读, 则函数覆盖文件, 函数成功。
			FILE_METHOD_CREATE_ALWAYS = CREATE_ALWAYS,	
			// 仅当文件不存在时创建一个新的文件。若指定的文件存在, 函数失败。
			FILE_METHOD_CREATE_NEW = CREATE_NEW,
			// 始终打开文件。若指定的文件存在, 则函数成功; 若指定的文件不存在, 且指定路径可可读, 则创建一个新的文件
			FILE_METHOD_OPEN_ALWAYS = OPEN_ALWAYS,
			// 仅当文件存在时打开文件
			FILE_METHOD_OPEN_EXISTING = OPEN_EXISTING,
			// 打开一个文件并截断文件, 使其大小变为 0 字节。若指定的文件不存在, 则函数失败。
			FILE_METHOD_TRUNCATE_EXISTING = TRUNCATE_EXISTING
		};
		
		class Path
		{
			
		};
		
		FILE_HANDLE OpenFile(PATH fileName, FileMethods method = FILE_METHOD_OPEN_EXISTING);
		FILE_HANDLE OpenFile(Path fileName, FileMethods method = FILE_METHOD_OPEN_EXISTING);
		
		bool ReadToBuffer(FILE_HANDLE hFile, void* pReadBuffer, DWORD dwByteToRead, DWORD* dwReadByteSize);
		bool Read(FILE_HANDLE hFile, void** pReadBuffer, DWORD* dwReadByteSize);
		
		bool Write(FILE_HANDLE hFile, void* pWriitenBuffer, DWORD dwByteToWrite, DWORD* dwWrittenByteSize);
		
		UINT AsyncReadToBuffer(FILE_HANDLE hFile, void* pReadBuffer, DWORD dwByteToRead, DWORD* dwReadByteSize);
		UINT AsyncReadToBuffer(PATH fileName, void* pReadBuffer, DWORD dwByteToRead, DWORD* dwReadByteSize);
		UINT AsyncRead(FILE_HANDLE hFile, void** pReadBuffer, DWORD* dwReadByteSize);
		UINT AsyncRead(PATH fileName, void** pReadBuffer, DWORD* dwReadByteSize);
	};
};