#pragma once

// mdxmini.h is a C header with a struct field named 'songdata' that shadows
// the typedef of the same name, triggering -Wchanges-meaning in C++23.
// Suppress the warning and wrap in extern "C" for correct linkage.

#ifdef __cplusplus
extern "C" {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchanges-meaning"
#endif

#include <mdxmini.h>

#ifdef __cplusplus
#pragma GCC diagnostic pop
} // extern "C"
#endif
