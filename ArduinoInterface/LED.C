/* Turn a LED or on off */

#include <cpm.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    int pin, value;

    if (argc != 3) {
        printf("Syntax: led pin value (0 = low, 1 = high)");
        return 1;
    }

    pin = atoi(argv[1]);
    value = atoi(argv[2]);

    bdos(220, (pin << 8) + 1);
    bdos(222, (pin << 8) + value);
}
