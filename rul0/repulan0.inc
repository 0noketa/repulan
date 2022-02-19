%define int_size 4
%define int_type_name DWORD
%define data_stack_size 0x100
%define param_stack_size 0x100
%define arg_stack_size 0x100
%define call_stack_size 0x100
%define extern_stack_size 0x100

%define begin_data_stack xchg esp, [data_sp]
%define end_data_stack xchg esp, [data_sp]

; stack of parameter
%define begin_param_stack xchg esp, [param_sp]
%define end_param_stack xchg esp, [param_sp]

; stack of dstack_sp points first arg for spreading_call
%define begin_arg_stack xchg esp, [arg_sp]
%define end_arg_stack xchg esp, [arg_sp]

; spreading_call counter
%define begin_call_stack xchg esp, [call_sp]
%define end_call_stack xchg esp, [call_sp]

; stack for copy of external environment
%define begin_extern_stack xchg esp, [extern_sp]
%define end_extern_stack xchg esp, [extern_sp]

%macro arg_push 1
    begin_arg_stack
    push %1
    end_arg_stack
%endmacro

%macro arg_pop 1
    begin_arg_stack
    pop %1
    end_arg_stack
%endmacro

%macro param_push 1
    begin_param_stack
    push %1
    end_param_stack
%endmacro

%macro param_pop 1
    begin_param_stack
    pop %1
    end_param_stack
%endmacro

%macro call_push 1
    begin_call_stack
    push %1
    end_call_stack
%endmacro

%macro call_pop 1
    begin_call_stack
    pop %1
    end_call_stack
%endmacro

%macro extern_push 1
    begin_extern_stack
    push %1
    end_extern_stack
%endmacro

%macro extern_pop 1
    begin_extern_stack
    pop %1
    end_extern_stack
%endmacro

%macro begin_args 0
    mov eax, esp
    arg_push eax
%endmacro

%macro repulan0_call 1
    end_data_stack
    call %1
    begin_data_stack
%endmacro

%macro repulan0_add 0
    pop edx
    pop eax
    add eax, edx
    push eax
%endmacro

%macro repulan0_sub 0
    pop edx
    pop eax
    xor ebx, ebx
    cmp eax, edx
    cmovb eax, ebx
    cmovb edx, ebx
    sub eax, edx
    push eax
%endmacro

%macro repulan0_range 0
    repulan0_call repulan0_range0
%endmacro

%macro repulan0_begin 0
    begin_extern_stack
    push ecx
    push ebx
    end_extern_stack

    begin_data_stack
%endmacro

%macro repulan0_end 0
    end_data_stack

    begin_extern_stack
    pop ebx
    pop ecx
    end_extern_stack
%endmacro


; global data_stack, data_sp
; global arg_stack, call_stack, param_stack, extern_stack

section .bss
data_stack: resd data_stack_size
arg_stack: resd arg_stack_size
call_stack: resd call_stack_size
param_stack: resd param_stack_size
extern_stack: resd extern_stack_size
section .data
data_sp: dd data_stack+(data_stack_size*int_size)
arg_sp: dd arg_stack+(arg_stack_size*int_size)
call_sp: dd call_stack+(call_stack_size*int_size)
param_sp: dd param_stack+(param_stack_size*int_size)
extern_sp: dd extern_stack+(extern_stack_size*int_size)
section .text

; repulan0_init:
;     mov eax, data_stack+(data_stack_size*int_size)
;     mov [data_sp], eax
;     mov eax, arg_stack+(arg_stack_size*int_size)
;     mov [arg_sp], eax
;     mov eax, call_stack+(call_stack_size*int_size)
;     mov [call_sp], eax
;     mov eax, param_stack+(param_stack_size*int_size)
;     mov [param_sp], eax
;     mov eax, extern_stack+(extern_stack_size*int_size)
;     mov [extern_sp], eax
;     ret

spreading_call:
    begin_data_stack

    ; edx: first arg
    ; call ecx times
    arg_pop edx
    mov ecx, edx
    sub ecx, esp
    shr ecx, 2

    cmp edx, esp
    jz .L2
.L1:
    pop eax
    param_push eax

    cmp edx, esp
    jnz .L1
.L2:
    ; ebx: func
    pop ebx

    or ecx, ecx
    jz .L4
.L3:
    begin_call_stack
    push ebx
    push ecx
    end_call_stack
    repulan0_call ebx
    begin_call_stack
    pop ecx
    pop ebx
    end_call_stack
    dec ecx
    or ecx, ecx
    jnz .L3
.L4:

    end_data_stack
    ret


repulan0_range0:
    begin_data_stack

    pop edx
    pop eax

    cmp eax, edx
    jae .L2
.L1:
    push eax

    inc eax
    cmp eax, edx
    jb .L1
.L2:

    end_data_stack
    ret

