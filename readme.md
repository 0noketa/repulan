# stack-based toy language

this language uses mixture of prefix and postfix notations.  

## comments

starts with odd(0...) "#", ends with even "#" or EOL.  

``` txt
code # comment # code # comment
code # comment # code
code
```

### comments in repunit-mode

ignores EOLs. designed for unreadable polyglots.

``` txt
code # comment # code # comment
comment # code # comment # code
code # comment # code
```

## lambdas

``` txt
\x { x x + }
\x { print(x x +) }

# lambda with "\\"" captures current scope #
1(1)=x  \y { \\z { [x y z] } }(n) =f  a=x b=y f(m)=z

# lambda with "for" counts up to passed value-1 #
for x { print(x) } (111(11))

# lambda with "foreach" do something with every element of passed list #
foreach x { print(x) } ([1 111:])

# "for\" and "foreach\" have capturing #
\x { for\ i { i x + } }(1) =f  [f(1111)] =a
```

## integers

if integer was used as function, returns multiple.  

``` txt
0 10 200 3000
```

### integers in repunit-mode

when source file name has extension *.rul, uses repunit-mode.
if repunit was used as function, returns integer with radix=arg.  
no other numeric literal can be used.  

``` txt
1 11 1111
```

## conditional branching

``` txt
# if x then y else z end #
x ? y | z ?!
# if x then y end #
x ? y ?!
# in any block, "|" and "?!" can be omitted #
[x ? y]
\x { x x0 == ? y | x x1 == ? z | x }
```

## spreading call

most unusual feature than repunits. it seems like compile-time macro, but it happens in run-time.  
"(" takes a function and memory stack top as arg0-1.
")" takes just one argument and calls function taken by "(".  
if multiple values ware passed, function will be applied to every arg via argument queue.

``` txt
# f(x) f(y) f(z) #
f(x y z)

# g(f(x y z)) #  #1#
# g(f(x) f(y) f(z)) #  #2#
# g(f(x)) g(f(y)) g(f(z)) #  #3#
g(f(x y z))
```

## lists

"[" ... "]" pushes list that contains captured stack emlements.  
if any list was used as function, returns an element.  

``` txt
[1(1) 111(1) 1111(1) dup]
```

## RPN operators

if any alithmetic operator was passed any repunit with unknown radix, uses 1 as radix.

``` txt
+ (x y -- x+y)
- (x:int y:int -- x-y)  # saturated. never return negative value #
- (list y -- list_exclude_y)
- (list callable -- filtered_list)
- (str str -- filtered_str)
* (x:int y:int -- x*y)
* (list sep:str -- joined_str)
* (list callable -- modified_list)
* (callable callable -- composite_function)
/ (x:int y:int -- x/y)
/ (str sep:str -- list_of_separated)
% (x y -- x%y)
: (x y -- x ... y-1)
dup (x -- x x)
swap (x y -- y x)
drop (x y --)
=name (x --)  # assignment #
restart (x -- ...)  # recursive tail-call #
reserve_restart (x -- ...)  # reserves recursive tail-call at end of function #
  # 1 4  2 4  3 4  2 4  4 4 #
  \x { x  x 111(1) == ? 11(1) reserve_restart ?!  1111(1) } (1 11111:)
'(' (f --)  # pushes function to queue and starts argument capturing # 
')' (...stack x0 x1 ... -- ...modified_stack f(x0) f(x1) ...)  # ends capturing and calls function # 
```

## builtin functions

### repunit

if argument was string, treats it as repeated "1" or error.  
if argument was integer, treats it as columns.

``` txt
# 111 #
repunit("111")
# 1111 #
repunit(1111(1))
```

### pop

popes value at passed index(0 at top) of the stack, and pushes it.

``` txt
# 11 111 1 #
1 11 111 pop(11)
```

### spread

pushes every element of passed list.

``` txt
# f(x y z) #
# [x y z]=a  f(a(1 1- len(a) :)) #
f(spread([x y z]))
```

### pass

returns function that applies passed function to first argument.

``` txt
# x(a) y(a) z(a) #
pass(a)(x y z)
```

### int

converts value to integer.

### str

converts value to string.

### len

Python's

``` py
len(x)
```

### sort

Python's

``` py
list(sorted(x)).
```

### unique

Python's

``` py
list(sorted(set(x)))
```

## functions for stand-alone interpreter

functions below does not work when used as library.

### import

imports a file. returns empty.

``` txt
import("mylib")
```

### eval

evaluates passed string as Repulan program.

``` txt
print eval* ("1 " ["11" "111" "1111" ""] "+ " * +)
```

### system

evaluates passed string as Python program. returns empty.

``` txt
# ${...}: Repulan's valiable #
# repulan_vars: dict of Repulan's valiables in current scope #
# repulan_int(): int() for Repulan's values #
# repulan_wrap_void_func(): wraps Python's function (without result) as Repulan's callable #
system("${f} = (lambda x: x + 1)")
print f* (1(1))
```

### print

returns empty.  
Python's

``` py
print(x)
```

### input

Python's

``` py
input(x)
```

## typeless subset without repunits

[here](./rul0/readme.md)

## license

none. public domain.
