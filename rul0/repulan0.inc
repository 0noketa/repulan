%macro define_default 2
%ifndef %1
%define %1 %2
%endif
%endmacro

define_default C_API_PREFIX, repulan0_
define_default INT_SIZE, 4
define_default INT_BITS, 3
define_default INT_TYPE_NAME, DWORD
define_default DATA_STACK_SIZE, 0x1000
define_default PARAM_STACK_SIZE, 0x200
define_default ARG0_STACK_SIZE, 0x200
define_default TEMP_STACK_SIZE, 0x100
define_default EXTERN_STACK_SIZE, 0x100
define_default ARRAY_BLOCK_SIZE_BITS, 10
define_default MAX_ARRAYS, 0x40
define_default ARRAY_STACK_SIZE, 0x100

%define c_api(name) C_API_PREFIX %+ name

%define ARRAY_BLOCK_SIZE (1 << (ARRAY_BLOCK_SIZE_BITS-1))
%define ARRAY_BUF_SIZE (MAX_ARRAYS*ARRAY_BLOCK_SIZE)

; <BASE are arg of method "get"
%define RUL0_OBJ_METHOD_BASE 0xFFFFFF80
%define RUL0_OBJ_METHOD_LEN 0xFFFFFFFF
%define RUL0_OBJ_METHOD_SPREAD 0xFFFFFFFE
%define RUL0_OBJ_METHOD_SET 0xFFFFFFFD
%define RUL0_OBJ_METHOD_REVERSE 0xFFFFFFFC
%define RUL0_OBJ_METHOD_OBJIDX 0xFFFFFFFB
%define RUL0_OBJ_METHOD_DEL 0xFFFFFFFA

%define RUL0_OBJIDX_INVALID 0xFFFFFFFF


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
    xor ebx, ebx
    dec ebx
    add eax, edx
    cmovc eax, ebx
    push eax
%endmacro

%macro rul0_sub 0
    pop edx
    pop eax
    xor ebx, ebx
    sub eax, edx
    cmovb eax, ebx
    push eax
%endmacro

%macro rul0_mul 0
    pop edx
    pop eax
    mul edx
    push eax
%endmacro

%macro rul0_div 0
    pop ebx
    pop eax
    xor edx, edx
    div ebx
    push eax
%endmacro

%macro rul0_mod 0
    pop ebx
    pop eax
    xor edx, edx
    div ebx
    push edx
%endmacro

%macro rul0_eq 0
    pop edx
    pop eax
    xor ebx, ebx
    xor ecx, ecx
    inc ebx
    cmp eax, edx
    cmove ecx, ebx
    push ecx
%endmacro

%macro rul0_neq 0
    pop edx
    pop eax
    xor ebx, ebx
    xor ecx, ecx
    inc ebx
    cmp eax, edx
    cmovne ecx, ebx
    push ecx
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
    sub esp, (%1*INT_SIZE)
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
data_stack: resd DATA_STACK_SIZE
arg0_stack: resd ARG0_STACK_SIZE
temp_stack: resd TEMP_STACK_SIZE
param_stack: resd PARAM_STACK_SIZE
extern_stack: resd EXTERN_STACK_SIZE
section .data
data_sp: dd data_stack+(DATA_STACK_SIZE*INT_SIZE)
arg0_sp: dd arg0_stack+(ARG0_STACK_SIZE*INT_SIZE)
temp_sp: dd temp_stack+(TEMP_STACK_SIZE*INT_SIZE)
param_sp: dd param_stack+(PARAM_STACK_SIZE*INT_SIZE)
extern_sp: dd extern_stack+(EXTERN_STACK_SIZE*INT_SIZE)
section .text

; repulan0_init:
;     mov eax, data_stack+(DATA_STACK_SIZE*INT_SIZE)
;     mov [data_sp], eax
;     mov eax, arg0_stack+(ARG0_STACK_SIZE*INT_SIZE)
;     mov [arg0_sp], eax
;     mov eax, temp_stack+(TEMP_STACK_SIZE*INT_SIZE)
;     mov [temp_sp], eax
;     mov eax, param_stack+(PARAM_STACK_SIZE*INT_SIZE)
;     mov [param_sp], eax
;     mov eax, extern_stack+(EXTERN_STACK_SIZE*INT_SIZE)
;     mov [extern_sp], eax
;     ret

