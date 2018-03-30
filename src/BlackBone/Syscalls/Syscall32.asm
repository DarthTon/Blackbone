.MODEL flat
.code

x64_enter proc
    retf
x64_enter endp

x64_exit proc
    mov dword ptr [esp + 4], 23h
    retf
x64_exit endp

     
_syscall_stub proc
    push ebp
    mov ebp, esp
    sub esp, 20h

    push 33h
    call x64_enter

    db 48h
    mov eax, 0DEADBEEFh

    call x64_exit                             

    mov dx, ds
    mov ss, dx

    mov esp, ebp
    pop ebp
    ret
_syscall_stub endp

end