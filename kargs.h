#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifndef KARGS_DEF
#define KARGS_DEF
#endif // KARGS_DEF

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

#define EXPAND(name, type) Ka__ArgType_##name,
    KA_TYPE_MAP(EXPAND)
#undef EXPAND

} Ka__ArgType;

// enum Error;
typedef enum {

#define EXPAND(name, _, tag) Ka_Error_##name = tag,
    KA_ERROR_MAP(EXPAND)
#undef EXPAND

} Ka_Error;

/*
 * impl kargs
 */
#ifdef KARGS_IMPLEMENTATION

/*
 * utilities
 */

// Specifies the behavior when kargs panics.
// You can define this macro yourself so it does something else
// other than this.
#ifndef ka_panic
#include <stdio.h>

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

// Required fields for dynamic arrays
#define KA__DA_FIELDS(type)                                                    \
    size_t length;                                                             \
    size_t capacity;                                                           \
    type *items;

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

typedef struct {
    KA__DA_FIELDS(char)
} Ka_StringView;

/// Gets the length of the string view.
static inline size_t ka_sv_len(Ka_StringView *view) { return view->length; }

/// Pushes a character onto the string view.
static inline void ka_sv_push(Ka_StringView *view, char c) {
    ka__da_append(view, c);
}

/// Creates a string view from a CStr.
static inline void ka_sv_push_cstr(Ka_StringView *view, const char *cstr) {
    while (*cstr) {
        ka_sv_push(view, *cstr);
        cstr++;
    }
}

/// Makes the string temporarily a CStr.
/// Use `ka_sv_decstr()` later to turn it back to normal.
static inline char *ka_sv_temp_cstr(Ka_StringView *view) {
    ka_sv_push(view, '\0');
    return view->items;
}

static inline void ka_sv_decstr(Ka_StringView *view) { view->length--; }

/// Creates a string view from a CStr.
static inline Ka_StringView ka_sv_from_cstr(const char *cstr) {
    size_t len = strlen(cstr);
    Ka_StringView view = {0};

    ka__da_reserve(&view, len);
    ka_sv_push_cstr(&view, cstr);

    return view;
}

// struct Arg;
typedef struct {
    bool filled;

    Ka__ArgType type;
    bool optional;
    const char *names;
    const char *description;

    // we'll put the parsed result here when there's actually data
    void *slot;
} Ka__Arg;

typedef struct {
    KA__DA_FIELDS(Ka__Arg)
} Ka__ArgsInfoContainer;

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

// struct FlagName;
typedef struct {
    Ka_StringView name;
    size_t arg_idx;
} Ka__FlagName;

typedef struct {
    KA__DA_FIELDS(Ka__FlagName)
} Ka__FlagNames;

static Ka__FlagNames ka__parse_flag_names(const char *s) {
    Ka__FlagNames parsed = {0};

    while (*s) {
        Ka__FlagParseResult result = ka__parse_flag_name(s);
        s = result.next;

        ka__da_append(&parsed,
                      ((Ka__FlagName){.name = result.str, .arg_idx = 0}));
    }

    return parsed;
}

// struct Args;
typedef struct {
    Ka__ArgsInfoContainer args_info;
    Ka__FlagNames flag_names;
} Ka_Args;

KARGS_DEF void ka_args_append(Ka_Args *args, Ka__Arg arg, Ka__FlagNames names) {
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

/// Frees the arg parser (`Ka_Args`) and all the dynamic arrays within.
KARGS_DEF void ka_args_free(Ka_Args args) {
    for (size_t i = 0; i < args.args_info.length; i++) {
        free(args.args_info.items[i].slot);
    }

    ka__da_free(&args.args_info);
    ka__da_free(&args.flag_names);
}

typedef struct {
    bool optional;
    char *description;
} Ka__ArgOptions;

#define fn_body(args, ctype, type_tag, flag_names, options)                    \
    Ka__FlagNames flags = ka__parse_flag_names((flag_names));                  \
                                                                               \
    void *slot = calloc(1, sizeof(ctype));                                     \
    ka_assert_allocated(slot);                                                 \
                                                                               \
    ka_args_append(args,                                                       \
                   (Ka__Arg){                                                  \
                       .names = flag_names,                                    \
                       .description = (options).description,                   \
                       .optional = (options).optional,                         \
                       .type = (type_tag),                                     \
                       .filled = false,                                        \
                       .slot = slot,                                           \
                   },                                                          \
                   flags);                                                     \
    return slot;
#define arg_fn(name, ctype, type_tag)                                          \
    KARGS_DEF ctype *ka__arg_##name(Ka_Args *args, const char *flag_names,     \
                                    Ka__ArgOptions options) {                  \
        fn_body(args, ctype, type_tag, flag_names, options)                    \
    }

