/* Copyright 2020 Gopinath Chennupati, Raviteja Vangara, Namita Kharat, Erik Skau and Boian Alexandrov,
Triad National Security, LLC. All rights reserved
This program was produced under U.S. Government contract 89233218CNA000001 for Los Alamos National Laboratory (LANL), which is operated by Triad National Security, LLC for the U.S. Department of Energy/National Nuclear Security Administration. All rights in the program are reserved by Triad National Security, LLC, and the U.S. Department of Energy/National Nuclear Security Administration. The Government is granted for itself and others acting on its behalf a nonexclusive, paid-up, irrevocable worldwide license in this material to reproduce, prepare derivative works, distribute copies to the public, perform publicly and display publicly, and to permit others to do so.
*/

#ifndef DISTNMF_DISTREORDER_HPP_
#define DISTNMF_DISTREORDER_HPP_

#include <unistd.h>
#include <armadillo>
#include <string>
#include "../planc-master/common/utils.hpp"
#include "../planc-master/common/distutils.hpp"
#include "../planc-master/distnmf/mpicomm.hpp"

/**
 * File name formats
 */

namespace planc {

template <class MATTYPE>
class DistReOrder {
 private:
  const MPICommunicator& m_mpicomm;
  int m_ownedm;
  int m_ownedn;
  int m_globalm;
  int m_globaln;

  int m_pr;
  int m_pc;
  int m_k;

  UROWVEC localMaxIDx; // vector of max element indices from each proc
  ROWVEC localMax;     // vector of max elements from each proc

  UROWVEC globalMaxIDx; // vector of max element indices from all proc(s)
  ROWVEC globalMax;     // vector of max elements from all proc(s)

  UROWVEC finalIDx;     //vector of final max element indices; 
                        //still contains the local indices
  UROWVEC globalIDx;    //Contains the global row indexes for m_k columns
  UVEC sortIDx;         //sort indices of W
  UVEC sharedSortIDx;   //rank 0 initialized with same sortIDx for pr * pc
  UVEC globalSortIDx;   // global sort indices of W

  UMAT globalIMx;       // matrix of final max-val indices across all proc(s)
  MAT globalM;          // matrix of final max elements across all proc(s)

  MAT W, H;             //Both the low rank factors
  MAT new_W, new_H;     //reordered low rank factors

  /**
   * Allocates matrices and vectors
   */
  void allocateData() {
    // Init vectors
    /*
    DISTPRINTINFO("k::"<< this->k <<"::localm::"<< this->m_ownedm <<"::localn::" 
                    << this->m_ownedn << "::globalm::" << this->m_globalm << 
                    "::globaln::"<< this->m_globaln <<"::MPI_SIZE::"<< MPI_SIZE);
    */
    localMax.zeros(this->m_k);
    localMaxIDx.zeros(this->m_k);
    globalMaxIDx.zeros(this->m_pr * this->m_pc * this->m_k);
    globalMax.zeros(this->m_pr * this->m_pc * this->m_k);
    finalIDx.zeros(this->m_k);
    globalIDx.zeros(this->m_k);
    sortIDx.zeros(this->m_k);
    sharedSortIDx.zeros(this->m_pr * this->m_pc, 1);
    globalSortIDx.zeros(this->m_k);

    // Init matrices
    globalIMx.zeros(this->m_pr * this->m_pc, this->m_k);
    globalM.zeros(this->m_pr * this->m_pc, this->m_k);

    // low-rank factor matrices
    new_W.zeros(this->W.n_rows, this->W.n_cols);
    new_H.zeros(this->H.n_rows, this->H.n_cols);
  }

  void freeMatrices() {
    localMax.clear();
    localMaxIDx.clear();
    globalMaxIDx.clear();
    globalMax.clear();
    finalIDx.clear();
    globalIDx.clear();
    globalIMx.clear();
    sortIDx.clear();
    globalSortIDx.clear();
    globalM.clear();
    W.clear();
    H.clear();
    new_W.clear();
    new_H.clear();
  }

 public:
    DistReOrder<MATTYPE>(const MAT &leftlowrankfactor, const MAT &rightlowrankfactor, 
            const MPICommunicator& communicator, const int k) 
        : m_mpicomm(communicator) {
        assert(leftlowrankfactor.n_cols == rightlowrankfactor.n_cols);
        this->m_k = k;
        this->m_pr = NUMROWPROCS;
        this->m_pc = NUMCOLPROCS;
        //Assign the low-rank factors
        this->W = leftlowrankfactor;
        this->H = rightlowrankfactor;
        this->m_ownedm  = leftlowrankfactor.n_rows;
        this->m_ownedn = rightlowrankfactor.n_cols;
        allocateData();
        PRINTROOT("distreorder()::constructor succesful");                        
    }

