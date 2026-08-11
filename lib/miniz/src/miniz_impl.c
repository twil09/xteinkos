/* Compiles the vendored miniz with Vix OS's configuration. Include order is
 * load-bearing: the config defines/renames must be seen before miniz.c. */
// clang-format off
#include "MinizConfig.h"

#include "../third_party/miniz.c"
// clang-format on
