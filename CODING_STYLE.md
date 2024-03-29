# Coding Style

The coding-style used for the project is derived from the K&R
https://www.kernel.org/doc/html/v4.10/process/coding-style.html

## Compiling Macros

`Makefile` macros the controls the engine behaviour are in the form `TOFU_XXX`. Macro that controls only the build process (for example, to specify the platform) doesn't have this prefix.

Behavioural macros are also present in the file `config.h`.

## Identifiers

For the identifiers is adopted the [snake-case]() style.

## Preprocessor Conditionals

The preferred form for preprocessor conditional is the "expanded one", that is

```c
#if defined(DEBUG)
   // ...
#endif  /* defined(DEBUG) */
```

over

```c
#ifdef DEBUG
   // ...
#endif  /* DEBUG */
```

This is due to better consistency when more than one condition is to be checked.

The only exception to this is for the include-guards (see below), which uses the compact version.

> Coincidentally this is the same approach used in Lua's codebase.

## Module Debugging

Every sub-system should use a macro with identifier `TOFU_<module-name>_DEBUG_ENABLED` to control a fine log/debug
level to be used. The macro should be defined in the `config.h` file.

## Type Definitions

Types are defined with [Pascal-case]() style. The following suffixes are used:

* `_s` for structures,
* `_e` for enumerations,
* `_u` for unions,
* `_t` for typedefs.

## Formatting Rules

* source files are terminated on a new line;
* trailing spaces are to be eliminated;
* 

## Single Header Libraries

Single header libraries are tolerated and used without any issue. They are, however, adapted into a header/module pair
for convenience and clarity.

For example, the single header library `stb_image.h` would be adapted by defining the header file

```c
#ifndef STB_IMAGE_H
#define STB_IMAGE_H

#include <stb/stb_image.h>

#endif  /* STB_IMAGE_H */
```

and the source file

```c
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
```

## Source Code Structure

An header file must be constructed as follows:

* copyright/license header,
* includes section,
* module local macro definitions,
* module local variables,
* functions and procedures.

Local scoped functions/procedures/variable are `static` and prefixed with the `_` (underscore) character. They are to be defined
just before they use. For example, if we a a function `foo_bar()` that uses the `_baz()` function, the would be defined
as follows

```c
static const int _value = 42;

static inline int _baz(void)
{
    return _value;
}

int foo_bar(void)
{
    return _baz();
}
```

> In case the body of the local function is small enough, declare it as `inline` as an hint for the compiler.   

## Include Position and Ordering

Include directives are placed at the top of the file, right after the file header banner.

They are defined with the following priority/order:

* quoted module `.h` file
* quoted local `.h` files
* angled-brackets project-specific library `.h` files
* angled-brackets external library `.h` files
* angled-brackets standard library `.h` files

This is an example usage.

```c
#include "configuration.h"

#include "internal/parser.h"

#include <core/version.h>

#include <GLFW/glfw3.h>

#include <string.h>
```

## Include Guards

[Include guards](https://en.wikipedia.org/wiki/Include_guard) are used in each and every `.h` file. They are assigned
with the following format:

```c
#ifndef PATH_FILE_H
#define PATH_FILE_H

#define FILE_H_INCLUDED

/* Body of the header file */

#endif  /* PATH_FILE_H */
```

with `PATH_FILE_H` being the capitalized relative (to the project `src` folder) pathname of the file. Any folder
separator character is replaced with the `_` (underscore) character. This will ensure that two distinct files with the
same name won't share the same guard definition.

We alse define the `FILE_H_INCLUDED` macro as it is useful anytime we need to test if a core header file is included
(e.g. `config.h` or `platform.h`).

## Object-Orientation

It's well established that even non OO languages can be used to implement an object-oriented approach to code.

In case polymorphism and abstraction is to be implemented (e.g. see the `fs.h` module) the preferred style is the
*vtable struct* approach. We defined a `struct` of functions pointers where the first formal argument is a pointer
to the object `struct`. In the object `struct` the v-table is the first field (so that *sub-classes* can extend
it while sharing a similar memory layout), followed by any required additional fields.

```c
typedef struct Object_s Object_t; // Opaque type, we expose only a pointer to it.

typedef struct Object_VTable_s {
    void (*dtor)(Object_t *self);
} Object_VTable_t;

struct Object_s {
    Object_VTable_t vtable;
};

static void _object_dtor(Object_t *self)
{
    *self = (Object_t){ 0 };
}

static void _object_ctor(Object_t *self) // Pass any other parameter, if required.
{
    *self = (Object_t){
            .vtable = (Object_VTable_t){
                .dtor = _object_dtor
            }
        };

    // Initialize the object fields, here.
}

Object_t *object_new()
{
    Object_t *object = malloc(sizeof(Object_t));
    _object_ctor(object);
    return object;
}

void object_delete(Object_t *self)
{
    self->vtable.dtor(self);
    free(self);
}
```

The constructor function is not present in the v-table, but it's defined and used to initialized the object structure.

## Binary Files

### Byte Ordering

Custom binary files are stored in a way that network-byte-order is (i.e. big-endian) is used to store informations.

## Lua FFI

### Enumerations

We are implementing enumerated values as strings with a (case insensitive) value among a list of available ones.

Typically we convert the string value to an `enum` (integer) value. We conventionally assume and strive to make that there's an implicit one-to-one relation between a string and the matching integer value. For example:

```c
// In the `input.h` file, the enumeration is defined as such.
typedef enum Input_Controller_Sticks_e {
    Input_Controller_Sticks_t_First = 0,
    INPUT_CONTROLLER_STICK_LEFT = Input_Controller_Sticks_t_First,
    INPUT_CONTROLLER_STICK_RIGHT,
    Input_Controller_Sticks_t_Last = INPUT_CONTROLLER_STICK_RIGHT,
    Input_Controller_Sticks_t_CountOf
} Input_Controller_Sticks_t;

// Later, in the `controller.c` file, we are defining this mapping.
static const char *_sticks[Input_Controller_Sticks_t_CountOf + 1] = {
    "left",
    "right",
    NULL
};
```

This need a special care and attention, but permits to avoid an intermediate int-to-enum decoupling array. This is both an optimization in (code) space and (execution) time.

> This, of course, requires that the enumeration starts from `0` and proceeds incrementally w/o any "hole".
