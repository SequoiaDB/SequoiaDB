/****************************************************************************
 *
 * Name: worker.cpp
 * Description: Thread API for sample programs
 *
 ******************************************************************************/
#include <assert.h>
#include "worker.hpp"

namespace sample
{
    // 定义不同平台的线程相关的辅助函数
#ifdef _WIN32
    static UINT32 __stdcall _threadMain(void* arg)
    {
        assert(NULL != arg);

        workThread* self;
        self = (workThread*)arg;
        self->func(self->args);
        return SDB_OK;
    }

    static INT32 _threadCreate(workThread *thread)
    {
        assert(NULL != thread);
        assert(NULL != thread->func);

        thread->thread = (HANDLE)_beginthreadex(NULL, 0, _threadMain,
            thread, 0, NULL);
        if (NULL == thread->thread)
        {
            return SDB_SYS ;
        }
        return SDB_OK ;
    }

    static INT32 _threadJoin(workThread* thread)
    {
        DWORD rc ;
        BOOL brc ;

        assert(NULL != thread) ;

        rc = WaitForSingleObject(thread->thread, INFINITE);
        if (WAIT_FAILED == rc)
        {
            return SDB_SYS ;
        }
        brc = CloseHandle( thread->thread ) ;
        if ( 0 == brc )
        {
            return SDB_SYS ;
        }
        return SDB_OK ;
    }
#else     /*POSIX*/
#include <signal.h>

    static void* _threadMain(void *arg)
    {
        workThread *self;
        sigset_t sigset;
        INT32 ret = -1;

        assert(NULL != arg);

        self = (workThread *)arg;
        ret = sigfillset(&sigset);
        assert(0 == ret);

        ret = pthread_sigmask( SIG_BLOCK, &sigset, NULL);
        assert(0 == ret);

        self->func(self->args);
        return NULL;
    }

    static INT32 _threadCreate(workThread* thread)
    {
        INT32 ret;

        assert(NULL != thread);
        assert(NULL != thread->func);


        ret = pthread_create(&thread->thread, NULL, _threadMain, thread);
        if (0 != ret)
        {
            return SDB_SYS;
        }
        return SDB_OK ;
    }

    static INT32 _threadJoin(workThread* thread)
    {
        INT32 ret ;

        assert(NULL != thread);

        ret = pthread_join(thread->thread, NULL);
        if ( 0 != ret )
        {
            return SDB_SYS ;
        }
        return SDB_OK ;
    }
#endif


    // 实现worker类的方法
    worker::worker(workerFunc func, void *args)
        : _thread(func, args)
    {
        _started = FALSE;
    }

    worker::~worker(){}

    INT32 worker::start()
    {
        INT32 rc = SDB_OK;

        assert(!_started);
        rc = _threadCreate(&_thread);
        if (SDB_OK == rc)
        {
            _started = TRUE;
        }
        return rc;
    }

    INT32 worker::waitStop()
    {
        INT32 rc = SDB_OK;

        assert(_started);
        rc = _threadJoin(&_thread);
        if (SDB_OK != rc)
        {
            _started = FALSE;
        }
        return rc;
    }
}