spreading_call:
    begin_data_stack

    ; edx: first arg
    ; call ecx times
    arg0_pop edx
    mov ecx, edx
    sub ecx, esp
    shr ecx, INT_BITS-1

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

%macro rul0_reserve_restart_flat 0
    pop eax

    begin_param_stack
    pop edx
    push eax
    push edx
    end_param_stack

    begin_temp_stack
    pop ecx
    inc ecx
    push ecx
    end_temp_stack
%endmacro

%macro rul0_reserve_restart_for 0
    pop eax

    begin_param_stack
    pop edx
    push eax
    push edx
    end_param_stack

    begin_temp_stack
    pop eax
    pop edx
    pop ecx
    inc ecx
    push ecx
    push edx
    push eax
    end_temp_stack
%endmacro

%macro rul0_restart_flat 1
    pop eax

    begin_param_stack
    pop INT_TYPE_NAME[%1]
    push eax
    end_param_stack

    jmp .recursion_target
%endmacro

%macro rul0_restart_for 1
    begin_temp_stack
    pop eax
    pop edx
    end_temp_stack

    pop eax

    begin_param_stack
    pop INT_TYPE_NAME[%1]
    push eax
    end_param_stack

    jmp .recursion_target
%endmacro

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




%macro select_method 1
    cmp %1, RUL0_OBJ_METHOD_BASE
    jb .METHOD_GET
%ifdef use_set
    cmp %1, RUL0_OBJ_METHOD_SET
    je .METHOD_SET
%endif
%ifdef use_len
    cmp %1, RUL0_OBJ_METHOD_LEN
    je .METHOD_LEN
%endif
%ifdef use_spread
    cmp %1, RUL0_OBJ_METHOD_SPREAD
    je .METHOD_SPREAD
%endif
%ifdef use_reverse
    cmp %1, RUL0_OBJ_METHOD_REVERSE
    je .METHOD_REVERSE
%endif
%ifdef use_del
    cmp %1, RUL0_OBJ_METHOD_DEL
    je .METHOD_DEL
%endif
; .METHOD_OBJIDX
%endmacro


%ifdef use_array

;; array values are functions
;; index>=RUL0_OBJ_METHOD_BASE means method call.
;; array(LEN) returns length
;; array(OBJIDX) returns object id. if array was constant, returns invalid id
;; array(REVERSE) returns function that uses reversed index.
;; array(SET) pops 2 values, replaces an element, and returns array. (x y a(-4) -- a[y]=x -- a)
;; array(DEL) deallocates this array

section .bss
array_buf: resd ARRAY_BLOCK_SIZE*MAX_ARRAYS
array_stack: resd ARRAY_STACK_SIZE

section .data
array_sizes: times MAX_ARRAYS dd 0
wrapped_arrays:
%assign i 0
%rep MAX_ARRAYS
    dd wrapped_array_ %+ i
%assign i i+1
%endrep
array_sp: dd array_stack+(ARRAY_STACK_SIZE*INT_SIZE)

section .text

alloc_array:
    begin_data_stack

    array_pop edx
    array_push edx
    cmp edx, esp
    jle .L_EMPTY

    mov edx, (MAX_ARRAYS*INT_SIZE)
.L_LOOP:
    sub edx, INT_SIZE
    mov eax, [array_sizes+edx]

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

    cmp eax, ARRAY_BLOCK_SIZE
    jl .L_SATURATED
    sub eax, ARRAY_BLOCK_SIZE
    add esp, eax

    array_pop eax
    array_push eax
    sub eax, esp
.L_SATURATED:
    shr eax, INT_BITS-1
    mov [array_sizes+edx], eax
    ; mov [new_array_size], eax
    shl eax, INT_BITS-1

    ; edx: offset of int32[*]

    ; addr of function (result of this function)
    temp_push INT_TYPE_NAME[wrapped_arrays+edx]
    ; mov [new_array_offset], edx
    shr edx, INT_BITS-1
    ; mov [new_array_idx], edx

    ; edx: index of int32[*]

    ; eax: first element addr+4 on the data stack
    array_pop eax
    array_push eax

    cmp eax, esp
    jle .L_END_COPY

    ; edx: element addr on result array
    shl edx, ARRAY_BLOCK_SIZE_BITS-1
    ; mov [new_array_offset], edx
    add edx, array_buf

    ; edx: &a[block_idx][0]  //uint32 a[MAX_ARRAYS+1][block_size]
