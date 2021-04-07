/* Copyright 2020 Gopinath Chennupati, Raviteja Vangara, Namita Kharat, Erik Skau and Boian Alexandrov,
Triad National Security, LLC. All rights reserved
This program was produced under U.S. Government contract 89233218CNA000001 for Los Alamos National Laboratory (LANL), which is operated by Triad National Security, LLC for the U.S. Department of Energy/National Nuclear Security Administration. All rights in the program are reserved by Triad National Security, LLC, and the U.S. Department of Energy/National Nuclear Security Administration. The Government is granted for itself and others acting on its behalf a nonexclusive, paid-up, irrevocable worldwide license in this material to reproduce, prepare derivative works, distribute copies to the public, perform publicly and display publicly, and to permit others to do so.

This file is modified from nnls_bpp.hpp file of Ramakrishna Kannan's Planc library.

*/

#ifndef NNLS_BPPNNLS_HPP_
#define NNLS_BPPNNLS_HPP_

#ifdef MKL_FOUND
#include <mkl.h>
#else
#include <lapacke.h>
#endif
#include <assert.h>
#include "ActiveSetNNLS.h"
#include "../planc-master/nnls/nnls.hpp"
#include "../planc-master/common/utils.hpp"
#include <set>
#include <algorithm>
#include <iomanip>
#include "ActiveSetNNLS.hpp"
#include "../planc-master/nnls/SortBooleanMatrix.hpp"

template <class MATTYPE, class VECTYPE>
class BPPNNLS : public NNLS<MATTYPE, VECTYPE> {
  public:
    BPPNNLS(MATTYPE input, VECTYPE rhs, bool prodSent = false):
        NNLS<MATTYPE, VECTYPE>(input, rhs, prodSent) {
    }
    BPPNNLS(MATTYPE input, MATTYPE RHS, bool prodSent = false) :
        NNLS<MATTYPE, VECTYPE>(input, RHS, prodSent) {
    }
    int solveNNLS() {
        int rcIterations = 0;
        if (this->k == 1) {
            rcIterations = solveNNLSOneRHS();
        } else {
            // r must be greater than 1 in this case.
            // we initialized r appropriately in the
            // constructor.
            rcIterations = solveNNLSMultipleRHS();
        }
        return rcIterations;
    }
  private:
    /*
     * This implementation is based on Algorithm 1 on Page 6 of paper
     * http://www.cc.gatech.edu/~hpark/papers/SISC_082117RR_Kim_Park.pdf.
     *
     */

