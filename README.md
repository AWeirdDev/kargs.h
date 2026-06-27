# kargs.h
A single-header command line argument parser written in C.

Just get this shit:

```cmd
wget https://raw.githubusercontent.com/AWeirdDev/kargs.h/refs/heads/main/kargs.h
```

And start using it:

```c
#define KARGS_IMPLEMENTATION
#include "kargs.h"

int main(int argc, char **argv) {
  // Zero initialize the args container.
  // It kind of acts like a context.
  Ka_Args args = {0};

  int *help = ka_arg_int(&args,
                         "-n --number", // possible flags the user can pass
                         .description = "Count to a number", .optional = true);

  // If you don't trust your OS, free this shit.
  ka_args_free(args);
}
```

Symbols are prefixed with `ka_` or `Ka_`.

## Acknowledgements

After watching some of [Mr. Zuzin](https://github.com/rexim)'s VODs I figured perhaps I really should start learning C just to know how miserable it is.
And it is. So, forgive me for not writing idiomatic C code, this is my very first time writing it.
