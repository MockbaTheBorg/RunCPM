/* Read photoresistor value */

#include <cpm.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    int pin;

    if (argc != 2) {
        printf("Syntax: photo pin");
        return 1;
    }

    pin = atoi(argv[1]);

    printf("Light: %d\n", bdoshl(223, pin << 8));
}
