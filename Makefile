.PHONY: all
all : encode_decode gf_tables exhaustive_ec_test

encode_decode: encode_decode.o erasure_code.o gf_base2.o
	gcc -o encode_decode encode_decode.o erasure_code.o gf_base2.o

gf_tables : gf_tables.o gf_base2.o
	gcc -o gf_tables gf_tables.o gf_base2.o

exhaustive_ec_test : exhaustive_ec_test.o erasure_code.o gf_base2.o queue.o
	gcc -pthread -o exhaustive_ec_test exhaustive_ec_test.o erasure_code.o gf_base2.o queue.o

exhaustive_ec_test.o : exhaustive_ec_test.c erasure_code.h queue.h
	gcc -c exhaustive_ec_test.c

encode_decode.o : encode_decode.c erasure_code.h
	gcc -c encode_decode.c

queue.o : queue.c
	gcc -c queue.c

gf_tables.o : gf_tables.c gf_base2.h
	gcc -c gf_tables.c

erasure_code.o : erasure_code.c erasure_code.h gf_base2.h
	gcc -c erasure_code.c

gf_base2.o : gf_base2.c gf_base2.h
	gcc -c gf_base2.c

.PHONY: clean
clean : 
	rm -f encode_decode gf_tables exhaustive_ec_test *.o
