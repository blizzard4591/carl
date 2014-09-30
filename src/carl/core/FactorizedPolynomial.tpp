/* 
 * File:   FactorizedPolynomial.tpp
 * Author: Florian Corzilius
 *
 * Created on September 4, 2014, 10:55 AM
 */

#include "FactorizedPolynomial.h"

#pragma once

namespace carl
{
    template<typename P>
    FactorizedPolynomial<P>::FactorizedPolynomial( const P& _polynomial, Cache<PolynomialFactorizationPair<P>>& _cache ):
        mCacheRef( 0 ),
        mrCache( _cache ),
        mCoefficient( _polynomial.coprimeFactor() )
    {
        Factorization<P> factorization;
        PolynomialFactorizationPair<P>* pfPair = new PolynomialFactorizationPair<P>( std::move( factorization), new P( _polynomial.coprimeCoefficients() ) );
        mCacheRef = mrCache.cache( pfPair );
        /*
         * The following is not very nice, but we know, that the hash won't change, once the polynomial 
         * representation is fixed, so we can add the factorizations content belatedly. It is necessary to do so
         * as otherwise the factorized polynomial (this) being the only factor, is not yet cached which leads to an assertion.
         */
        pfPair->mFactorization.insert( std::make_pair( *this, 1 ) ); 
    }
    
    template<typename P>
    FactorizedPolynomial<P>::FactorizedPolynomial( const P& _polynomial, Factorization<P>&& _factorization, Coeff<P>& _coefficient, Cache<PolynomialFactorizationPair<P>>& _cache ):
        mCacheRef( 0 ),
        mrCache( _cache ),
        mCoefficient( _coefficient )
    {
        assert( mCoefficient == _polynomial.coprimeFactor() );
        mCacheRef = mrCache.cache( new PolynomialFactorizationPair<P>( std::move( _factorization ), new P( _polynomial ) ) );
    }
    
    template<typename P>
    FactorizedPolynomial<P>::FactorizedPolynomial( Factorization<P>&& _factorization, Coeff<P>& _coefficient, Cache<PolynomialFactorizationPair<P>>& _cache ):
        mCacheRef( 0 ),
        mrCache( _cache ),
        mCoefficient( _coefficient )
    {
        mCacheRef = mrCache.cache( new PolynomialFactorizationPair<P>( std::move( _factorization ) ) );
    }
    
    template<typename P>
    FactorizedPolynomial<P>::FactorizedPolynomial( const FactorizedPolynomial<P>& _toCopy ):
        mCacheRef( _toCopy.mCacheRef ),
        mrCache( _toCopy.mrCache )
    {
        mrCache.reg( _toCopy.mCacheRef );
    }
    
    template<typename P>
    FactorizedPolynomial<P>::~FactorizedPolynomial()
    {
        mrCache.dereg( mCacheRef );
    }
    
    template<typename P>
    FactorizedPolynomial<P>& FactorizedPolynomial<P>::operator=( const FactorizedPolynomial<P>& _fpoly )
    {
        assert( &mrCache == &_fpoly.cache() );
        mrCache.dereg( mCacheRef );
        mCacheRef = _fpoly.cacheRef();
        mrCache.reg( mCacheRef );
        return *this;
    }
        
    template<typename P>
    bool operator==( const FactorizedPolynomial<P>& _fpolyA, const FactorizedPolynomial<P>& _fpolyB )
    {
        assert( &_fpolyA.cache() == &_fpolyB.cache() );
        return _fpolyA.content() == _fpolyB.content();
    }
        
    template<typename P>
    bool operator<( const FactorizedPolynomial<P>& _fpolyA, const FactorizedPolynomial<P>& _fpolyB )
    {
        assert( &_fpolyA.cache() == &_fpolyB.cache() );
        return _fpolyA.content() < _fpolyB.content();
    }

    template<typename P>
    const FactorizedPolynomial<P> operator+( const FactorizedPolynomial<P>& _fpolyA, const FactorizedPolynomial<P>& _fpolyB )
    {
        _fpolyA.strengthenActivity();
        _fpolyB.strengthenActivity();
        assert( &_fpolyA.cache() == &_fpolyB.cache() );
        // TODO (matthias) implementation
    }

