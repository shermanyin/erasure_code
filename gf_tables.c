#include <stdio.h>
#include <stdlib.h>
#include "gf_base2.h"

const char * usage = "usage: gf_tables degree g(x)\n"
"   degree: degree of the base 2 Galois Field\n"
"   g(x):   the coefficients of the irreducible polynomial used for multiplication\n"
"Example: 'gf_tables 3 11', or 'gf_tables 8 283'";

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Requires 2 parameters.\n\n");
        printf("%s\n\n", usage);
        exit(1);
    }

    uint32_t m = atoi(argv[1]);
    uint32_t g = atoi(argv[2]);

    int rc = gf_init(m, g);

    if (rc) {
        printf("Error initializing gf.\n");
        exit(1);
    }

    gf_print_mult_tbl();
    gf_print_mult_inv_tbl();

    gf_cleanup();

    return 0;
}