.L_COPY:
    sub eax, INT_SIZE

    push INT_TYPE_NAME[eax]
    pop INT_TYPE_NAME[edx]

    add edx, INT_SIZE

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
    param_push RUL0_OBJ_METHOD_OBJIDX

    rul0_call edx

    pop edx

    cmp edx, RUL0_OBJIDX_INVALID
    je .L_CONST

    shl edx, INT_BITS
    mov [array_sizes+edx], eax
.L_CONST:
    end_data_stack
    ret

wrapped_array_empty:
    begin_data_stack

    param_pop eax

    select_method eax
.METHOD_OBJIDX:
    push RUL0_OBJIDX_INVALID
    end_data_stack
    ret
.METHOD_GET:
%ifdef use_len
.METHOD_LEN:
%endif
    xor eax, eax
    push eax
    end_data_stack
    ret
%ifdef use_set
.METHOD_SET:
    pop edx
    pop edx
    end_data_stack
    ret
%endif
%ifdef use_spread
.METHOD_SPREAD:
    end_data_stack
    ret
%endif
%ifdef use_reverse
.METHOD_REVERSE:
    push wrapped_array_empty
    end_data_stack
    ret
%endif
%ifdef use_del
.METHOD_DEL:
    end_data_stack
    ret
%endif

%macro wrapped_array 1
wrapped_array_ %+ %1 %+ :
    begin_data_stack

    param_pop edx

    select_method edx
.METHOD_OBJIDX:
    push %1
    end_data_stack
    ret
.METHOD_GET:
    and edx, (1 << (ARRAY_BLOCK_SIZE_BITS-1))-1
    shl edx, INT_BITS-1
    push INT_TYPE_NAME[array_buf+(ARRAY_BLOCK_SIZE*%1)+edx]
    end_data_stack
    ret
%ifdef use_set
.METHOD_SET:
    pop edx
    and edx, (1 << (ARRAY_BLOCK_SIZE_BITS-1))-1
    shl edx, INT_BITS-1
    pop eax
    mov INT_TYPE_NAME[array_buf+(ARRAY_BLOCK_SIZE*%1)+edx], eax
    end_data_stack
    ret
%endif
%ifdef use_len
.METHOD_LEN:
    push INT_TYPE_NAME[array_sizes+(%1*INT_SIZE)]
    end_data_stack
    ret
%endif
%ifdef use_spread
.METHOD_SPREAD:
    mov eax, [array_sizes+(%1*INT_SIZE)]
    mov edx, array_buf+(ARRAY_BLOCK_SIZE*%1)
    or eax, eax
    jz .end_spread
.begin_spread:
    push INT_TYPE_NAME[edx]
    add edx, INT_SIZE
    dec eax
    or eax, eax
    jnz .begin_spread
.end_spread:
    end_data_stack
    ret
%endif
%ifdef use_reverse
.METHOD_REVERSE:
    push wrapped_array_ %+ %1 %+ _reversed
    end_data_stack
    ret
%endif
%ifdef use_del
.METHOD_DEL:
    xor eax, eax
    mov [array_sizes+(%1*INT_SIZE)], eax
    end_data_stack
    ret
%endif

%ifdef use_reverse
wrapped_array_ %+ %1 %+ _reversed %+ :
    begin_data_stack

    param_pop edx

    select_method edx
.METHOD_OBJIDX:
    push RUL0_OBJIDX_INVALID
    end_data_stack
    ret
.METHOD_GET:
    and edx, (1 << (ARRAY_BLOCK_SIZE_BITS-1))-1
    mov eax, [array_sizes+(%1*INT_SIZE)]
    dec eax
    sub eax, edx
    shl eax, INT_BITS-1
    push INT_TYPE_NAME[array_buf+(ARRAY_BLOCK_SIZE*%1)+eax]
    end_data_stack
    ret
%ifdef use_set
.METHOD_SET:
    pop edx
    and edx, (1 << (ARRAY_BLOCK_SIZE_BITS-1))-1
    mov eax, [array_sizes+(%1*INT_SIZE)]
    dec eax
    sub eax, edx
    shl eax, INT_BITS-1
    pop edx
    mov INT_TYPE_NAME[array_buf+(ARRAY_BLOCK_SIZE*%1)+eax], edx
    end_data_stack
    ret
%endif
%ifdef use_len
.METHOD_LEN:
    push INT_TYPE_NAME[array_sizes+(%1*INT_SIZE)]
    end_data_stack
    ret
