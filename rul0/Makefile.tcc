
sample_main: sample.o
	tcc sample_main.c sample.o
sample.o: sample.asm
	nasm -felf sample.asm
sample.asm:
	python repulan0c.py --extern_func=cfunc < sample.rul0 > sample.asm

arr_main: arr.o
	tcc arr_main.c arr.o
arr.o: arr.asm
	nasm -felf arr.asm
arr.asm:
	python repulan0c.py --extern_func=begin_capture --extern_func=end_capture --extern_func=cfunc < arr.rul0 > arr.asm

interop_main: interop.o
	tcc interop_main.c interop.o
interop.o: interop.asm
	nasm -felf interop.asm
interop.asm:
	python repulan0c.py --extern_func=print_int --extern_func=print_char --extern_func=println_int --extern_func=loop --extern_func=loop2 < interop.rul0 > interop.asm

branch_main: branch.o
	tcc branch_main.c branch.o
branch.o: branch.asm
	nasm -felf branch.asm
branch.asm:
	python repulan0c.py --extern_func=print_int --extern_func=input_int --extern_var=use_prompt --extern_var=no_prompt < branch.rul0 > branch.asm
