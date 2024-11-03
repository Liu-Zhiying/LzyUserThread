#ifndef USERTHREAD_H
#define USERTHREAD_H

#include "Context.h"

typedef void (*UserThreadEntry)(void*);

void* CreateUserThread(UserThreadEntry pEntry, void* param);
extern "C" BOOL EndCurrentUserThread();
extern "C" BOOL PauseUserThread();
extern "C" BOOL PauseUserThreadWithTimeout(DWORD timeoutMs);
extern "C" BOOL UserThreadDispatcher();

#endif