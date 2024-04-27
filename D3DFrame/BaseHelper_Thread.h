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
			
			// 线程池单例
			static ThreadPool* GetInstance(UINT threadCount = THREAD_DEF_COUNT);

			ThreadPool(UINT threadCount);
			~ThreadPool();
			
			// 向线程池提交任务
			UINT CommitThreadTask(ThreadTask task);

			// 获取线程池的锁(需要和 ExitPoolSection 配套使用); 若锁被其它线程使用, 则线程停止运行, 等待获取线程锁
			void EnterPoolSection();
			// 关闭线程池的锁(需要和 EnterPoolSection 配套使用)
			void ExitPoolSection();

			// 等待线程池所有任务处理完毕
			void WaitForTaskComplete();

			// 提供给其它模块的线程锁接口
			void WaitForSignalObject(UINT signalObj);
			
		private:
			Thread* Threads;             	// 线程池
			UINT ThreadCount;               // 线程数量
			
			std::unordered_map<UINT, DWORD> SignalObjects;
			std::unordered_map<UINT, THREAD_EVENT> Events;
			std::queue<ThreadTask> Tasks;            // 线程处理任务

			THREAD_MUTEX ThreadPoolSection;          // 线程池互斥锁
			THREAD_EVENT ThreadPoolTaskEmpty;        // 线程池事件: 当任务列表为空时, 事件激活; 反之复位
			THREAD_EVENT ThreadPoolTaskNoEmpty;      // 线程池事件: 当任务列表不为空时, 事件激活; 反之复位

			UINT RunningThreadCount = 0;     // = ThreadCount
			UINT WorkingThreadCount = 0;
			
			UINT SignalCounter = 0;
		};
	};
};