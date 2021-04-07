/* Copyright 2020 Gopinath Chennupati, Raviteja Vangara, Namita Kharat, Erik Skau and Boian Alexandrov,
Triad National Security, LLC. All rights reserved
This program was produced under U.S. Government contract 89233218CNA000001 for Los Alamos National Laboratory (LANL), which is operated by Triad National Security, LLC for the U.S. Department of Energy/National Nuclear Security Administration. All rights in the program are reserved by Triad National Security, LLC, and the U.S. Department of Energy/National Nuclear Security Administration. The Government is granted for itself and others acting on its behalf a nonexclusive, paid-up, irrevocable worldwide license in this material to reproduce, prepare derivative works, distribute copies to the public, perform publicly and display publicly, and to permit others to do so.

This file includes distributed custom clustering and stability criterion based on distributed silhouette statistics.
*/

#ifndef DISTNMF_DISTCLUST_HPP_
#define DISTNMF_DISTCLUST_HPP_

#include <unistd.h>
#include <armadillo>
#include <string>
#include "../planc-master/common/utils.hpp"
#include "../planc-master/common/distutils.hpp"
#include "../planc-master/distnmf/mpicomm.hpp"
#include "../planc-master/distnmf/distnmftime.hpp"

#include <ctime>
#define CUBE arma::cube
#define FIELD arma::field

/**
 * File name formats
 */

namespace planc {

template <class MATTYPE>
class DistClust {
 private:
  const MPICommunicator& m_mpicomm;
  DistNMFTime time_stats;
  int m_ownedm;
  int m_ownedn;
  int m_slices;

  int m_pr;
  int m_pc;
  int m_k;

  MAT median_W, median_H;       //Both the median low rank factors
  MAT tempWc, tempHc;           //temporary matrices to hold all columns of W and H

  CUBE Wcube, Hcube;            //Both the cubes for low rank matrices 
  CUBE Wcube_new;

  MAT mad_W;                    //Mean absolute deviations from the median_W

  CUBE localWcubenorm, Wcubenorm; //column-wise L2-norms of the Wcube
  CUBE finalWcubenorm, fWcnorm;   //cross check the normalization
  CUBE localaiDotProd;            //m_slices x m_slices x m_k
  CUBE globalaiDotProd;           //m_slices x m_slices x m_k
  MAT aiAvgDist;                        // avg dist of i^th data-point to the rest in Cluster_i
  CUBE localbiDotProd;
  CUBE globalbiDotProd; 
  FIELD<CUBE> partDotProd;
  MAT biAvgDist;
  MAT si;                         //Final Silhouette widths

  MAT Centroids;
  CUBE localCosDist, globalCosDist;

  /**
   * Allocates matrices and vectors
   */
  void allocateData() {
    // Init matrices
    median_W.zeros(this->m_ownedm, this->m_k);
    median_H.zeros(this->m_ownedn, this->m_k);

    // low-rank factor matrices
    tempWc.zeros(this->m_ownedm, this->m_slices);
    tempHc.zeros(this->m_ownedn, this->m_slices);

    mad_W.zeros(this->m_ownedm, this->m_k);

    Wcube_new.zeros(this->m_ownedm, this->m_k, this->m_slices);

    //clustStability (Silhouette) data
    localWcubenorm.zeros(1, this->m_k, this->m_slices);
    Wcubenorm.zeros(1, this->m_k, this->m_slices);

    finalWcubenorm.zeros(1, this->m_k, this->m_slices);
    fWcnorm.zeros(1, this->m_k, this->m_slices);

    localaiDotProd.zeros(this->m_slices, this->m_slices, this->m_k);
    globalaiDotProd.zeros(this->m_slices, this->m_slices, this->m_k);
    aiAvgDist.zeros(this->m_slices, this->m_k); //m_k clusters and each has m_slices (data points)
    localbiDotProd.zeros(this->m_slices, this->m_slices, (this->m_k-1) );
    //partDotProd.zeros(this->m_k, 1);
    globalbiDotProd.zeros( this->m_slices, this->m_slices, (this->m_k-1) );
    biAvgDist.zeros(this->m_slices, this->m_k);
    si.zeros(this->m_slices, this->m_k);

    Centroids.zeros(this->m_k, this->m_ownedm);
    localCosDist.zeros(this->m_k, this->m_k, this->m_slices);
    globalCosDist.zeros(this->m_k, this->m_k, this->m_slices);
  }

