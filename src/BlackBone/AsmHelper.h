#pragma once

#ifdef _M_AMD64
#define AsmJitHelper AsmHelper64
#include "AsmHelper64.h"
#else
#include "AsmHelper32.h"
#define AsmJitHelper AsmHelper32
#endif // _M_AMD64
