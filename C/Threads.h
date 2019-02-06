/* Threads.h -- multithreading library
2017-06-18 : Igor Pavlov : Public domain */

#ifndef __7Z_THREADS_H
#define __7Z_THREADS_H

#ifdef _WIN32
#include <windows.h>
#else
#define LPVOID void *
#define BOOL int
#define TRUE 1
#define FALSE 0
#include <pthread.h>
#endif

#include "7zTypes.h"

EXTERN_C_BEGIN

typedef struct _CThread
{
#ifdef _WIN32
    HANDLE hnd;
#else
    pthread_t tid;
#endif
    int _created;
} CThread;


#define Thread_Construct(p) (p)->_created = 0
#define Thread_WasCreated(p) ((p)->_created != 0)
WRes Thread_Close(CThread *p);
WRes Thread_Wait(CThread *p);

typedef
#ifdef UNDER_CE
  DWORD
#else
  unsigned
#endif
  THREAD_FUNC_RET_TYPE;

#define THREAD_FUNC_CALL_TYPE MY_STD_CALL
#define THREAD_FUNC_DECL THREAD_FUNC_RET_TYPE THREAD_FUNC_CALL_TYPE
typedef THREAD_FUNC_RET_TYPE (THREAD_FUNC_CALL_TYPE * THREAD_FUNC_TYPE)(void *);
WRes Thread_Create(CThread *p, THREAD_FUNC_TYPE func, LPVOID param);

typedef struct _CEvent
{
    int _created;
    int _manual_reset;
    int _state;
#ifdef _WIN32
    HANDLE hnd;
#else
    pthread_mutex_t _mutex;
    pthread_cond_t  _cond;
#endif
} CEvent;
typedef CEvent CAutoResetEvent;
typedef CEvent CManualResetEvent;
#define Event_Construct(event) (event)->_created = 0
#define Event_IsCreated(event) ((event)->_created)
WRes Event_Wait(CEvent *event);
WRes Event_Close(CEvent *event);
WRes Event_Set(CEvent *p);
WRes Event_Reset(CEvent *p);
WRes ManualResetEvent_Create(CManualResetEvent *p, int signaled);
WRes ManualResetEvent_CreateNotSignaled(CManualResetEvent *p);
WRes AutoResetEvent_Create(CAutoResetEvent *p, int signaled);
WRes AutoResetEvent_CreateNotSignaled(CAutoResetEvent *p);

typedef struct _CSemaphore
{
    int _created;
    UInt32 _count;
    UInt32 _maxCount;
#ifdef _WIN32
    HANDLE hnd;
#else
    pthread_mutex_t _mutex;
    pthread_cond_t  _cond;
#endif
} CSemaphore;
#define Semaphore_Construct(p) (p)->_created = 0
#define Semaphore_IsCreated(p) (*(p) != NULL)
WRes Semaphore_Wait(CSemaphore *p);
WRes Semaphore_Close(CSemaphore *p);
WRes Semaphore_Create(CSemaphore *p, UInt32 initCount, UInt32 maxCount);
WRes Semaphore_ReleaseN(CSemaphore *p, UInt32 num);
WRes Semaphore_Release1(CSemaphore *p);

typedef struct {
#ifdef _WIN32
    CRITICAL_SECTION cs;
#else
    pthread_mutex_t _mutex;
#endif
} CCriticalSection;

WRes CriticalSection_Init(CCriticalSection *p);
void CriticalSection_Delete(CCriticalSection *p);
void CriticalSection_Enter(CCriticalSection *p);
void CriticalSection_Leave(CCriticalSection *p);

EXTERN_C_END

#endif
