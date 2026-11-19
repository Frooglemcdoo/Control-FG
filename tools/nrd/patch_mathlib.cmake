# MathLib v11 uses frexp only for its exponent output. Newer MSVC headers mark
# the fractional return value nodiscard. Explicitly discard it, retaining the
# call and exponent update rather than disabling warnings for the dependency.
set(path "${MATHLIB_SOURCE}/ml.h")
file(READ "${path}" source)
set(before "    frexp(fValue, &exponent);")
set(after "    (void)frexp(fValue, &exponent);")
string(REGEX MATCHALL "    frexp\\(fValue, &exponent\\)" calls "${source}")
list(LENGTH calls count)
if(count EQUAL 2)
 string(REPLACE "${before}" "${after}" source "${source}")
 file(WRITE "${path}" "${source}")
else()
 string(REGEX MATCHALL "    \\(void\\)frexp\\(fValue, &exponent\\)" calls "${source}")
 list(LENGTH calls count)
 if(NOT count EQUAL 2)
  message(FATAL_ERROR "MathLib frexp calls changed; review compatibility patch")
 endif()
endif()
