#include "PatternSearch.h"
#include "../Include/Macro.h"
#include "../Include/Winheaders.h"
#include "../Process/Process.h"

#include <algorithm>
#include <memory>

namespace blackbone
{

PatternSearch::PatternSearch( const std::vector<uint8_t>& pattern, size_t logAlignment /*= 0*/ )
    : _pattern( pattern ), logAlignment(logAlignment)
{
}

PatternSearch::PatternSearch( const std::initializer_list<uint8_t>&& pattern, size_t logAlignment /*= 0*/ )
    : _pattern( pattern ), logAlignment(logAlignment)
{
}

PatternSearch::PatternSearch( const std::string& pattern, size_t logAlignment /*= 0*/ )
    : _pattern( pattern.begin(), pattern.end() ), logAlignment(logAlignment)
{
}

PatternSearch::PatternSearch( const char* pattern, size_t len /*= 0*/, size_t logAlignment /*= 0*/ )
    : _pattern( pattern, pattern + (len ? len : strlen( pattern )) ), logAlignment(logAlignment)
{
}

PatternSearch::PatternSearch( const uint8_t* pattern, size_t len /*= 0*/, size_t logAlignment /*= 0*/ )
    : _pattern( pattern, pattern + (len ? len : strlen( (const char*)pattern )) ), logAlignment(logAlignment)
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
bool PatternSearch::SearchWithHandler(
    uint8_t wildcard,
        void* scanStart,
        size_t scanSize,
		MatchHandler handler,
        ptr_t value_offset /*= 0*/
    ) const
{
    const uint8_t* cstart = (const uint8_t*)scanStart;
    const uint8_t* cend   = cstart + scanSize;

    // TODO: Would it be beneficial to use logAlignment here as well?

    auto comparer = [&wildcard]( uint8_t val1, uint8_t val2 )
    {
        return (val1 == val2 || val2 == wildcard);
    };

    bool running = true;
    while (running)
    {
        const uint8_t* res = std::search( cstart, cend, _pattern.begin(), _pattern.end(), comparer );
        if (res >= cend)
            break;

        if (value_offset != 0)
        	running = !handler( REBASE( res, scanStart, value_offset ) );
            //out.emplace_back( REBASE( res, scanStart, value_offset ) );
        else
            //out.emplace_back( reinterpret_cast<ptr_t>(res) );
        	running = !handler( reinterpret_cast<ptr_t>(res) );

        cstart = res + _pattern.size();
    }

    return !running;
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
bool PatternSearch::SearchWithHandler(
	void* scanStart,
	size_t scanSize,
	MatchHandler handler,
	ptr_t value_offset /*= 0*/
	) const
{
	size_t bad_char_skip[UCHAR_MAX + 1];

    const uint8_t* haystack = reinterpret_cast<const uint8_t*>(scanStart);
    const uint8_t* haystackEnd = haystack + scanSize - _pattern.size();
    const uint8_t* needle   = &_pattern[0];
    uintptr_t       nlen     = _pattern.size();
    uintptr_t       scan     = 0;
    uintptr_t       last     = nlen - 1;
    size_t alignMask		 = 0xFFFFFFFFFFFFFFFFL << logAlignment;
    size_t alignOffs		 = (1 << logAlignment) - 1;

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
    bool running = true;
    //while (haystack <= haystackEnd  &&  out.size() < maxMatches)
    while (haystack <= haystackEnd  &&  running)
    {
        for (scan = last; haystack[scan] == needle[scan]; --scan)
        {
            if (scan == 0)
            {
                if (value_offset != 0)
                    //out.emplace_back( REBASE( haystack, scanStart, value_offset ) );
                	running = !handler( REBASE( haystack, scanStart, value_offset ) );
                else
                    //out.emplace_back( reinterpret_cast<ptr_t>(haystack) );
                	running = !handler( reinterpret_cast<ptr_t>(haystack) );

                break;
            }
        }

        haystack += bad_char_skip[haystack[last]];

        if (logAlignment != 0) {
        	haystack = (const uint8_t*) (size_t(haystack+alignOffs) & alignMask);
        }
    }

    return !running;
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
bool PatternSearch::SearchRemoteWithHandler(
    Process& remote,
    uint8_t wildcard,
    ptr_t scanStart,
    size_t scanSize,
    MatchHandler handler
    ) const
{
    uint8_t *pBuffer = reinterpret_cast<uint8_t*>(VirtualAlloc( NULL, scanSize, MEM_COMMIT, PAGE_READWRITE ));

    bool stopped = false;
    if (pBuffer && remote.memory().Read( scanStart, scanSize, pBuffer ) == STATUS_SUCCESS)
    	stopped = SearchWithHandler( wildcard, pBuffer, scanSize, handler, scanStart );

    if (pBuffer)
        VirtualFree( pBuffer, 0, MEM_RELEASE );

    return stopped;
}

/// <summary>
/// Search pattern in remote process
/// </summary>
/// <param name="remote">Remote process</param>
/// <param name="scanStart">Starting address</param>
/// <param name="scanSize">Size of region to scan</param>
/// <param name="out">Found results</param>
/// <returns>Number of found addresses</returns>
bool PatternSearch::SearchRemoteWithHandler(
    Process& remote,
    ptr_t scanStart,
    size_t scanSize,
    MatchHandler handler
    ) const
{
    uint8_t *pBuffer = reinterpret_cast<uint8_t*>(VirtualAlloc( NULL, scanSize, MEM_COMMIT, PAGE_READWRITE ));

    bool stopped = false;
    if (pBuffer && remote.memory().Read( scanStart, scanSize, pBuffer ) == STATUS_SUCCESS)
    	stopped = SearchWithHandler( pBuffer, scanSize, handler, scanStart );

    if (pBuffer)
        VirtualFree( pBuffer, 0, MEM_RELEASE );

    return stopped;
}

/// <summary>
/// Search pattern in whole address space of remote process
/// </summary>
/// <param name="remote">Remote process</param>
/// <param name="useWildcard">True if pattern contains wildcards</param>
/// <param name="wildcard">Pattern wildcard</param>
/// <param name="out">Found results</param>
/// <returns>Number of found addresses</returns>
bool PatternSearch::SearchRemoteWholeWithHandler(
    Process& remote,
    bool useWildcard,
    uint8_t wildcard,
    MatchHandler handler
    ) const
{
    MEMORY_BASIC_INFORMATION64 mbi = { 0 };
    size_t  bufsize = 1 * 1024 * 1024;  // 1 MB
    uint8_t *buf = reinterpret_cast<uint8_t*>(VirtualAlloc( 0, bufsize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE ));

    auto native = remote.core().native();

    bool running = true;
    for (ptr_t memptr = native->minAddr(); memptr < native->maxAddr()  &&  running; memptr = mbi.BaseAddress + mbi.RegionSize)
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
        	running = !SearchWithHandler( wildcard, buf, static_cast<size_t>(mbi.RegionSize), handler, memptr );
        else
        	running = !SearchWithHandler( buf, static_cast<size_t>(mbi.RegionSize), handler, memptr );
    }

    VirtualFree( buf, 0, MEM_RELEASE );

    return !running;
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
size_t PatternSearch::Search(
    uint8_t wildcard,
    void* scanStart,
    size_t scanSize,
    std::vector<ptr_t>& out,
    ptr_t value_offset /*= 0*/,
	size_t maxMatches /*= SIZE_MAX*/
    ) const
{
	if (out.size() >= maxMatches)
		return out.size();

	auto handler = std::bind(PatternSearch::collectAllMatchHandler, std::placeholders::_1, std::ref(out), maxMatches);
	SearchWithHandler(wildcard, scanStart, scanSize, handler, value_offset);

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
size_t PatternSearch::Search( 
    void* scanStart,
    size_t scanSize, 
    std::vector<ptr_t>& out,
    ptr_t value_offset /*= 0*/,
	size_t maxMatches /*= SIZE_MAX*/
    ) const
{
	if (out.size() >= maxMatches)
		return out.size();

	auto handler = std::bind(PatternSearch::collectAllMatchHandler, std::placeholders::_1, std::ref(out), maxMatches);
	SearchWithHandler(scanStart, scanSize, handler, value_offset);

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
size_t PatternSearch::SearchRemote( 
    Process& remote, 
    uint8_t wildcard, 
    ptr_t scanStart, 
    size_t scanSize, 
    std::vector<ptr_t>& out,
	size_t maxMatches /*= SIZE_MAX*/
    ) const
{
	if (out.size() >= maxMatches)
		return out.size();

	auto handler = std::bind(PatternSearch::collectAllMatchHandler, std::placeholders::_1, std::ref(out), maxMatches);
	SearchRemoteWithHandler(remote, wildcard, scanStart, scanSize, handler);

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
size_t PatternSearch::SearchRemote( 
    Process& remote, 
    ptr_t scanStart, 
    size_t scanSize, 
    std::vector<ptr_t>& out,
	size_t maxMatches /*= SIZE_MAX*/
    ) const
{
	if (out.size() >= maxMatches)
		return out.size();

	auto handler = std::bind(PatternSearch::collectAllMatchHandler, std::placeholders::_1, std::ref(out), maxMatches);
	SearchRemoteWithHandler(remote, scanStart, scanSize, handler);

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
size_t PatternSearch::SearchRemoteWhole( 
    Process& remote, 
    bool useWildcard, 
    uint8_t wildcard, 
    std::vector<ptr_t>& out,
	size_t maxMatches /*= SIZE_MAX*/
    ) const
{
	out.clear();

	if (out.size() >= maxMatches)
		return out.size();

	auto handler = std::bind(PatternSearch::collectAllMatchHandler, std::placeholders::_1, std::ref(out), maxMatches);
	SearchRemoteWholeWithHandler(remote, useWildcard, wildcard, handler);

	return out.size();
}


}
