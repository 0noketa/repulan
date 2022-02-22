
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
