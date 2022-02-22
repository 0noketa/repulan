%define int_size 4
%define int_type_name DWORD
%define data_stack_size 0x100
%define param_stack_size 0x100
%define arg0_stack_size 0x100
%define temp_stack_size 0x100
%define extern_stack_size 0x100
%define array_block_size_bits 7
%define array_block_size (1 << (array_block_size_bits-1))
%define max_arrays 0x20
%define array_stack_size 0x100


%define array_buf_size (max_arrays*array_block_size)

%ifdef use_auto_vars
%define begin_data_stack xchg esp, [data_sp]
%define end_data_stack xchg esp, [data_sp]
%else
%define begin_data_stack xchg esp, ebp
%define end_data_stack xchg esp, ebp
%endif

; stack of parameter
%define begin_param_stack xchg esp, [param_sp]
%define end_param_stack xchg esp, [param_sp]

; stack of dstack_sp points first arg for spreading_call
%define begin_arg0_stack xchg esp, [arg0_sp]
%define end_arg0_stack xchg esp, [arg0_sp]

; saves spreading_call counter while call. and used in array functions.
%define begin_temp_stack xchg esp, [temp_sp]
%define end_temp_stack xchg esp, [temp_sp]

; stack for copy of external environment
%define begin_extern_stack xchg esp, [extern_sp]
%define end_extern_stack xchg esp, [extern_sp]

; stack for first element of new array
%define begin_array_stack xchg esp, [array_sp]
%define end_array_stack xchg esp, [array_sp]

%macro arg0_push 1
    begin_arg0_stack
    push %1
    end_arg0_stack
%endmacro

%macro arg0_pop 1
    begin_arg0_stack
    pop %1
    end_arg0_stack
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

%macro temp_push 1
    begin_temp_stack
    push %1
    end_temp_stack
%endmacro

%macro temp_pop 1
    begin_temp_stack
    pop %1
    end_temp_stack
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

%macro array_push 1
    begin_array_stack
    push %1
    end_array_stack
%endmacro

%macro array_pop 1
    begin_array_stack
    pop %1
    end_array_stack
%endmacro

%macro begin_args 0
    pop eax
    arg0_push eax
    mov eax, esp
    arg0_push eax
%endmacro

%macro begin_array 0
    mov eax, esp
    array_push eax
%endmacro

%macro rul0_call 1
    end_data_stack
    call %1
    begin_data_stack
%endmacro

%macro rul0_add 0
    pop edx
    pop eax
    add eax, edx
    push eax
%endmacro

%macro rul0_sub 0
    pop edx
    pop eax
    xor ebx, ebx
    cmp eax, edx
    cmovb eax, ebx
    cmovb edx, ebx
    sub eax, edx
    push eax
%endmacro

%macro rul0_swap 0
    pop eax
    pop edx
    push eax
    push edx
%endmacro

%macro rul0_dup 0
    pop eax
    push eax
    push eax
%endmacro

%macro rul0_drop 0
    pop eax
%endmacro

%macro rul0_range 0
    rul0_call rul0_range0
%endmacro

%macro rul0_begin 0
    begin_extern_stack
    push ecx
    push ebx
    end_extern_stack

%ifndef use_auto_vars
    xchg ebp, [data_sp]
%endif

    begin_data_stack
%endmacro

%macro rul0_end 0
    end_data_stack

%ifndef use_auto_vars
    xchg ebp, [data_sp]
%endif

    begin_extern_stack
    pop ebx
    pop ecx
    end_extern_stack
%endmacro


%macro rul0_alloc_auto_vars 1
    push ebp
    mov ebp, esp
    sub esp, (%1*int_size)
%endmacro
%macro rul0_dealloc_auto_vars 0
    mov esp, ebp
    pop ebp
%endmacro

%macro wrap_c_func 2
%1:
    begin_data_stack
    param_pop edx

    rul0_end

    push edx
    call %2
    pop edx

    rul0_begin

    push eax

    end_data_stack
    ret
%endmacro

; global data_stack, data_sp
; global arg0_stack, temp_stack, param_stack, extern_stack

section .bss
data_stack: resd data_stack_size
arg0_stack: resd arg0_stack_size
temp_stack: resd temp_stack_size
param_stack: resd param_stack_size
extern_stack: resd extern_stack_size
section .data
data_sp: dd data_stack+(data_stack_size*int_size)
arg0_sp: dd arg0_stack+(arg0_stack_size*int_size)
temp_sp: dd temp_stack+(temp_stack_size*int_size)
param_sp: dd param_stack+(param_stack_size*int_size)
extern_sp: dd extern_stack+(extern_stack_size*int_size)
section .text

; repulan0_init:
;     mov eax, data_stack+(data_stack_size*int_size)
;     mov [data_sp], eax
;     mov eax, arg0_stack+(arg0_stack_size*int_size)
;     mov [arg0_sp], eax
;     mov eax, temp_stack+(temp_stack_size*int_size)
;     mov [temp_sp], eax
;     mov eax, param_stack+(param_stack_size*int_size)
;     mov [param_sp], eax
;     mov eax, extern_stack+(extern_stack_size*int_size)
;     mov [extern_sp], eax
;     ret

spreading_call:
    begin_data_stack

    ; edx: first arg
    ; call ecx times
    arg0_pop edx
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
    arg0_pop ebx

    or ecx, ecx
    jz .L4
