#include "PatternSearch.h"
#include "../Include/Macro.h"
#include "../Include/Winheaders.h"
#include "../Process/Process.h"

#include <algorithm>
#include <memory>

namespace blackbone
{

PatternSearch::PatternSearch( const std::vector<uint8_t>& pattern )
    : _pattern( pattern )
{
}

PatternSearch::PatternSearch( const std::initializer_list<uint8_t>&& pattern )
    : _pattern( pattern )
{
}

PatternSearch::PatternSearch( const std::string& pattern )
    : _pattern( pattern.begin(), pattern.end() )
{
}

PatternSearch::PatternSearch( const char* pattern, size_t len /*= 0*/ )
    : _pattern( pattern, pattern + (len ? len : strlen( pattern )) )
{
}

PatternSearch::PatternSearch( const uint8_t* pattern, size_t len /*= 0*/ )
    : _pattern( pattern, pattern + (len ? len : strlen( (const char*)pattern )) )
{ 
}

PatternSearch::~PatternSearch()
{
}

/// <summary>
/// Default pattern matching with wildcards.
/// std::search is approximately 2x faster than naive approach.
/// </summary>
/// <param name="wildcard">Pattern wildcard</param>
/// <param name="scanStart">Starting address</param>
/// <param name="scanSize">Size of region to scan</param>
/// <param name="out">Found results</param>
/// <param name="value_offset">Value that will be added to resulting addresses</param>
/// <returns>Number of found addresses</returns>
size_t PatternSearch::Search( uint8_t wildcard, void* scanStart, size_t scanSize, std::vector<ptr_t>& out, ptr_t value_offset /*= 0*/ )
{
    const uint8_t* cstart = (const uint8_t*)scanStart;
    const uint8_t* cend   = cstart + scanSize;

    for (;;)
    {
        const uint8_t* res = std::search( cstart, cend, _pattern.begin(), _pattern.end(),
                                          [&wildcard]( uint8_t val1, uint8_t val2 ){ return (val1 == val2 || val2 == wildcard); } );

        if (res >= cend)
            break;

        if (value_offset != 0)
            out.emplace_back( REBASE( res, scanStart, value_offset ) );
        else
            out.emplace_back( reinterpret_cast<ptr_t>(res) );

        cstart = res + _pattern.size();
    }

    return out.size();
}

/// <summary>
/// Full pattern match, no wildcards.
/// Uses Boyer–Moore–Horspool algorithm.
/// </summary>
/// <param name="scanStart">Starting address</param>
/// <param name="scanSize">Size of region to scan</param>
/// <param name="out">Found results</param>
/// <param name="value_offset">Value that will be added to resulting addresses</param>
/// <returns>Number of found addresses</returns>
size_t PatternSearch::Search( void* scanStart, size_t scanSize, std::vector<ptr_t>& out, ptr_t value_offset /*= 0*/ )
{
    size_t bad_char_skip[UCHAR_MAX + 1];

    const uint8_t* haystack = reinterpret_cast<const uint8_t*>(scanStart);
    const uint8_t* needle   = &_pattern[0];
    uintptr_t       nlen     = _pattern.size();
    uintptr_t       scan     = 0;
    uintptr_t       last     = nlen - 1;

    //
    // Preprocess
    //
    for (scan = 0; scan <= UCHAR_MAX; ++scan)
        bad_char_skip[scan] = nlen;

    for (scan = 0; scan < last; ++scan)
        bad_char_skip[needle[scan]] = last - scan;

    //
    // Search
    //
    while (scanSize >= static_cast<size_t>(nlen))
    {
        for (scan = last; haystack[scan] == needle[scan]; --scan)
        {
            if (scan == 0)
            {
                if (value_offset != 0)
                    out.emplace_back( REBASE( haystack, scanStart, value_offset ) );
                else
                    out.emplace_back( reinterpret_cast<ptr_t>(haystack) );

                break;
            }
        }

        scanSize -= bad_char_skip[haystack[last]];
        haystack += bad_char_skip[haystack[last]];
    }

    return out.size();
}

/// <summary>
/// Search pattern in remote process
/// </summary>
/// <param name="remote">Remote process</param>
/// <param name="wildcard">Pattern wildcard</param>
/// <param name="scanStart">Starting address</param>
/// <param name="scanSize">Size of region to scan</param>
/// <param name="out">Found results</param>
/// <returns>Number of found addresses</returns>
size_t PatternSearch::SearchRemote( Process& remote, uint8_t wildcard, ptr_t scanStart, size_t scanSize, std::vector<ptr_t>& out )
{
    uint8_t *pBuffer = reinterpret_cast<uint8_t*>(VirtualAlloc( NULL, scanSize, MEM_COMMIT, PAGE_READWRITE ));

    if (pBuffer && remote.memory().Read( scanStart, scanSize, pBuffer ) == STATUS_SUCCESS)
        Search( wildcard, pBuffer, scanSize, out, scanStart );

    if (pBuffer)
        VirtualFree( pBuffer, 0, MEM_RELEASE );

    return out.size();
}

/// <summary>
/// Search pattern in remote process
/// </summary>
/// <param name="remote">Remote process</param>
/// <param name="scanStart">Starting address</param>
/// <param name="scanSize">Size of region to scan</param>
/// <param name="out">Found results</param>
/// <returns>Number of found addresses</returns>
size_t PatternSearch::SearchRemote( Process& remote, ptr_t scanStart, size_t scanSize, std::vector<ptr_t>& out )
{
    uint8_t *pBuffer = reinterpret_cast<uint8_t*>(VirtualAlloc( NULL, scanSize, MEM_COMMIT, PAGE_READWRITE ));

    if (pBuffer && remote.memory().Read( scanStart, scanSize, pBuffer ) == STATUS_SUCCESS)
        Search( pBuffer, scanSize, out, scanStart );

    if (pBuffer)
        VirtualFree( pBuffer, 0, MEM_RELEASE );

    return out.size();
}

/// <summary>
/// Search pattern in whole address space of remote process
/// </summary>
/// <param name="remote">Remote process</param>
/// <param name="useWildcard">True if pattern contains wildcards</param>
/// <param name="wildcard">Pattern wildcard</param>
/// <param name="out">Found results</param>
/// <returns>Number of found addresses</returns>
size_t PatternSearch::SearchRemoteWhole( Process& remote, bool useWildcard, uint8_t wildcard, std::vector<ptr_t>& out )
{
    MEMORY_BASIC_INFORMATION64 mbi = { 0 };
    size_t  bufsize = 1 * 1024 * 1024;  // 1 MB
    uint8_t *buf = reinterpret_cast<uint8_t*>(VirtualAlloc( 0, bufsize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE ));

    out.clear();

    auto native = remote.core().native();

    for (ptr_t memptr = native->minAddr(); memptr < native->maxAddr(); memptr = mbi.BaseAddress + mbi.RegionSize)
    {
        auto status = remote.core().native()->VirtualQueryExT( memptr, &mbi );

        if (status == STATUS_INVALID_PARAMETER || status == STATUS_ACCESS_DENIED)
            break;
        else if (status != STATUS_SUCCESS)
            continue;

        // Filter regions
        if (mbi.State != MEM_COMMIT || mbi.Protect == PAGE_NOACCESS/*|| !(mbi.Protect & PAGE_READWRITE)*/)
            continue;

        // Reallocate buffer
        if (mbi.RegionSize > bufsize)
        {
            bufsize = static_cast<size_t>(mbi.RegionSize);
            VirtualFree( buf, 0, MEM_RELEASE );
            buf = reinterpret_cast<uint8_t*>(VirtualAlloc( 0, bufsize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE ));
        }

        if (remote.memory().Read( memptr, static_cast<size_t>(mbi.RegionSize), buf ) != STATUS_SUCCESS)
            continue;

        if (useWildcard)
            Search( wildcard, buf, static_cast<size_t>(mbi.RegionSize), out, memptr );
        else
            Search( buf, static_cast<size_t>(mbi.RegionSize), out, memptr );
    }

    VirtualFree( buf, 0, MEM_RELEASE );

    return out.size();
}


}