    template<typename P>
    const FactorizedPolynomial<P> operator-( const FactorizedPolynomial<P>& _fpolyA, const FactorizedPolynomial<P>& _fpolyB )
    {
        _fpolyA.strengthenActivity();
        _fpolyB.strengthenActivity();
        assert( &_fpolyA.cache() == &_fpolyB.cache() );
        // TODO (matthias) implementation
    }

    template<typename P>
    const FactorizedPolynomial<P> operator*( const FactorizedPolynomial<P>& _fpolyA, const FactorizedPolynomial<P>& _fpolyB )
    {
        _fpolyA.strengthenActivity();
        _fpolyB.strengthenActivity();
        assert( &_fpolyA.cache() == &_fpolyB.cache() );
        // TODO (matthias) implementation
    }

    template<typename P>
    const FactorizedPolynomial<P> lazyDiv( const FactorizedPolynomial<P>& _fpolyA, const FactorizedPolynomial<P>& _fpolyB )
    {
        _fpolyA.strengthenActivity();
        _fpolyB.strengthenActivity();
        assert( &_fpolyA.cache() == &_fpolyB.cache() );
        Factorization<P> resultFactorization;
        const Factorization<P>& factorizationA = _fpolyA.rFactorization();
        const Factorization<P>& factorizationB = _fpolyB.rFactorization();
        auto factorA = factorizationA.begin();
        auto factorB = factorizationB.begin();
        while( factorA != factorizationA.end() && factorB != factorizationB.end() )
        {
            if( factorA->first == factorB->first )
            {
                if ( factorA->second > factorB->second )
                    resultFactorization.insert( resultFactorization.end(), std::pair<FactorizedPolynomial<P>, size_t>(factorA->first, factorA->second - factorB->second ) );
                factorA++;
                factorB++;
            }
            else if( factorA->first < factorB->first )
            {
                // TODO (matthias) okay? or std::pair<FactorizedPolynomial<P>, size_t>( ... )
                resultFactorization.insert( resultFactorization.end(), *factorA );
                factorA++;
            }
            else
                factorB++;
        }
        while ( factorA != factorizationA.end() )
        {
            resultFactorization.insert( resultFactorization.end(), *factorA );
            factorA++;
        }

        Coeff<P> coefficientResult = _fpolyA.rCoefficient() / _fpolyB.rCoefficient();
        return FactorizedPolynomial<P>( std::move( resultFactorization ), coefficientResult, _fpolyA.mrCache );
    }

    template<typename P>
    const FactorizedPolynomial<P> commonDivisor( const FactorizedPolynomial<P>& _fpolyA, const FactorizedPolynomial<P>& _fpolyB, FactorizedPolynomial<P>& _fpolyRestA, FactorizedPolynomial<P>& _fpolyRestB )
    {
        _fpolyA.strengthenActivity();
        _fpolyB.strengthenActivity();
        assert( &_fpolyA.cache() == &_fpolyB.cache() );
        Factorization<P> cdFactorization, restAFactorization, restBFactorization;
        const Factorization<P>& factorizationA = _fpolyA.rFactorization();
        const Factorization<P>& factorizationB = _fpolyB.rFactorization();
        auto factorA = factorizationA.begin();
        auto factorB = factorizationB.begin();
        while( factorA != factorizationA.end() && factorB != factorizationB.end() )
        {
            if( factorA->first == factorB->first )
            {
                // TODO (matthias) okay? or std::pair<FactorizedPolynomial<P>, size_t>( ... )
                cdFactorization.insert( cdFactorization.end(), factorA->second < factorB->second ? *factorA : *factorB );
                factorA++;
                factorB++;
            }
            else if( factorA->first < factorB->first )
            {
                restAFactorization.insert( restAFactorization.end(), *factorA );
                factorA++;
            }
            else
            {
                restBFactorization.insert( restBFactorization.end(), *factorB );
                factorB++;
            }
        }
        while ( factorA != factorizationA.end() )
        {
            restAFactorization.insert( restAFactorization.end(), *factorA );
            factorA++;
        }
        while ( factorB != factorizationB.end() )
        {
            restBFactorization.insert( restBFactorization.end(), *factorB );
            factorB++;
        }

        Coeff<P> coefficientCommon = carl::gcd( _fpolyA.rCoefficient(), _fpolyB.rCoefficient() );
        Coeff<P> coefficientRestA = _fpolyA.rCoefficient() / coefficientCommon;
        Coeff<P> coefficientRestB = _fpolyB.rCoefficient() / coefficientCommon; 
        _fpolyRestA = FactorizedPolynomial<P>( std::move( restAFactorization ), coefficientRestA, _fpolyRestA.mrCache);
        _fpolyRestB = FactorizedPolynomial<P>( std::move( restBFactorization ), coefficientRestB, _fpolyRestB.mrCache);
        return FactorizedPolynomial<P>( std::move( cdFactorization ), coefficientCommon, _fpolyA.mrCache );
    }