    int solveNNLSOneRHS() {
        // local to this function.
        UINT MAX_ITERATIONS = this->n * 2;
        UVEC F;
        UVEC G(this->n);
        STDVEC allIdxs;
        // initalizations
        for (UINT i = 0; i < G.n_rows; i++) {
            G(i) = i;
            allIdxs.push_back(i);
        }
        VECTYPE y = -this->Atb;
#ifdef _VERBOSE
        INFO << std::endl <<  "C : " << this->AtA;
        INFO << "b : " << this->Atb;
        INFO << "Init y:" << y;
#endif
        UINT alpha = 3;
        UINT beta = this->n + 1;
        int numIterations = 0;
        std::setprecision(64);
        bool solutionFound = false;
        // main loop
        while (numIterations < MAX_ITERATIONS) {
            // compute V = {i \in F : x_i < 0} union {i \in G : y_i < 0}
            // iterate over F to find x_i < 0
            UVEC V1, V2;
            STDVEC Vs;
            V1 = find(this->x(F) < 0);
            V2 = find(y(G) < 0);

            STDVEC Fv1 = arma::conv_to<STDVEC>::from(F(V1));
            STDVEC Gv2 = arma::conv_to<STDVEC>::from(G(V2));
            std::set_union(Fv1.begin(),
                           Fv1.end(),
                           Gv2.begin(),
                           Gv2.end(),
                           std::inserter(Vs, Vs.begin()));

            UVEC V = arma::conv_to<UVEC>::from(Vs);
#ifdef _VERBOSE
            INFO << "xf<0 : " << V1.size() << std::endl << V1;
            INFO << "yg<0 : " << V2.size() << std::endl << V2;
            INFO << "V :" << V.size() << std::endl << V;
#endif

            // Terminate loop if there is nothing to swap.
            // solution found.
            if (V.empty()) {
                solutionFound = true;
                break;
            }
            // Step 5 of the algorithm.
            if (V.size() < beta) {
                beta = V.size();
                alpha = 3;
            } else {
                if (alpha >= 1) {
                    alpha--;  // step 6 of the algorithm
                } else {
                    // Step 7 of the algorithm.
                    alpha = 0;
                    int temp = V(V.n_rows - 1);
                    V.clear();
                    V = temp;
                }
            }
            // step 8 of the algorithm 1.
            fixAllSets(F, G, V, allIdxs);
#ifdef _VERBOSE
            INFO << "V:" << V.size() << std::endl << V;
            INFO << "a : " << alpha << ", b:" << beta << std::endl;
            INFO << "F:" << F.size() << std::endl << F << std::endl;
            INFO << "G:" << G.size() << std::endl << G << std::endl;
            INFO << "b4 x:" << this->x << std::endl;
            INFO << "b4 y:" << y << std::endl;
#endif
            y.zeros();
            this->x(G).zeros();
            // clear up and save only the passive set with the optimal solution
            // for x and active set in y.
            // Step 9 of algorithm 1;
            // this->x(F) = arma::solve(this->AtA(F,F),this->Atb.rows(F));
            this->x(F) = solveSymmetricLinearEquations(this->AtA(F, F),
                         this->Atb.rows(F));
            VECTYPE lhs = (this->AtA * this->x);
            y = lhs - this->Atb;
#ifdef _VERBOSE
            INFO << "after x:" << this->x << std::endl;
            INFO << "lhs y:" << lhs;
            INFO << "lhs(0) " << lhs.row(0) << " first b" << this->Atb.row(0)
                 << "diff value " << lhs.row(0) - this->Atb.row(0) << std::endl;
            INFO << "after y:" << y(G) << std::endl;
#endif
            this->x(G).zeros();
            y(F).zeros();
            fixNumericalError<VECTYPE>(&this->x);
            fixNumericalError<VECTYPE>(&y);
            // according to lawson and hanson if for alpha==0, the computed
            // x at V is negative, it is because of the numerical error of
            // y at V being negative. Set it to zero.
            if (alpha == 0) {
                int temp = V(0);
                if (this->x(temp) < 0) {
                    WARN << "invoking lawson and hanson's fix" << std::endl;
                    y(temp) = 0;
                    this->x(temp) = 0;
                }
            }
            numIterations++;
        }
        if (numIterations >= MAX_ITERATIONS && !solutionFound) {
            ERR << "max iterations passed. calling Hanson's ActivesetNNLS"
                << std::endl;
            ERR << "current x initialized for Hanson's algo : " <<  std::endl
                << this->x;
            ActiveSetNNLS<double> anls(this->n, this->n);
            double rNorm;
            anls.solve(this->AtA.memptr(), static_cast<int>(this->n),
                       this->Atb.memptr(), this->x.memptr(), rNorm);
            double *nnlsDual = anls.getDual();
            INFO << "Activeset NNLS Dual:" << std::endl;
            for (int i = 0; i < this->n; i++) {
                INFO << nnlsDual[i] << std::endl;
            }
        }
        return numIterations;
    }
    /*
     * This is the implementation of Algorithm 2 at Page 8 of the paper
     * http:// www.cc.gatech.edu/~hpark/papers/SISC_082117RR_Kim_Park.pdf.
     */

