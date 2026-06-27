#include <stdbool.h>

#ifndef KARGS_ALLOC
#include <stdlib.h>
#endif // KARGS_ALLOC

#ifndef KARGS_DEF
#define KARGS_DEF
#endif // KARGS_DEF

#define KA_SUPPORTED_TYPES(X)                                                  \
  X(int, int)                                                                  \
  X(string, char *)                                                            \
  X(boolean, bool)

typedef enum {

#define EXPAND(name, type) Ka_ArgType_##name,
  KA_SUPPORTED_TYPES(EXPAND)
#undef EXPAND

} Ka_ArgType;

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
  do {                                                                         \
    fprintf(stderr, "kargs panicked: %s\n", reason);                           \
    exit(1);                                                                   \
  } while (0)
#endif // ka_panic

// Specifies the behavior when asserting whether the allocation
// had failed. This just detects null pointers. Stoopid ahh
#define ka_assert_allocated(ptr)                                               \
  if ((ptr) == NULL) {                                                         \
    ka_panic("allocation failed");                                             \
  }

// Required fields for dynamic arrays
#define KA__DA_FIELDS(type)                                                    \
  size_t length;                                                               \
  size_t capacity;                                                             \
  type *items;

#define ka__da_reserve(da, expected_capacity)                                  \
  do {                                                                         \
    if ((expected_capacity) > (da)->capacity) {                                \
      if ((da)->capacity == 0) {                                               \
        (da)->capacity = 128;                                                  \
      }                                                                        \
                                                                               \
      while ((expected_capacity) > (da)->capacity) {                           \
        (da)->capacity *= 2;                                                   \
      }                                                                        \
                                                                               \
      void *ptr = realloc((da)->items, (da)->capacity * sizeof(*(da)->items)); \
      ka_assert_allocated(ptr);                                                \
                                                                               \
      (da)->items = ptr;                                                       \
    }                                                                          \
  } while (0)

#define ka__da_free(da) free((da)->items)

#define ka__da_append(da, item)                                                \
  do {                                                                         \
    ka__da_reserve((da), (da)->length + 1);                                    \
    (da)->items[(da)->length++] = (item);                                      \
  } while (0)

typedef struct {
  KA__DA_FIELDS(char)
} Ka__StringView;

static inline size_t ka__sv_len(Ka__StringView *view) { return view->length; }

static inline void ka__sv_push(Ka__StringView *view, char c) {
  ka__da_append(view, c);
}

typedef struct {
  char type; // 0 = SHORT, 1 = LONG
  Ka__StringView text;
} Ka__FlagName;

static Ka__FlagName ka__parse_flag_name(const char *s) {
  Ka__StringView collector = {0};
  int n_dashes = 0;

  while (*s) {
    if (ka__sv_len(&collector) == 0) {
      if (*s == '-')
        n_dashes += 1;
      else if (*s != ' ')
        ka__sv_push(&collector, *s);

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
  KA__DA_FIELDS(Ka__FlagName)
} Ka__FlagNames;

static Ka__FlagNames ka__parse_flag_names(const char *s) {
  Ka__FlagNames parsed = {0};

  while (*s) {
    Ka__FlagName name = ka__parse_flag_name(s);
    ka__da_append(&parsed, name);
    s++;
  }

  return parsed;
}

typedef struct {
  Ka_ArgType type;
  Ka__FlagNames names;

  const char *description;

  // we'll put the parsed result here when there's actually data
  void *slot;
} Ka_Arg;

typedef struct {
  KA__DA_FIELDS(Ka_Arg)
} Ka_Args;

KARGS_DEF void ka_args_append(Ka_Args *args, Ka_Arg arg) {
  ka__da_append(args, arg);
}

/// Frees the arg parser (`Ka_Args`) and all the dynamic arrays within.
KARGS_DEF void ka_args_free(Ka_Args args) {
  for (size_t i = 0; i < args.length; i++) {
    free(args.items[i].slot);
  }

  ka__da_free(&args);
}

typedef struct {
  bool optional;
  char *description;
} Ka_ArgOptions;

#define ka__arg_fn_body(args, ctype, type_tag, flag_names, options)            \
  Ka__FlagNames flags = ka__parse_flag_names((flag_names));                    \
                                                                               \
  void *slot = malloc(sizeof(ctype));                                          \
  ka_assert_allocated(slot);                                                   \
                                                                               \
  ka_args_append(args, (Ka_Arg){                                               \
                           .type = (type_tag),                                 \
                           .names = flags,                                     \
                           .description = (options).description,               \
                           .slot = slot,                                       \
                       });                                                     \
  return slot;

#define ka__arg_fn(name, ctype, type_tag)                                      \
  KARGS_DEF ctype *ka__arg_##name(Ka_Args *args, const char *flag_names,       \
                                  Ka_ArgOptions options) {                     \
    ka__arg_fn_body(args, ctype, type_tag, flag_names, options)                \
  }

#define EXPAND(name, ctype) ka__arg_fn(name, ctype, Ka_ArgType_##name)
KA_SUPPORTED_TYPES(EXPAND)
#undef EXPAND

#define ka_arg_boolean(args, flag_names, ...)                                  \
  ka__arg_boolean(args, flag_names, (Ka_ArgOptions){__VA_ARGS__})

#define ka_arg_string(args, flag_names, ...)                                   \
  ka__arg_string(args, flag_names, (Ka_ArgOptions){__VA_ARGS__})

#define ka_arg_int(args, flag_names, ...)                                      \
  ka__arg_int(args, flag_names, (Ka_ArgOptions){__VA_ARGS__})

#endif // KARGS_IMPLEMENTATION
