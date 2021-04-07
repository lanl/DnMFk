/* Copyright 2020 Gopinath Chennupati, Raviteja Vangara, Namita Kharat, Erik Skau and Boian Alexandrov,
Triad National Security, LLC. All rights reserved
This program was produced under U.S. Government contract 89233218CNA000001 for Los Alamos National Laboratory (LANL), which is operated by Triad National Security, LLC for the U.S. Department of Energy/National Nuclear Security Administration. All rights in the program are reserved by Triad National Security, LLC, and the U.S. Department of Energy/National Nuclear Security Administration. The Government is granted for itself and others acting on its behalf a nonexclusive, paid-up, irrevocable worldwide license in this material to reproduce, prepare derivative works, distribute copies to the public, perform publicly and display publicly, and to permit others to do so.

This file is modified from distanlsbpp.hpp file of Ramakrishnan Kannan's Planc library to include parameters required for distnmfk library
*/
#ifndef DISTNMF_DISTANLSBPP_HPP_
#define DISTNMF_DISTANLSBPP_HPP_
#include <string>
#include "../planc-master/distnmf/aunmf.hpp"
#include "nnls_bpp.hpp"

/**
 * Provides the updateW and updateH for the
 * distributed ANLS/BPP algorithm.
 */

#ifdef BUILD_CUDA
#define ONE_THREAD_MATRIX_SIZE 1000
#include <omp.h>
#else
#define ONE_THREAD_MATRIX_SIZE giventInput.n_cols + 5
#endif

namespace planc {

template <class INPUTMATTYPE>
class DistANLSBPP : public DistAUNMF<INPUTMATTYPE> {
 private:
  ROWVEC localWnorm;
  ROWVEC Wnorm;

  void allocateMatrices() {}

  /**
   * Multi threaded ANLS/BPP using openMP
   */
  void updateOtherGivenOneMultipleRHS(const MAT& giventGiven,
                                      const MAT& giventInput, MAT* othermat) {
    UINT numThreads = (giventInput.n_cols / ONE_THREAD_MATRIX_SIZE) + 1;
#pragma omp parallel for schedule(dynamic)
    for (UINT i = 0; i < numThreads; i++) {
      UINT spanStart = i * ONE_THREAD_MATRIX_SIZE;
      UINT spanEnd = (i + 1) * ONE_THREAD_MATRIX_SIZE - 1;
      if (spanEnd > giventInput.n_cols - 1) {
        spanEnd = giventInput.n_cols - 1;
      }
      // if it is exactly divisible, the last iteration is unnecessary.
      BPPNNLS<MAT, VEC>* subProblem;
      if (spanStart <= spanEnd) {
        if (spanStart == spanEnd) {
          subProblem = new BPPNNLS<MAT, VEC>(
              giventGiven, (VEC)giventInput.col(spanStart), true);
        } else {  // if (spanStart < spanEnd)
          subProblem = new BPPNNLS<MAT, VEC>(
              giventGiven, (MAT)giventInput.cols(spanStart, spanEnd), true);
        }
#ifdef MPI_VERBOSE
#pragma omp parallel
        {
          DISTPRINTINFO("Scheduling " << worh << " start=" << spanStart
                                      << ", end=" << spanEnd
                                      << ", tid=" << omp_get_thread_num());
        }
#endif
        subProblem->solveNNLS();
#ifdef MPI_VERBOSE
#pragma omp parallel
        {
          DISTPRINTINFO("completed " << worh << " start=" << spanStart
                                     << ", end=" << spanEnd
                                     << ", tid=" << omp_get_thread_num()
                                     << " cpu=" << sched_getcpu());
        }
#endif
        if (spanStart == spanEnd) {
          ROWVEC solVec = subProblem->getSolutionVector().t();
          (*othermat).row(i) = solVec;
        } else {  // if (spanStart < spanEnd)
          (*othermat).rows(spanStart, spanEnd) =
              subProblem->getSolutionMatrix().t();
        }
        subProblem->clear();
        delete subProblem;
      }
    }
  }

 protected:
  /**
   * update W given HtH and AHt
   * AHtij is of size \f$ k \times \frac{globalm}/{p}\f$.
   * this->W is of size \f$\frac{globalm}{p} \times k \f$
   * this->HtH is of size kxk
  */
  void updateW() {
    updateOtherGivenOneMultipleRHS(this->HtH, this->AHtij, &this->W);
    this->Wt = this->W.t();
  }
  /**
   * updateH given WtAij and WtW
   *  WtAij is of size \f$k \times \frac{globaln}{p} \f$
   * this->H is of size \f$ \frac{globaln}{p} \times k \f$
   * this->WtW is of size kxk
   */  
  void updateH() {
    updateOtherGivenOneMultipleRHS(this->WtW, this->WtAij, &this->H);
    this->Ht = this->H.t();
  }

 public:
  DistANLSBPP(const INPUTMATTYPE& input, const MAT& leftlowrankfactor,
              const MAT& rightlowrankfactor,
              const MPICommunicator& communicator, const int numkblks, const std::string relerr_dir)
      : DistAUNMF<INPUTMATTYPE>(input, leftlowrankfactor, rightlowrankfactor,
                                communicator, numkblks, relerr_dir) {
    localWnorm.zeros(this->k);
    Wnorm.zeros(this->k);
    PRINTROOT("DistANLSBPP() constructor successful");
  }

  ~DistANLSBPP() {
    /*
       tempHtH.clear();
       tempWtW.clear();
       tempAHtij.clear();
       tempWtAij.clear();
     */
  }
};  // class DistANLSBPP2D

}  // namespace planc

#endif  // DISTNMF_DISTANLSBPP_HPP_
