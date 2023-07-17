#pragma once
#include "../Misc/Utils.h"
#include "../Misc/Trace.hpp"
#include "../Misc/StackTrace.h"
#include <sstream>
#include <stdexcept>

namespace blackbone
{

class nt_exception : public std::exception
{
public:
    explicit nt_exception( const char* message )
        : _message( message )
        , _status( STATUS_UNSUCCESSFUL )
    {
        capture_stack_trace();
    }

    explicit nt_exception( NTSTATUS status )
        : _status( status )
    {
        _message = Utils::GetErrorDescriptionA( status );
        capture_stack_trace();
    }

    template<typename... Args>
    nt_exception( const char* fmt, Args... args )
        : _status( STATUS_UNSUCCESSFUL )
    {
        char buf[1024] = { };
        sprintf_s( buf, fmt, args... );
        _message = buf;

        capture_stack_trace();
    }

    template<typename... Args>
    nt_exception( NTSTATUS status, const char* fmt, Args... args )
        : _status( status )
    {
        char buf[1024] = { };
        sprintf_s( buf, fmt, args... );

        std::ostringstream printer;
        printer << buf << ". NTSTATUS 0x" << std::hex << status << ": " << Utils::GetErrorDescriptionA( status );
        _message = printer.str();

        capture_stack_trace();
    }

    const char* what() const
    {
        return _message.c_str();
    }

    NTSTATUS status() const
    {
        return _status;
    }

    const char* stack_trace() const
    {
        return _stackTrace.c_str();
    }

private:
    void capture_stack_trace()
    {
#ifdef _DEBUG
        CONTEXT ctx = {};
        RtlCaptureContext( &ctx );
        _stackTrace = debug::StackTrace::Capture( &ctx );
#endif // _DEBUG
    }

private:
    std::string _message;
    std::string _stackTrace;
    NTSTATUS _status;
};

#ifdef _DEBUG
#define LOG_ENTRY "Exception in %s - %s:%d. Message: %s\nStack trace:\n%s", __FUNCTION__, __FILE__, __LINE__, e.what(), e.stack_trace()
#else
#define LOG_ENTRY "Exception in %s - %s:%d. Message: %s", __FUNCTION__, __FILE__, __LINE__, e.what()
#endif

#define THROW_AND_LOG(...) \
{ \
    auto e = nt_exception( __VA_ARGS__ ); \
    BLACKBONE_TRACE( LOG_ENTRY ); \
    throw e; \
}

#define THROW_WITH_STATUS_AND_LOG(status, ...) \
{ \
    auto e = nt_exception( status, __VA_ARGS__ ); \
    BLACKBONE_TRACE( LOG_ENTRY ); \
    throw e; \
}

#define THROW_ON_FAIL_AND_LOG(status, ...) if (!NT_SUCCESS( status )) THROW_WITH_STATUS_AND_LOG( status, __VA_ARGS__ );

}