    ~DistReOrder() {
      //freeMatrices();
    }

    void finalGlobalSortIDx() {
        /* local Max and IDx */
        localMax = arma::max(this->W,0);
        localMaxIDx = arma::index_max(this->W, 0) + this->m_ownedm;
        //TODO! Need to take care of the topk maxima and indices here
        /* Global Max and IDx */
        MPITIC;  // gather globalMax, globalMaxIDx
        int sendcnt = this->m_k;
        int recvcnt = this->m_k;
        globalMax.zeros();
        MPI_Gather(localMax.memptr(), sendcnt, MPI_DOUBLE, globalMax.memptr(),
                recvcnt, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        globalMaxIDx.zeros();
        MPI_Gather(localMaxIDx.memptr(), sendcnt, MPI_DOUBLE, globalMaxIDx.memptr(),
                recvcnt, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        double temp = MPITOC;  // gather globalMax, globalMaxIDx
        //TODO! Need to report time later  
        /* finalIDx, globalM, globalIMx and globalIDx */   
        if(MPI_RANK == 0)   {
            globalIMx = arma::reshape(globalMaxIDx.t(), this->m_k, 
                                (this->m_pr * this->m_pc)).t();
            globalM = arma::reshape(globalMax, this->m_k, 
                                (this->m_pr * this->m_pc)).t();
            //globalM.print("globalM = ");
            //globalIMx.print("globalIMx = ");
            finalIDx = arma::index_max(globalM, 0);
            for(int ki = 0; ki < this->m_k; ki++)   {
                globalIDx(ki) = globalIMx(finalIDx(ki),ki);
            }
            //globalIDx.print("globalIDx = ");
            sortIDx = arma::sort_index(globalIDx);
            sortIDx.print("Rank 0 sortIDx = ");
            sharedSortIDx = arma::repmat(sortIDx, this->m_pr * this->m_pc, 1);
            //H.print("H in rank 0 = ");
            //globalSortIDx.print("Rank 0 globalSortIDx = "); 
        }
        //Scatter the sortIDx to all the procs in MPI_COMM_WORLD
        globalSortIDx.zeros();
        MPI_Scatter(this->sharedSortIDx.memptr(), sendcnt, MPI_DOUBLE, this->globalSortIDx.memptr(),
                    recvcnt, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    }

    void reorderW()   {
        finalGlobalSortIDx();
        MPI_Barrier(MPI_COMM_WORLD);
        //Now is the time for reordering the columns in W
        this->sortIDx.print("local sortIDx = ");
        this->globalSortIDx.print("globalSortIDX = ");
        //W.print("local W = ");
        //new_W.zeros();
        for(int col = 0; col < this->m_k; col++)    {
            //W is stored as row-major order
            new_W.col(col) = W.col(globalSortIDx(col));
        }
        if(MPI_RANK == 0) { new_W.print("local new_W = "); std::cout<<"current k "<<this->m_k<<std::endl; }
    }

    void reorderH()   {
        /*
        if(arma::sum(sortIDx) == 0){
            finalGlobalSortIDx();
        }*/
        MPI_Barrier(MPI_COMM_WORLD);
        //Now is the time for reordering the rows in H
        //new_H.zeros();
        for(int col = 0; col < this->m_k; col++)    {
            //In H also, swapping the cols, because, it is in column-major order
            new_H.col(col) = H.col(this->globalSortIDx(col));
        } 
    }

    /// Returns the left low rank factor matrix W
    MAT getLeftLowRankFactor() { return new_W; }
    /// Returns the right low rank factor matrix H
    MAT getRightLowRankFactor() { return new_H; }

  }; //class DistReOrder

}  // namespace planc

// run with mpi run 3.
void testDistClust(char argc, char* argv[]) {
  planc::MPICommunicator mpicomm(argc, argv);
  planc::DistReOrder<MAT> dc(arma::randn(10, 4), 
                  arma::randu(4,8), mpicomm, 8);;

  dc.reorderW();
}

#endif  // DISTNMF_DISTREORDER_HPP_
