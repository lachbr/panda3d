#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <malloc.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cmdlib.h"
#include "messages.h"
#include "log.h"
#include "threads.h"
#include "blockmem.h"

#ifdef __GNUC__
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#include <pthread.h>
#endif
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
#endif

#include "hlassert.h"

BSPThread::BSPThread() : Thread("bspthread", "bspthread_sync"),
                         _func(nullptr),
                         _val(0),
                         _finished(false)
{
}

void BSPThread::thread_main()
{
        //Thread::thread_main();
        (*_func)(_val);
        _finished = true;
}

void BSPThread::set_function(q_threadfunction *func)
{
        _func = func;
}

void BSPThread::set_value(int val)
{
        _val = val;
}

volatile bool BSPThread::is_finished() const
{
        return _finished;
}

ThreadPriority g_threadpriority = TP_normal;
LightMutex g_global_lock("bspToolsGlobalMutex");
pvector<PT(BSPThread)> g_threadhandles;

#define THREADTIMES_SIZE 100
#define THREADTIMES_SIZEf (float)(THREADTIMES_SIZE)

static int dispatch = 0;
static int workcount = 0;
static int oldf = 0;
static bool pacifier = false;
static bool threaded = false;
static double threadstart = 0;
static double threadtimes[THREADTIMES_SIZE];

int GetThreadWork()
{
        int r, f, i;
        double ct, finish, finish2, finish3;
        static const char *s1 = NULL; // avoid frequent call of Localize() in PrintConsole
        static const char *s2 = NULL;

        ThreadLock();
        if (s1 == NULL)
                s1 = Localize("  (%d%%: est. time to completion %ld/%ld/%ld secs)   ");
        if (s2 == NULL)
                s2 = Localize("  (%d%%: est. time to completion <1 sec)   ");

        if (dispatch == 0)
        {
                oldf = 0;
        }

        if (dispatch > workcount)
        {
                Developer(DEVELOPER_LEVEL_ERROR, "dispatch > workcount!!!\n");
                ThreadUnlock();
                return -1;
        }
        if (dispatch == workcount)
        {
                Developer(DEVELOPER_LEVEL_MESSAGE, "dispatch == workcount, work is complete\n");
                ThreadUnlock();
                return -1;
        }
        if (dispatch < 0)
        {
                Developer(DEVELOPER_LEVEL_ERROR, "negative dispatch!!!\n");
                ThreadUnlock();
                return -1;
        }

        f = THREADTIMES_SIZE * dispatch / workcount;
        if (pacifier)
        {
                printf("\r%6d /%6d", dispatch, workcount);

                if (f != oldf)
                {
                        ct = I_FloatTime();
                        /* Fill in current time for threadtimes record */
                        for (i = oldf; i <= f; i++)
                        {
                                if (threadtimes[i] < 1)
                                {
                                        threadtimes[i] = ct;
                                }
                        }
                        oldf = f;

                        if (f > 10)
                        {
                                finish = (ct - threadtimes[0]) * (THREADTIMES_SIZEf - f) / f;
                                finish2 = 10.0 * (ct - threadtimes[f - 10]) * (THREADTIMES_SIZEf - f) / THREADTIMES_SIZEf;
                                finish3 = THREADTIMES_SIZEf * (ct - threadtimes[f - 1]) * (THREADTIMES_SIZEf - f) / THREADTIMES_SIZEf;

                                if (finish > 1.0)
                                {
                                        printf(s1, f, (long)(finish), (long)(finish2),
                                               (long)(finish3));
                                }
                                else
                                {
                                        printf(s2, f);
                                }
                        }
                }
        }
        else
        {
                if (f != oldf)
                {
                        oldf = f;
                        switch (f)
                        {
                        case 10:
                        case 20:
                        case 30:
                        case 40:
                        case 50:
                        case 60:
                        case 70:
                        case 80:
                        case 90:
                        case 100:
                                /*
                                case 5:
                                case 15:
                                case 25:
                                case 35:
                                case 45:
                                case 55:
                                case 65:
                                case 75:
                                case 85:
                                case 95:
                                */
                                printf("%d%%...", f);
                        default:
                                break;
                        }
                }
        }

        r = dispatch;
        dispatch++;

        ThreadUnlock();
        return r;
}

q_threadfunction *workfunction;

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4100) // unreferenced formal parameter
#endif
static void ThreadWorkerFunction(int unused)
{
        int work;

        while ((work = GetThreadWork()) != -1)
        {
                workfunction(work);
        }
}

#ifdef _WIN32
#pragma warning(pop)
#endif

void RunThreadsOnIndividual(int workcnt, bool showpacifier, q_threadfunction func)
{
        workfunction = func;
        RunThreadsOn(workcnt, showpacifier, ThreadWorkerFunction);
}

