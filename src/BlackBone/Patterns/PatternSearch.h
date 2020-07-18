#pragma once

#include "../Include/Types.h"

#include <string>
#include <vector>
#include <functional>
#include <initializer_list>

namespace blackbone
{

class PatternSearch
{
public:
	/// <summary>
	/// Callback to handle a matching address for the Search*WithHandler() methods.
	/// If the handler returns true, the search is stopped, else the search continues.
	/// </summary>
	typedef std::function<bool (ptr_t)> MatchHandler;

public:
	// logAlignment can be used to speed-up the search in some cases. For example, if you know that the start of the pattern
	// is always 8-byte-aligned, you can pass logAlignment=3 (2^3 = 8) to skip searching at all addresses that aren't multiples
	// of 8. Note that for smaller alignments and depending on the exact pattern, this may not always be faster (it may even be
	// a tiny bit slower), so profile it if you care about performance.
    BLACKBONE_API PatternSearch( const std::vector<uint8_t>& pattern, size_t logAlignment = 0 );
    BLACKBONE_API PatternSearch( const std::initializer_list<uint8_t>&& pattern, size_t logAlignment = 0 );
    BLACKBONE_API PatternSearch( const std::string& pattern, size_t logAlignment = 0 );
    BLACKBONE_API PatternSearch( const char* pattern, size_t len = 0, size_t logAlignment = 0 );
    BLACKBONE_API PatternSearch( const uint8_t* pattern, size_t len = 0, size_t logAlignment = 0 );

    BLACKBONE_API ~PatternSearch() = default;

    /// <summary>
    /// Default pattern matching with wildcards and a callback handler for matches.
    /// std::search is approximately 2x faster than naive approach.
    /// </summary>
    /// <param name="wildcard">Pattern wildcard</param>
    /// <param name="scanStart">Starting address</param>
    /// <param name="scanSize">Size of region to scan</param>
    /// <param name="handler">Callback that is called for every match. If it returns true, the search is stopped prematurely.</param>
    /// <param name="value_offset">Value that will be added to resulting addresses</param>
    /// <returns>true if the callback handler ever returned true (i.e. the search ended prematurely), false otherwise.</returns>
    BLACKBONE_API bool SearchWithHandler(
        uint8_t wildcard,
        void* scanStart,
        size_t scanSize,
		MatchHandler handler,
        ptr_t value_offset = 0
        ) const;

    /// <summary>
    /// Full pattern match, no wildcards, with a callback handler for matches.
    /// Uses Boyer–Moore–Horspool algorithm.
    /// </summary>
    /// <param name="scanStart">Starting address</param>
    /// <param name="scanSize">Size of region to scan</param>
    /// <param name="handler">Callback that is called for every match. If it returns true, the search is stopped prematurely.</param>
    /// <param name="value_offset">Value that will be added to resulting addresses</param>
    /// <returns>true if the callback handler ever returned true (i.e. the search ended prematurely), false otherwise.</returns>
    BLACKBONE_API bool SearchWithHandler(
        void* scanStart,
        size_t scanSize,
		MatchHandler handler,
        ptr_t value_offset = 0
        ) const;

    /// <summary>
    /// Search pattern in remote process with a callback handler for matches
    /// </summary>
    /// <param name="remote">Remote process</param>
    /// <param name="wildcard">Pattern wildcard</param>
    /// <param name="scanStart">Starting address</param>
    /// <param name="scanSize">Size of region to scan</param>
    /// <param name="handler">Callback that is called for every match. If it returns true, the search is stopped prematurely.</param>
    /// <returns>true if the callback handler ever returned true (i.e. the search ended prematurely), false otherwise.</returns>
    BLACKBONE_API bool SearchRemoteWithHandler(
        class Process& remote,
        uint8_t wildcard,
        ptr_t scanStart,
        size_t scanSize,
        MatchHandler handler
        ) const;

    /// <summary>
    /// Search pattern in remote process with a callback handler for matches
    /// </summary>
    /// <param name="remote">Remote process</param>
    /// <param name="scanStart">Starting address</param>
    /// <param name="scanSize">Size of region to scan</param>
    /// <param name="handler">Callback that is called for every match. If it returns true, the search is stopped prematurely.</param>
    /// <returns>true if the callback handler ever returned true (i.e. the search ended prematurely), false otherwise.</returns>
    BLACKBONE_API bool SearchRemoteWithHandler(
        class Process& remote,
        ptr_t scanStart,
        size_t scanSize,
        MatchHandler handler
        ) const;

