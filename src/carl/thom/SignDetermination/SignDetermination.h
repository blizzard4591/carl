/* 
 * File:   SignDeterminationGeneric.h
 * Author: tobias
 *
 * Created on 14. Juni 2016, 19:18
 */

#pragma once

#include <Eigen/Dense>
#include <cmath>

#include "../TarskiQuery/TarskiQuery.h"
#include "../ThomUtil.h"
#include "../ThomSettings.h"

namespace carl {

/*
 * Calculates the set of sign conditions realized by the polynomials in the list p on the roots of z.
 * This is essential to many algorithms dealing with thom encondings of RANs.
 * 
 * z ist the zero set. In the univariate case this can only contain one polynomial.
 * (in theory, it could contain more, but then gcd would have to be computed and we dont want this)
 * in the multivariate case, it should contain as many polynomials as there are variables
 */
template<typename Polynomial, typename Settings = ThomDefaultSettings>
std::vector<SignCondition> signDetermination(const std::vector<Polynomial>& p, const std::vector<Polynomial>& z);

template<typename Polynomial, typename Settings = ThomDefaultSettings>
std::vector<SignCondition> signDetermination(const std::vector<Polynomial>& p, const Polynomial& z) {
        return signDetermination(p, std::vector<Polynomial>(1, z));
}

// todo: offer a version that takes bounds?

}



