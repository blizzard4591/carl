/** 
 * @file   adaption_gmpxx/operations.h
 * @ingroup gmpxx
 * @author Gereon Kremer <gereon.kremer@cs.rwth-aachen.de>
 * @author Sebastian Junges
 * 
 * @warning This file should never be included directly but only via operations.h
 */

#pragma once
#include "../../util/platform.h"
#include <cstddef>
#include <limits.h>
#include <sstream>
#include <vector>
#include "typetraits.h"
#include "boost/algorithm/string.hpp"
#include "../constants.h"
#include "../operations.h"

namespace carl {


/**
 * Informational functions
 * 
 * The following functions return informations about the given numbers.
 */
inline bool isZero(const mpz_class& n) {
	return constant_zero<mpz_class>::get() == n;
}

inline bool isZero(const mpq_class& n) {
	return constant_zero<mpz_class>::get() == n;
}

inline bool isOne(const mpz_class& n) {
	return constant_one<mpz_class>::get() == n;
}

inline bool isOne(const mpq_class& n) {
	return constant_one<mpz_class>::get() == n;
}

inline bool isPositive(const mpz_class& n) {
	return n > carl::constant_zero<mpz_class>().get();
}	
	
inline bool isPositive(const mpq_class& n) {
	return n > carl::constant_zero<mpz_class>().get();
}

inline bool isNegative(const mpz_class& n) {
	return n < carl::constant_zero<mpz_class>().get();
}	
	
inline bool isNegative(const mpq_class& n) {
	return n < carl::constant_zero<mpz_class>().get();
}
	
inline mpz_class getNum(const mpq_class& n) {
	return n.get_num();
}

inline mpz_class getNum(const mpz_class& n) {
	return n;
}

inline mpz_class getDenom(const mpq_class& n) {
	return n.get_den();
}

inline mpz_class getDenom(const mpz_class& n) {
	return n;
}

inline bool isInteger(const mpq_class& n) {
	 return 0 != mpz_divisible_p(n.get_num_mpz_t(), n.get_den_mpz_t());
}

inline bool isInteger(const mpz_class&) {
	return true;
}

/**
 * Get the bit size of the representation of a integer.
 * @param n An integer.
 * @return Bit size of n.
 */
inline std::size_t bitsize(const mpz_class& n) {
	return mpz_sizeinbase(n.__get_mp(),10);
}
/**
 * Get the bit size of the representation of a fraction.
 * @param n A fraction.
 * @return Bit size of n.
 */
inline std::size_t bitsize(const mpq_class& n) {
	return mpz_sizeinbase(getNum(n).__get_mp(),10) + mpz_sizeinbase(getDenom(n).__get_mp(),10);
}

/**
 * Conversion functions
 * 
 * The following function convert types to other types.
 */

inline double toDouble(const mpq_class& n) {
	return n.get_d();
}
inline double toDouble(const mpz_class& n) {
	return n.get_d();
}

template<typename Integer>
inline Integer toInt(const mpz_class& n);
template<>
inline signed long int toInt<signed long int>(const mpz_class& n) {
    assert(n <= std::numeric_limits<signed long int>::max());
    assert(n >= std::numeric_limits<signed long int>::min());
    return mpz_get_si(n.get_mpz_t());
}
template<>
inline unsigned long int toInt<unsigned long int>(const mpz_class& n) {
    assert(n <= std::numeric_limits<unsigned long int>::max());
    assert(n >= std::numeric_limits<unsigned long int>::min());
    return mpz_get_ui(n.get_mpz_t());
}
template<>
inline int toInt<int>(const mpz_class& n) {
    std::cout << "mpz_class to int" << std::endl;
    assert(n <= std::numeric_limits<int>::max());
    assert(n >= std::numeric_limits<int>::min());
    return (int)mpz_get_si(n.get_mpz_t());
}
template<>
inline unsigned toInt<unsigned>(const mpz_class& n) {
    assert(n <= std::numeric_limits<unsigned>::max());
    assert(n >= std::numeric_limits<unsigned>::min());
    return (unsigned)mpz_get_ui(n.get_mpz_t());
}
template<typename Integer>
inline Integer toInt(const mpq_class& n);

/**
 * Convert a fraction to an integer.
 * This method assert, that the given fraction is an integer, i.e. that the denominator is one.
 * @param n A fraction.
 * @return An integer.
 */
template<>
inline mpz_class toInt<mpz_class>(const mpq_class& n) {
	assert(isInteger(n));
	return getNum(n);
}

/**
 * Convert a fraction to an unsigned.
 * @param n A fraction.
 * @return n as unsigned.
 */
template<>
inline unsigned toInt<unsigned>(const mpq_class& n) {
	return toInt<unsigned>(toInt<mpz_class>(n));
}
template<>
inline unsigned long int toInt<unsigned long int>(const mpq_class& n) {
	return toInt<unsigned long int>(toInt<mpz_class>(n));
}
template<>
inline int toInt<int>(const mpq_class& n) {
    std::cout << "mpq_class to int" << std::endl;
	return toInt<int>(toInt<mpz_class>(n));
}
template<>
inline signed long int toInt<signed long int>(const mpq_class& n) {
	return toInt<signed long int>(toInt<mpz_class>(n));
}

template<typename T>
inline T rationalize(double n);

template<typename T>
inline T rationalize(float n);

template<typename T>
inline T rationalize(int n);

template<typename T>
inline T rationalize(size_t n);

template<typename T>
inline T rationalize(const std::string& n);

#ifdef USE_CLN_NUMBERS
template<typename T>
inline T rationalize(const PreventConversion<cln::cl_RA>&);
#endif

template<typename T>
inline T rationalize(const PreventConversion<mpq_class>&);

template<>
inline mpq_class rationalize<mpq_class>(float f) {
	return mpq_class(f);
}

template<>
inline mpq_class rationalize<mpq_class>(double d) {
	return mpq_class(d);
}

template<>
inline mpq_class rationalize<mpq_class>(size_t n) {
	return mpq_class(n);
}

template<>
inline mpq_class rationalize<mpq_class>(int n) {
	return mpq_class(n);
}

template<>
mpq_class rationalize<mpq_class>(const std::string& inputstring);

template<>
inline mpq_class rationalize<mpq_class>(const PreventConversion<mpq_class>& n) {
	return n;
}

#ifdef USE_CLN_NUMBERS
template<>
mpq_class rationalize<mpq_class>(const PreventConversion<cln::cl_RA>& n);
#endif

/**
 * Basic Operators
 * 
 * The following functions implement simple operations on the given numbers.
 */

inline mpz_class abs(const mpz_class& n) {
	mpz_class res;
	mpz_abs(res.get_mpz_t(), n.get_mpz_t());
	return res;
}

inline mpq_class abs(const mpq_class& n) {
	mpq_class res;
	mpq_abs(res.get_mpq_t(), n.get_mpq_t());
	return res;
}

inline mpz_class floor(const mpq_class& n) {
	if (isZero(mpz_class(n.get_num_mpz_t()))) return carl::constant_zero<mpz_class>::get();
	mpz_class res;
	mpz_fdiv_q(res.get_mpz_t(), n.get_num_mpz_t(), n.get_den_mpz_t());
	return res;
}

inline mpz_class floor(const mpz_class& n) {
	return n;
}

inline mpz_class ceil(const mpq_class& n) {
	if (isZero(mpz_class(n.get_num_mpz_t()))) return carl::constant_zero<mpz_class>::get();
	mpz_class res;
	mpz_cdiv_q(res.get_mpz_t(), n.get_num_mpz_t(), n.get_den_mpz_t());
	return res;
}
inline mpz_class ceil(const mpz_class& n) {
	return n;
}

inline mpz_class gcd(const mpz_class& a, const mpz_class& b) {
	mpz_class res;
	mpz_gcd(res.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
	return res;
}

inline mpz_class lcm(const mpz_class& a, const mpz_class& b) {
	mpz_class res;
	mpz_lcm(res.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
	return res;
}

inline mpq_class gcd(const mpq_class& a, const mpq_class& b) {
    mpz_class resNum;
	mpz_gcd(resNum.get_mpz_t(), getNum(a).get_mpz_t(), getNum(b).get_mpz_t());
	mpz_class resDen;
	mpz_lcm(resDen.get_mpz_t(), getDenom(a).get_mpz_t(), getDenom(b).get_mpz_t());
	mpq_class resqNum;
	mpq_set_z(resqNum.get_mpq_t(), resNum.get_mpz_t());
	mpq_class resqDen;
	mpq_set_z(resqDen.get_mpq_t(), resDen.get_mpz_t());
	mpq_class res;
	mpq_div(res.get_mpq_t(), resqNum.get_mpq_t(), resqDen.get_mpq_t());
	return res;
}

/**
 * Calculate the greatest common divisor of two integers.
 * Stores the result in the first argument.
 * @param a First argument.
 * @param b Second argument.
 * @return Updated a.
 */
inline mpz_class& gcd_assign(mpz_class& a, const mpz_class& b) {
    a = carl::gcd(a,b);
	return a;
}

/**
 * Calculate the greatest common divisor of two integers.
 * Stores the result in the first argument.
 * @param a First argument.
 * @param b Second argument.
 * @return Updated a.
 */
inline mpq_class& gcd_assign(mpq_class& a, const mpq_class& b) {
    a = carl::gcd(a,b);
	return a;
}

inline mpq_class lcm(const mpq_class& a, const mpq_class& b) {
    mpz_class resNum;
	mpz_lcm(resNum.get_mpz_t(), getNum(a).get_mpz_t(), getNum(b).get_mpz_t());
	mpz_class resDen;
	mpz_gcd(resDen.get_mpz_t(), getDenom(a).get_mpz_t(), getDenom(b).get_mpz_t());
	mpq_class resqNum;
	mpq_set_z(resqNum.get_mpq_t(), resNum.get_mpz_t());
	mpq_class resqDen;
	mpq_set_z(resqDen.get_mpq_t(), resDen.get_mpz_t());
	mpq_class res;
	mpq_div(res.get_mpq_t(), resqNum.get_mpq_t(), resqDen.get_mpq_t());
	return res;
}

template<>
inline mpz_class pow(const mpz_class& b, std::size_t e) {
	mpz_class res;
	mpz_pow_ui(res.get_mpz_t(), b.get_mpz_t(), e);
	return res;
}

template<>
inline mpq_class pow(const mpq_class& b, std::size_t e) {
	mpz_class den = b.get_den();
	mpz_class powDen;
	mpz_pow_ui(powDen.get_mpz_t(), den.get_mpz_t(), e);
	mpz_class num = b.get_num();
	mpz_class powNum;
	mpz_pow_ui(powNum.get_mpz_t(), num.get_mpz_t(), e);
	mpq_class resNum;
	mpq_set_z(resNum.get_mpq_t(), powNum.get_mpz_t());
	mpq_class resDen;
	mpq_set_z(resDen.get_mpq_t(), powDen.get_mpz_t());
	mpq_class res;
	mpq_div(res.get_mpq_t(), resNum.get_mpq_t(), resDen.get_mpq_t());
	return res;
}

/**
 * Calculate the square root of a fraction if possible.
 * 
 * @param a The fraction to calculate the square root for.
 * @param b A reference to the rational, in which the result is stored.
 * @return true, if the number to calculate the square root for is a square;
 *         false, otherwise.
 */
bool sqrtp(const mpq_class& a, mpq_class& b);

std::pair<mpq_class,mpq_class> sqrt(const mpq_class& a);

/**
 * Compute square root in a fast but less precise way.
 * Use cln::sqrt() to obtain an approximation. If the result is rational, i.e. the result is exact, use this result.
 * Otherwise use the nearest integers as bounds on the square root.
 * @param a Some number.
 * @return [x,x] if sqrt(a) = x is rational, otherwise [y,z] for y,z integer and y < sqrt(a) < z. 
 */
std::pair<mpq_class,mpq_class> sqrt_fast(const mpq_class& a);

inline mpz_class mod(const mpz_class& n, const mpz_class& m) {
    // TODO: In order to have the same result as division of native signed integer we have to 
    //       make it that complicated, as mpz_mod always returns positive integer. Maybe there is a better way.
	mpz_class res;
	mpz_mod(res.get_mpz_t(), abs(n).get_mpz_t(), m.get_mpz_t());
	return isNegative(n) ? mpz_class(-res) : res;
}

inline mpz_class remainder(const mpz_class& n, const mpz_class& m) {
	return mod(n,m);
}

inline mpz_class quotient(const mpz_class& n, const mpz_class& d)
{
    // TODO: In order to have the same result as division of native signed integer we have to 
    //       make it that complicated, as mpz_div does round differently. Maybe there is a better way.
	mpz_class res;
	mpz_div(res.get_mpz_t(), abs(n).get_mpz_t(), abs(d).get_mpz_t());
	return isNegative(n) == isNegative(d) ? res : mpz_class(-res);
}

inline mpz_class operator/(const mpz_class& n, const mpz_class& d)
{
	return quotient(n,d);
}

inline mpq_class quotient(const mpq_class& n, const mpq_class& d)
{
	mpq_class res;
	mpq_div(res.get_mpq_t(), n.get_mpq_t(), d.get_mpq_t());
	return res;
}

inline mpq_class operator/(const mpq_class& n, const mpq_class& d)
{
	return quotient(n,d);
}

inline void divide(const mpz_class& dividend, const mpz_class& divisor, mpz_class& quotient, mpz_class& remainder) {
	mpz_divmod(quotient.get_mpz_t(), remainder.get_mpz_t(), dividend.get_mpz_t(), divisor.get_mpz_t());
}

/**
 * Divide two fractions.
 * @param a First argument.
 * @param b Second argument.
 * @return \f$ a / b \f$.
 */
inline mpq_class div(const mpq_class& a, const mpq_class& b) {
	return carl::quotient(a,b);
}

/**
 * Divide two integers.
 * Asserts that the remainder is zero.
 * @param a First argument.
 * @param b Second argument.
 * @return \f$ a / b \f$.
 */
inline mpz_class div(const mpz_class& a, const mpz_class& b) {
	assert(carl::mod(a, b) == 0);
	return carl::quotient(a, b);
}

/**
 * Divide two integers.
 * Asserts that the remainder is zero.
 * Stores the result in the first argument.
 * @param a First argument.
 * @param b Second argument.
 * @return \f$ a / b \f$.
 */
inline mpz_class& div_assign(mpz_class& a, const mpz_class& b) {
	a = carl::quotient(a, b);
    return a;
}
/**
 * Divide two integers.
 * Asserts that the remainder is zero.
 * Stores the result in the first argument.
 * @param a First argument.
 * @param b Second argument.
 * @return \f$ a / b \f$.
 */
inline mpq_class& div_assign(mpq_class& a, const mpq_class& b) {
	a = carl::quotient(a, b);
    return a;
}

inline mpq_class operator *(const mpq_class& lhs, const mpq_class& rhs)
{
	mpq_class res;
	mpq_mul(res.get_mpq_t(), lhs.get_mpq_t(), rhs.get_mpq_t());
	return res;
}

std::string toString(const mpq_class& _number, bool _infix);

std::string toString(const mpz_class& _number, bool _infix);

}

