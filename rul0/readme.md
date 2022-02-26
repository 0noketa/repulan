# typeless subset to x86 compiler

## available elements

``` txt
# ... #
\x{}
for x{}
f(x) f(x y z)
[]
+ - * / % :  # do not use for non-integer values #
swap dup drop tail_recall reserve_tail_recall
```

and external C functions (auto wrapping)

## builtin functions

* a(i) -- element_at_i
  * a is any object. currently only arrays are object.
* v a set(i) -- a
  * replaces value at i. returns a for spreading_call. this function will be replaced.
* len(a) -- length
* spread(a) -- a(0 len(a):)
* reverse(a) -- reversed_a
  * reversed object shares elements
* del(a) --
  * deallocates object.

## dependency of generated code

### exec

any IA32-like platform that has CMOVcc instruction.

### build

NASM (necessary).  
it has not startup-code and platform-specific library. you can use as thread-unsafe no-dependency library.  
you need some external code and linker (or loader) for executable output and I/O.

## C interface

``` c
// initializes and executes global code. this function can be renamed by compiler option.
extern void repulan0_main(void);

// functions below work correctly only in external functions called in repulan0_main() (this behavior will be changed).

extern void repulan0_push(uintptr_t value);
extern uintptr_t repulan0_pop();
extern uintptr_t *repulan0_sp();  // will be changed. because stack direction depends on x86.
// calls rul0 function
extern void repulan0_call_proc(uintptr_t proc, uintptr_t arg);
// calls rul0 function that pushes any result. and returns poped result.
extern uintptr_t repulan0_call_func(uintptr_t func, uintptr_t arg);

// object interface
extern uintptr_t repulan0_get(uintptr_t obj, uintptr_t idx);
extern uintptr_t repulan0_len(uintptr_t obj);
extern void repulan0_spread(uintptr_t obj);
extern uintptr_t repulan0_reverse(uintptr_t obj);
extern void repulan0_del(uintptr_t obj);
// if result was true, do not use idx as array index.
extern uintptr_t repulan0_is_method_id(uintptr_t idx);
```

## difference to Repulan

* every numeric literal is any decimal integer, not repunit.  
* [] allocates array and function statically bound to it. no GC exists. but generic del() exists.  
* arrays are functions, and methods use any invalid index as method ID.
* behavior of array-index clamping will be changed.  
* () can be used just for functions and objects. () with other values never work and are unsafe.  

## license

none. public domain.
