#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#ifndef KARGS_DEF
#define KARGS_DEF
#endif // KARGS_DEF

//
// ==== impl Kargs ====
//
#ifdef KARGS_IMPLEMENTATION

// utilities
#define ka_panic(reason)                    \
    do {                                    \
        fprintf(stderr, "%s\n", reason);    \
        exit(1);                            \
    } while (0)

#define ka_assert_nonnull(ptr)                 \
    if ((ptr) == NULL) {                       \
        ka_panic("please buy ram");            \
    }                                          \

#define KA_DA_FIELDS(type)  \
    size_t length;          \
    size_t capacity;        \
    type* items;

#define ka__da_reserve(da, expected_capacity)                                           \
    do {                                                                                \
        if ((expected_capacity) > (da)->capacity) {                                     \
            if ((da)->capacity == 0) {                                                  \
                (da)->capacity = 128;                                                   \
            }                                                                           \
                                                                                        \
            while ((expected_capacity) > (da)->capacity) {                              \
                (da)->capacity *= 2;                                                    \
            }                                                                           \
                                                                                        \
            void* ptr = realloc((da)->items, (da)->capacity * sizeof(*(da)->items));    \
            ka_assert_nonnull(ptr);                                                     \
                                                                                        \
            (da)->items = ptr;                                                          \
        }                                                                               \
    } while (0)

#define ka__da_free(da) free((da)->items)

#define ka__da_append(da, item)                      \
    do {                                            \
        ka__da_reserve((da), (da)->length + 1);      \
        (da)->items[(da)->length++] = (item);       \
    } while (0)

typedef struct {
    KA_DA_FIELDS(char)
} Ka__StringView;

static inline size_t ka__sv_len(Ka__StringView* view) {
    return view->length;
}

static inline void ka__sv_push(Ka__StringView* view, char c) {
    ka__da_append(view, c);
}


typedef struct {
    char type; // 0 = SHORT, 1 = LONG
    Ka__StringView text;
} Ka__FlagName;

static Ka__FlagName ka__parse_flag_name(const char* s) {
    Ka__StringView collector = {0};
    int n_dashes = 0;

    while (*s) {
        if (ka__sv_len(&collector) == 0) {
            if (*s == '-') n_dashes += 1;
            else if (*s != ' ') ka__sv_push(&collector, *s);

            if (n_dashes >= 3)
                ka_panic("expected maximum of two dashes, got 3 or more");
        } else {
            if (*s == ' ')
                break;

            ka__sv_push(&collector, *s);
        }

        s++;
    }


    return (Ka__FlagName){
        .type = n_dashes - 1,
        .text = collector,
    };
}

typedef struct {
    KA_DA_FIELDS(Ka__FlagName)
} Ka__FlagNames;

static Ka__FlagNames ka__parse_flag_names(const char* s) {
    Ka__FlagNames parsed = {0};

    while (*s) {
        Ka__FlagName name = ka__parse_flag_name(s);
        ka__da_append(&parsed, name);
        s++;
    }

    return parsed;
}

#define SUPPORTED_TYPES(X) \
    X(Int, int) \
    X(String, char[]) \
    X(Bool, bool)

typedef enum {

#define EXPAND(name, type) Ka_ArgType_##name,
SUPPORTED_TYPES(EXPAND)
#undef EXPAND

} Ka_ArgType;

typedef struct {
    Ka_ArgType type;
    Ka__FlagNames names;

    const char* description;

    // we'll put the parsed result here when there's actually data
    void* slot;
} Ka_Arg;

typedef struct {
    KA_DA_FIELDS(Ka_Arg)
} Ka_Args;

KARGS_DEF void ka_args_append(Ka_Args* args, Ka_Arg arg) {
    ka__da_append(args, arg);
}

typedef struct {
    bool optional;
    char* description;
} Ka_ArgOptions;

#define ka_arg_bool(args, names, ...) \
    ka__arg_bool(args, names, (Ka_ArgOptions){ __VA_ARGS__ })

static bool* ka__arg_bool(Ka_Args* args, const char* flag_names, Ka_ArgOptions options) {
    Ka__FlagNames flags = ka__parse_flag_names(flag_names);

    void* slot = malloc(sizeof(bool));
    ka_assert_nonnull(slot);

    ka_args_append(
        args,
        (Ka_Arg){
            .type = Ka_ArgType_Bool,
            .names = flags,
            .description = options.description,
            .slot = slot,
        }
    );

    return (bool*)slot;
}

#endif // KARGS_IMPLEMENTATION
