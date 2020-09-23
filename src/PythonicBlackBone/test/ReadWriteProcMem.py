import sys
sys.path.append("..")
from BlackBone import PythonicBlackBone


# Intance of BlackBone python Classs
# Constructor Takes 2 Params ProcId and dwDesiredAccess (default = PROCESS_ALL_ACCESS )
bb=PythonicBlackBone(4284)

# Address to read From
addr = 0x0063DE0C

# data is the Variable that will end up holding the Read Value from that address 
# Tho, we need to Parse it to a C_TYPE in this case 'Double' you can find more on DataTypes 
data = bb.ParseType(bb.DataTypes.DOUBLE.value)

# Result of ReadProcessMemory (Bool)
res = bb.RPM(addr , data)

print('+ RPM result: {} - err code: {}'.format(res,bb.CheckLastError()))
print('data: {}\n'.format(data.value))

# in Case of WriteProcessMemory we can directly parse our value as a C_Type , simply pass it as the second param 
# in this case we want to write 1.5
_wdata = bb.ParseType(bb.DataTypes.FLOAT.value, 1.5)
res = bb.WPM(addr, _wdata)

print('+WPM result: {} - err code: {}'.format(res,bb.CheckLastError()))
print('data: {}'.format(_wdata.value))

# This will CloseHandle , and Invoke the built in __del__ function .
bb.DestroyHandle()