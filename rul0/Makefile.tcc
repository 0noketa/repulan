
sample_main: sample.o
	tcc sample_main.c sample.o
sample.o: sample.asm
	nasm -felf sample.asm
sample.asm:
	python repulan0c.py --extern_func=cfunc < sample.rul0 > sample.asm
