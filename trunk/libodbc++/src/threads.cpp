#include <odbc++/threads.h>
#include <odbc++/types.h>

#include <errno.h>


#if defined (ODBCXX_ENABLE_THREADS)

using namespace odbc;
using namespace std;

Mutex::Mutex()
{
#if defined(WIN32)

  InitializeCriticalSection(&mutex_);

#else

  if(pthread_mutex_init(&mutex_,NULL)!=0) {
    throw SQLException
      ("[libodbc++]: OS error, mutex initialization failed");
  }

#endif
}

Mutex::~Mutex()
{
#if defined(WIN32)
  DeleteCriticalSection(&mutex_);
#else
  pthread_mutex_destroy(&mutex_);
#endif
}


void Mutex::lock()
{
#if defined(WIN32)

  EnterCriticalSection(&mutex_);

#else

  if(pthread_mutex_lock(&mutex_)!=0) {
    throw SQLException
      ("[libodbc++]: OS error, mutex lock failed");
  }

#endif
}


void Mutex::unlock()
{
#if defined(WIN32)
  
  LeaveCriticalSection(&mutex_);

#else

  if(pthread_mutex_unlock(&mutex_)!=0) {
    throw SQLException
      ("[libodbc++]: OS error, mutex unlock failed");
  }

#endif
}


#endif // ODBCXX_ENABLE_THREADS
