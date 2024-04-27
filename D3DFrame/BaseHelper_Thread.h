#pragma once
#include "Base.h"

typedef BASE_HANDLE THREAD_HANDLE;
typedef BASE_HANDLE THREAD_EVENT;
typedef CRITICAL_SECTION THREAD_MUTEX;
typedef LPCRITICAL_SECTION THREAD_LPMUTEX;

namespace BaseHelper
{
	namespace Thread
	{
		struct ThreadTask;
		class ThreadPool;
		
		typedef void (CALLBACK *THREAD_CALLBACK)(ThreadPool* pool, void* param);

		struct ThreadTask
		{
			THREAD_CALLBACK Callback;
			void* Param;
			UINT SignalObject;
			ThreadTask(THREAD_CALLBACK callback, void* param): Callback(callback), Param(param)
			{}
		};

		class ThreadPool
		{   
			static const UINT THREAD_DEF_COUNT = 2;
			static const UINT THREAD_DEF_SIGNALOBJECT_COUNT = 65536;
			typedef struct Thread
			{
				THREAD_HANDLE hThread;
				ThreadPool* pool;
				DWORD threadId;
				bool bDestory;
			}THREAD;

			static DWORD WINAPI ThreadProc(void* param);

		public:
			ThreadPool(const ThreadPool&) = delete;
			ThreadPool& operator=(const ThreadPool& ) = delete;
			
			// �̳߳ص���
			static ThreadPool* GetInstance(UINT threadCount = THREAD_DEF_COUNT);

			ThreadPool(UINT threadCount);
			~ThreadPool();
			
			// ���̳߳��ύ����
			UINT CommitThreadTask(ThreadTask task);

			// ��ȡ�̳߳ص���(��Ҫ�� ExitPoolSection ����ʹ��); �����������߳�ʹ��, ���߳�ֹͣ����, �ȴ���ȡ�߳���
			void EnterPoolSection();
			// �ر��̳߳ص���(��Ҫ�� EnterPoolSection ����ʹ��)
			void ExitPoolSection();

			// �ȴ��̳߳��������������
			void WaitForTaskComplete();

			// �ṩ������ģ����߳����ӿ�
			void WaitForSignalObject(UINT signalObj);
			
		private:
			Thread* Threads;             	// �̳߳�
			UINT ThreadCount;               // �߳�����
			
			std::unordered_map<UINT, DWORD> SignalObjects;
			std::unordered_map<UINT, THREAD_EVENT> Events;
			std::queue<ThreadTask> Tasks;            // �̴߳�������

			THREAD_MUTEX ThreadPoolSection;          // �̳߳ػ�����
			THREAD_EVENT ThreadPoolTaskEmpty;        // �̳߳��¼�: �������б�Ϊ��ʱ, �¼�����; ��֮��λ
			THREAD_EVENT ThreadPoolTaskNoEmpty;      // �̳߳��¼�: �������б�Ϊ��ʱ, �¼�����; ��֮��λ

			UINT RunningThreadCount = 0;     // = ThreadCount
			UINT WorkingThreadCount = 0;
			
			UINT SignalCounter = 0;
		};
	};
};