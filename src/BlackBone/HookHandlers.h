#pragma once

#include "LocalHookBase.h"

namespace blackbone
{

class NoClass
{
};

template<typename Fn, class C>
struct HookHandler;

}

#include "HookHandlerCdecl.h"

#ifndef _M_AMD64
#include "HookHandlerStdcall.h"
#include "HookHandlerThiscall.h"
#include "HookHandlerFastcall.h"
#endif