%endif
%ifdef use_spread
.METHOD_SPREAD:
    mov eax, [array_sizes+(%1*INT_SIZE)]
    mov edx, array_buf+(ARRAY_BLOCK_SIZE*%1)
    shl eax, INT_BITS-1
    add edx, eax
    shr eax, INT_BITS-1
    or eax, eax
    jz .end_spread
.begin_spread:
    sub edx, INT_SIZE
    push INT_TYPE_NAME[edx]
    dec eax
    or eax, eax
    jnz .begin_spread
.end_spread:
    end_data_stack
    ret
%endif
.METHOD_REVERSE:
    push wrapped_array_ %+ %1
    end_data_stack
    ret
%ifdef use_del
.METHOD_DEL:
    end_data_stack
    ret
%endif

%endif

%endmacro

%assign i 0
%rep MAX_ARRAYS
wrapped_array i
%assign i i+1
%endrep

%endif


%ifdef use_set
; ...vals obj set(keys...)
object_set:
    begin_data_stack

    ; func
    pop eax
    temp_push eax

    ; idx
    param_pop edx
    push edx

    param_push RUL0_OBJ_METHOD_SET
    rul0_call eax

    temp_pop eax
    push eax

    end_data_stack
    ret
%endif

%ifdef use_len
object_len:
    begin_data_stack
    param_pop eax

    param_push RUL0_OBJ_METHOD_LEN
    rul0_call eax

    end_data_stack
    ret
%endif

%ifdef use_spread
object_spread:
    begin_data_stack
    param_pop eax

    param_push RUL0_OBJ_METHOD_SPREAD
    rul0_call eax

    end_data_stack
    ret
%endif

%ifdef use_reverse
object_reverse:
    begin_data_stack
    param_pop eax

    param_push RUL0_OBJ_METHOD_REVERSE
    rul0_call eax

    end_data_stack
    ret
%endif

%ifdef use_del
object_del:
    begin_data_stack
    param_pop eax

    param_push RUL0_OBJ_METHOD_DEL
    rul0_call eax

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

global c_api(push), c_api(pop), c_api(sp)
global c_api(call_func), c_api(call_proc)

;; cdecl
c_api(push):
    mov eax, [esp+(1*INT_SIZE)]
    rul0_begin

    push eax

    rul0_end
    ret

c_api(pop):
    rul0_begin

    pop eax

    rul0_end
    ret

c_api(sp):
    rul0_begin

    mov eax, esp

    rul0_end
    ret

c_api(get):
    mov eax, [esp+(INT_SIZE*2)]
    mov edx, [esp+INT_SIZE]
    rul0_begin

%ifdef use_array
    param_push eax
    rul0_call edx

    pop eax
%else
    xor eax, eax
%endif

    rul0_end
    ret

c_api(len):
    mov edx, [esp+INT_SIZE]
    rul0_begin

%ifdef use_len
    param_push edx
    rul0_call object_len

    pop eax
%else
    xor eax, eax
%endif

    rul0_end
    ret

c_api(spread):
    mov edx, [esp+INT_SIZE]
    rul0_begin

%ifdef use_spread
    param_push edx
    rul0_call object_spread
%endif

    rul0_end
    ret

c_api(reverse):
    mov edx, [esp+INT_SIZE]
    rul0_begin

%ifdef use_reverse
    param_push edx
    rul0_call object_reverse

    pop eax
%else
    xor eax, eax
%endif

    rul0_end
    ret

c_api(del):
    mov edx, [esp+INT_SIZE]
    rul0_begin

%ifdef use_del
    param_push edx
    rul0_call object_del
%endif

    rul0_end
    ret

c_api(is_method_id):
    mov edx, [esp+INT_SIZE]
    rul0_begin

    xor eax, eax
    xor ecx, ecx
    inc ecx
    cmp edx, RUL0_OBJ_METHOD_BASE
    cmovge eax, ecx

    rul0_end
    ret

c_api(call_func):
    mov eax, [esp+(INT_SIZE*2)]
    mov edx, [esp+INT_SIZE]
    rul0_begin

    param_push eax
    rul0_call edx

    pop eax

    rul0_end
    ret

c_api(call_proc):
    mov eax, [esp+(INT_SIZE*2)]
    mov edx, [esp+INT_SIZE]
    rul0_begin

    param_push eax
    rul0_call edx

    rul0_end
    ret

; global repulan0_array_buf
; repulan0_array_buf:
;     mov eax, array_buf

;     ret
