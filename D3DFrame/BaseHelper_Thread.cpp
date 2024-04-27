#include "BaseHelper_Thread.h"

using namespace BaseHelper::Thread;

DWORD WINAPI ThreadPool::ThreadProc(void* param)
{
    THREAD* thread = (THREAD*)param;
    ThreadPool* pool = thread->pool;
    
    pool->EnterPoolSection();
    ++pool->RunningThreadCount;
    pool->ExitPoolSection();
    while(1)
    {
        pool->EnterPoolSection();
        while(pool->Tasks.size() == 0)
        {
            SetEvent(pool->ThreadPoolTaskEmpty);
            ResetEvent(pool->ThreadPoolTaskNoEmpty);
            pool->ExitPoolSection();
            if(thread->bDestory)
                break;
            WaitForSingleObject(pool->ThreadPoolTaskNoEmpty, INFINITE);
            pool->EnterPoolSection();
        }

        if(thread->bDestory)
            break;

        ThreadTask task = pool->Tasks.front();
        pool->Tasks.pop();
        
        ++pool->WorkingThreadCount;
        
        pool->ExitPoolSection();
		pool->SignalObjects[task.SignalObject] = thread->threadId;
		
		ResetEvent(pool->Events[thread->threadId]);
        task.Callback(pool, task.Param);
		SetEvent(pool->Events[thread->threadId]);
		
        pool->EnterPoolSection();
        --pool->WorkingThreadCount;
        pool->ExitPoolSection();
    }

    pool->EnterPoolSection();
    --pool->RunningThreadCount;
    pool->ExitPoolSection();
    return 0;
}

ThreadPool::ThreadPool(UINT threadCount)
{
    ThreadCount = threadCount;
    Threads = (THREAD*)malloc(sizeof(THREAD) * ThreadCount);

    InitializeCriticalSection(&ThreadPoolSection);
    ThreadPoolTaskNoEmpty = CreateEvent(NULL, 1, 0, NULL);
    ThreadPoolTaskEmpty = CreateEvent(NULL, 1, 1, NULL);

    for(UINT i = 0; i < ThreadCount; ++i)
    {
        Threads[i].pool = this;
        Threads[i].bDestory = 0;
        Threads[i].hThread = CreateThread(NULL, 0, ThreadProc, &Threads[i], 0, &Threads[i].threadId);
		THREAD_EVENT event = CreateEvent(NULL, 0, 0, NULL);
		Events[Threads[i].threadId] = event;
    }
}

ThreadPool::~ThreadPool()
{
    std::queue<ThreadTask> empty;
    std::swap(empty, Tasks);

    for(UINT i = 0; i < ThreadCount; ++i)
        Threads[i].bDestory = 1;
    
    SetEvent(ThreadPoolTaskNoEmpty);        // 假设线程都在等待任务
    
    while(RunningThreadCount != 0);

    for(UINT i = 0; i < ThreadCount; ++i)
        TerminateThread(Threads[i].hThread, 0);
    
    for(auto& item: Events)
        CloseHandle(item.second);
	
    CloseHandle(ThreadPoolTaskEmpty);
    CloseHandle(ThreadPoolTaskNoEmpty);
    DeleteCriticalSection(&ThreadPoolSection);
    free(Threads);
}

UINT ThreadPool::CommitThreadTask(ThreadTask task)
{
	task.SignalObject = SignalCounter++;
    Tasks.push(task);
    SetEvent(ThreadPoolTaskNoEmpty);
    ResetEvent(ThreadPoolTaskEmpty);

	if(SignalCounter >= THREAD_DEF_SIGNALOBJECT_COUNT)
		SignalCounter = 0;
	
	SignalObjects[task.SignalObject] = 0;
	return task.SignalObject;
}

void ThreadPool::EnterPoolSection()
{
    EnterCriticalSection(&ThreadPoolSection);
}

void ThreadPool::ExitPoolSection()
{
    LeaveCriticalSection(&ThreadPoolSection);
}

void ThreadPool::WaitForTaskComplete()
{
    while(WorkingThreadCount != 0 || Tasks.size() != 0);
}

ThreadPool* ThreadPool::GetInstance(UINT threadCount)
{
    static ThreadPool pool(threadCount);
    return &pool;
}

void ThreadPool::WaitForSignalObject(UINT signalObj)
{
	while(SignalObjects[signalObj] == 0)
        Sleep(1000);                // 让等待线程休眠
    
    WaitForSingleObject(Events[SignalObjects[signalObj]], 1000);
}