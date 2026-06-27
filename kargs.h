/*

   KARGS - Single-header argument parser

   version: 0.0.1
   author: https://github.com/AWeirdDev
   license: MIT

   https://github.com/AWeirdDev/kargs.h

 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef KARGS_DEF
#define KARGS_DEF
#endif // KARGS_DEF

// Specifies the behavior when kargs panics.
// You can define this macro yourself so it does something else
// other than this.
#ifndef ka_panic
#define ka_panic(reason)                                                       \
    do {                                                                       \
        fprintf(stderr, "kargs panicked: %s\n", reason);                       \
        exit(1);                                                               \
    } while (0)
#endif // ka_panic

// Specifies the behavior when asserting whether the allocation
// had failed. This just detects null pointers. Stoopid ahh
#define ka_assert_allocated(ptr)                                               \
    if ((ptr) == NULL) {                                                       \
        ka_panic("allocation failed");                                         \
    }

#ifndef KA_TYPE_MAP
#define KA_TYPE_MAP(X)                                                         \
    X(int, int)                                                                \
    X(string, char *)                                                          \
    X(boolean, bool)
#endif // KA_TYPE_MAP

#ifndef KA_ERROR_MAP
#define KA_ERROR_MAP(X)                                                        \
    X(OK, "", 0)                                                               \
    X(UNKNOWN_FLAG, "unknown flag: %s", -1)                                    \
    X(MISSING_FLAG, "missing flag: %s", -2)                                    \
    X(EXPECTED_VALUE, "expected value after flag: %s", -3)                     \
    X(INVALID_INT, "invalid integer value for flag: %s", -4)
#endif // KA_ERROR_MAP

// enum ArgType;
typedef enum {

#define EXPAND(name, type) Ka_ArgType_##name,
    KA_TYPE_MAP(EXPAND)
#undef EXPAND

} Ka_ArgType;

// enum Error;
typedef enum {

#define EXPAND(name, _, tag) Ka_Error_##name = tag,
    KA_ERROR_MAP(EXPAND)
#undef EXPAND

} Ka_Error;

// Required fields for dynamic arrays
#define KA__DA_FIELDS(type)                                                    \
    size_t length;                                                             \
    size_t capacity;                                                           \
    type *items;

typedef struct {
    KA__DA_FIELDS(char)
} Ka_StringView;

KARGS_DEF Ka_StringView ka_sv_from_cstr(const char *cstr);

/// Gets the length of the string view.
KARGS_DEF size_t ka_sv_len(Ka_StringView *view);

/// Pushes a char to the string view.
KARGS_DEF void ka_sv_push(Ka_StringView *view, char c);

/// Pushes a CStr to the string view.
KARGS_DEF void ka_sv_push_cstr(Ka_StringView *view, const char *cstr);

/// Makes the string temporarily a CStr.
/// Use `ka_sv_decstr()` later to turn it back to normal.
KARGS_DEF char *ka_sv_temp_cstr(Ka_StringView *view);

KARGS_DEF void ka_sv_decstr(Ka_StringView *view);

// struct Arg;
typedef struct {
    bool filled;

    Ka_ArgType type;
    bool optional;
    const char *names;
    const char *description;

    // we'll put the parsed result here when there's actually data
    void *slot;
} Ka_Arg;

typedef struct {
    KA__DA_FIELDS(Ka_Arg)
} Ka_ArgsInfoContainer;

// struct FlagName;
typedef struct {
    Ka_StringView name;
    size_t arg_idx;
} Ka_FlagName;

typedef struct {
    KA__DA_FIELDS(Ka_FlagName)
} Ka_FlagNames;

// struct Args;
typedef struct {
    Ka_ArgsInfoContainer args_info;
    Ka_FlagNames flag_names;
} Ka_Args;

KARGS_DEF Ka_FlagNames ka_parse_flag_names(const char *s);

/// Append an argument to `Args`.
KARGS_DEF void ka_args_append(Ka_Args *args, Ka_Arg arg, Ka_FlagNames names);

/// Frees the arg parser (`Ka_Args`) and all the dynamic arrays within.
KARGS_DEF void ka_args_free(Ka_Args args);

typedef struct {
    bool optional;
    char *description;
} Ka_ArgOptions;

#define arg_fn(name, ctype, type_tag)                                          \
    KARGS_DEF ctype *ka__arg_##name(Ka_Args *args, const char *flag_names,     \
                                    Ka_ArgOptions options);

#define EXPAND(name, ctype) arg_fn(name, ctype, Ka_ArgType_##name)

KA_TYPE_MAP(EXPAND)

#undef EXPAND
#undef arg_fn
#undef fn_body

// # Arg types
//
// For example:
//
// ```c
// bool *help = ka_arg_boolean(
//   &args,
//   "-h --help",
//   .optional = true,
//   .description = "Displays a help message"
// );
// ```

#define ka_arg_boolean(args, flag_names, ...)                                  \
    ka__arg_boolean(args, flag_names, (Ka_ArgOptions){__VA_ARGS__})

#define ka_arg_string(args, flag_names, ...)                                   \
    ka__arg_string(args, flag_names, (Ka_ArgOptions){__VA_ARGS__})

#define ka_arg_int(args, flag_names, ...)                                      \
    ka__arg_int(args, flag_names, (Ka_ArgOptions){__VA_ARGS__})

typedef struct {
    Ka_Error error;
    Ka_StringView error_data;
} Ka_Result;

KARGS_DEF const char *ka_get_error_message(Ka_Error error);
KARGS_DEF void ka_print_error(Ka_Result result);

KARGS_DEF Ka_Result ka_args_parse(Ka_Args *args, int argc, char **argv);

KARGS_DEF void ka_print_help(Ka_Args *args);

/// Invokes the entrypoint. This function returns nothing when it passes,
/// and exits the program & print help on error.
KARGS_DEF void ka_args_entry(Ka_Args *args, int argc, char **argv);

/*
 * === KArgs implementation ===
 */
