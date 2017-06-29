gf_tables : gf_tables.o gf_base2.o gf_base2.h
	gcc -o gf_tables gf_tables.o gf_base2.o

gf_base2.o : gf_base2.c gf_base2.h
	gcc -c gf_base2.c

clean : 
	rm -f gf_tables *.o
