#ifndef LADR_TOGGLE_H
#define LADR_TOGGLE_H

#include <complex>
#include <iostream>
#include "ladr_common.h"

#pragma comment(lib, "ladr.lib")

template <typename T_>
static inline void _ladr_check(T_ *data, unsigned len, double eps,
                               const char *id = NULL)
{
	std::cerr << __func__ ": type is not supported" << std::endl;
}

// Define all the check function types that can be called
#define def_check_func(TYPE, ENUM) \
template <> static inline void \
_ladr_check< TYPE >(TYPE *data, unsigned len, double eps, const char *id) \
{ \
	ladr_dll_check((void *)data, len, eps, ENUM, id); \
}

def_check_func(float, LADR_CHECK_T::CHK_FLT)
def_check_func(double, LADR_CHECK_T::CHK_DBL)
def_check_func(int, LADR_CHECK_T::CHK_INT)
def_check_func(std::complex<float>, LADR_CHECK_T::CHK_CMP_FLT)
def_check_func(std::complex<double>, LADR_CHECK_T::CHK_CMP_DBL)

// Macro trick to accepts variadic arguments of different types
#define _ladr_config(option, value, ...) \
do { \
	decltype(value) __vals[] = { value, __VA_ARGS__ }; \
	unsigned __i; \
	for (__i = 0; __i < sizeof(__vals) / sizeof(value); __i++) { \
		ladr_dll_config(option, (unsigned long)__vals[__i]); \
	} \
} while (0)

#endif