#ifdef KARGS_IMPLEMENTATION

#define ka__da_reserve(da, expected_capacity)                                  \
    do {                                                                       \
        if ((expected_capacity) > (da)->capacity) {                            \
            if ((da)->capacity == 0) {                                         \
                (da)->capacity = 128;                                          \
            }                                                                  \
                                                                               \
            while ((expected_capacity) > (da)->capacity) {                     \
                (da)->capacity *= 2;                                           \
            }                                                                  \
                                                                               \
            void *ptr =                                                        \
                realloc((da)->items, (da)->capacity * sizeof(*(da)->items));   \
            ka_assert_allocated(ptr);                                          \
                                                                               \
            (da)->items = ptr;                                                 \
        }                                                                      \
    } while (0)

#define ka__da_free(da) free((da)->items)

#define ka__da_append(da, item)                                                \
    do {                                                                       \
        ka__da_reserve((da), (da)->length + 1);                                \
        (da)->items[(da)->length++] = (item);                                  \
    } while (0)

/// Gets the length of the string view.
KARGS_DEF size_t ka_sv_len(Ka_StringView *view) { return view->length; }

/// Pushes a character onto the string view.
KARGS_DEF void ka_sv_push(Ka_StringView *view, char c) {
    ka__da_append(view, c);
}

/// Creates a string view from a CStr.
KARGS_DEF void ka_sv_push_cstr(Ka_StringView *view, const char *cstr) {
    while (*cstr) {
        ka_sv_push(view, *cstr);
        cstr++;
    }
}

KARGS_DEF char *ka_sv_temp_cstr(Ka_StringView *view) {
    ka_sv_push(view, '\0');
    return view->items;
}

KARGS_DEF void ka_sv_decstr(Ka_StringView *view) { view->length--; }

/// Creates a string view from a CStr.
KARGS_DEF Ka_StringView ka_sv_from_cstr(const char *cstr) {
    size_t len = strlen(cstr);
    Ka_StringView view = {0};

    ka__da_reserve(&view, len);
    ka_sv_push_cstr(&view, cstr);

    return view;
}

typedef struct {
    Ka_StringView str;
    const char *next;
} Ka__FlagParseResult;

static Ka__FlagParseResult ka__parse_flag_name(const char *s) {
    Ka_StringView collector = {0};
    int n_dashes = 0;

    while (*s) {
        if (ka_sv_len(&collector) == 0) {
            if (*s == '-') {
                n_dashes += 1;
            }
            if (*s != ' ')
                ka_sv_push(&collector, *s);

            if (n_dashes >= 3)
                ka_panic("expected maximum of two dashes, got "
                         "3 or more");
        } else {
            if (*s == ' ') {
                s++;
                break;
            }

            ka_sv_push(&collector, *s);
        }

        s++;
    }

    return (Ka__FlagParseResult){.str = collector, .next = s};
}