.L3:
    begin_temp_stack
    push ebx
    push ecx
    end_temp_stack
    rul0_call ebx
    begin_temp_stack
    pop ecx
    pop ebx
    end_temp_stack
    dec ecx
    or ecx, ecx
    jnz .L3
.L4:

    end_data_stack
    ret


rul0_range0:
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



%ifdef use_array

;; array values are functions
;; array(-1) returns length
;; array(-2) returns number of array. if array was constant, returns -1
;; array {length, element0, ...}

section .bss
array_buf: resd array_block_size*max_arrays
array_stack: resd array_stack_size

section .data
array_sizes: times max_arrays dd 0xFFFFFFFF
wrapped_arrays:
%assign i 0
%rep max_arrays
    dd wrapped_array_ %+ i
%assign i i+1
%endrep
array_sp: dd array_stack+(array_stack_size*int_size)

section .text

alloc_array:
    begin_data_stack

    array_pop edx
    array_push edx
    cmp edx, esp
    jle .L_EMPTY

    mov edx, (max_arrays*int_size)
.L_LOOP:
    sub edx, int_size
    mov eax, [array_sizes+edx]

    inc eax
    or eax, eax
    jz .L_FOUND

    or edx, edx
    jnz .L_LOOP

    ; failed to alloc
    jmp .L_EMPTY
.L_FOUND:
    ; results

    ; size
    array_pop eax
    array_push eax
    sub eax, esp
    shr eax, 2
    mov [array_sizes+edx], eax
    ; mov [new_array_size], eax
    shl eax, 2

    ; edx: offset of int32[*]

    ; addr of function (result of this function)
    temp_push int_type_name[wrapped_arrays+edx]
    ; mov [new_array_offset], edx
    shr edx, 2
    ; mov [new_array_idx], edx

    ; edx: index of int32[*]

    ; eax: first element addr+4 on the data stack
    array_pop eax
    array_push eax

    cmp eax, esp
    jle .L_END_COPY

    ; edx: element addr on result array
    shl edx, array_block_size_bits-1
    ; mov [new_array_offset], edx
    add edx, array_buf

    ; edx: &a[block_idx][0]  //uint32 a[max_arrays+1][block_size]
.L_COPY:
    sub eax, int_size

    push int_type_name[eax]
    pop int_type_name[edx]

    add edx, int_size

    cmp eax, esp
    jg .L_COPY
.L_END_COPY:
    array_pop edx
    mov esp, edx
    temp_pop eax
    push eax

    end_data_stack
    ret
.L_EMPTY:
    push wrapped_array_empty
    end_data_stack
    ret


deallocate_array:
    begin_data_stack

    param_pop edx
    param_push 0xFFFFFFFE

    rul0_call edx

    pop edx

    ; exit if index is -1
    xor eax, eax
    dec eax
    cmp edx, eax
    je .L_CONST

    shl edx, 2
    mov [array_sizes+edx], eax
.L_CONST:
    end_data_stack
    ret

wrapped_array_empty:
    begin_data_stack

    param_pop eax

    xor edx, edx

    cmp eax, 0xFFFFFFFE
    jne .L2

    mov edx, 0xFFFFFFFF
.L2:
    push edx

    end_data_stack
    ret

%macro wrapped_array 1
wrapped_array_ %+ %1 %+ :
    begin_data_stack

    param_pop edx

    cmp edx, 0xFFFFFFFE
    je .L_IDX

    cmp edx, 0xFFFFFFFF
    je .L_LEN

    shl edx, 2
    push int_type_name[array_buf+(array_block_size*%1)+edx]
    end_data_stack
    ret
.L_LEN:
    push int_type_name[array_sizes+(%1*int_size)]
    end_data_stack
    ret
.L_IDX:
    push %1
    end_data_stack
    ret
%endmacro

%assign i 0
%rep max_arrays
wrapped_array i
%assign i i+1
%endrep

array_len:
    begin_data_stack
    param_pop eax
    push eax

    begin_args
    push 0xFFFFFFFF
    rul0_call spreading_call

    end_data_stack
    ret

%endif







; for C (do not use outside repulan0_main)

; global new_array_idx, new_array_offset, new_array_size

; section .data
; new_array_idx dd 0
; new_array_offset dd 0
; new_array_size dd 0
section .text

global repulan0_push, repulan0_pop, repulan0_sp
global repulan0_call_func, repulan0_call_proc

;; cdecl
repulan0_push:
    mov eax, [esp+(1*int_size)]
    rul0_begin

    push eax

    rul0_end
    ret

repulan0_pop:
    rul0_begin

    pop eax

    rul0_end
    ret

repulan0_sp:
    rul0_begin

    mov eax, esp

    rul0_end
    ret

repulan0_call_func:
    mov eax, [esp+(int_size*2)]
    mov edx, [esp+int_size]
    rul0_begin

    push edx

    begin_args
    push eax
    rul0_call spreading_call

    pop eax

    rul0_end
    ret

repulan0_call_proc:
    mov eax, [esp+(int_size*2)]
    mov edx, [esp+int_size]
    rul0_begin

    push edx

    begin_args
    push eax
    rul0_call spreading_call

    rul0_end
    ret

; global repulan0_array_buf
; repulan0_array_buf:
;     mov eax, array_buf

;     ret