#define EXPAND(name, ctype) arg_fn(name, ctype, Ka__ArgType_##name)

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
    ka__arg_boolean(args, flag_names, (Ka__ArgOptions){__VA_ARGS__})

#define ka_arg_string(args, flag_names, ...)                                   \
    ka__arg_string(args, flag_names, (Ka__ArgOptions){__VA_ARGS__})

#define ka_arg_int(args, flag_names, ...)                                      \
    ka__arg_int(args, flag_names, (Ka__ArgOptions){__VA_ARGS__})

static int ka__cmp(const void *a, const void *b) {
    const Ka__FlagName *flag_a = a, *flag_b = b;

    // NOTE: temporarily converted to a cstring already (see [^1])
    return strcmp(flag_a->name.items, flag_b->name.items);
}

static inline void ka__qsort_args(Ka_Args *args) {
    qsort(args->flag_names.items, args->flag_names.length, sizeof(Ka__FlagName),
          ka__cmp);
}

static inline size_t ka__bsearch_args(Ka_Args *args, Ka_StringView target) {
    Ka__FlagName key = {.name = target, .arg_idx = 0};
    Ka__FlagName *result =
        bsearch(&key, args->flag_names.items, args->flag_names.length,
                sizeof(Ka__FlagName), ka__cmp);

    if (!result) {
        return SIZE_MAX;
    }

    return result->arg_idx;
}

static inline Ka_StringView ka__get_usage(Ka__Arg *arg) {
    Ka_StringView str = ka_sv_from_cstr(arg->names);

    switch (arg->type) {
#define EXPAND(name, _)                                                        \
    case Ka__ArgType_##name:                                                   \
        ka_sv_push_cstr(&str, " <" #name ">");                                 \
        break;
        KA_TYPE_MAP(EXPAND)
#undef EXPAND

    default:
        ka_panic("unknown arg type");
    }

    return str;
}

typedef struct {
    Ka_Error error;
    Ka_StringView error_data;
} Ka_Result;

#define KA_RESULT_OK                                                           \
    (Ka_Result) {}

static inline Ka_Result ka__parse_and_apply_to_slot(Ka__Arg *arg, char *s) {
    switch (arg->type) {
    case Ka__ArgType_int:;
        char *end;
        long x = strtol(s, &end, 10);

        if (*end == '\0') {
            *(int *)arg->slot = (int)x;
            return KA_RESULT_OK;
        } else {
            return (Ka_Result){.error = Ka_Error_INVALID_INT,
                               ka__get_usage(arg)};
        }

    case Ka__ArgType_string:;
        *(char **)arg->slot = s;
        return KA_RESULT_OK;

    case Ka__ArgType_boolean:;
        *(bool *)arg->slot = (strcmp(s, "true")) == 0;
        return KA_RESULT_OK;

    default:
        ka_panic("unknown arg type");
    }
}

KARGS_DEF Ka_Result ka_args_parse(Ka_Args *args, int argc, char **argv) {
    ka__qsort_args(args);
    Ka__Arg *current = NULL;

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
        if (current->type == Ka__ArgType_boolean) {
            *(bool *)current->slot = true;
            current->filled = true;
        }
    }

    // if there are any leftover stuff, tell the user about it
    if (current != NULL) {
        return (Ka_Result){.error = Ka_Error_EXPECTED_VALUE,
                           .error_data = ka__get_usage(current)};
    }

    for (size_t i = 0; i < args->args_info.length; i++) {
        Ka__Arg *arg = &args->args_info.items[i];
        if (!arg->optional && !arg->filled) {
            return (Ka_Result){.error = Ka_Error_MISSING_FLAG,
                               .error_data = ka_sv_from_cstr(arg->names)};
        }
    }

    return KA_RESULT_OK;
}

const char *ka_get_error_message(Ka_Error error) {
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

#endif // KARGS_IMPLEMENTATION