#ifndef SINGLE_THREADED

#define USED
#ifdef _WIN32
#include <windows.h>
#endif

int g_numthreads = DEFAULT_NUMTHREADS;

int GetCurrentThreadNumber()
{
        Thread *th = Thread::get_current_thread();

        int num = 0;

        for (size_t i = 0; i < g_threadhandles.size(); i++)
        {
                if (g_threadhandles[i] == th)
                {
                        num = i;
                        break;
                }
        }

        return num;
}

void ThreadSetPriority(ThreadPriority type)
{
}

void ThreadSetDefault()
{
#ifdef _WIN32
        SYSTEM_INFO info;
#endif

        if (g_numthreads == -1) // not set manually
        {
#ifdef _WIN32
                GetSystemInfo(&info);
                g_numthreads = info.dwNumberOfProcessors;
#elif defined(__GNUC__)
                g_numthreads = DEFAULT_NUMTHREADS;
#endif
                if (g_numthreads < 1 || g_numthreads > 32)
                {
                        g_numthreads = 1;
                }
        }
}

void ThreadLock()
{
        if (!threaded)
        {
                return;
        }
        g_global_lock.acquire();
}

void ThreadUnlock()
{
        if (!threaded)
        {
                return;
        }
        g_global_lock.release();
}

q_threadfunction *q_entry;

void RunThreadsOn(int workcnt, bool showpacifier, q_threadfunction func)
{
        string threadid[MAX_THREADS];
        int i;
        double start, end;

        g_threadhandles.clear();

        threadstart = I_FloatTime();
        start = threadstart;
        for (i = 0; i < THREADTIMES_SIZE; i++)
        {
                threadtimes[i] = 0;
        }
        dispatch = 0;
        workcount = workcnt;
        oldf = -1;
        pacifier = showpacifier;
        threaded = true;
        q_entry = func;

        if (workcount < dispatch)
        {
                Developer(DEVELOPER_LEVEL_ERROR, "RunThreadsOn: Workcount(%i) < dispatch(%i)\n", workcount, dispatch);
        }
        hlassume(workcount >= dispatch, assume_BadWorkcount);

        //
        // Create all the threads (suspended)
        //
        for (i = 0; i < g_numthreads; i++)
        {
                PT(BSPThread)
                hThread = new BSPThread;
                hThread->set_function(func);
                hThread->set_value(i);
                hThread->set_pipeline_stage(Thread::get_main_thread()->get_pipeline_stage());

                g_threadhandles.push_back(hThread);
        }

        // Start all the threads
        for (i = 0; i < g_threadhandles.size(); i++)
        {
                if (!g_threadhandles[i]->start(g_threadpriority, false))
                {
                        Fatal(assume_THREAD_ERROR, "Unable to start thread #%d", i);
                }
        }

        // Wait for threads to complete
        for (i = 0; i < g_threadhandles.size(); i++)
        {
                Developer(DEVELOPER_LEVEL_MESSAGE, "WaitForSingleObject on thread #%d [%08X]\n", i, g_threadhandles[i]);
                while (true)
                {
                        if (!g_threadhandles[i]->is_started())
                                break;
                        if (g_threadhandles[i]->is_finished())
                                break;
                }
        }

        q_entry = NULL;
        threaded = false;
        end = I_FloatTime();
        if (pacifier)
        {
                printf("\r%60s\r", "");
        }
        Log(" (%.2f seconds)\n", end - start);
}

#endif /*SINGLE_THREADED */

/*====================
| Begin SINGLE_THREADED
=*/
#ifdef SINGLE_THREADED

int g_numthreads = 1;

void ThreadSetPriority(q_threadpriority type)
{
}

void threads_InitCrit()
{
}

void threads_UninitCrit()
{
}

void ThreadSetDefault()
{
        g_numthreads = 1;
}

void ThreadLock()
{
}

void ThreadUnlock()
{
}

void RunThreadsOn(int workcnt, bool showpacifier, q_threadfunction func)
{
        int i;
        double start, end;

        dispatch = 0;
        workcount = workcnt;
        oldf = -1;
        pacifier = showpacifier;
        threadstart = I_FloatTime();
        start = threadstart;
        for (i = 0; i < THREADTIMES_SIZE; i++)
        {
                threadtimes[i] = 0.0;
        }

        if (pacifier)
        {
                setbuf(stdout, NULL);
        }
        func(0);

        end = I_FloatTime();

        if (pacifier)
        {
                printf("\r%60s\r", "");
        }

        Log(" (%.2f seconds)\n", end - start);
}

#endif

/*=
| End SINGLE_THREADED
=====================*/
