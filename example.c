#include <stdio.h>

#define KARGS_IMPLEMENTATION
#include "kargs.h"


int main() {
    Ka_Args args = {0};
    bool* b = ka_arg_bool(&args, "--hello", .description = "yeah", .optional = true);
}
