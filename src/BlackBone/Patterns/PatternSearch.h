#pragma once

#include "../Include/Types.h"

#include <string>
#include <vector>
#include <initializer_list>

namespace blackbone
{

class PatternSearch
{
public:
    BLACKBONE_API PatternSearch( const std::vector<uint8_t>& pattern );
    BLACKBONE_API PatternSearch( const std::initializer_list<uint8_t>&& pattern );
    BLACKBONE_API PatternSearch( const std::string& pattern );
    BLACKBONE_API PatternSearch( const char* pattern, size_t len = 0 );
    BLACKBONE_API PatternSearch( const uint8_t* pattern, size_t len = 0 );

    BLACKBONE_API ~PatternSearch();

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
    BLACKBONE_API size_t Search( uint8_t wildcard, void* scanStart, size_t scanSize, std::vector<ptr_t>& out, ptr_t value_offset = 0 );

    /// <summary>
    /// Full pattern match, no wildcards.
    /// Uses Boyer–Moore–Horspool algorithm.
    /// </summary>
    /// <param name="scanStart">Starting address</param>
    /// <param name="scanSize">Size of region to scan</param>
    /// <param name="out">Found results</param>
    /// <param name="value_offset">Value that will be added to resulting addresses</param>
    /// <returns>Number of found addresses</returns>
    BLACKBONE_API size_t Search( void* scanStart, size_t scanSize, std::vector<ptr_t>& out, ptr_t value_offset = 0 );

    /// <summary>
    /// Search pattern in remote process
    /// </summary>
    /// <param name="remote">Remote process</param>
    /// <param name="wildcard">Pattern wildcard</param>
    /// <param name="scanStart">Starting address</param>
    /// <param name="scanSize">Size of region to scan</param>
    /// <param name="out">Found results</param>
    /// <returns>Number of found addresses</returns>
    BLACKBONE_API size_t SearchRemote( class Process& remote, uint8_t wildcard, ptr_t scanStart, size_t scanSize, std::vector<ptr_t>& out );

    /// <summary>
    /// Search pattern in remote process
    /// </summary>
    /// <param name="remote">Remote process</param>
    /// <param name="scanStart">Starting address</param>
    /// <param name="scanSize">Size of region to scan</param>
    /// <param name="out">Found results</param>
    /// <returns>Number of found addresses</returns>
    BLACKBONE_API size_t SearchRemote( class Process& remote, ptr_t scanStart, size_t scanSize, std::vector<ptr_t>& out );

    /// <summary>
    /// Search pattern in whole address space of remote process
    /// </summary>
    /// <param name="remote">Remote process</param>
    /// <param name="useWildcard">True if pattern contains wildcards</param>
    /// <param name="wildcard">Pattern wildcard</param>
    /// <param name="out">Found results</param>
    /// <returns>Number of found addresses</returns>
    BLACKBONE_API size_t SearchRemoteWhole( class Process& remote, bool useWildcard, uint8_t wildcard, std::vector<ptr_t>& out );

private:
    std::vector<uint8_t> _pattern;      // Pattern to search
};

}