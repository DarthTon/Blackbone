import ctypes as c
from ctypes import wintypes as w
import enum

# Github : x544D

class PythonicBlackBone():
    
    class DataTypes(enum.Enum):
        BOOL    = 0
        INT16   = 1
        INT32   = 2
        INT64   = 3
        FLOAT   = 4
        DOUBLE  = 5
        LONG    = 6
        ULONG   = 7
        LLONG   = 8
        ULLONG  = 9
        SIZE_T  = 10
        CHAR    = 11
        BYTE    = 12
        WCHAR   = 13
        VOIDP   = 14


    def ParseType(self, i , value=None):
        _ = {
            0:c.c_bool,
            1:c.c_int16,
            2:c.c_int32,
            3:c.c_int64,
            4:c.c_float,
            5:c.c_double,
            6:c.c_long,
            7:c.c_ulong,
            8:c.c_longlong,
            9:c.c_ulonglong,
            10:c.c_size_t,
            11:c.c_char,
            12:c.c_byte,
            13:c.c_wchar,
            14:c.c_void_p,
        }
        F=_[i]
        if value:return F(value=value)
        else: return F()



    def __init__(self, ProcessId , DesiredAccess=0x000F0000|0x00100000|0xFFF):
        if ProcessId is None:
            print("+ Please Give a valid PID .")
            exit(0)

        print("\t[ AN EASY WIN32API PYTHON WRAPPER CTYPE BASED ]\n\t- This is still under Dev .. !")
        self.pid    = ProcessId
        self.access = DesiredAccess

        self.k32    = c.windll.kernel32
        self.OpenProcess = self.k32.OpenProcess
        self.OpenProcess.argtypes = [w.DWORD,w.BOOL,w.DWORD]
        self.OpenProcess.restype = w.HANDLE

        self.ReadProcessMemory = self.k32.ReadProcessMemory
        self.ReadProcessMemory.argtypes = [w.HANDLE,w.LPCVOID,w.LPVOID,c.c_size_t,c.POINTER(c.c_size_t)]
        self.ReadProcessMemory.restype = w.BOOL

        self.WriteProcessMemory = self.k32.WriteProcessMemory
        self.WriteProcessMemory.argtypes = [w.HANDLE,w.LPCVOID,w.LPVOID,c.c_size_t,c.POINTER(c.c_size_t)]
        self.WriteProcessMemory.restype = w.BOOL

        self.GetLastError = self.k32.GetLastError
        self.GetLastError.argtypes = None
        self.GetLastError.restype = w.DWORD

        self.CloseHandle = self.k32.CloseHandle
        self.CloseHandle.argtypes = [w.HANDLE]
        self.CloseHandle.restype = w.BOOL

        self.hProc = self.OpenProcess(self.access , False, self.pid)
        if not self.hProc:
            print('+ Failed To open a handle to the Target Process .')
            exit(0)

    def CheckLastError(self):
        return self.GetLastError()

    def DestroyHandle(self):
        self.CloseHandle(self.hProc)
        del self

    def RPM(self, address, data):
        ''' ReadProcessMemory'''
        return self.ReadProcessMemory(self.hProc, address, c.byref(data) , c.sizeof(data), None)

    def WPM(self, address, data):
        ''' WriteProcessMemory '''
        return self.WriteProcessMemory(self.hProc, address, c.byref(data) , c.sizeof(data), None)

    def __del__(self):
        print(f"+ Instance {type(self).__name__} Destroyed .")



if __name__ == "__main__":
    print('+ Please Intanciate the Class first .')
    exit(0)