#include <stdio.h>
#define KARGS_IMPLEMENTATION
#include "kargs.h"

int main(int argc, char **argv) {
    // Zero initialize the args container.
    // It kind of acts like a context.
    Ka_Args args = {0};

    int *number =
        ka_arg_int(&args,
                   "-n --number", // possible flags the user can pass
                   .description = "Count to a number", .optional = true);

    ka__qsort_args(&args);

    Ka_Result result = ka_args_parse(&args, argc - 1, argv + 1);
    if (result.error) {
        ka_print_error(result);
        return 1;
    }

    printf("%d", *number);

    // If you don't trust your OS, free this shit.
    ka_args_free(args);
}