  void freeMatrices() {
    median_W.clear();
    median_H.clear();

    tempWc.clear();
    tempHc.clear();

    Wcube.clear();
    Hcube.clear();
    Wcube_new.clear();

    mad_W.clear();

    localWcubenorm.clear();
    Wcubenorm.clear();

    finalWcubenorm.clear();
    fWcnorm.clear();

    localaiDotProd.clear();
    globalaiDotProd.clear();

    aiAvgDist.clear();
    localbiDotProd.clear();
    partDotProd.clear();
    globalbiDotProd.clear();
    biAvgDist.clear();
    si.clear();

    Centroids.clear();
    localCosDist.clear();
  }

 public:
    DistClust<MATTYPE>(const CUBE &Wall, const CUBE &Hall, 
            const MPICommunicator& communicator, const int k) 
        : m_mpicomm(communicator),
        time_stats(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)   {
        assert(Wall.n_cols == Hall.n_cols);
        this->m_k = k;
        this->m_pr = NUMROWPROCS;
        this->m_pc = NUMCOLPROCS;
        //Assign the low-rank factors
        this->Wcube = Wall;
        this->Hcube = Hall;
        this->m_ownedm  = Wall.n_rows;
        this->m_ownedn = Hall.n_rows;
        this->m_slices = Wall.n_slices;
        allocateData();
        PRINTROOT("distclust()::constructor succesful"); 

        // Column-wise normalize so that the cosine-distance is just a dot product
        normalizeWcube(); 
        // if(MPI_RANK == 0)   {
        //   this->Wcube.print("Wcube after = ");
        // }
        //MPI_Barrier(MPI_COMM_WORLD);
        //Error check to see the columns are unit-norm vectors
        // finalWcubenorm = sum(this->Wcube % this->Wcube, 0);
        /* check the unit-norm vecs of normalized W, Looks normalization works ...! */
        // TO DO! Need to report time later
        // mpitic();
        // MPI_Allreduce(finalWcubenorm.memptr(), fWcnorm.memptr(), this->m_k * this->m_slices,  
        //         MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        // double temp = mpitoc();
        // if(MPI_RANK == 0) { fWcnorm.print("fWcnorm = "); }                       
    }

    ~DistClust() {
      //freeMatrices();
    }

    /* Distributed mean absolute deviations (MAD) */
    VEC distMAD(int median_c)  {
      VEC mad;
      mad.zeros(this->m_ownedm);
      for(int s = 0; s < this->m_slices; s++)    {
        mad += arma::abs(this->tempWc.col(s) - this->median_W.col(median_c));  
      }
      mad = mad / this->m_slices;
      return mad;
    }

    void distMedian()  {
        //Find median W and H for one column at a time across all procs
        for(int ki = 0; ki < this->m_k; ki++)   { // For each column in rank 'k'
            //tempWc/Hc contains 'm_slices' points (a column is a point in same cluster)
            this->tempWc.zeros();
            this->tempHc.zeros();
            for(int s = 0; s < this->m_slices; s++)    {   // For each slice
                this->tempWc.col(s) = this->Wcube.slice(s).col(ki);
                this->tempHc.col(s) = this->Hcube.slice(s).col(ki);
            }
            // The median's are calculated on each row of the W, H
            //Therefore, the median W and H are of size (m/p x k) and (k x n/p) 
            this->median_W.col(ki) = arma::median(this->tempWc, 1); //per proc median W
            this->median_H.col(ki) = arma::median(this->tempHc, 1); //per proc median H
            mad_W.col(ki) = distMAD(ki); //mean absolute deviations
        }
        
        // if(MPI_RANK == 0)   {
        //     median_W.print("median_W = ");
        //     median_H.print("median_H = ");
        //     mad_W.print("mad_W = ");
        // }
    }