KARGS_DEF Ka_FlagNames ka_parse_flag_names(const char *s) {
    Ka_FlagNames parsed = {0};

    while (*s) {
        Ka__FlagParseResult result = ka__parse_flag_name(s);
        s = result.next;

        ka__da_append(&parsed,
                      ((Ka_FlagName){.name = result.str, .arg_idx = 0}));
    }

    return parsed;
}

KARGS_DEF void ka_args_append(Ka_Args *args, Ka_Arg arg, Ka_FlagNames names) {
    // first append the flag names, which is for lookup
    for (size_t i = 0; i < names.length; i++) {
        names.items[i].arg_idx = args->args_info.length;
        ka_sv_temp_cstr(
            &names.items[i].name); // [^1] TEMPORARY CSTR OPERATED HERE
        ka__da_append(&args->flag_names, names.items[i]);
    }
    ka__da_free(&names);

    // then we append the info, which is the metadata
    ka__da_append(&args->args_info, arg);
}

KARGS_DEF void ka_args_free(Ka_Args args) {
    for (size_t i = 0; i < args.args_info.length; i++) {
        free(args.args_info.items[i].slot);
    }

    ka__da_free(&args.args_info);
    ka__da_free(&args.flag_names);
}

#define fn_body(args, ctype, type_tag, flag_names, options)                    \
    Ka_FlagNames flags = ka_parse_flag_names((flag_names));                    \
                                                                               \
    void *slot = calloc(1, sizeof(ctype));                                     \
    ka_assert_allocated(slot);                                                 \
                                                                               \
    ka_args_append(args,                                                       \
                   (Ka_Arg){                                                   \
                       .names = flag_names,                                    \
                       .description = (options).description,                   \
                       .optional = (options).optional,                         \
                       .type = (type_tag),                                     \
                       .filled = false,                                        \
                       .slot = slot,                                           \
                   },                                                          \
                   flags);                                                     \
    return (ctype *)slot;
#define arg_fn(name, ctype, type_tag)                                          \
    KARGS_DEF ctype *ka__arg_##name(Ka_Args *args, const char *flag_names,     \
                                    Ka_ArgOptions options) {                   \
        fn_body(args, ctype, type_tag, flag_names, options)                    \
    }

#define EXPAND(name, ctype) arg_fn(name, ctype, Ka_ArgType_##name)

KA_TYPE_MAP(EXPAND)

#undef EXPAND
#undef arg_fn
#undef fn_body

#define ka_arg_boolean(args, flag_names, ...)                                  \
    ka__arg_boolean(args, flag_names, (Ka_ArgOptions){__VA_ARGS__})

#define ka_arg_string(args, flag_names, ...)                                   \
    ka__arg_string(args, flag_names, (Ka_ArgOptions){__VA_ARGS__})

#define ka_arg_int(args, flag_names, ...)                                      \
    ka__arg_int(args, flag_names, (Ka_ArgOptions){__VA_ARGS__})

static int ka__cmp(const void *a, const void *b) {
    const Ka_FlagName *flag_a = a, *flag_b = b;

    // NOTE: temporarily converted to a cstring already (see [^1])
    return strcmp(flag_a->name.items, flag_b->name.items);
}

static inline void ka__qsort_args(Ka_Args *args) {
    qsort(args->flag_names.items, args->flag_names.length, sizeof(Ka_FlagName),
          ka__cmp);
}

static inline size_t ka__bsearch_args(Ka_Args *args, Ka_StringView target) {
    Ka_FlagName key = {.name = target, .arg_idx = 0};
    Ka_FlagName *result =
        bsearch(&key, args->flag_names.items, args->flag_names.length,
                sizeof(Ka_FlagName), ka__cmp);

    if (!result) {
        return SIZE_MAX;
    }

    return result->arg_idx;
}

