#ifndef GITEN_UTIL_COMPARE_H
#define GITEN_UTIL_COMPARE_H

#include <Enums.h>
#include <Ints.h>

GZ_ENUM_BEGIN(ComparisonOperator)
COMPARE_NOT_EQUAL = 0, COMPARE_EQUAL = 1, COMPARE_LESS_EQUAL = 2, COMPARE_GREATER_EQUAL = 3,
                       COMPARE_LESS = 4, COMPARE_GREATER = 5,
                       GZ_ENUM_END(ComparisonOperator)

                           i16 CompareInt(i32 a, i32 b);
b32 CompareByOp(ComparisonOperator op, i32 a, i32 b);

#endif // GITEN_UTIL_COMPARE_H