    int solveNNLSMultipleRHS() {
        UINT currentIteration = 0;
        UINT MAX_ITERATIONS = this->n * 2;
        MATTYPE Y = -this->AtB;
        UVEC Fv(this->n * this->k);
        Fv.zeros();
        UVEC Gv(this->n * this->k);
        arma::umat V(this->n, this->k);
        STDVEC allIdxs;
        UVEC alphaZeroIdxs(this->k);
        bool solutionFound = false;
        for (UINT i = 0; i < this->n * this->k; i++) {
            Gv(i) = i;
            allIdxs.push_back(i);
        }
        UVEC alpha(this->k), beta(this->k);
        alpha.ones();
        alpha = alpha * 3;
        beta.ones();
        beta = beta * (this->n + 1);
#ifdef _VERBOSE
        INFO << "Gv :" << Gv.size() << std::endl << Gv;
        INFO << "Rank : " << arma::rank(this->AtA) << std::endl;
        INFO << "Condition : " << cond(this->AtA) << std::endl;
        INFO << "a: " << std::endl << alpha;
        INFO << "b: " << std::endl << beta;
        INFO << "Y: " << std::endl << Y;
        INFO << "Rank : " << arma::rank(this->AtA) << std::endl;
        INFO << "Condition : " << cond(this->AtA) << std::endl;
#endif
        while (currentIteration < MAX_ITERATIONS) {
            UVEC V1 = find(this->X(Fv) < 0);
            UVEC V2 = find(Y(Gv) < 0);
            // Fv was initialized to zeros because the find during
            // first iteration with empty Fv gave an error.
            // However, zero Fv was giving wrong Gv during fixAllSets.
            // Hence clearing Fv as the zero Idx really does not
            // belong to Fv in the initialization.
            if (currentIteration == 0) {
                Fv.clear();
            }
#ifdef _VERBOSE
            INFO << "X(Fv)<0 : " << V1.size() << std::endl <<  V1;
            INFO << "Y(Gv)<0 : " << V2.size() << std::endl << V2;
#endif
            STDVEC Vs;

            STDVEC Fvv1 = arma::conv_to<STDVEC>::from(Fv(V1));
            STDVEC Gvv2 = arma::conv_to<STDVEC>::from(Gv(V2));
            std::set_union(Fvv1.begin(),
                           Fvv1.end(),
                           Gvv2.begin(),
                           Gvv2.end(),
                           std::inserter(Vs, Vs.begin()));

            UVEC VIdx = arma::conv_to<UVEC>::from(Vs);
            if (VIdx.empty()) {
#ifdef _VERBOSE
                INFO << "Terminating the loop" << std::endl;
#endif
                solutionFound = true;
                break;
            }
            V.zeros();
            V(VIdx).ones();
#ifdef _VERBOSE
            INFO << "V:" << V.size() << std::endl << V;
#endif
            STDVEC nonOptCols;
            /*
             * Armadillo gives linearIdx for find function.
             * finding non optimum columns from the non optimum linear indices
             * is done here.
             */
            //           for (UINT i = 0; i < VIdx.size(); i++)
            //           {
            //               nonOptCols.push_back(VIdx(i) % r);
            //           }
            //           std::sort(nonOptCols.begin(), nonOptCols.end());
            //           // remove the duplicates.
            //           nonOptCols.erase(std::unique(nonOptCols.begin(), nonOptCols.end()),
            //                   nonOptCols.end());
            UVEC NonOptCols = find(sum(V) != 0);
#ifdef _VERBOSE
            INFO << "NonOptCols:" << NonOptCols.size() << NonOptCols;
#endif
            alphaZeroIdxs.ones();
            alphaZeroIdxs = alphaZeroIdxs * -1;
            for (UINT i = 0; i < NonOptCols.size(); i++) {
                int currentIdx = NonOptCols(i);
                // Step 7 of the algorithm.
                if (sum(V.col(currentIdx)) < beta(currentIdx)) {
                    beta(currentIdx) = sum(V.col(currentIdx));
                    alpha(currentIdx) = 3;
                } else {
                    if (alpha(currentIdx) >= 1) {
                        alpha(currentIdx)--;  // step 8 of the algorithm
                    } else {
                        // Step 9 of the algorithm.
                        alpha(currentIdx) = 0;
                        UVEC temp = find(V.col(currentIdx) != 0);
#ifdef _VERBOSE
                        INFO << "temp : " << std::endl << temp << "max :"
                             << temp.max() << std::endl;
#endif
                        V.col(currentIdx).zeros();
                        V(temp.max(), currentIdx) = 1;
                        alphaZeroIdxs(currentIdx) = temp.max();
                    }
                }
#ifdef _VERBOSE
                INFO << "idx:" << currentIdx << endl;
                INFO << "V:" << V.col(currentIdx);
                INFO << "a : " << alpha(currentIdx)
                     << ", b:" << beta(currentIdx) << endl;
#endif
            }
            VIdx = find(V != 0);
            // step 10 of the algorithm 2
            fixAllSets(Fv, Gv, VIdx, allIdxs);
#ifdef _VERBOSE
            INFO << "F:" << std::endl << Fv << std::endl;
            INFO << "G:" << std::endl << Gv << std::endl;
            INFO << "VIdx:" << std::endl << VIdx << std::endl;
#endif
            Y(Fv).zeros();
            this->X(Gv).zeros();
#ifdef _VERBOSE
            INFO << "b4 x:" << std::endl << this->X << std::endl;
            INFO << "b4 y:" << std::endl << Y << std::endl;
#endif
            // solve LSQ with multiple RHS.
            // Step 11 of Algorithm 2.
            arma::umat PassiveSet(this->n, this->k);
            PassiveSet.zeros();
            PassiveSet(Fv).ones();
            UVEC FvCols = find(sum(PassiveSet) != 0);
            this->X.cols(FvCols) = solveNormalEqComb(this->AtA,
                                   this->AtB.cols(FvCols),
                                   PassiveSet.cols(FvCols));
            Y.cols(FvCols) = (this->AtA * this->X.cols(FvCols))
                             - this->AtB.cols(FvCols);
            fixNumericalError<MATTYPE>(&this->X);
            fixNumericalError<MATTYPE>(&Y);
            // according to lawson and hanson if for alpha==0, the computed
            // x at V is negative, it is because of the numerical error of
            // y at V being negative. Set it to zero.
            for (UINT i = 0; i < alphaZeroIdxs.size(); i++) {
                if (alphaZeroIdxs(i) != -1) {
                    if (this->X(alphaZeroIdxs(i), i) < 0) {
                        WARN << "invoking lawson and hanson's fix for col"
                             << i << "it = " << currentIteration << std::endl;
                        Y(alphaZeroIdxs(i), i) = 0;
                        this->X(alphaZeroIdxs(i), i) = 0;
                    }
                }
            }

#ifdef _VERBOSE
            INFO << "after x:" << std::endl <<  this->X;
            INFO << "after y:" << std::endl << Y;
#endif
            currentIteration++;
        }
        if (currentIteration >= MAX_ITERATIONS && !solutionFound) {
            ERR << "something wrong. appears to be infeasible" << std::endl;
#ifdef _VERBOSE
            INFO << "X : " << this->X.n_rows << "x" << this->X.n_cols
                 << " AtB:" << this->AtB.n_rows << "x" << this->AtB.n_cols
                 << " AtA" << this->AtA.n_rows << "x" << this->AtA.n_cols
                 << " : r=" << this->k << " :p=" << this->m
                 << " :q=" << this->n << std::endl;
            std::ostringstream fileName, fileName2;
            int temp = rand();
            fileName << "errinputmatrix" << temp;
            INFO << "input file matrix " << fileName.str() << std::endl;
            this->AtA.save(fileName.str());
            fileName2 << "errrhsmatrix" << temp;
            INFO << "rhs file matrix " << fileName2.str() << std::endl;
            this->AtB.save(fileName2.str());
            sleep(60);
            exit(EXIT_FAILURE);
#endif
            INFO << "calling classical activeset" << std::endl;
            for (UINT i = 0; i < this->k; i++) {
                UVEC V1 = find(this->X.col(i) < 0);
                UVEC V2 = find(Y.col(i) < 0);
                if (!V1.empty() || !V2.empty()) {
#ifdef _VERBOSE
                    WARN << "col " << i << " not optimal " << std::endl;
                    WARN << "current x initialized for Hanson's algo : "
                         <<  std::endl << this->X.col(i);
#endif
                    ActiveSetNNLS<double> anls(this->n, this->n);
                    double rNorm;
                    double *currentX = new double[this->n];
                    double *currentRHS = new double[this->n];
                    for (UINT j = 0; j < this->n; j++) {
                        currentX[j] = this->X(j, i);
                        currentRHS[j] = this->AtB(j, i);
                    }
                    anls.solve(this->AtA.memptr(), static_cast<int>(this->n),
                               currentRHS, currentX, rNorm);
                    for (UINT j = 0; j < this->n; j++) {
                        this->X(j, i) = currentX[j];
                    }
                }
            }
        }
        return currentIteration;
    }
    /*
     * This function to support the step 10 of the algorithm 2.
     * This is implementation of the paper
     * Fast algorithm for the solution of large-scale non-negativity-constrained least squares problems
     * M. H. Van Benthem and M. R. Keenan, J. Chemometrics 2004; 18: 441-450
     * Motivated out of implementation from Jingu's solveNormalEqComb.m
     */
    MATTYPE solveNormalEqComb(MATTYPE AtA, MATTYPE AtB, arma::umat PassSet) {
        MATTYPE Z;
        UVEC Pv = find(PassSet != 0);
        UVEC anyZeros = find(PassSet == 0);
        if (anyZeros.empty()) {
            // Everything is the in the passive set.
            // INFO << "starting empty activeset solve" << endl;
            // Z = arma::solve(AtA,AtB);
            // INFO << "exiting empty activeset solve" << endl;
            Z = solveSymmetricLinearEquations(AtA, AtB);
        } else {
            Z.resize(AtB.n_rows, AtB.n_cols);
            Z.zeros();
            UINT k1 = PassSet.n_cols;
            if (k1 == 1) {
                // INFO << "entry one col pass set" << endl;
                // Z(Pv)=arma::solve(AtA(Pv,Pv),AtB(Pv));
                // INFO << "exit one col pass set" << endl;
                Z(Pv) = solveSymmetricLinearEquations(AtA(Pv, Pv), AtB(Pv));
            } else {
                // we have to group passive set columns that are same.
                // find the correlation matrix of passive set matrix.
                std::vector<UWORD> sortedIdx, beginIdx;
                computeCorrelationScore(PassSet, sortedIdx, beginIdx);
                // UVEC solved(k1);
                // solved.zeros();
                // assert(corrPassMat.n_rows==PassSet.n_cols);
                // assert(corrPassMat.n_cols==PassSet.n_cols);
                for (UINT i = 1; i < beginIdx.size(); i++) {
                    // if(!solved(i))
                    // {
                    // UVEC samePassiveSetCols=find(corrPassMat.col(i)==1);
                    UWORD sortedBeginIdx = beginIdx[i - 1];
                    UWORD sortedEndIdx = beginIdx[i];
                    UVEC samePassiveSetCols(std::vector<UWORD>
                                            (sortedIdx.begin() + sortedBeginIdx,
                                             sortedIdx.begin() + sortedEndIdx));
                    // solved(samePassiveSetCols).ones();
                    UVEC currentPassiveSet = find(PassSet.col( sortedIdx[sortedBeginIdx] ) == 1);
#ifdef _VERBOSE
                    INFO << "samePassiveSetCols:" << std::endl
                         <<  samePassiveSetCols;
                    INFO << "currPassiveSet : " << std::endl
                         << currentPassiveSet;
                    INFO << "AtA:" << std::endl
                         << AtA(currentPassiveSet, currentPassiveSet);
                    INFO << "AtB:" << std::endl
                         << AtB(currentPassiveSet, samePassiveSetCols);
#endif
                    Z(currentPassiveSet, samePassiveSetCols) =
                        solveSymmetricLinearEquations(AtA(currentPassiveSet, currentPassiveSet),
                                                      AtB(currentPassiveSet, samePassiveSetCols));
                }
            }
        }
#ifdef _VERBOSE
        INFO << "Returning mat Z:" << std::endl << Z;
#endif
        return Z;
    }
    /*
     * This constructs the sets F, G and V based on
     * equation 3.5a and 3.5b. This is also the
     * step 8 of algorithm1 and step 10 of algorithm 2.
     */
    void fixAllSets(UVEC &F, UVEC &G, UVEC &V, STDVEC &allIdxs) {
        // G = (G-V) union (V intersection F)
        std::set<int> temp1, temp2;
        STDVEC vecG = arma::conv_to<STDVEC>::from(G);
        STDVEC vecV = arma::conv_to<STDVEC>::from(V);
        STDVEC vecF = arma::conv_to<STDVEC>::from(F);
        std::set_difference(vecG.begin(), vecG.end(), vecV.begin(), vecV.end(),
                            std::inserter(temp1, temp1.begin()));
        std::set_intersection(vecV.begin(), vecV.end(), vecF.begin(), vecF.end(),
                              std::inserter(temp2, temp2.begin()));
        STDVEC Gs;
        std::set_union(temp1.begin(), temp1.end(), 
                       temp2.begin(), temp2.end(),
                       std::inserter(Gs, Gs.begin()));
        G = arma::conv_to<UVEC>::from(Gs);



        // F = (F-V) union (V intersection G)
        // Generally FUG = allIdxs and FnG=null.
        // G is small in size. Hence we find G first
        // and use it to find F.
        //       std::set<int> temp1, temp2, temp3, temp4;
        STDVEC newF;
        //       // std library didn't work. Hence used boost.
        std::set_difference(allIdxs.begin(), allIdxs.end(),
                            Gs.begin(), Gs.end(),
                            std::inserter(newF, newF.begin()));
        //       boost::set_intersection(arma::conv_to<STDVEC>::from(V),
        //               arma::conv_to<STDVEC>::from(G),
        //               std::inserter(temp2, temp2.begin()));
        //       boost::set_union(temp1, temp2, std::inserter(newF, newF.begin()));
        F.clear();
        F = arma::conv_to<UVEC>::from(newF);
    }
#ifdef _VERBOSE
    void printSet(std::set<int> a) {
        for (std::set<int>::iterator it = a.begin(); it != a.end(); it++) {
            cout << *it << ",";
        }
    }
#endif

