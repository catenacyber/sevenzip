/* Threads.c -- multithreading library
2017-06-26 : Igor Pavlov : Public domain */

#include "Precomp.h"

#ifndef UNDER_CE
#ifdef _WIN32
#include <process.h>
#endif
#endif

#ifndef _WIN32
#include <string.h>
#include <errno.h>
#endif

#include "Threads.h"

#ifdef _WIN32
static WRes GetError()
{
  DWORD res = GetLastError();
  return res ? (WRes)res : 1;
}

static WRes HandleToWRes(HANDLE h) { return (h != NULL) ? 0 : GetError(); }
static WRes BOOLToWRes(BOOL v) { return v ? 0 : GetError(); }

WRes HandlePtr_Close(HANDLE *p)
{
  if (*p != NULL)
  {
    if (!CloseHandle(*p))
      return GetError();
    *p = NULL;
  }
  return 0;
}

WRes Handle_WaitObject(HANDLE h) { return (WRes)WaitForSingleObject(h, INFINITE); }
#endif

WRes Thread_Create(CThread *p, THREAD_FUNC_TYPE func, LPVOID param)
{
  /* Windows Me/98/95: threadId parameter may not be NULL in _beginthreadex/CreateThread functions */
  int ret;

  p->_created = 0;
  #ifdef UNDER_CE
  
  DWORD threadId;
  p->hnd = CreateThread(0, 0, func, param, 0, &threadId);
  ret = HandleToWRes(p->hnd);
  #elif defined(_WIN32)

  unsigned threadId;
  p->hnd = (HANDLE)_beginthreadex(NULL, 0, func, param, 0, &threadId);
  ret = HandleToWRes(p->hnd);

  #else
    pthread_attr_t attr;
    ret = pthread_attr_init(&attr);
    if (ret) return ret;

    ret = pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
    if (ret) return ret;

    ret = pthread_create(&p->tid, &attr, (void * (*)(void *))func, param);
    //not checking return
    pthread_attr_destroy(&attr);

  #endif
  if (ret == 0) {
    p->_created = 1;
  }

  /* maybe we must use errno here, but probably GetLastError() is also OK. */
  return ret;
}

WRes Thread_Close(CThread *p) {
#ifdef _WIN32
    return HandlePtr_Close(&p->hnd);
#else
    p->_created = 0;
    return SZ_OK;
#endif
}

WRes Thread_Wait(CThread *p) {
#ifdef _WIN32
    return Handle_WaitObject(p->hnd);
#else
    int ret;

    if (p->_created == 0) return EINVAL;

    void *status;
    ret = pthread_join(p->tid, &status);
    p->_created = 0;
    return ret;
#endif
}

static WRes Event_Create(CEvent *p, BOOL manualReset, int signaled)
{
    int ret;

    p->_created = 0;
#ifdef _WIN32
  p->hnd = CreateEvent(NULL, manualReset, (signaled ? TRUE : FALSE), NULL);
  ret = HandleToWRes(*p);
#else
    pthread_mutexattr_t mutexattr;

    memset(&mutexattr,0,sizeof(mutexattr));
    ret = pthread_mutexattr_init(&mutexattr);
    if (ret) return ret;

    ret = pthread_mutexattr_settype(&mutexattr,PTHREAD_MUTEX_ERRORCHECK);
    if (ret) return ret;

    ret = pthread_mutex_init(&p->_mutex,&mutexattr);
    if (ret) return ret;

    ret = pthread_cond_init(&p->_cond,0);

#endif
    if (ret) return ret;

    p->_manual_reset = manualReset;
    p->_state        = (signaled ? TRUE : FALSE);
    p->_created = 1;
    return ret;
}

WRes Event_Close(CEvent *p) {
#ifdef _WIN32
    return HandlePtr_Close(&p->hnd);
#else
    if (p->_created)
    {
        p->_created = 0;
        pthread_mutex_destroy(&p->_mutex);
        pthread_cond_destroy(&p->_cond);
    }
    return 0;
#endif
}

WRes Event_Wait(CEvent *p) {
#ifdef _WIN32
    return Handle_WaitObject(p->hnd);
#else
    pthread_mutex_lock(&p->_mutex);
    while (p->_state == FALSE)
    {
        pthread_cond_wait(&p->_cond, &p->_mutex);
    }
    if (p->_manual_reset == FALSE)
    {
        p->_state = FALSE;
    }
    pthread_mutex_unlock(&p->_mutex);
    return 0;
#endif
}

WRes Event_Set(CEvent *p) {
#ifdef _WIN32
return BOOLToWRes(SetEvent(p->hnd));
#else
    int ret = pthread_mutex_lock(&p->_mutex);
    if (ret) return ret;

    p->_state = TRUE;
    ret = pthread_cond_broadcast(&p->_cond);
    pthread_mutex_unlock(&p->_mutex);
    return ret;
#endif
}

WRes Event_Reset(CEvent *p) {
#ifdef _WIN32
return BOOLToWRes(ResetEvent(p->hnd));
#else
    int ret = pthread_mutex_lock(&p->_mutex);
    if (ret) return ret;
    p->_state = FALSE;
    ret = pthread_mutex_unlock(&p->_mutex);
    return ret;
#endif
}

