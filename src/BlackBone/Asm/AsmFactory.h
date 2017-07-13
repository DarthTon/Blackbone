#pragma once
#include "../Config.h"
#include "../Include/Types.h"

#include "AsmHelper64.h"
#include "AsmHelper32.h"

namespace blackbone
{

using AsmHelperPtr = std::unique_ptr<IAsmHelper>;

/// <summary>
/// Get suitable asm generator
/// </summary>
class AsmFactory
{
public:
    enum eAsmArch
    {
        asm32,      // x86
        asm64       // x86_64
    };

    /// <summary>
    /// Get suitable asm generator
    /// </summary>
    /// <param name="arch">Desired CPU architecture</param>
    /// <returns>AsmHelperBase interface</returns>
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

    /// <summary>
    /// Get suitable asm generator
    /// </summary>
    /// <param name="mt">Desired PE module architecture</param>
    /// <returns>AsmHelperBase interface</returns>
    static AsmHelperPtr GetAssembler( eModType mt )
    {
        if (mt == mt_default)
            mt = sizeof( intptr_t ) > sizeof( int32_t ) ? mt_mod64 : mt_mod32;
        
        switch (mt)
        {
        case mt_mod32:
            return GetAssembler( asm32 );
        case mt_mod64:
            return GetAssembler( asm64 );
        default:
            return nullptr;
        }
    }

    /// <summary>
    /// Get suitable asm generator
    /// </summary>
    /// <param name="wow64process">Target process CPU architecture</param>
    /// <returns>AsmHelperBase interface</returns>
    static AsmHelperPtr GetAssembler( bool wow64process )
    {
        return GetAssembler( wow64process ? asm32 : asm64 );
    }


    /// <summary>
    /// Get default asm generator
    /// </summary>
    /// <returns></returns>
    static AsmHelperPtr GetAssembler()
    {
#ifdef USE64
        return std::make_unique<AsmHelper64>();
#else
        return std::make_unique<AsmHelper32>();
#endif
    }
};

}