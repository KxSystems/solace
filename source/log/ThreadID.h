
#ifdef _WIN32

#define THREAD_ID ::GetCurrentThreadId()

#elif __linux__

#include <sys/syscall.h>
#include <unistd.h>
#define THREAD_ID syscall(SYS_gettid)

#else

#include <pthread.h>
#define THREAD_ID (long)((void*)pthread_self())

#endif