    MATTYPE solveSymmetricLinearEquations(MATTYPE A, MATTYPE B) {
        lapack_int info = 0;
        lapack_int n = A.n_cols;
        lapack_int nrhs = B.n_cols;
        lapack_int lda = A.n_rows;
        lapack_int ldb = A.n_rows;
        lapack_int CONSTONE = 1;
        if (n <= 0 || nrhs <= 0) {
            ERR << "something wrong in input" << " n=" << n
                << " nrhs=" << nrhs << std::endl;
            exit(EXIT_FAILURE);
        }
        LAPACKE_dposv(LAPACK_COL_MAJOR, 'U', n, nrhs, A.memptr(), lda, B.memptr(), ldb);
        if ((signed int)info != 0) {
            ERR << "something wrong in dpotsv call to blas info = "
                << (signed int)info <<  std::endl;
            ERR << " A = " << A.n_rows << "x" << A.n_cols << "r(A)="
                << arma::rank(A)  << std::endl << A;
            ERR << " B = " << B.n_rows << "x" << B.n_cols << std::endl << B;
            exit(EXIT_FAILURE);
        }
        return B;
    }
    /*
    * Passset is a binary matrix where every column represents
    * one datapoint. The objective is to returns a low triangular
    * correlation matrix with 1 if the strings are equal. Zero otherwise
    */
    void computeCorrelationScore(arma::umat &PassSet, std::vector<UWORD> &sortedIdx,
                                 std::vector<UWORD> &beginIndex) {
        SortBooleanMatrix<arma::umat> sbm(PassSet);
        sortedIdx = sbm.sortIndex();
        BooleanArrayComparator<arma::umat> bac(PassSet);
        uint beginIdx = 0;
        beginIndex.clear();
        beginIndex.push_back(beginIdx);
        for (uint i = 0; i < sortedIdx.size(); i++) {
            if (i == sortedIdx.size() - 1 || bac(sortedIdx[i], sortedIdx[i + 1]) == true) {
                beginIdx = i + 1;
                beginIndex.push_back(beginIdx);
            }
        }
    }

    /*
    * Given a matrix, the last column of the matrix will be
    * checked if it reappears again. Every column in the matrix
    * is sorted index.
    */
    bool detectCycle(arma::umat &X) {
        UVEC lastColumn = X.col(X.n_cols - 1);
        for (uint i = 0; i < X.n_cols - 2; i++) {
            arma::umat compVec = (X.col(i) == lastColumn);
            if (sum(compVec.col(0)) == X.n_rows)
                return true;
        }
    }
};
#endif /* NNLS_BPPNNLS_HPP_ */
