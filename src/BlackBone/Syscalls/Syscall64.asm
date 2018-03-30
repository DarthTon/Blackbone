.code
     
syscall_stub proc
    mov eax, ecx

    ; validate number of arguments
    cmp edx, 1
    jl skip

    mov r10, r8
    cmp edx, 2
    jl skip

    xchg rdx, r9
    cmp r9d, 3
    jl skip

    mov r8, [rsp + 28h]
    cmp r9d, 4
    jl skip

    mov r9, [rsp + 30h]

skip:
    add rsp, 10h    ; skip first 2 args
    syscall
    sub rsp, 10h
    ret
syscall_stub endp

end