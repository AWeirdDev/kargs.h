#include <stdio.h>
#define KARGS_IMPLEMENTATION
#include "kargs.h"

int main(int argc, char **argv) {
    // Zero initialize the args container.
    // It kind of acts like a context.
    Ka_Args args = {0};

    int *number =
        ka_arg_int(&args, "-n --number", .description = "Count to a number",
                   .optional = false);

    char **name = ka_arg_string(
        &args, "--name", .description = "The name of you", .optional = false);

    ka_args_entry(&args, argc, argv);

    for (int i = 0; i < *number; i++) {
        printf("%s\n", *name);
    }

    // If you don't trust your OS, free this shit.
    ka_args_free(args);
}
