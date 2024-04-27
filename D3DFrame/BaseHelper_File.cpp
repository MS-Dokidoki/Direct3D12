#include "BaseHelper_File.h"
#include "BaseHelper_Thread.h"

using namespace BaseHelper;

struct AnsycReadInfo
{
	FILE_HANDLE hFile;
	void* pReadBuffer;
	DWORD dwByteToRead;
	DWORD* dwReadByteSize;
};

struct AnsycReadInfo1
{
	PATH path;
	void* pReadBuffer;
	DWORD dwByteToRead;
	DWORD* dwReadByteSize;
};

static void CALLBACK ansycReadCallback(Thread::ThreadPool* pool, void* param)
{
	AnsycReadInfo* info = (AnsycReadInfo*)param;

	if(info->dwByteToRead != -1)
		File::ReadToBuffer(info->hFile, info->pReadBuffer, info->dwByteToRead, info->dwReadByteSize);
	else
		File::Read(info->hFile, (void**)info->pReadBuffer, info->dwReadByteSize);
	
	BASE_MFREE(info);
	info = NULL;
}

static void CALLBACK ansycReadCallback1(Thread::ThreadPool* pool, void* param)
{
	AnsycReadInfo1* info = (AnsycReadInfo1*)param;
	FILE_HANDLE hFile = File::OpenFile(info->path);
	if(hFile)
	{
		if(info->dwByteToRead != -1)
			File::ReadToBuffer(hFile, info->pReadBuffer, info->dwByteToRead, info->dwReadByteSize);
		else
			File::Read(hFile, (void**)info->pReadBuffer, info->dwReadByteSize);
	}
}

FILE_HANDLE File::OpenFile(PATH fileName, File::FileMethods method)
{
	FILE_HANDLE hFile = CreateFileW(fileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, method, FILE_ATTRIBUTE_NORMAL, NULL);
	return hFile == INVALID_HANDLE_VALUE? NULL: hFile;
}

FILE_HANDLE File::OpenFile(File::Path fileName, FileMethods method)
{
	return NULL;
}

bool File::ReadToBuffer(FILE_HANDLE hFile, void* pReadBuffer, DWORD dwByteToRead, DWORD* dwReadByteSize)
{
	return ReadFile(hFile, pReadBuffer, dwByteToRead, dwReadByteSize, NULL);
}

bool File::Read(FILE_HANDLE hFile, void** pReadBuffer, DWORD* dwReadByteSize)
{
	if(hFile)
	{
		UINT bytesize = GetFileSize(hFile, 0);
		*pReadBuffer = BASE_MALLOC(bytesize);
		if(*pReadBuffer)
			return ReadFile(hFile, *pReadBuffer, bytesize, dwReadByteSize, NULL);
		else
			// Output log
			return 0;
	}
	
	return 0;
}

bool File::Write(FILE_HANDLE hFile, void* pWriitenBuffer, DWORD dwByteToWrite, DWORD* dwWrittenByteSize)
{
	return WriteFile(hFile, pWriitenBuffer,  dwByteToWrite, dwWrittenByteSize, NULL);
}

UINT File::AsyncReadToBuffer(FILE_HANDLE hFile, void* pReadBuffer, DWORD dwByteToRead, DWORD* dwReadByteSize)
{
	if(hFile)
	{
		AnsycReadInfo* info = (AnsycReadInfo*)BASE_MALLOC(sizeof(AnsycReadInfo));
		if(info)
		{
			info->hFile = hFile;
			info->pReadBuffer = pReadBuffer;
			info->dwByteToRead = dwByteToRead;
			info->dwReadByteSize = dwReadByteSize;
			
			Thread::ThreadPool* pool = Thread::ThreadPool::GetInstance();
			return pool->CommitThreadTask(Thread::ThreadTask(ansycReadCallback, info));
		}
		// Output log
		return NULL;
	}
	// Output log
	return NULL;
}


UINT File::AsyncReadToBuffer(PATH path, void* pReadBuffer, DWORD dwByteToRead, DWORD* dwReadByteSize)
{
	if(path)
	{
		AnsycReadInfo1* info = (AnsycReadInfo1*)BASE_MALLOC(sizeof(AnsycReadInfo1));
		if(info)
		{
			info->path = path;
			info->pReadBuffer = pReadBuffer;
			info->dwByteToRead = dwByteToRead;
			info->dwReadByteSize = dwReadByteSize;
			
			Thread::ThreadPool* pool = Thread::ThreadPool::GetInstance();
			return pool->CommitThreadTask(Thread::ThreadTask(ansycReadCallback1, info));
		}
		// Output log
		return NULL;
	}
	// Output log
	return NULL;
}

UINT File::AsyncRead(FILE_HANDLE hFile, void** pReadBuffer, DWORD* dwReadByteSize)
{
	if(hFile)
	{
		AnsycReadInfo* info = (AnsycReadInfo*)BASE_MALLOC(sizeof(AnsycReadInfo));
		if(info)
		{
			info->hFile = hFile;
			info->pReadBuffer = pReadBuffer;
			info->dwByteToRead = -1;
			info->dwReadByteSize = dwReadByteSize;
			
			Thread::ThreadPool* pool = Thread::ThreadPool::GetInstance();
			return pool->CommitThreadTask(Thread::ThreadTask(ansycReadCallback, info));
		}
		// Output log
		return NULL;
	}
	// Output log
	return NULL;
}

UINT File::AsyncRead(PATH path, void** pReadBuffer, DWORD* dwReadByteSize)
{
	if(path)
	{
		AnsycReadInfo1* info = (AnsycReadInfo1*)BASE_MALLOC(sizeof(AnsycReadInfo1));
		if(info)
		{
			info->path = path;
			info->pReadBuffer = pReadBuffer;
			info->dwByteToRead = -1;
			info->dwReadByteSize = dwReadByteSize;
			
			Thread::ThreadPool* pool = Thread::ThreadPool::GetInstance();
			return pool->CommitThreadTask(Thread::ThreadTask(ansycReadCallback1, info));
		}
		// Output log
		return NULL;
	}
	// Output log
	return NULL;
}