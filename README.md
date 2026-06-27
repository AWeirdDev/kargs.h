# kargs.h
A single-header command line argument parser written in C.

Just get this shit:

```cmd
wget https://raw.githubusercontent.com/AWeirdDev/kargs.h/refs/heads/main/kargs.h
```

And start using it:

```c
#include <stdio.h>

#define KARGS_IMPLEMENTATION
#include "kargs.h"

int main(int argc, char **argv) {
    // Zero initialize the args container.
    // It kind of acts like a context.
    Ka_Args args = {0};

    // You get pointers to the values
    int *number =
        ka_arg_int(&args, "-n --number", .description = "Count to a number",
                   .optional = false);
    char **name = ka_arg_string(
        &args, "--name", .description = "Your name", .optional = false);

    // This serves as the entrypoint, it basically does all the job
    ka_args_entry(&args, argc, argv);

    // Then you can use the value
    for (int i = 0; i < *number; i++) {
        printf("%s\n", *name);
    }

    // If you don't trust your OS, free this shit.
    ka_args_free(args);
}
```

Symbols are prefixed with `ka_` or `Ka_`.

## Acknowledgements

After watching some of [Mr. Zuzin](https://github.com/rexim)'s VODs I figured perhaps I really should start learning C just to know how miserable it is.
And it is. So, forgive me for not writing idiomatic C code, this is my very first time writing it.