WRes ManualResetEvent_Create(CManualResetEvent *p, int signaled) { return Event_Create(p, TRUE, signaled); }
WRes AutoResetEvent_Create(CAutoResetEvent *p, int signaled) { return Event_Create(p, FALSE, signaled); }
WRes ManualResetEvent_CreateNotSignaled(CManualResetEvent *p) { return ManualResetEvent_Create(p, 0); }
WRes AutoResetEvent_CreateNotSignaled(CAutoResetEvent *p) { return AutoResetEvent_Create(p, 0); }


WRes Semaphore_Create(CSemaphore *p, UInt32 initCount, UInt32 maxCount)
{
    int ret;

    p->_created = 0;
#ifdef _WIN32
  p->hnd = CreateSemaphore(NULL, (LONG)initCount, (LONG)maxCount, NULL);
  ret = HandleToWRes(p->hnd);
#else
    pthread_mutexattr_t mutexattr;
    memset(&mutexattr,0,sizeof(mutexattr));
    ret = pthread_mutexattr_init(&mutexattr);
    if (ret) return ret;
    ret = pthread_mutexattr_settype(&mutexattr,PTHREAD_MUTEX_ERRORCHECK);
    if (ret) return ret;
    ret = pthread_mutex_init(&p->_mutex,&mutexattr);
    if (ret) return ret;
    ret = pthread_cond_init(&p->_cond,0);
#endif
    if (ret) return ret;

    p->_count    = initCount;
    p->_maxCount = maxCount;
    p->_created = 1;
    return ret;
}

WRes Semaphore_Close(CSemaphore *p) {
#ifdef _WIN32
    return HandlePtr_Close(&p->hnd);
#else
    if (p->_created)
    {
        p->_created = 0;
        pthread_mutex_destroy(&p->_mutex);
        pthread_cond_destroy(&p->_cond);
    }
    return 0;
#endif
}

WRes Semaphore_Wait(CSemaphore *p) {
#ifdef _WIN32
    return Handle_WaitObject(p->hnd);
#else
    pthread_mutex_lock(&p->_mutex);
    while (p->_count < 1)
    {
        pthread_cond_wait(&p->_cond, &p->_mutex);
    }
    p->_count--;
    pthread_mutex_unlock(&p->_mutex);
    return 0;
#endif
}

static WRes Semaphore_Release(CSemaphore *p, UInt32 releaseCount, UInt32 *previousCount)
  {
#ifdef _WIN32
return BOOLToWRes(ReleaseSemaphore(*p, (LONG)releaseCount, (LONG)previousCount));
#else
    int ret;
    if (releaseCount < 1) return EINVAL;

    ret = pthread_mutex_lock(&p->_mutex);
    if (ret) return ret;
    UInt32 newCount = p->_count + releaseCount;
    if (newCount > p->_maxCount) {
        pthread_mutex_unlock(&p->_mutex);
        return EINVAL;
    }
    p->_count = newCount;
    ret = pthread_cond_broadcast(&p->_cond);
    pthread_mutex_unlock(&p->_mutex);
    return ret;
#endif
  }
WRes Semaphore_ReleaseN(CSemaphore *p, UInt32 num)
  { return Semaphore_Release(p, num, NULL); }
WRes Semaphore_Release1(CSemaphore *p) { return Semaphore_ReleaseN(p, 1); }

WRes CriticalSection_Init(CCriticalSection *p)
{
#ifdef _WIN32
  /* InitializeCriticalSection can raise only STATUS_NO_MEMORY exception */
  #ifdef _MSC_VER
  __try
  #endif
  {
    InitializeCriticalSection(&p->cs);
    /* InitializeCriticalSectionAndSpinCount(p, 0); */
  }
  #ifdef _MSC_VER
  __except (EXCEPTION_EXECUTE_HANDLER) { return 1; }
  #endif
  return 0;
#else
    int ret;
    pthread_mutexattr_t mutexattr;
    if (!p) return EINTR;
    memset(&mutexattr,0,sizeof(mutexattr));
    ret = pthread_mutexattr_init(&mutexattr);
    if (ret) return ret;
    ret = pthread_mutexattr_settype(&mutexattr,PTHREAD_MUTEX_ERRORCHECK);
    if (ret) return ret;
    ret = pthread_mutex_init(&p->_mutex,&mutexattr);
    return ret;
#endif
}

void CriticalSection_Delete(CCriticalSection *p) {
#ifdef _WIN32
    DeleteCriticalSection(&p->cs);
#else
    pthread_mutex_destroy(&p->_mutex);
#endif
}

void CriticalSection_Enter(CCriticalSection *p) {
#ifdef _WIN32
    EnterCriticalSection(&p->cs);
#else
    pthread_mutex_lock(&p->_mutex);
#endif
}

void CriticalSection_Leave(CCriticalSection *p) {
#ifdef _WIN32
    LeaveCriticalSection(&p->cs);
#else
    pthread_mutex_unlock(&p->_mutex);
#endif
}
