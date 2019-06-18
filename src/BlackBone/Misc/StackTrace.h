#pragma once
#include "../Config.h"
#include "../Include/Winheaders.h"
#include <string>
#include <sstream>
#include <vector>
#include <Psapi.h>
#include <algorithm>
#include <iterator>

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "dbghelp.lib")

namespace blackbone::debug
{

struct module_data 
{
    std::string image_name;
    std::string module_name;
    void* base_address ;
    DWORD load_size = 0;

    explicit module_data( void* base = nullptr )
        : base_address( base )
    {
    }

    bool operator <( const module_data& rhs ) const
    {
        return base_address < rhs.base_address;
    }
};

class symbol 
{
public:
    symbol( HANDLE process, DWORD64 address )
    {
        memset( &_sym, 0x00, sizeof( _reserved ) );

        _sym.SizeOfStruct = sizeof( _sym );
        _sym.MaxNameLength = max_name_len;

        SymGetSymFromAddr64( process, address, &_displacement, &_sym );
    }

    std::string name() const
    {
        return _sym.Name;
    }

    std::string undecorated_name() const
    {
        if (*_sym.Name == '\0')
            return "<couldn't map PC to fn name>";

        std::vector<char> und_name( max_name_len );
        UnDecorateSymbolName( _sym.Name, &und_name[0], max_name_len, UNDNAME_COMPLETE );
        return std::string( &und_name[0], strlen( &und_name[0] ) );
    }

    DWORD64 offset( DWORD64 pc )
    {
        return pc - _sym.Address;
    }

private:
    static constexpr int max_name_len = 1024;
    DWORD64 _displacement = 0;
    union
    {
        IMAGEHLP_SYMBOL64 _sym;
        uint8_t _reserved[sizeof( IMAGEHLP_SYMBOL64 ) + max_name_len];
    };
};

class get_mod_info 
{
public:
    get_mod_info( HANDLE process )
        : _process( process )
    {
    }

    module_data operator()( HMODULE module )
    {
        module_data ret;
        char temp[buffer_length] = { };
        MODULEINFO mi = { };

        GetModuleInformation( _process, module, &mi, sizeof( mi ) );
        ret.base_address = mi.lpBaseOfDll;
        ret.load_size = mi.SizeOfImage;

        GetModuleFileNameExA( _process, module, temp, sizeof( temp ) );
        ret.image_name = temp;

        GetModuleBaseNameA( _process, module, temp, sizeof( temp ) );
        ret.module_name = temp;

        SymLoadModule64( 
            _process, nullptr, 
            ret.image_name.data(), 
            ret.module_name.data(), 
            reinterpret_cast<DWORD64>(ret.base_address), 
            ret.load_size 
            );

        return ret;
    }

private:
    HANDLE _process = nullptr;
    static constexpr int buffer_length = 4096;
};

class StackTrace
{
public:
    static std::string Capture( PCONTEXT ctx )
    {
        HANDLE process = GetCurrentProcess();
        HANDLE thread = GetCurrentThread();

        std::ostringstream result;

        if (!SymInitializeW( process, nullptr, false ))
            return "Error. Failed to initialize symbols";

        DWORD symOptions = SymGetOptions();
        symOptions |= SYMOPT_LOAD_LINES | SYMOPT_UNDNAME;
        SymSetOptions( symOptions );

        auto modules = load_modules( process );
        DWORD image_type = ImageNtHeader( GetModuleHandleW( nullptr ) )->FileHeader.Machine;
        DWORD64 lastPC = 0;

#ifdef USE64
        int skipCount = 2;
#else
        int skipCount = 1;
#endif

        for(STACKFRAME64 frame = first_frame( ctx ); frame.AddrReturn.Offset != 0;)
        {
            // Sometimes same frame gets walked twice, don't print it second time
            if(lastPC != frame.AddrPC.Offset)
            {
                if (skipCount <= 0)
                    dump_frame( process, modules, frame, result );
                else
                    skipCount--;
            }

            lastPC = frame.AddrPC.Offset;
            if (!StackWalk64( image_type, process, thread, &frame, ctx, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr ))
                break;
        }

        SymCleanup( process );

        return result.str();
    }

private:
    static std::vector<module_data> load_modules( HANDLE process )
    {
        DWORD cbNeeded = 0;
        std::vector<module_data> modules;
        std::vector<HMODULE> module_handles( 1 );

        EnumProcessModules( process, &module_handles[0], static_cast<DWORD>(module_handles.size() * sizeof( HMODULE )), &cbNeeded );
        module_handles.resize( cbNeeded / sizeof( HMODULE ) );
        EnumProcessModules( process, &module_handles[0], static_cast<DWORD>(module_handles.size() * sizeof( HMODULE )), &cbNeeded );

        std::transform( module_handles.begin(), module_handles.end(), std::back_inserter( modules ), get_mod_info( process ) );
        std::sort( modules.begin(), modules.end(), std::less<module_data>() );

        return modules;
    }

    static module_data find_module( const std::vector<module_data>& modules, void* pc )
    {
        auto range = std::upper_bound( modules.begin(), modules.end(), module_data( pc ) );
        if (range != modules.end())
        {
            if(range != modules.begin())
                range--;

            if (range->base_address <= pc && pc < static_cast<uint8_t*>(range->base_address) + range->load_size)
                return *range;
        }

        return module_data();
    }

    static STACKFRAME64 first_frame( PCONTEXT ctx )
    {
        STACKFRAME64 frame = { };

#ifdef USE64
        frame.AddrPC.Offset = ctx->Rip;
        frame.AddrStack.Offset = ctx->Rsp;
        frame.AddrFrame.Offset = ctx->Rbp;
#else
        frame.AddrPC.Offset = ctx->Eip;
        frame.AddrStack.Offset = ctx->Esp;
        frame.AddrFrame.Offset = ctx->Ebp;
#endif
        frame.AddrPC.Mode = AddrModeFlat;
        frame.AddrStack.Mode = AddrModeFlat;
        frame.AddrFrame.Mode = AddrModeFlat;

        // Dummy value
        frame.AddrReturn.Offset = 0xDEAD;

        return frame;
    }

    static void dump_frame( HANDLE process, const std::vector<module_data>& modules, const STACKFRAME64& frame, std::ostringstream& printer )
    {
        if (frame.AddrPC.Offset != 0)
        {
            printer << "  ";

            auto mod = find_module( modules, reinterpret_cast<void*>(frame.AddrPC.Offset) );
            if (!mod.module_name.empty())
                printer << mod.module_name << "!";

            symbol sym( process, frame.AddrPC.Offset );
            printer << sym.undecorated_name();

            IMAGEHLP_LINE64 line = { };
            line.SizeOfStruct = sizeof( line );
            DWORD offset;

            if (SymGetLineFromAddr64( process, frame.AddrPC.Offset, &offset, &line ))
                printer << "  " << line.FileName << ":" << line.LineNumber;

            printer << std::endl;
        }
        else
            printer << "(No Symbols: PC == 0)";
    }
};

}