namespace carl {
        
using Eigen::MatrixXf;
using Eigen::VectorXf;
using Eigen::Matrix3f;
using Eigen::Vector3f;
using Eigen::FullPivLU;
using Eigen::PartialPivLU;

MatrixXf adaptedMat(const std::vector<std::vector<unsigned>>& A, const std::vector<SignCondition>& sign);

const std::vector<SignCondition> basicSignCondition = {{Sign::ZERO}, {Sign::POSITIVE}, {Sign::NEGATIVE}};
const std::vector<std::vector<unsigned>> basicAdaptedList = {{0}, {1}, {2}};
const Matrix3f basicMatrixInverse = adaptedMat(basicAdaptedList, basicSignCondition).inverse();

template<typename T>
std::vector<T> operator*(const T elem, const std::vector<T>& vec) {
        std::vector<T> res(vec);
        res.insert(res.begin(), elem);
        return res;
}

float sigmaToTheAlpha(const std::vector<unsigned>& alpha, const SignCondition& sigma) {
        assert(alpha.size() == sigma.size());
        float res = 1.0f;
        for(unsigned i = 0; i < alpha.size(); i++) {
                res *= (float)std::pow((float)sigma[i], alpha[i]);
        }
        return res;
}


MatrixXf adaptedMat(const std::vector<std::vector<unsigned>>& A, const std::vector<SignCondition>& sign) {
        MatrixXf res(A.size(), sign.size());
        for(unsigned i = 0; i < A.size(); i++) {
                for(unsigned j = 0; j < sign.size(); j++) {
                        res(i, j) = sigmaToTheAlpha(A[i], sign[j]);
                }
        }
        return res;
}



// https://forum.kde.org/viewtopic.php?f=74&t=50952
MatrixXf kroneckerProduct(const MatrixXf& m1, const MatrixXf& m2) {
        MatrixXf m3(m1.rows() * m2.rows(), m1.cols() * m2.cols());
        for (unsigned i = 0; i < m1.cols(); i++) {
                for (unsigned j = 0; j < m1.rows(); j++) {
                        m3.block(i * m2.rows(), j * m2.cols(), m2.rows(), m2.cols()) =  m1(i, j) * m2;
                }
        }
        return m3;
}

template<typename Polynomial, typename Settings>
VectorXf compute_dprime(const std::vector<Polynomial>& P,
                        const std::vector<std::vector<unsigned>>& B,
                        const std::vector<std::vector<unsigned>>& Ada,
                        const TarskiQueryManager<Polynomial>& tarskiQuery) {
        // asserts
        assert(B.front().size() == 1); // must hold for all vectors in B
        assert(P.size() == Ada.front().size() + 1);
        
        PRINT("Input to compute_dprime:");
        PRINTV(P); PRINTV(B); PRINTV(Ada);
        
        // first calculate the list of mappings B x Ada
        std::vector<std::vector<unsigned>> bxada;
        for(unsigned i = 0; i < Ada.size(); i++) {
                for(unsigned j = 0; j < B.size(); j++) {
                        std::vector<unsigned> newMapping(B[j]);
                        auto it = newMapping.end();
                        newMapping.insert(it, Ada[i].begin(), Ada[i].end());
                        bxada.push_back(newMapping);
                }
        }
        PRINTV(bxada);
        assert(P.size() == bxada.front().size());
        VectorXf res(bxada.size()); // we already know the length of the resulting vector
        for(unsigned i = 0; i < bxada.size(); i++) {
                // evaluate the i-th element of bxada on P
                Polynomial poly = P.front();
                poly = poly.divideBy(poly).quotient; // now poly is the 1-polynomial ... somewhat stupid. optimize this
                for(unsigned j = 0; j < P.size(); j++) {
                        poly *= P[j].pow(bxada[i][j]);
                        if(Settings::SIGN_DETERMINATION_REDUCE) {
                                // poly = poly.remainder(z);
                                // todo implement!!
                        }
                }
                PRINTV(poly);
                int taq = tarskiQuery(poly);
                res(i) = (float)taq;
                 
        }
        PRINTV2(res);
        LINE;
        return res;
}



std::vector<SignCondition> calculateR(unsigned which, const std::vector<SignCondition>& sign, const std::vector<SignCondition>& newSign) {
        assert(which == 2 || which == 3);
        std::vector<SignCondition> res;
        for(const SignCondition& sigma : sign) {
                unsigned count = 0;
                for(const SignCondition& tau : newSign) {
                        if(extends(tau, sigma)) {
                                count++;
                                // TODO: early break
                        }
                }
                if(count >= which) {
                        res.push_back(sigma);
                }
        }
        return res;
}

std::vector<std::vector<unsigned>> firstNLines(const unsigned n, const MatrixXf& mat, const std::vector<std::vector<unsigned>>& ada) {
        assert(n > 0);
        // first n lines as a matrix
        MatrixXf firstLines(n, mat.cols());
        for(unsigned i = 0; i < n; i++) {
                for(unsigned j = 0; j < mat.cols(); j++) {
                        firstLines(i, j) = mat(i, j);
                }
        }
        PRINTV2(firstLines);
        FullPivLU<MatrixXf> dec(firstLines);
        PRINTV(dec.rank());
        // not sure if it is meant to be that the first n lines are always linearly independent
        // or we have to find the "first" lin. ind. n lines.
        assert(dec.rank() == n);
        std::vector<std::vector<unsigned>> res(ada.begin(), ada.begin() + n); // this is just the first n elements of ada
        return res;
}


std::vector<std::vector<unsigned>> adaptedFamily(const std::vector<SignCondition>& sign,                // Sign(Q,Z)
                                                 const std::vector<SignCondition>& newSign,             // Sign({P} u Q, Z)
                                                 const std::vector<std::vector<unsigned>>& ada) {       // Ada(Q, Z)
        assert(sign.front().size() > 0);
        // TODO implement for the case where Q is empty
        unsigned r_1 = (unsigned)sign.size();
        PRINTV(r_1);
        std::vector<std::vector<unsigned>> res(ada);
        for(auto& a : res) {
                a.insert(a.begin(), 0); // NOT SURE WHICH ONE!!
                //a.push_back(0);
        }
        
        if(newSign.size() != r_1) {
                // this means r_2 != 0
                // compute r_2
                std::vector<SignCondition> sign2 = calculateR(2, sign, newSign);
                unsigned r_2 = (unsigned)sign2.size();
                PRINTV(sign2);
                PRINTV(r_2);
                MatrixXf mat2 = adaptedMat(ada, sign2);
                PRINTV2(mat2);
                std::vector<std::vector<unsigned>> A_2 = firstNLines(r_2, mat2, ada);
                for(const std::vector<unsigned>& a : A_2) {
                        res.push_back((unsigned)1 * a);
                }
                PRINTV(res);
                if((unsigned)newSign.size() != r_1 + r_2) {
                        std::vector<SignCondition> sign3 = calculateR(3, sign, newSign);
                        unsigned r_3 = (unsigned)sign3.size();
                        PRINTV(r_3);
                        std::vector<std::vector<unsigned>> A_3 = firstNLines(r_3, mat2, ada);
                        for(const auto& a : A_3) {
                                res.push_back((unsigned)2 * a);
                        }
                        PRINTV(res);
                }       
        }       
        return res;
        
}



template<typename Polynomial, typename Settings = ThomDefaultSettings>
std::vector<SignCondition> signDetermination(const std::vector<Polynomial>& p, const std::vector<Polynomial>& z) {
        static_assert(is_polynomial<Polynomial>::value, "only call 'signDetermination' on polynomial types");
        PRINT("Input to signDetermination:");
        PRINTV(p); PRINTV(z);
        assert(isZeroDimensionalSystem(z)); // because otherwise the set Zeros(z) is infinite
        assert((unsigned)p.size() > 0);
        
        // set up the tarski query manager for this sign determination
        TarskiQueryManager<Polynomial> tarskiQuery(z);
               
        int r = tarskiQuery(1); // this is just the total number of roots of z
        assert(r >= 0);
        PRINTV(r);
        if(r == 0) return std::vector<SignCondition>(); // then z has no roots, so there are no realisable sign conditions on the roots of z
        LINE;
        
        // main loop
        std::vector<SignCondition> lastSign; // this is an "ordered set" (lexicografically with 0 < 1 < -1)
        std::vector<std::vector<unsigned>> lastAda;
        for(int i = (int)p.size() - 1; i >= 0; i--) {
                PRINTV(i);
                LINE;
                std::vector<SignCondition> sign; // our first aim in this step is to compute this
                
                int taq1 = tarskiQuery(p[i]);
                int taq2 = tarskiQuery(p[i] * p[i]);
                assert(std::abs(taq1) <= r && std::abs(taq2) <= r);
                Vector3f t((float)r, (float)taq1, (float)taq2); // right hand side of equation system
                PRINTV2(t);
                Vector3f c = (Vector3f) (basicMatrixInverse * t);
                PRINTV2(c);
                
                if(c(0) != 0.0f) sign.push_back(SignCondition(1, Sign::ZERO));
                if(c(1) != 0.0f) sign.push_back(SignCondition(1, Sign::POSITIVE));
                if(c(2) != 0.0f) sign.push_back(SignCondition(1, Sign::NEGATIVE));
                PRINTV(sign);
                assert(sign.size() > 0 && sign.size() <= 3);
                
                std::vector<std::vector<unsigned>> B(basicAdaptedList.begin(), basicAdaptedList.begin() + sign.size());
                assert(B.size() == sign.size());
                PRINTV(B);
                MatrixXf M = adaptedMat(B, sign);
                PRINTV(M);
                
                LINE;
                
                if(i == (unsigned)p.size() - 1) {
                        lastSign = sign;
                        lastAda = B;
                        if(i == 0) { // then we are already done
                                return sign;
                        }
                }
                else { // i < p.size()
                        VectorXf d_prime = compute_dprime<Polynomial,Settings>(std::vector<Polynomial>(p.begin() + i, p.end()), B, lastAda, tarskiQuery);
                        MatrixXf M_prime = kroneckerProduct(adaptedMat(lastAda, lastSign), M);
                        PRINTV2(M_prime);
                        assert(M_prime.rows() == M_prime.cols() && M_prime.rows() == d_prime.size()); // assert square matrix and d_prime has correct length
                        
                        // do the linear algebra stuff
                        assert(FullPivLU<MatrixXf>(M_prime).rank() == M_prime.cols()); // assert M_prime is invertible
                        PartialPivLU<MatrixXf> dec(M_prime);
                        VectorXf c_prime = dec.solve(d_prime);
                        PRINTV2(c_prime);
                        
                        std::vector<SignCondition> newSign;
                        assert(c_prime.size() == sign.size() * lastSign.size());
                        unsigned k = 0;
                        for(unsigned i = 0; i < lastSign.size(); i++) {
                                for(unsigned j = 0; j < sign.size(); j++) {
                                       if((int)c_prime(k) != 0) {
                                               newSign.push_back(SignCondition(sign[j].front() * lastSign[i]));
                                       }
                                       k++;
                                }
                        }
                        PRINTV(newSign);
                        if(i == 0) { // then we are done!
                                return newSign;
                        }
                        
                        LINE;
                        PRINT("compute the ADAPTED FAMILY now ... inputs to this are:");
                        PRINTV(lastSign);
                        PRINTV(newSign);
                        PRINTV(lastAda);
                        
                        std::vector<std::vector<unsigned>> newAda = adaptedFamily(lastSign, newSign, lastAda);
                        PRINTV(newAda);
                        
                        LINE;
                        
                        lastSign = newSign;
                        lastAda = newAda;
                        
                        LINE;
                }
                
                
        } // end of main loop
        
        // this is to supress a warning - we should never get here
        assert(false);
        return std::vector<SignCondition>();
        
}

} // namespace carl