    template<typename P>
    const FactorizedPolynomial<P> commonMultiple( const FactorizedPolynomial<P>& _fpolyA, const FactorizedPolynomial<P>& _fpolyB )
    {
        _fpolyA.strengthenActivity();
        _fpolyB.strengthenActivity();
        assert( &_fpolyA.cache() == &_fpolyB.cache() );
        Factorization<P> cmFactorization;
        const Factorization<P>& factorizationA = _fpolyA.rFactorization();
        const Factorization<P>& factorizationB = _fpolyB.rFactorization();
        auto factorA = factorizationA.begin();
        auto factorB = factorizationB.begin();
        while( factorA != factorizationA.end() && factorB != factorizationB.end() )
        {
            if( factorA->first == factorB->first )
            {
                // TODO (matthias) okay? or std::pair<FactorizedPolynomial<P>, size_t>( ... )
                cmFactorization.insert( cmFactorization.end(), factorA->second > factorB->second ? *factorA : *factorB );
                factorA++;
                factorB++;
            }
            else if( factorA->first < factorB->first )
            {
                cmFactorization.insert( cmFactorization.end(), *factorA );
                factorA++;
            }
            else
            {
                cmFactorization.insert( cmFactorization.end(), *factorB );
                factorB++;
            }
        }
        while ( factorA != factorizationA.end() )
        {
            cmFactorization.insert( cmFactorization.end(), *factorA );
            factorA++;
        }
        while ( factorB != factorizationB.end() )
        {
            cmFactorization.insert( cmFactorization.end(), *factorB );
            factorB++;
        }

        Coeff<P> coefficientCommon = carl::lcm( _fpolyA.rCoefficient(), _fpolyB.rCoefficient() );
        return FactorizedPolynomial<P>( std::move( cmFactorization ), coefficientCommon, _fpolyA.mrCache );
    }

    template<typename P>
    const FactorizedPolynomial<P> gcd( const FactorizedPolynomial<P>& _fpolyA, const FactorizedPolynomial<P>& _fpolyB, FactorizedPolynomial<P>& _fpolyRestA, FactorizedPolynomial<P>& _fpolyRestB )
    {
        _fpolyA.strengthenActivity();
        _fpolyB.strengthenActivity();
        bool rehashFPolyA = false;
        bool rehashFPolyB = false;
        Factorization<P> restAFactorization, restBFactorization;
        Factorization<P> gcdFactorization( gcd( _fpolyA.content(), _fpolyB.content(), restAFactorization, restBFactorization, rehashFPolyA, rehashFPolyB ) );

        if( rehashFPolyA )
            _fpolyA.rehash();
        if( rehashFPolyB )
            _fpolyB.rehash();

        Coeff<P> coefficientCommon = carl::gcd( _fpolyA.rCoefficient(), _fpolyB.rCoefficient() );
        Coeff<P> coefficientRestA = _fpolyA.rCoefficient() / coefficientCommon;
        Coeff<P> coefficientRestB = _fpolyB.rCoefficient() / coefficientCommon; 
        _fpolyRestA = FactorizedPolynomial<P>( std::move( restAFactorization ), coefficientRestA, _fpolyRestA.mrCache);
        _fpolyRestB = FactorizedPolynomial<P>( std::move( restBFactorization ), coefficientRestB, _fpolyRestB.mrCache);
        return FactorizedPolynomial<P>( std::move( gcdFactorization ), coefficientCommon, _fpolyA.mrCache );
    }
    
    template <typename P>
    std::ostream& operator<<( std::ostream& _out, const FactorizedPolynomial<P>& _fpoly )
    {
        return (_out << _fpoly.content());
    }
} // namespace carl