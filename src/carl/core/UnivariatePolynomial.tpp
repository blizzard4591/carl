/** 
 * @file:   UnivariatePolynomial.tpp
 * @author: Sebastian Junges
 *
 * @since August 26, 2013
 */

#pragma once
#include "UnivariatePolynomial.h"
#include <algorithm>
#include <iomanip>
#include "../util/SFINAE.h"
#include "logging.h"

namespace carl
{

template<typename Coeff>
UnivariatePolynomial<Coeff>::UnivariatePolynomial(Variable::Arg mainVar)
: mMainVar(mainVar), mCoefficients()
{
	
}
template<typename Coeff>
UnivariatePolynomial<Coeff>::UnivariatePolynomial(Variable::Arg mainVar, const Coeff& c, exponent e) :
mMainVar(mainVar),
mCoefficients(e+1,(Coeff)c-c) // We would like to use 0 here, but Coeff(0) is not always constructable (some methods need more parameter)
{
	if(c != 0)
	{
		mCoefficients[e] = c;
	}
	else
	{
		mCoefficients.clear();
	}
}

template<typename Coeff>
UnivariatePolynomial<Coeff>::UnivariatePolynomial(Variable::Arg mainVar, std::initializer_list<Coeff> coefficients)
: mMainVar(mainVar), mCoefficients(coefficients)
{
	
}

template<typename Coeff>
UnivariatePolynomial<Coeff>::UnivariatePolynomial(Variable::Arg mainVar, const std::vector<Coeff>& coefficients)
: mMainVar(mainVar), mCoefficients(coefficients)
{
	
}

template<typename Coeff>
UnivariatePolynomial<Coeff>::UnivariatePolynomial(Variable::Arg mainVar, std::vector<Coeff>&& coefficients)
: mMainVar(mainVar), mCoefficients(coefficients)
{
}

template<typename Coeff>
UnivariatePolynomial<Coeff>::UnivariatePolynomial(Variable::Arg mainVar, const std::map<unsigned, Coeff>& coefficients)
: mMainVar(mainVar)
{
	mCoefficients.reserve(coefficients.rbegin()->first);
	for( const std::pair<unsigned, Coeff>& expAndCoeff : coefficients)
	{
		if(expAndCoeff.first != mCoefficients.size() + 1)
		{
			mCoefficients.resize(expAndCoeff.first, (Coeff)0);
		}
		mCoefficients.push_back(expAndCoeff.second);
	}
}

template<typename Coeff>
Coeff UnivariatePolynomial<Coeff>::evaluate(const Coeff& value) const 
{
	Coeff result(0);
	Coeff var = 1;
	for(const Coeff& coeff : mCoefficients)
	{
		result += (coeff * var);
		var *= value;
	}
    return result;
}

template<typename Coeff>
UnivariatePolynomial<Coeff> UnivariatePolynomial<Coeff>::derivative(unsigned nth ) const
{
	UnivariatePolynomial<Coeff> result(mMainVar);
	result.mCoefficients.reserve(mCoefficients.size()-nth);
	// nth == 1 is most common case and can be implemented more efficient.
	if(nth == 1)
	{
		typename std::vector<Coeff>::const_iterator it = mCoefficients.begin();
		unsigned i = 0;
		for(it += nth; it != mCoefficients.end(); ++it)
		{
			++i;
			result.mCoefficients.push_back(i * *it);
		}
		return result;
	}
	else
	{
		// here we handle nth > 1.
		unsigned c = 1;
		for(unsigned k = 2; k <= nth; ++k)
		{
			c *= k;
		}
		typename std::vector<Coeff>::const_iterator it = mCoefficients.begin();
		unsigned i = nth;
		for(it += nth; it != mCoefficients.end(); ++it)
		{
			result.mCoefficients.push_back(c * *it);
			++i;
			c /= (i - nth);
			c *= i;
		}
		return result;
	}
}

template<typename Coeff>
UnivariatePolynomial<Coeff> UnivariatePolynomial<Coeff>::reduce(const UnivariatePolynomial& divisor) const
{
	assert(degree() >= divisor.degree());
	assert(!divisor.isZero());
	//std::cout << *this << " / " << divisor << std::endl;
	unsigned degdiff = degree() - divisor.degree();
	Coeff factor = lcoeff()/divisor.lcoeff();
	UnivariatePolynomial<Coeff> result(mMainVar);
	result.mCoefficients.reserve(mCoefficients.size()-1);
	if(degdiff > 0)
	{
		result.mCoefficients.assign(mCoefficients.begin(), mCoefficients.begin() + degdiff);
	}
	
	// By construction, the leading coefficient will be zero.
	for(unsigned i=0; i < mCoefficients.size() - degdiff -1; ++i)
	{
		result.mCoefficients.push_back(mCoefficients[i + degdiff] - factor * divisor.mCoefficients[i]);
	}
	// strip zeros from the end as we might have pushed zeros.
	result.stripLeadingZeroes();
	
	if(result.degree() < divisor.degree())
	{
		return result;
	}
	else 
	{	
		return result.reduce(divisor);
	}
}



/**
 * See Algorithm 2.2 in GZL92.
 * @param a
 * @param b
 * @param s
 * @param t
 * @return 
 */
template<typename Coeff>
UnivariatePolynomial<Coeff> UnivariatePolynomial<Coeff>::extended_gcd(const UnivariatePolynomial& a, const UnivariatePolynomial& b, UnivariatePolynomial& s, UnivariatePolynomial& t)
{
	assert(a.mMainVar == b.mMainVar);
	assert(a.mMainVar == s.mMainVar);
	assert(a.mMainVar == t.mMainVar);
	
	LOGMSG_DEBUG("carl.core", "UnivEEA: a=" << a << ", b=" << b );
	const Variable& x = a.mMainVar;
	UnivariatePolynomial<Coeff> c = a;
	UnivariatePolynomial<Coeff> d = b;
	c.normalizeCoefficients();
	d.normalizeCoefficients();
	c = c.normalized();
	d = d.normalized();
	
	UnivariatePolynomial<Coeff> c1 = a.one();
	UnivariatePolynomial<Coeff> c2(x);
	
	UnivariatePolynomial<Coeff> d1(x);
	UnivariatePolynomial<Coeff> d2 = a.one();
	
	while(!d.isZero())
	{
		DivisionResult<UnivariatePolynomial<Coeff>> divres = c.divide(d);
		assert(divres.remainder == c - divres.quotient * d);
		UnivariatePolynomial r1 = c1 - divres.quotient*d1;
		UnivariatePolynomial r2 = c2 - divres.quotient*d2;
		LOGMSG_TRACE("carl.core", "UnivEEA: q=" << divres.quotient << ", r=" << divres.remainder);
		LOGMSG_TRACE("carl.core", "UnivEEA: r1=" << c1 << "-" << divres.quotient << "*" << d1 << "==" << c1 - divres.quotient * d1 );
		LOGMSG_TRACE("carl.core", "UnivEEA: r2=" << c2 << "-" << divres.quotient << "*" << d2 << "==" << c2 - divres.quotient * d2 );
		c = d;
		c1 = d1;
		c2 = d2;
		d = divres.remainder;
		d1 = r1;
		d2 = r2;
		c.normalizeCoefficients();
		d.normalizeCoefficients();
		
		LOGMSG_TRACE("carl.core", "UnivEEA: c=" << c << ", d=" << d );
		LOGMSG_TRACE("carl.core", "UnivEEA: c1=" << c1 << ", c2=" << c2 );
		LOGMSG_TRACE("carl.core", "UnivEEA: d1=" << d1 << ", d2=" << d2 );
	}
	s = c1 / (a.lcoeff() * c.lcoeff());
	t = c2 / (b.lcoeff() * c.lcoeff());
	c = c.normalized();
	c.normalizeCoefficients();
	s.normalizeCoefficients();
	t.normalizeCoefficients();
	LOGMSG_DEBUG("carl.core", "UnivEEA: g=" << c << ", s=" << s << ", t=" << t );
	LOGMSG_TRACE("carl.core", "UnivEEA: " << c << "==" << s*a + t*b << "==" << s*a << " + " << t*b );
	assert(c == s*a + t*b);
	return c;
}

template<typename Coeff>
UnivariatePolynomial<Coeff> UnivariatePolynomial<Coeff>::gcd(const UnivariatePolynomial& a, const UnivariatePolynomial& b)
{
	// We want degree(b) <= degree(a).
	if(a.degree() < b.degree()) return gcd_recursive(b,a);
	else return gcd_recursive(a,b);
}


template<typename Coeff>
UnivariatePolynomial<Coeff> UnivariatePolynomial<Coeff>::gcd_recursive(const UnivariatePolynomial& a, const UnivariatePolynomial& b)
{
	if(b.isZero()) return a;
	else return gcd_recursive(b, a.reduce(b));
}

template<typename Coeff>
UnivariatePolynomial<Coeff>& UnivariatePolynomial<Coeff>::mod(const Coeff& modulus)
{
	for(Coeff& coeff : mCoefficients)
	{
		coeff = carl::mod(coeff, modulus);
	}
	return *this;
}

template<typename Coeff>
UnivariatePolynomial<Coeff> UnivariatePolynomial<Coeff>::mod(const Coeff& modulus) const
{
	UnivariatePolynomial<Coeff> result;
	result.mCoefficients.reserve(mCoefficients.size());
	for(auto coeff : mCoefficients)
	{
		result.mCoefficients.push_back(mod(coeff, modulus));
	}
	result.stripLeadingZeroes();
	return result;
}

template<typename Coeff>
Coeff UnivariatePolynomial<Coeff>::cauchyBound() const
{
	// We could also use SFINAE, but this gives clearer error messages.
	// Just in case, if we want to use SFINAE, the right statement would be
	// template<typename t = Coefficient, typename std::enable_if<is_field<t>::value, int>::type = 0>
	static_assert(is_field<Coeff>::value, "Cauchy bounds are only defined for field-coefficients");
	Coeff maxCoeff = mCoefficients.front() > 0 ? mCoefficients.front() : -mCoefficients.front();
	for(typename std::vector<Coeff>::const_iterator it = ++mCoefficients.begin(); it != --mCoefficients.end(); ++it)
	{
        Coeff absOfCoeff = abs( *it );
		if( absOfCoeff > maxCoeff ) 
		{
			maxCoeff = absOfCoeff;
		}
	}
	
	return 1 + maxCoeff/lcoeff();
}

template<typename Coeff>
template<typename C, EnableIf<is_field<C>>>
UnivariatePolynomial<Coeff> UnivariatePolynomial<Coeff>::normalized() const
{
	if(isZero())
	{
		return *this;
	}
	Coeff tmp(lcoeff());
	return *this/tmp;
}

template<typename Coeff>
template<typename C, DisableIf<is_field<C>>>
UnivariatePolynomial<Coeff> UnivariatePolynomial<Coeff>::normalized() const
{
	// TODO implement
}
	
template<typename Coeff>
Coeff UnivariatePolynomial<Coeff>::coprimeFactor() const
{
	static_assert(is_number<Coeff>::value, "We can only make integer coefficients if we have a number type before.");
	typename std::vector<Coeff>::const_iterator it = mCoefficients.begin();
	typename IntegralT<Coeff>::type num = getNum(*it);
	typename IntegralT<Coeff>::type den = getDenom(*it);
	for(++it; it != mCoefficients.end(); ++it)
	{
		num = carl::gcd(num, getNum(*it));
		den = carl::lcm(den, getDenom(*it));
	}
	return den/num;
}

template<typename Coeff>
template<typename Integer>
UnivariatePolynomial<Integer> UnivariatePolynomial<Coeff>::coprimeCoefficients() const
{
	static_assert(is_number<Coeff>::value, "We can only make integer coefficients if we have a number type before.");
	Coeff factor = coprimeFactor();
	// Notice that even if factor is 1, we create a new polynomial
	UnivariatePolynomial<Integer> result;
	result.mCoefficients.reserve(mCoefficients.size());
	for(const Coeff& coeff : mCoefficients)
	{
		result.mCoefficients.push_back(coeff * factor);
	}
	return result;
}	

template<typename Coeff>
DivisionResult<UnivariatePolynomial<Coeff>> UnivariatePolynomial<Coeff>::divide(const UnivariatePolynomial<Coeff>& divisor) const
{
	assert(!divisor.isZero());
	DivisionResult<UnivariatePolynomial<Coeff>> result(UnivariatePolynomial<Coeff>(mMainVar), *this);
	assert(*this == divisor * result.quotient + result.remainder);
	if(divisor.degree() > degree())
	{
		return result;
	}
	result.quotient.mCoefficients.resize(1+mCoefficients.size()-divisor.mCoefficients.size(),(Coeff)0);
	
	do
	{
		Coeff factor = result.remainder.lcoeff()/divisor.lcoeff();
		unsigned degdiff = result.remainder.degree() - divisor.degree();
		result.remainder -= UnivariatePolynomial<Coeff>(mMainVar, factor, degdiff) * divisor;
		result.quotient.mCoefficients[degdiff] += factor;
	}
	while(divisor.degree() <= result.remainder.degree() && !result.remainder.isZero());
	assert(*this == divisor * result.quotient + result.remainder);
	return result;
}

template<typename Coeff>
bool UnivariatePolynomial<Coeff>::divides(const UnivariatePolynomial& dividant) const
{
	return dividant.divide(*this).remainder.isZero();
}

template<typename Coeff>
Coeff UnivariatePolynomial<Coeff>::modifiedCauchyBound() const
{
	// We could also use SFINAE, but this gives clearer error messages.
	// Just in case, if we want to use SFINAE, the right statement would be
	// template<typename t = Coefficient, typename std::enable_if<is_field<t>::value, int>::type = 0>
	static_assert(is_field<Coeff>::value, "Modified Cauchy bounds are only defined for field-coefficients");
	LOG_NOTIMPLEMENTED();
}

template<typename Coeff>
template<typename C, EnableIf<is_instantiation_of<GFNumber, C>>>
UnivariatePolynomial<typename IntegralT<Coeff>::type> UnivariatePolynomial<Coeff>::toIntegerDomain() const
{
	UnivariatePolynomial<typename IntegralT<Coeff>::type> res(mMainVar);
	res.mCoefficients.reserve(mCoefficients.size());
	for(const Coeff& c : mCoefficients)
	{
		assert(isInteger(c));
		res.mCoefficients.push_back(c.representingInteger());
	}
	res.stripLeadingZeroes();
	return res;
}

template<typename Coeff>
template<typename C, DisableIf<is_instantiation_of<GFNumber, C>>>
UnivariatePolynomial<typename IntegralT<Coeff>::type> UnivariatePolynomial<Coeff>::toIntegerDomain() const
{
	UnivariatePolynomial<typename IntegralT<Coeff>::type> res(mMainVar);
	res.mCoefficients.reserve(mCoefficients.size());
	for(const Coeff& c : mCoefficients)
	{
		assert(isInteger(c));
		res.mCoefficients.push_back((typename IntegralT<Coeff>::type)c);
	}
	res.stripLeadingZeroes();
}

template<typename Coeff>
//template<typename T = Coeff, EnableIf<!std::is_same<IntegralT<Coeff>, bool>::value>>
UnivariatePolynomial<GFNumber<typename IntegralT<Coeff>::type>> UnivariatePolynomial<Coeff>::toFiniteDomain(const GaloisField<typename IntegralT<Coeff>::type>* galoisField) const
{
	UnivariatePolynomial<GFNumber<typename IntegralT<Coeff>::type>> res(mMainVar);
	res.mCoefficients.reserve(mCoefficients.size());
	for(const Coeff& c : mCoefficients)
	{
		assert(isInteger(c));
		res.mCoefficients.push_back(GFNumber<typename IntegralT<Coeff>::type>(c,galoisField));
	}
	res.stripLeadingZeroes();
	return res;
	
}

template<typename Coeff>
std::map<UnivariatePolynomial<Coeff>, unsigned> UnivariatePolynomial<Coeff>::factorization() const
{
    LOGMSG_TRACE("carl.core", "UnivFactor: " << *this );
    std::map<UnivariatePolynomial<Coeff>, unsigned> result;
    if(isConstant()) // Constant.
    {
        LOGMSG_TRACE("carl.core", "UnivFactor: add the factor (" << *this << ")^" << 1 );
        result.insert(std::pair<UnivariatePolynomial<Coeff>, unsigned>(*this, 1));
        return result;
    }
    // Make the polynomial's coefficients coprime (integral and with gcd 1).
    UnivariatePolynomial<Coeff> remainingPoly(mainVar());
    Coeff factor = coprimeFactor();
    if(factor == 1)
    {
        remainingPoly = *this;
    }
    else
    {
        // Store the rational factor and make the polynomial's coefficients coprime.
        LOGMSG_TRACE("carl.core", "UnivFactor: add the factor (" << UnivariatePolynomial<Coeff>(mainVar(), (Coeff) 1 / factor) << ")^" << 1 );
        result.insert(std::pair<UnivariatePolynomial<Coeff>, unsigned>(UnivariatePolynomial<Coeff>(mainVar(), (Coeff) 1 / factor), 1));
        remainingPoly.mCoefficients.reserve(mCoefficients.size());
        for(const Coeff& coeff : mCoefficients)
        {
            remainingPoly.mCoefficients.push_back(coeff * factor);
        }
    }
    assert(mCoefficients.size() > 1);
    // Exclude the factors  (x-r)^i  with  r rational.
    remainingPoly = excludeLinearFactors<int>(remainingPoly, result, INT_MAX);
    assert(!remainingPoly.isConstant() || remainingPoly.lcoeff() == (Coeff)1);
    if(!remainingPoly.isConstant())
    {
        // Calculate the square free factorization.
        std::map<unsigned, UnivariatePolynomial<Coeff>> sff = remainingPoly.squareFreeFactorization();
//        factor = (Coeff) 1;
        for(auto expFactorPair = sff.begin(); expFactorPair != sff.end(); ++expFactorPair)
        {
//            Coeff cpf = expFactorPair->second.coprimeFactor();
//            if(cpf != (Coeff) 1)
//            {
//                factor *= pow(expFactorPair->second.coprimeFactor(), expFactorPair->first);
//                expFactorPair->second /= cpf;
//            }
            if(!expFactorPair->second.isConstant() || expFactorPair->second.lcoeff() != (Coeff) 1)
            {
                auto retVal = result.insert(std::pair<UnivariatePolynomial<Coeff>, unsigned>(expFactorPair->second, expFactorPair->first));
                LOGMSG_TRACE("carl.core", "UnivFactor: add the factor (" << expFactorPair->second << ")^" << expFactorPair->first );
                if(!retVal.second)
                {
                    retVal.first->second += expFactorPair->first;
                }
            }
        }
//        if(factor != (Coeff) 1)
//        {
//            LOGMSG_TRACE("carl.core", "UnivFactor: add the factor (" << UnivariatePolynomial<Coeff>(mainVar(), {factor}) << ")^" << 1 );
//            // Add the constant factor to the factors.
//            if( result.begin()->first.isConstant() )
//            {
//                factor *= result.begin()->first.lcoeff();
//                result.erase( result.begin() );
//            }
//            result.insert(result.begin(), std::pair<UnivariatePolynomial<Coeff>, unsigned>(UnivariatePolynomial<Coeff>(mainVar(), {factor}), 1));
//        }
    }
    return result;
}

template<typename Coeff>
template<typename Integer>
UnivariatePolynomial<Coeff> UnivariatePolynomial<Coeff>::excludeLinearFactors(const UnivariatePolynomial<Coeff>& poly, std::map<UnivariatePolynomial<Coeff>, unsigned>& linearFactors, const Integer& maxInt)
{
    LOGMSG_TRACE("carl.core", "UnivELF: " << poly );
    UnivariatePolynomial<Coeff> result(poly.mainVar());
    // Exclude the factor x^i from result.
    auto cf = poly.coefficients().begin();
    if(*cf == 0) // result is of the form a_n * x^n + ... + a_k * x^k (a>k, k>0)
    {
        unsigned k = 0;
        while(*cf == 0)
        {
            assert(cf != poly.coefficients().end());
            ++cf;
            ++k;
        }
        // Take x^k as a factor.
        auto retVal = linearFactors.insert(std::pair<UnivariatePolynomial<Coeff>, unsigned>(UnivariatePolynomial<Coeff>(poly.mainVar(), {(Coeff)0, (Coeff)1}), k));
        LOGMSG_TRACE("carl.core", "UnivELF: add the factor (" << retVal.first->first << ")^" << k );
        if(!retVal.second)
        {
            retVal.first->second += k;
        }
        // Construct the remainder:  result := a_n * x^{n-k} + ... + a_{k-1} * x + a_k
        std::vector<Coeff> cfs;
        cfs.reserve(poly.coefficients().size()-k);
        cfs = std::vector<Coeff>(cf, poly.coefficients().end());
        result = UnivariatePolynomial<Coeff>(poly.mainVar(), std::move(cfs));
        LOGMSG_TRACE("carl.core", "UnivELF: remainder is  " << result );
    }
    else
    {
        result = poly;
    }
    // Check whether the polynomial is already a linear factor.
    if(result.degree() > 1)
    {
        // Exclude the factor (x-r)^i, with r rational and r!=0, from result.
        assert(result.coefficients().size() > 1);
        typename IntegralT<Coeff>::type lc = abs(getNum(result.lcoeff()));
        typename IntegralT<Coeff>::type tc = abs(getNum(result.coefficients().front()));
        if( maxInt != 0 && (tc > maxInt || lc > maxInt) )
        {
            return result;
        }
        Integer lcAsInt = toInt<Integer>(lc);
        Integer tcAsInt = toInt<Integer>(tc);
        Integer halfOfLcAsInt = lcAsInt == 1 ? 1 : lcAsInt/2;
        Integer halfOfTcAsInt = tcAsInt == 1 ? 1 : tcAsInt/2;
        std::vector<std::pair<Integer, Integer>> shiftedTcs;
        bool positive = true;
        bool tcFactorsFound = false;
        std::vector<Integer> tcFactors = std::vector<Integer>(1, 1); // TODO: store the divisors of some numbers during compilation
        auto tcFactor = tcFactors.begin();
        bool lcFactorsFound = false;
        std::vector<Integer> lcFactors = std::vector<Integer>(1, 1); // TODO: store the divisors of some numbers during compilation
        auto lcFactor = lcFactors.begin();
        while(true)
        {
            LOGMSG_TRACE("carl.core", "UnivELF: try rational  " << (positive ? "-" : "") << *tcFactor << "/" << *lcFactor);
            // Check whether the numerator of the rational to consider divides the trailing coefficient of all
            // zero-preserving shifts {result(x+x_0) | for some found x_0 with result(x_0)!=0 and x_0 integer}
            auto shiftedTc = shiftedTcs.begin();
            for(; shiftedTc != shiftedTcs.end(); ++shiftedTc)
            {
                // we need to be careful with overflows in the following lines
                if(maxInt/(*lcFactor) >= shiftedTc->first)
                {
                    Integer divisor = (*lcFactor) * shiftedTc->first;
                    if( divisor != *tcFactor )
                    {
                        if( !(divisor < 0 && *tcFactor < 0 && maxInt + divisor >= -(*tcFactor)) && !(divisor > 0 && *tcFactor > 0 && maxInt - divisor >= *tcFactor ) )
                        {
                            if( divisor > *tcFactor )
                            {
                                divisor = divisor - *tcFactor;
                            }
                            else
                            {
                                divisor = *tcFactor - divisor;
                            }
                            if(carl::mod(shiftedTc->second, divisor) != 0)
                            {
                                break;
                            }
                        }
                    }
                }
            }
            if(shiftedTc == shiftedTcs.end())
            {
                Coeff posRatZero = positive ? (Coeff(*tcFactor) / Coeff(*lcFactor)) : -(Coeff(*tcFactor) / Coeff(*lcFactor));
                LOGMSG_TRACE("carl.core", "UnivELF: consider possible non zero rational factor  " << posRatZero);
                Coeff image = result.syntheticDivision(posRatZero);
                if(image == 0)
                {
                    // Remove all linear factor with the found zero from result.
                    UnivariatePolynomial<Coeff> linearFactor(result.mainVar(), {-posRatZero, (Coeff)1});
                    while(image == 0)
                    {
                        auto retVal = linearFactors.insert(std::pair<UnivariatePolynomial<Coeff>, unsigned>(linearFactor, 1));
                        LOGMSG_TRACE("carl.core", "UnivELF: add the factor (" << linearFactor << ")^" << 1 );
                        if(!retVal.second)
                        {
                            ++retVal.first->second;
                        }
                        // Check whether result is a linear factor now.
                        if(result.degree() <= 1)
                        {
                            goto LinearFactorRemains;
                        }
                        image = result.syntheticDivision(posRatZero);
                    }
                }
                else if(isInteger(posRatZero))
                {
                    // Add a zero-preserving shift.
                    assert(isInteger(image));
                    typename IntegralT<Coeff>::type imageInt = abs(getNum(image));
                    if( imageInt <= maxInt )
                    {
                        LOGMSG_TRACE("carl.core", "UnivELF: new shift with " << getNum(posRatZero) << " to " << abs(getNum(image)));
                        shiftedTcs.push_back(std::pair<Integer, Integer>(toInt<Integer>(getNum(posRatZero)), toInt<Integer>(abs(getNum(image)))));
                    }
                }
            }
            // Find the next numerator-denominator combination.
            if(shiftedTc == shiftedTcs.end() && positive)
            {
                positive = false;
            }
            else
            {
                positive = true;
                if(lcFactorsFound)
                {
                    ++lcFactor;
                }
                else
                {
                    lcFactors.push_back(lcFactors.back());
                    while(lcFactors.back() <= halfOfLcAsInt)
                    {
                        ++lcFactors.back();
                        if(carl::mod(lcAsInt, lcFactors.back()) == 0)
                        {
                            break;
                        }
                    }
                    if(lcFactors.back() > halfOfLcAsInt)
                    {
                        lcFactors.pop_back();
                        lcFactorsFound = true;
                        lcFactor = lcFactors.end();
                    }
                    else
                    {
                        lcFactor = --(lcFactors.end());
                    }
                }
                if(lcFactor == lcFactors.end())
                {
                    if(tcFactorsFound)
                    {
                        ++tcFactor;
                    }
                    else
                    {
                        tcFactors.push_back(tcFactors.back());
                        while(tcFactors.back() <= halfOfTcAsInt)
                        {
                            ++(tcFactors.back());
                            if(carl::mod(tcAsInt, tcFactors.back()) == 0)
                            {
                                break;
                            }
                        }
                        if(tcFactors.back() > halfOfTcAsInt)
                        {
                            tcFactors.pop_back();
                            tcFactor = tcFactors.end();
                        }
                        else
                        {
                            tcFactor = --(tcFactors.end());
                        }
                    }
                    if(tcFactor == tcFactors.end())
                    {
                        Coeff factor = result.coprimeFactor();
                        if(factor != (Coeff) 1)
                        {
                            result *= factor;
                            LOGMSG_TRACE("carl.core", "UnivFactor: add the factor (" << UnivariatePolynomial<Coeff>(result.mainVar(), std::initializer_list<Coeff>({(Coeff)1/factor})) << ")^" << 1 );
                            // Add the constant factor to the factors.
                            if( linearFactors.begin()->first.isConstant() )
                            {
                                factor = (Coeff)1 / factor;
                                factor *= linearFactors.begin()->first.lcoeff();
                                linearFactors.erase(linearFactors.begin());
                            }
                            linearFactors.insert(linearFactors.begin(), std::pair<UnivariatePolynomial<Coeff>, unsigned>(UnivariatePolynomial<Coeff>(result.mainVar(), std::initializer_list<Coeff>({factor})), 1));
                        }
                        return result;
                    }
                    lcFactor = lcFactors.begin();
                }
            }
        }
        assert(false);
    }
LinearFactorRemains:
    Coeff factor = result.lcoeff();
    if(factor != (Coeff) 1)
    {
        result /= factor;
        LOGMSG_TRACE("carl.core", "UnivFactor: add the factor (" << UnivariatePolynomial<Coeff>(result.mainVar(), factor) << ")^" << 1 );
        // Add the constant factor to the factors.
        if( linearFactors.begin()->first.isConstant() )
        {
            factor *= linearFactors.begin()->first.lcoeff();
            linearFactors.erase(linearFactors.begin());
        }
        linearFactors.insert(linearFactors.begin(), std::pair<UnivariatePolynomial<Coeff>, unsigned>(UnivariatePolynomial<Coeff>(result.mainVar(), factor), 1));
    }
    auto retVal = linearFactors.insert(std::pair<UnivariatePolynomial<Coeff>, unsigned>(result, 1));
    LOGMSG_TRACE("carl.core", "UnivELF: add the factor (" << result << ")^" << 1 );
    if(!retVal.second)
    {
        ++retVal.first->second;
    }
    return UnivariatePolynomial<Coeff>(result.mainVar(), (Coeff)1);
}

template<typename Coeff>
Coeff UnivariatePolynomial<Coeff>::syntheticDivision(const Coeff& zeroOfDivisor)
{
    if(coefficients().empty()) return Coeff(0);
    if(coefficients().size() == 1) return coefficients().back();
    std::vector<Coeff> secondRow;
    secondRow.reserve(coefficients().size());
    secondRow.push_back(Coeff(0));
    std::vector<Coeff> thirdRow(coefficients().size(), Coeff(0));
    size_t posThirdRow = coefficients().size()-1; 
    auto coeff = coefficients().rbegin();
    thirdRow[posThirdRow] = (*coeff) + secondRow.front();
    ++coeff;
    while(coeff != coefficients().rend())
    {
        secondRow.push_back(zeroOfDivisor*thirdRow[posThirdRow]);
        --posThirdRow;
        thirdRow[posThirdRow] = (*coeff) + secondRow.back();
        ++coeff;
    }
    assert(posThirdRow == 0);
    LOGMSG_TRACE("carl.core", "UnivSynDiv: (" << *this << ")[x -> " << zeroOfDivisor << "]  =  " << thirdRow.front());
    if(thirdRow.front() == 0)
    {
        thirdRow.erase(thirdRow.begin());
        this->mCoefficients.swap(thirdRow);
        LOGMSG_TRACE("carl.core", "UnivSynDiv: reduced by ((" << abs(getDenom(thirdRow.front())) << ")*" << mainVar() << " + (" << (thirdRow.front()<0 ? "-" : "") << abs(getNum(thirdRow.front())) << "))  ->  " << *this);
        return Coeff(0);
    }
    return thirdRow.front();
}

template<typename Coeff>
std::map<unsigned, UnivariatePolynomial<Coeff>> UnivariatePolynomial<Coeff>::squareFreeFactorization() const
{
    LOGMSG_TRACE("carl.core", "UnivSSF: " << *this);
    std::map<unsigned,UnivariatePolynomial<Coeff>> result;
	if(characteristic<Coeff>::value != 0 && degree() >= characteristic<Coeff>::value)
    {
        LOGMSG_TRACE("carl.core", "UnivSSF: degree greater than characteristic!");
        result.insert(std::pair<unsigned, UnivariatePolynomial<Coeff>>(1, *this));
        LOGMSG_TRACE("carl.core", "UnivSSF: add the factor (" << *this << ")^1");
    }
    else
    {
        UnivariatePolynomial<Coeff> b = this->derivative();
        LOGMSG_TRACE("carl.core", "UnivSSF: b = " << b);
        UnivariatePolynomial<Coeff> s(mainVar());
        UnivariatePolynomial<Coeff> t(mainVar());
        UnivariatePolynomial<Coeff> c = extended_gcd((*this), b, s, t); // TODO: use gcd instead
        typename IntegralT<Coeff>::type numOfCpf = getNum(c.coprimeFactor());
        if(numOfCpf != 1) // TODO: is this maybe only necessary because the extended_gcd returns a polynomial with non-integer coefficients but it shouldn't?
        {
            c *= (Coeff) numOfCpf;
        }
        LOGMSG_TRACE("carl.core", "UnivSSF: c = " << c);
        if(c.isZero())
        {
            result.insert(std::pair<unsigned, UnivariatePolynomial<Coeff>>(1, *this));
            LOGMSG_TRACE("carl.core", "UnivSSF: add the factor (" << *this << ")^1");
        }
        else
        {
            UnivariatePolynomial<Coeff> w = (*this).divide(c).quotient;
            LOGMSG_TRACE("carl.core", "UnivSSF: w = " << w);
            UnivariatePolynomial<Coeff> y = b.divide(c).quotient;
            LOGMSG_TRACE("carl.core", "UnivSSF: y = " << y);
            UnivariatePolynomial<Coeff> z = y-w.derivative();
            LOGMSG_TRACE("carl.core", "UnivSSF: z = " << z);
            unsigned i = 1;
            while(!z.isZero())
            {
                LOGMSG_TRACE("carl.core", "UnivSSF: next iteration");
                UnivariatePolynomial<Coeff> g = extended_gcd(w, z, s, t); // TODO: use gcd instead
                numOfCpf = getNum(g.coprimeFactor());
                if(numOfCpf != 1) // TODO: is this maybe only necessary because the extended_gcd returns a polynomial with non-integer coefficients but it shouldn't?
                {
                    g *= (Coeff) numOfCpf;
                }
                LOGMSG_TRACE("carl.core", "UnivSSF: g = " << g);
                assert(result.find(i) == result.end());
                result.insert(std::pair<unsigned, UnivariatePolynomial<Coeff>>(i, g));
                LOGMSG_TRACE("carl.core", "UnivSSF: add the factor (" << g << ")^" << i);
                ++i;
                w = w.divide(g).quotient;
                LOGMSG_TRACE("carl.core", "UnivSSF: w = " << w);
                y = z.divide(g).quotient;
                LOGMSG_TRACE("carl.core", "UnivSSF: y = " << y);
                z = y - w.derivative();
                LOGMSG_TRACE("carl.core", "UnivSSF: z = " << z);
            }
            result.insert(std::pair<unsigned, UnivariatePolynomial<Coeff>>(i, w));
            LOGMSG_TRACE("carl.core", "UnivSSF: add the factor (" << w << ")^" << i);
        }
    }
    return result;
}

template<typename Coeff>
UnivariatePolynomial<Coeff> UnivariatePolynomial<Coeff>::operator -() const
{
	UnivariatePolynomial result(mMainVar);
	result.mCoefficients.reserve(mCoefficients.size());
	for(auto c : mCoefficients)
	{
		result.mCoefficients.push_back(-c);
	}
	
	return result;		 
}

template<typename Coeff>
UnivariatePolynomial<Coeff>& UnivariatePolynomial<Coeff>::operator+=(const Coeff& rhs)
{
	if(rhs == 0) return *this;
	if(mCoefficients.empty())
	{
		// Adding non-zero rhs to zero.
		mCoefficients.resize(1, rhs);
	}
	else
	{
		mCoefficients.front() += rhs;
		if(mCoefficients.size() == 1 && mCoefficients.front() == (Coeff)0) 
		{
			// Result is zero.
			mCoefficients.clear();
		}
	}
	return *this;
}

template<typename Coeff>
UnivariatePolynomial<Coeff>& UnivariatePolynomial<Coeff>::operator+=(const UnivariatePolynomial& rhs)
{
	assert(mMainVar == rhs.mMainVar);
	
	if(rhs.isZero())
	{
		return *this;
	}
	
	if(mCoefficients.size() < rhs.mCoefficients.size())
	{
		for(unsigned i = 0; i < mCoefficients.size(); ++i)
		{
			mCoefficients[i] += rhs.mCoefficients[i];
		}
		mCoefficients.insert(mCoefficients.end(), rhs.mCoefficients.end() - ((unsigned)(rhs.mCoefficients.size() - mCoefficients.size())), rhs.mCoefficients.end());
	}
	else
	{
		for(unsigned i = 0; i < rhs.mCoefficients.size(); ++i)
		{
			mCoefficients[i] += rhs.mCoefficients[i]; 
		}
	}
	stripLeadingZeroes();
	return *this;
}

template<typename C>
UnivariatePolynomial<C> operator+(const UnivariatePolynomial<C>& lhs, const UnivariatePolynomial<C>& rhs)
{
	UnivariatePolynomial<C> res(lhs);
	res += rhs;
	return res;
}

template<typename C>
UnivariatePolynomial<C> operator+(const UnivariatePolynomial<C>& lhs, const C& rhs)
{
	UnivariatePolynomial<C> res(lhs);
	res += rhs;
	return res;
}

template<typename C>
UnivariatePolynomial<C> operator+(const C& lhs, const UnivariatePolynomial<C>& rhs)
{
	return rhs + lhs;
}
	

template<typename Coeff>
UnivariatePolynomial<Coeff>& UnivariatePolynomial<Coeff>::operator-=(const Coeff& rhs)
{
	LOG_INEFFICIENT();
	return *this += -rhs;
}

template<typename Coeff>
UnivariatePolynomial<Coeff>& UnivariatePolynomial<Coeff>::operator-=(const UnivariatePolynomial& rhs)
{
	LOG_INEFFICIENT();
	return *this += -rhs;
}


template<typename C>
UnivariatePolynomial<C> operator-(const UnivariatePolynomial<C>& lhs, const UnivariatePolynomial<C>& rhs)
{
	UnivariatePolynomial<C> res(lhs);
	res -= rhs;
	return res;
}

template<typename C>
UnivariatePolynomial<C> operator-(const UnivariatePolynomial<C>& lhs, const C& rhs)
{
	UnivariatePolynomial<C> res(lhs);
	res -= rhs;
	return res;
}

template<typename C>
UnivariatePolynomial<C> operator-(const C& lhs, const UnivariatePolynomial<C>& rhs)
{
	return rhs - lhs;
}

template<typename Coeff>
UnivariatePolynomial<Coeff>& UnivariatePolynomial<Coeff>::operator*=(const Coeff& rhs)
{
	if(rhs == 0)
	{
		mCoefficients.clear();
		return *this;
	}
	for(Coeff& c : mCoefficients)
	{
		c *= rhs;
	}
	
	if(is_finite_domain<Coeff>::value)
	{
		stripLeadingZeroes();
	}
	
	return *this;		
}


template<typename Coeff>
template<typename I, DisableIf<std::is_same<Coeff, I>>...>
UnivariatePolynomial<Coeff>& UnivariatePolynomial<Coeff>::operator*=(const typename IntegralT<Coeff>::type& rhs)
{
	static_assert(std::is_same<Coeff, I>::value, "Do not provide template parameters");
	if(rhs == (I)0)
	{
		mCoefficients.clear();
		return *this;
	}
	for(Coeff& c : mCoefficients)
	{
		c *= rhs;
	}
	return *this;		
}

template<typename Coeff>
UnivariatePolynomial<Coeff>& UnivariatePolynomial<Coeff>::operator*=(const UnivariatePolynomial& rhs)
{
	assert(mMainVar == rhs.mMainVar);
	if(rhs.isZero())
	{
		mCoefficients.clear();
		return *this;
	}
	
	std::vector<Coeff> newCoeffs; 
	newCoeffs.reserve(mCoefficients.size() + rhs.mCoefficients.size());
	for(unsigned e = 0; e < mCoefficients.size() + rhs.degree(); ++e)
	{
		newCoeffs.push_back((Coeff)0);
		for(unsigned i = 0; i < mCoefficients.size() && i <= e; ++i)
		{
			if(e - i < rhs.mCoefficients.size())
			{
				newCoeffs.back() += mCoefficients[i] * rhs.mCoefficients[e-i];
			}
		}
	}
	mCoefficients.swap(newCoeffs);
	stripLeadingZeroes();
	return *this;
}


template<typename C>
UnivariatePolynomial<C> operator*(const UnivariatePolynomial<C>& lhs, const UnivariatePolynomial<C>& rhs)
{
	UnivariatePolynomial<C> res(lhs);
	res *= rhs;
	return res;
}

template<typename C>
UnivariatePolynomial<C> operator*(const UnivariatePolynomial<C>& lhs, const C& rhs)
{
	UnivariatePolynomial<C> res(lhs);
	res *= rhs;
	return res;
}

template<typename C>
UnivariatePolynomial<C> operator*(const C& lhs, const UnivariatePolynomial<C>& rhs)
{
	return rhs * lhs;
}

template<typename C>
UnivariatePolynomial<C> operator*(const UnivariatePolynomial<C>& lhs, const typename IntegralT<C>::type& rhs)
{
	UnivariatePolynomial<C> res(lhs);
	res *= rhs;
	return res;
}

template<typename C>
UnivariatePolynomial<C> operator*(const typename IntegralT<C>::type& lhs, const UnivariatePolynomial<C>& rhs)
{
	return rhs * lhs;
}


template<typename Coeff>
UnivariatePolynomial<Coeff>& UnivariatePolynomial<Coeff>::operator/=(const Coeff& rhs)
{
	if(!is_field<Coeff>::value)
	{
		LOGMSG_WARN("carl.core", "Division by coefficients is only defined for field-coefficients");
	}
	assert(rhs != 0);
	for(Coeff& c : mCoefficients)
	{
		c /= rhs;
	}
	return *this;		
}

template<typename C>
UnivariatePolynomial<C> operator/(const UnivariatePolynomial<C>& lhs, const C& rhs)
{
	static_assert(is_field<C>::value, "Division by coefficients is only defined for field-coefficients");
	assert(rhs != 0);
	UnivariatePolynomial<C> res(lhs);
	return res /= rhs;
}

template<typename C>
bool operator==(const UnivariatePolynomial<C>& lhs, const UnivariatePolynomial<C>& rhs)
{
	if(lhs.mMainVar == rhs.mMainVar)
	{
		return lhs.mCoefficients == rhs.mCoefficients;
	}
	else
	{
		// in different variables, polynomials can still be equal if constant.
		if(lhs.isZero() && rhs.isZero()) return true;
		if(lhs.isConstant() && rhs.isConstant() && lhs.lcoeff() == rhs.lcoeff()) return true;
		return false;
	}
}
template<typename C>
bool operator==(const UnivariatePolynomialPtr<C>& lhs, const UnivariatePolynomialPtr<C>& rhs)
{
	if (lhs == nullptr && rhs == nullptr) return true;
	if (lhs == nullptr || rhs == nullptr) return false;
	return *lhs == *rhs;
}

template<typename C>
bool operator==(const UnivariatePolynomial<C>& lhs, const C& rhs)
{	
	if(lhs.isZero())
	{
		return rhs == 0;
	}
	if(lhs.isConstant() && lhs.lcoeff() == rhs) return true;
	return false;
}

template<typename C>
bool operator==(const C& lhs, const UnivariatePolynomial<C>& rhs)
{
	return rhs == lhs;
}


template<typename C>
bool operator!=(const UnivariatePolynomial<C>& lhs, const UnivariatePolynomial<C>& rhs)
{
	return !(lhs == rhs);
}
template<typename C>
bool operator!=(const UnivariatePolynomialPtr<C>& lhs, const UnivariatePolynomialPtr<C>& rhs)
{
	if (lhs == nullptr && rhs == nullptr) return false;
	if (lhs == nullptr || rhs == nullptr) return true;
	return *lhs != *rhs;
}

template<typename C>
bool UnivariatePolynomial<C>::less(const UnivariatePolynomial<C>& rhs, ComparisonOrder order) {
	switch (order) {
		case CauchyBound: {
			C a = this->cauchyBound();
			C b = rhs.cauchyBound();
			if (a < b) return true;
			return (a == b) && this->less(rhs);
		}
		case LowDegree:
			if (this->degree() < rhs.degree()) return true;
			return (this->degree() == rhs.degree()) && this->less(rhs);
		case Default:
		case Memory:
			return this < &rhs;
	}
}
template<typename C>
bool less(const UnivariatePolynomial<C>& lhs, const UnivariatePolynomial<C>& rhs)
{
	return lhs.less(rhs);
}
template<typename C>
bool less(const UnivariatePolynomialPtr<C>& lhs, const UnivariatePolynomialPtr<C>& rhs)
{
	if (lhs == nullptr) return rhs != nullptr;
	if (rhs == nullptr) return true;
	return lhs->less(*rhs);
}

template<typename C>
bool operator<(const UnivariatePolynomial<C>& lhs, const UnivariatePolynomial<C>& rhs)
{
	if(lhs.mMainVar == rhs.mMainVar)
	{
		if(lhs.coefficients().size() == rhs.coefficients().size())
        {
            auto iterLhs = lhs.coefficients().rbegin();
            auto iterRhs = rhs.coefficients().rbegin();
            while(iterLhs != lhs.coefficients().rend())
            {
                assert(iterRhs != rhs.coefficients().rend());
                if(*iterLhs == *iterRhs)
                {
                    ++iterLhs;
                    ++iterRhs;
                }
                else
                {
                    return *iterLhs < *iterRhs;
                }
            }
        }
		return lhs.coefficients().size() < rhs.coefficients().size();
	}
    return lhs.mMainVar < rhs.mMainVar;
}

template<typename C>
std::ostream& operator<<(std::ostream& os, const UnivariatePolynomial<C>& rhs)
{
	if(rhs.isZero()) return os << "0";
	for(size_t i = 0; i < rhs.mCoefficients.size()-1; ++i )
	{
		const C& c = rhs.mCoefficients[rhs.mCoefficients.size()-i-1];
		if(c != 0)
		{
			os << "(" << c << ")*" << rhs.mMainVar << "^" << rhs.mCoefficients.size()-i-1 << " + ";
		}
	}
	os << rhs.mCoefficients[0];
	return os;
}
}
