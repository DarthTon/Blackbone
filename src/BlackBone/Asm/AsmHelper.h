#pragma once
#include "../Config.h"

#ifdef USE64
#define AsmJitHelper AsmHelper64
#include "AsmHelper64.h"
#else
#include "AsmHelper32.h"
#define AsmJitHelper AsmHelper32
#endif // USE64
