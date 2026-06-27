#include <stdio.h>

#define KARGS_IMPLEMENTATION
#include "kargs.h"

int main() {
  Ka_Args args = {0};
  ka_arg_int(&args, "--hello");
  ka_arg_string(&args, "--sus", .optional = false);

  printf("desc = %s", args.items->description);

  ka_args_free(args);
}
