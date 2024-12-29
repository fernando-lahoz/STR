#include <stdio.h>
#include <stdlib.h>

#include "plot.h"

int main()
{
    PLOT_init();
    getc(stdin);
    PLOT_remove();
    return EXIT_SUCCESS;
}