static inline Ka_StringView ka__get_usage(Ka_Arg *arg) {
    Ka_StringView str = ka_sv_from_cstr(arg->names);

    switch (arg->type) {
#define EXPAND(name, _)                                                        \
    case Ka_ArgType_##name:                                                    \
        ka_sv_push_cstr(&str, " <" #name ">");                                 \
        break;
        KA_TYPE_MAP(EXPAND)
#undef EXPAND

    default:
        ka_panic("unknown arg type");
    }

    return str;
}

#define KA_RESULT_OK                                                           \
    (Ka_Result) {}

static inline Ka_Result ka__parse_and_apply_to_slot(Ka_Arg *arg, char *s) {
    switch (arg->type) {
    case Ka_ArgType_int:;
        char *end;
        long x = strtol(s, &end, 10);

        if (*end == '\0') {
            *(int *)arg->slot = (int)x;
            return KA_RESULT_OK;
        } else {
            return (Ka_Result){.error = Ka_Error_INVALID_INT,
                               ka__get_usage(arg)};
        }

    case Ka_ArgType_string:;
        *(char **)arg->slot = s;
        return KA_RESULT_OK;

    case Ka_ArgType_boolean:;
        *(bool *)arg->slot = (strcmp(s, "true")) == 0;
        return KA_RESULT_OK;

    default:
        ka_panic("unknown arg type");
    }
}

KARGS_DEF Ka_Result ka_args_parse(Ka_Args *args, int argc, char **argv) {
    ka__qsort_args(args);
    Ka_Arg *current = NULL;

    for (int i = 0; i < argc; i++) {
        // first, we treat it as a possible flag
        Ka_StringView maybe_flag = ka_sv_from_cstr(argv[i]);

        if (current != NULL) {
            Ka_Result result = ka__parse_and_apply_to_slot(
                current, ka_sv_temp_cstr(&maybe_flag));

            if (result.error)
                return result;

            current->filled = true;
            current = NULL;
            continue;
        }

        size_t maybe_arg_idx = ka__bsearch_args(args, maybe_flag);

        if (maybe_arg_idx == SIZE_MAX) {
            return (Ka_Result){.error = Ka_Error_UNKNOWN_FLAG,
                               .error_data = maybe_flag};
        }

        current = &args->args_info.items[maybe_arg_idx];
        if (current->type == Ka_ArgType_boolean) {
            *(bool *)current->slot = true;
            current->filled = true;
        }
    }

    // if there are any leftover stuff, tell the user about it
    if (current != NULL && !current->filled) {
        return (Ka_Result){.error = Ka_Error_EXPECTED_VALUE,
                           .error_data = ka__get_usage(current)};
    }

    for (size_t i = 0; i < args->args_info.length; i++) {
        Ka_Arg *arg = &args->args_info.items[i];
        if (!arg->optional && !arg->filled) {
            return (Ka_Result){.error = Ka_Error_MISSING_FLAG,
                               .error_data = ka_sv_from_cstr(arg->names)};
        }
    }

    return KA_RESULT_OK;
}

KARGS_DEF const char *ka_get_error_message(Ka_Error error) {
    switch (error) {
#define EXPAND(name, msg, _)                                                   \
    case Ka_Error_##name:                                                      \
        return msg;

        KA_ERROR_MAP(EXPAND)
#undef EXPAND

    default:
        return "";
    }
}

/// Prints the error from the result.
KARGS_DEF void ka_print_error(Ka_Result result) {
    fprintf(stderr, ka_get_error_message(result.error),
            ka_sv_temp_cstr(&result.error_data));
    fprintf(stderr, "\n");
    ka__da_free(&result.error_data);
}

KARGS_DEF void ka_print_help(Ka_Args *args) {
    // find longest name lol
    size_t max_len = 0;
    for (size_t i = 0; i < args->args_info.length; i++) {
        size_t len = strlen(args->args_info.items[i].names);
        if (len > max_len)
            max_len = len;
    }

    fprintf(stdout, "Arguments:\n");
    for (size_t i = 0; i < args->args_info.length; i++) {
        Ka_Arg *arg = &args->args_info.items[i];
        fprintf(stdout, "  %-*s    %s\n", (int)max_len, arg->names,
                arg->description);
    }
}

KARGS_DEF void ka_args_entry(Ka_Args *args, int argc, char **argv) {
    Ka_Result result = ka_args_parse(args, argc - 1, argv + 1);
    if (result.error) {
        ka_print_error(result);
        exit(1);
    }
}

#endif // KARGS_IMPLEMENTATION
