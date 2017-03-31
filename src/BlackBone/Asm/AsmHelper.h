#pragma once
#include "../Config.h"
#include "../Include/Types.h"

#include "AsmHelper64.h"
#include "AsmHelper32.h"

#ifdef USE64
#define AsmJitHelper AsmHelper64
#else
#define AsmJitHelper AsmHelper32
#endif

namespace blackbone
{

typedef std::unique_ptr<AsmHelperBase> AsmHelperPtr;

class AsmFactory
{
public:
    enum eAsmArch
    {
        asm32,
        asm64
    };

    static AsmHelperPtr GetAssembler( eAsmArch arch )
    {
        switch (arch)
        {
        case asm32:
            return std::make_unique<AsmHelper32>();
        case asm64:
            return std::make_unique<AsmHelper64>();
        default:
            return nullptr;
        }
    }

    static AsmHelperPtr GetAssembler( eModType mt )
    {
        if (mt == mt_default)
            mt = sizeof( intptr_t ) > sizeof( int32_t ) ? mt_mod64 : mt_mod32;
        
        switch (mt)
        {
        case blackbone::mt_mod32:
            return GetAssembler( asm32 );
        case blackbone::mt_mod64:
            return GetAssembler( asm64 );
        default:
            return nullptr;
        }
    }

    static AsmHelperPtr GetAssembler( bool wow64process )
    {
        if (wow64process)
            return GetAssembler( asm32 );
        else
            return GetAssembler( asm64 );
    }
};

}