    /// <summary>
    /// Search pattern in whole address space of remote process with a callback handler for matches
    /// </summary>
    /// <param name="remote">Remote process</param>
    /// <param name="useWildcard">True if pattern contains wildcards</param>
    /// <param name="wildcard">Pattern wildcard</param>
    /// <param name="handler">Callback that is called for every match. If it returns true, the search is stopped prematurely.</param>
    /// <returns>true if the callback handler ever returned true (i.e. the search ended prematurely), false otherwise.</returns>
    BLACKBONE_API bool SearchRemoteWholeWithHandler(
        class Process& remote,
        bool useWildcard,
        uint8_t wildcard,
        MatchHandler handler
        ) const;






    /// <summary>
    /// Default pattern matching with wildcards.
    /// std::search is approximately 2x faster than naive approach.
    /// </summary>
    /// <param name="wildcard">Pattern wildcard</param>
    /// <param name="scanStart">Starting address</param>
    /// <param name="scanSize">Size of region to scan</param>
    /// <param name="out">Found results</param>
    /// <param name="value_offset">Value that will be added to resulting addresses</param>
    /// <param name="maxMatches">Maximum number of matches to collect</param>
    /// <returns>Number of found addresses</returns>
    BLACKBONE_API size_t Search( 
        uint8_t wildcard, 
        void* scanStart, 
        size_t scanSize, 
        std::vector<ptr_t>& out, 
        ptr_t value_offset = 0,
		size_t maxMatches = SIZE_MAX
        ) const;

    /// <summary>
    /// Full pattern match, no wildcards.
    /// Uses Boyer–Moore–Horspool algorithm.
    /// </summary>
    /// <param name="scanStart">Starting address</param>
    /// <param name="scanSize">Size of region to scan</param>
    /// <param name="out">Found results</param>
    /// <param name="value_offset">Value that will be added to resulting addresses</param>
    /// <param name="maxMatches">Maximum number of matches to collect</param>
    /// <returns>Number of found addresses</returns>
    BLACKBONE_API size_t Search( 
        void* scanStart, 
        size_t scanSize, 
        std::vector<ptr_t>& out, 
        ptr_t value_offset = 0,
		size_t maxMatches = SIZE_MAX
        ) const;

    /// <summary>
    /// Search pattern in remote process
    /// </summary>
    /// <param name="remote">Remote process</param>
    /// <param name="wildcard">Pattern wildcard</param>
    /// <param name="scanStart">Starting address</param>
    /// <param name="scanSize">Size of region to scan</param>
    /// <param name="out">Found results</param>
    /// <param name="maxMatches">Maximum number of matches to collect</param>
    /// <returns>Number of found addresses</returns>
    BLACKBONE_API size_t SearchRemote( 
        class Process& remote, 
        uint8_t wildcard, 
        ptr_t scanStart,
        size_t scanSize, 
        std::vector<ptr_t>& out,
		size_t maxMatches = SIZE_MAX
        ) const;

    /// <summary>
    /// Search pattern in remote process
    /// </summary>
    /// <param name="remote">Remote process</param>
    /// <param name="scanStart">Starting address</param>
    /// <param name="scanSize">Size of region to scan</param>
    /// <param name="out">Found results</param>
    /// <param name="maxMatches">Maximum number of matches to collect</param>
    /// <returns>Number of found addresses</returns>
    BLACKBONE_API size_t SearchRemote( 
        class Process& remote, 
        ptr_t scanStart, 
        size_t scanSize, 
        std::vector<ptr_t>& out,
		size_t maxMatches = SIZE_MAX
        ) const;

    /// <summary>
    /// Search pattern in whole address space of remote process
    /// </summary>
    /// <param name="remote">Remote process</param>
    /// <param name="useWildcard">True if pattern contains wildcards</param>
    /// <param name="wildcard">Pattern wildcard</param>
    /// <param name="out">Found results</param>
    /// <param name="maxMatches">Maximum number of matches to collect</param>
    /// <returns>Number of found addresses</returns>
    BLACKBONE_API size_t SearchRemoteWhole( 
        class Process& remote, 
        bool useWildcard, 
        uint8_t wildcard, 
        std::vector<ptr_t>& out,
		size_t maxMatches = SIZE_MAX
        ) const;

private:
    static inline bool collectAllMatchHandler(ptr_t addr, std::vector<ptr_t>& out, size_t maxMatches)
    {
    	out.emplace_back(addr);
		return out.size() >= maxMatches;
    }

private:
    std::vector<uint8_t> _pattern;      // Pattern to search
    size_t logAlignment;
};

}