    /* 
      Distributed reordering of the columns in all Ws using a centroid 
      and K-means kind of an algorithm
    */ 
   void distReorder() {
     // Only for the testing purpose will be removed later
    //  CUBE WcubeBefore = Wcube;
    //  WcubeBefore.reshape( this->m_ownedm, this->m_k * this->m_slices, 1 );
    //  MAT reshaped_WcubeBefore = WcubeBefore.slice(0);
    //  std::stringstream sw;
    //  sw << "/lustre/scratch3/turquoise/gchennupati/factors/Wall_before_k" <<  this->m_k << "_" << MPI_SIZE << "_" << MPI_RANK;
    //  reshaped_WcubeBefore.save(sw.str(), arma::raw_ascii);
     // Remove until here after testing the clustering

     Centroids = Wcube.slice(0).t(); 
     Wcube_new.slice(0) = Centroids.t(); 

     for(int it = 0; it < 3; it++)  {
       localCosDist.zeros();
      // Calc local Cosine-distances from Centroid to W_i
      for(int s = 0; s < this->m_slices; s++) {
        // Find the dot product first
        localCosDist.slice(s) = Centroids * Wcube.slice(s);
        //  if(MPI_RANK == 0)  { Wcube.slice(s).print("Wcube.slice(s) = "); }
      }
      globalCosDist.zeros();
      // Calc global Cosine-distances or angular distances from Centroid to Ws in Wall
      MPI_Allreduce(localCosDist.memptr(), globalCosDist.memptr(), this->m_k * this->m_k * this->m_slices,
                MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
      // Angular similarity or Cosine Similarity
      globalCosDist = 1 - globalCosDist;

      // if(MPI_RANK == 0)  {
      //   globalCosDist.print("globalCosDist = ");
      // }  
      // Calc the orders of the clusters in Ws of Wall
      for(int s = 0; s < this->m_slices; s++)  {
        for(UWORD ki = 0; ki < this->m_k; ki++)  {
          //Find the global min distance in each m_k x m_k matrix 
          UWORD idx = globalCosDist.slice(s).index_min();
          UVEC cart_idx = arma::ind2sub(size(globalCosDist.slice(s)), idx);
          ROWVEC row_sum = sum(globalCosDist.slice(s), 0);
          VEC col_sum = sum(globalCosDist.slice(s), 1);
          // Update the corresponding centroid row and W column to find next global min dist
          globalCosDist.slice(s).row(cart_idx(0)) = row_sum;
          globalCosDist.slice(s).col(cart_idx(1)) = col_sum;
          //Reorder the clusters into a new Wcube
          Wcube_new.slice(s).col(cart_idx(0)) = Wcube.slice(s).col(cart_idx(1));
        }
      }
      this->Wcube = Wcube_new;
      distMedian(); // Calculate the median W
      if(it != 0)  {
        Centroids = median_W.t();
      }
     }
     //if(MPI_RANK == 0)  { Wcube.print("New Wcube is = "); }
   }

    /* Calculate the stability of the NMFk clusters
       We calculate the Silhouettes for now 
       Mean Absolute Deviation (MAD) is calculated in distMean()
     */
    void distClustStability() {
      clock_t begin = clock();
      MPITIC;
      try {
        distCalcAi();
        distCalcBi();

        // Wcube.reshape( this->m_ownedm, this->m_k * this->m_slices, 1 );
        // MAT reshaped_W = Wcube.slice(0);

        // if(MPI_RANK == 0) { cout<<"Reshaped W = \n"<<reshaped_W<<endl; }

        // std::stringstream sw;
        // sw << "/lustre/scratch3/turquoise/gchennupati/5760x3840/factors/Wall_after_k" <<  this->m_k << "_" << MPI_SIZE << "_" << MPI_RANK;
        // reshaped_W.save(sw.str(), arma::raw_ascii);

      } catch (const std::exception& e) {
          std::cout<<"Exception "<<e.what()<<std::endl;
      }
      double temp = MPITOC;
      this->time_stats.compute_duration(temp);
      clock_t end = clock();
      double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
      PRINTROOT("completed clustering for k=" << this->m_k << "::taken::" << this->time_stats.duration());
      PRINTROOT("clustering elpased time = "<< elapsed_secs);
    }

    /* Calculate a(i) = cosine_dist(i, j)*/
    void distCalcAi() {
      for(int ki = 0; ki < this->m_k; ki++) {
        CUBE tempCluster = Wcube( arma::span(), arma::span(ki, ki), arma::span() );
        tempCluster.reshape( this->m_ownedm, this->m_slices, tempCluster.n_cols );
        MAT tempClust = tempCluster.slice(0);
        //Calculate the dot product of a cluster data points
        localaiDotProd.slice(ki) = tempClust.t() * tempClust;
        //cout<<"Shapes are "<<size(localaiDotProd)<<" and "<<size(tempClust)<<endl;
      
      }
      MPI_Allreduce(localaiDotProd.memptr(), globalaiDotProd.memptr(), this->m_slices * this->m_slices * this->m_k,
              MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
      // Following line is the most important, because we calc cosine similarity not cosine distance
      globalaiDotProd = 1 - globalaiDotProd;
      //cout<<"Shapes are "<<size(globalaiDotProd)<<" and "<<size((sum(this->globalaiDotProd.slice(0), 1) - 1) / (this->m_slices - 1))<<endl;
      // Similarity of a cluster smaller the a_i better the cluster assignment of a point
      for(int ki = 0; ki < this->m_k; ki++) {
        this->aiAvgDist.col(ki) = sum(this->globalaiDotProd.slice(ki), 1)  / (this->m_slices - 1);
        //cout<<"cluster "<<ki<<" sum is "<<(sum(this->globalaiDotProd.slice(ki), 1) - 1) / (this->m_slices - 1)<<endl;
      } 
      // if(MPI_RANK == 0)   {
      //   //this->globalaiDotProd.print("globalaiDotProd = ");
      //   this->aiAvgDist.print("Avg ai distances = ");
      // }
      
      // Clear temporary data

    }

    /* Calculate b(i) = minimum cosine_dist(i, j) 
       from point i in cluster i to all other points in cluster j (averaged)
       Repeat it for all the cluster where i != j and the minimum avg distance 
       is the distance for that point.
    */
    void distCalcBi() {
      for(int ki = 0; ki < this->m_k; ki++)  {
       localbiDotProd.zeros();
       CUBE tempCluster = Wcube( arma::span(), arma::span(ki, ki), arma::span() );
       tempCluster.reshape( this->m_ownedm, this->m_slices, tempCluster.n_cols );
       MAT tempClust = tempCluster.slice(0);
       int count = 0;
       for(int kj = 0; kj < this->m_k; kj++)  { 
         if(ki != kj) {
          CUBE tempCluster2 = Wcube( arma::span(), arma::span(kj, kj), arma::span() );
          tempCluster2.reshape( this->m_ownedm, this->m_slices, tempCluster.n_cols );
          MAT tempClust2 = tempCluster2.slice(0);
          localbiDotProd.slice(count) = tempClust.t() * tempClust2;
          count++;
          //cout<<"count increment "<<count<<endl;
         }
        }
        globalbiDotProd.zeros();
        //partDotProd(ki, 0) = localbiDotProd;
        MPI_Allreduce(localbiDotProd.memptr(), globalbiDotProd.memptr(), this->m_slices * this->m_slices * (this->m_k-1),
              MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        // Following is the most important, because we calc cosine similarity not cosine distance
        globalbiDotProd = arma::abs(1 - globalbiDotProd);
        
        CUBE tempMean = arma::mean(globalbiDotProd, 1);
        tempMean.reshape( tempMean.n_rows, tempMean.n_slices, 1 );
        MAT allMeans = tempMean.slice(0);
        biAvgDist.col(ki) = arma::min( allMeans, 1 );
        // if(MPI_RANK == 0)   {
        //   //this->globalbiDotProd.print("globalbiDotProd = ");
        //   cout<<"means = \n"<< tempMean.slice(0)<<endl;
        // }

        // Clear the temporary data
        allMeans.clear();
      }
      
      // Following line is the most important, because we calc cosine similarity not cosine distance
      // for(UWORD ki = 0; ki < this->m_k; ki++) {
      //   globalbiDotProd(ki, 0) = 1 - globalbiDotProd(ki, 0);
      //   CUBE tempMean = arma::mean(globalbiDotProd(ki, 0), 1);
      //   tempMean.reshape( this->m_slices, this->m_k-1, 1 );
      //   biAvgDist.col(ki) = arma::min(tempMean.slice(0), 1);
      // }
      
      // for(int ki = 0; ki < this->m_k; ki++) {
      //   biAvgDist.col(ki) = min(globalbiDotProd.slice(ki), 1);
      // }
      /* Final Silhouette for all data points */
      this->si = (biAvgDist - aiAvgDist) / arma::max(biAvgDist, aiAvgDist);
      
      /*
      if(MPI_RANK == 0)   {
        //this->globalbiDotProd.print("globalbiDotProd = ");
        // this->biAvgDist.print("Avg bi distances = ");
        // this->si.print("Final Silhouettes = ");
        std::stringstream si;
        si << "/lustre/scratch3/turquoise/gchennupati/5760x3840/silhouettes/Si_at_k" << this->m_k;   
        this->si.save(si.str(), arma::raw_ascii);
        //cout<<"bi-ai "<<(biAvgDist-aiAvgDist)<<endl;
        //cout<<"Max(bi, ai) "<<max(biAvgDist, aiAvgDist)<<endl;
      }
      */
    }

    /* Returns the final Silhouettes at a given rank 'k' */
    MAT getSilhouettes()  { return si; }

    /* 
      Normalize columns of Wcube with the respective L2-norm
     */
    void normalizeWcube()  {
      localWcubenorm = sum(this->Wcube % this->Wcube, 0);
      mpitic();
      MPI_Allreduce(localWcubenorm.memptr(), Wcubenorm.memptr(), this->m_k * this->m_slices, 
                MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
      double temp = mpitoc();
      // if(MPI_RANK == 0) { 
      //  Wcubenorm.print("global Wcube_sq_norm "); 
      // }
      //this->time_stats.allgather_duration(temp);
      for(int s = 0; s < this->m_slices; s++) {
        for (int c = 0; c < this->m_k; c++) {
          if ( Wcubenorm.slice(s)(c) > 1 ) {
            double norm_const = sqrt( Wcubenorm.slice(s)(c) );
            this->Wcube.slice(s).col(c) = this->Wcube.slice(s).col(c) / norm_const;
            // this->Hcube.slice(s).col(i) = this->Hcube.slice(s).col(i) * norm_const;
          }
        }
      }
    }

    /// Returns the left low rank (median) factor matrix W
    MAT getLeftLowRankFactor() { return median_W; }

    /// Returns the right low rank (median) factor matrix H
    MAT getRightLowRankFactor() { return median_H; }

  }; //class DistReOrder

}  // namespace planc

#endif  // DISTNMF_DISTCLUST_HPP_
