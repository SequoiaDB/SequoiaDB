/****************************************************************************
 *
 * Name: worker.hpp
 * Description: Thread API for sample programs
 *
 ******************************************************************************/
#ifndef WORKER_HPP__
#define WORKER_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "string.h"
#ifdef _WINDOWS
#include <process.h>
#include <Windows.h>
#else /* POSIX */
#include <pthread.h>
#endif


namespace sample
{
    typedef void (*workerFunc)(void *);

    struct workThread
    {
#ifdef _WIN32
        HANDLE     thread;
#else /*POSIX*/
        pthread_t  thread;
#endif
        workerFunc func;
        void*      args;

        /* 构造函数 */
        workThread(workerFunc func, void *args)
            : func(func), args(args)
        {
            memset(&thread, 0, sizeof(thread));
        }
    };

    class worker
    {
    public:
        /* 构造函数 */
        worker(workerFunc func, void *args);
        /* 析构函数 */
        ~worker();
        INT32 start();
        INT32 waitStop();

    private:
        workThread   _thread;
        BOOLEAN      _started;
    };
}

#endif // WORKER_HPP__

