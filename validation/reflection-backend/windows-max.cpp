// Load standard headers first, then reproduce windows.h's macro in the actual
// production backend. The macro must remain active across the include.
#include <array>
#include <cstdint>
#include <cstddef>
#include <limits>
#include <cassert>
#include <map>
#include <vector>
#include <iostream>
#define max(a,b) (((a) > (b)) ? (a) : (b))
#include "test_backend.cpp"
#ifndef max
#error Regression must compile with the Windows-style max macro active
#endif
