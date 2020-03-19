#include <sys/syscall.h>
#include <unistd.h>
#define THREAD_ID syscall(SYS_gettid)
