#ifndef THREADS_FIXED_POINT_H
#define THREADS_FIXED_POINT_H

#include <stdint.h>

#define FRACTION (1 << 14)
#define TO_REAL(num) (num * FRACTION)
#define TO_INT(num) (num / FRACTION)

static inline int32_t R_plus_I(int32_t real, int integer)
{
   return real + TO_REAL(integer);
}

static inline int32_t I_plus_R(int integer, int32_t real)
{
   return R_plus_I(real, integer);
}

static inline int32_t R_plus_R(int32_t real1, int32_t real2)
{
   return real1 + real2;
}

static inline int32_t R_minus_I(int32_t real, int integer)
{
   return real - TO_REAL(integer);
}

static inline int32_t I_minus_R(int integer, int32_t real)
{
   return TO_REAL(integer) - real;
}

static inline int32_t R_minus_R(int32_t real1, int32_t real2)
{
   return real1 - real2;
}

static inline int32_t I_multiply_R(int integer, int32_t real)
{
   return integer * real;
}

static inline int32_t R_multiply_I(int32_t real, int integer)
{
   return I_multiply_R(integer, real);
}

static inline int32_t R_multiply_R(int32_t real1, int32_t real2)
{
   return TO_INT((int64_t)real1 * (int64_t)real2);
}

static inline int32_t R_divide_I(int32_t real, int integer)
{
   return real / integer;
}

static inline int32_t I_divide_R(int integer, int32_t real)
{
   return TO_REAL(integer) / real;
}

static inline int32_t R_divide_R(int32_t real1, int32_t real2)
{
   return TO_REAL((int64_t)real1) / (int64_t)real2;
}

#endif /* threads/fixed_point.h */
