.PHONY: all
all : encode_decode gf_tables

encode_decode: encode_decode.o erasure_code.o gf_base2.o
	gcc -o encode_decode encode_decode.o erasure_code.o gf_base2.o

gf_tables : gf_tables.o gf_base2.o gf_base2.h
	gcc -o gf_tables gf_tables.o gf_base2.o

encode_decode.o : encode_decode.c erasure_code.h
	gcc -c encode_decode.c

gf_tables.o : gf_tables.c gf_base2.h
	gcc -c gf_tables.c

erasure_code.o : erasure_code.c erasure_code.h gf_base2.h
	gcc -c erasure_code.c

gf_base2.o : gf_base2.c gf_base2.h
	gcc -c gf_base2.c

.PHONY: clean
clean : 
	rm -f encode_decode gf_tables *.o
