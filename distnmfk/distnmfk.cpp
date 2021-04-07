/* Copyright 2020 Gopinath Chennupati, Raviteja Vangara, Namita Kharat, Erik Skau and Boian Alexandrov,
Triad National Security, LLC. All rights reserved
This program was produced under U.S. Government contract 89233218CNA000001 for Los Alamos National Laboratory (LANL), which is operated by Triad National Security, LLC for the U.S. Department of Energy/National Nuclear Security Administration. All rights in the program are reserved by Triad National Security, LLC, and the U.S. Department of Energy/National Nuclear Security Administration. The Government is granted for itself and others acting on its behalf a nonexclusive, paid-up, irrevocable worldwide license in this material to reproduce, prepare derivative works, distribute copies to the public, perform publicly and display publicly, and to permit others to do so.
This function computes the stability of the hidden varibles via a custom k-means like clustering with the same 
number of components and silhouette statistics */

#include <string>
#include "../planc-master/common/distutils.hpp"
#include "../planc-master/distnmf/distnmf.hpp"
#include "parsecommand.hpp"
#include "../planc-master/common/utils.hpp"
#include "../planc-master/distnmf/distals.hpp"
#include "dist_anls_bpp.hpp"
#include "../planc-master/distnmf/distaoadmm.hpp"
#include "../planc-master/distnmf/disthals.hpp"
#include "dist_io.hpp"
#include "../planc-master/distnmf/distmu.hpp"
#include "../planc-master/distnmf/mpicomm.hpp"
#include "../planc-master/distnmf/naiveanlsbpp.hpp"
#include "distreorder.hpp"
#include "distclust.hpp"
//#include "distbcd.hpp"
#ifdef BUILD_CUDA
#include <cuda.h>
#include <cuda_runtime.h>
#endif
#define CUBE arma::cube
#define FIELD arma::field
namespace planc {
class DistNMFk  {
private:
  int m_argc;
  char **m_argv;
  int m_k;
  UWORD m_globalm, m_globaln;
  std::string m_Afile_name;
  std::string m_outputfile_name;
  int m_num_it;
  int m_pr;
  int m_pc;
  FVEC m_regW;
  FVEC m_regH;
  algotype m_nmfalgo;
  double m_sparsity;
  //double objective_err;
  iodistributions m_distio;
  uint m_compute_error;
  int m_num_k_blocks;
  static const int kprimeoffset = 17;
  static const int kPrimeOffset = 10;
  normtype m_input_normalization;

  int upper_k;
  int num_perturbs;
  std::string m_outputfile_sils;

#ifdef BUILD_CUDA
  void printDevProp(cudaDeviceProp devProp) {
    printf("Major revision number: %d\n", devProp.major);
    printf("Minor revision number: %d\n", devProp.minor);
    printf("Name: %s\n", devProp.name);
    printf("Total global memory: %u\n", devProp.totalGlobalMem);
    printf("Total shared memory per block: %u\n", devProp.sharedMemPerBlock);
    printf("Total registers per block: %d\n", devProp.regsPerBlock);
    printf("Warp size: %d\n", devProp.warpSize);
    printf("Maximum memory pitch: %u\n", devProp.memPitch);
    printf("Maximum threads per block: %d\n", devProp.maxThreadsPerBlock);
    for (int i = 0; i < 3; ++i)
      printf("Maximum dimension %d of block: %d\n", i,
             devProp.maxThreadsDim[i]);
    for (int i = 0; i < 3; ++i)
      printf("Maximum dimension %d of grid: %d\n", i, devProp.maxGridSize[i]);
    printf("Clock rate: %d\n", devProp.clockRate);
    printf("Total constant memory: %u\n", devProp.totalConstMem);
    printf("Texture alignment: %u\n", devProp.textureAlignment);
    printf("Concurrent copy and execution: %s\n",
           (devProp.deviceOverlap ? "Yes" : "No"));
    printf("Number of multiprocessors: %d\n", devProp.multiProcessorCount);
    printf("Kernel execution timeout: %s\n",
           (devProp.kernelExecTimeoutEnabled ? "Yes" : "No"));
    return;
  }

  void gpuQuery() {
    int devCount;
    cudaGetDeviceCount(&devCount);
    printf("CUDA Device Query...\n");
    printf("There are %d CUDA devices.\n", devCount);
    // Iterate through devices
    for (int i = 0; i < devCount; ++i) {
      // Get device properties
      printf("\nCUDA Device #%d\n", i); 
      cudaDeviceProp devProp;
      cudaGetDeviceProperties(&devProp, i);
      printDevProp(devProp);
    }
  }
#endif

  void printConfig() {
      std::cout << "a::" << this->m_nmfalgo << "::i::" << this->m_Afile_name
         << "::k::" << this->m_k << "::m::" << this->m_globalm
         << "::n::" << this->m_globaln << "::t::" << this->m_num_it
         << "::pr::" << this->m_pr << "::pc::" << this->m_pc
         << "::error::" << this->m_compute_error
         << "::distio::" << this->m_distio << "::regW::"
         << "l2::" << this->m_regW(0) << "::l1::" << this->m_regW(1)
         << "::regH::"
         << "l2::" << this->m_regH(0) << "::l1::" << this->m_regH(1)
         << "::num_k_blocks::" << this->m_num_k_blocks
         << "::normtype::" << this->m_input_normalization << std::endl;
  }

template <class NMFTYPE>
  void nmfk1D() {
    std::string rand_prefix("rand_");
    MPICommunicator mpicomm(this->m_argc, this->m_argv);
#ifdef BUILD_SPARSE
    DistIO<SP_MAT> dio(mpicomm, m_distio);
#else   // ifdef BUILD_SPARSE
    DistIO<MAT> dio(mpicomm, m_distio);
#endif  // ifdef BUILD_SPARSE

    if (m_Afile_name.compare(0, rand_prefix.size(), rand_prefix) == 0) {
      dio.readInput(m_Afile_name, this->m_globalm, this->m_globaln, this->m_k,
                    this->m_sparsity, this->m_pr, this->m_pc,
                    this->m_input_normalization);
    } else {
      dio.readInput(m_Afile_name);
    }
#ifdef BUILD_SPARSE
    SP_MAT Arows(dio.Arows());
    SP_MAT Acols(dio.Acols());
#else   // ifdef BUILD_SPARSE
    MAT Arows(dio.Arows());
    MAT Acols(dio.Acols());
#endif  // ifdef BUILD_SPARSE

    if (m_Afile_name.compare(0, rand_prefix.size(), rand_prefix) != 0) {
      this->m_globaln = Arows.n_cols;
      this->m_globalm = Acols.n_rows;
    }
    INFO << mpicomm.rank()
         << "::Completed generating 1D rand Arows=" << PRINTMATINFO(Arows)
         << "::Acols=" << PRINTMATINFO(Acols) << std::endl;
#ifdef WRITE_RAND_INPUT
    dio.writeRandInput();
#endif  // ifdef WRITE_RAND_INPUT
    MAT Arows_new(dio.Arows());
    Arows_new.zeros();
    MAT Acols_new(dio.Acols());
    Acols_new.zeros();
    MAT Beta(Arows.n_rows, Arows.n_cols);
    Beta.ones();
    Acols_new = Acols + Acols_new;
    Arows_new = Arows + Arows_new;
    // nmfk main loop -- Iterate over k=2:100 (you can get it from cmd later)
    // For each 'k', run nmf for 30 iter with a small perturbation in A.
    for(int k=2; k <= 100; k++) {
        this->m_k = k;
        //if(mpicomm.rank() <= mpicomm.size())  {
        for(int iter = 0; iter < 10; iter++)   {
            // don't worry about initializing with the
            // same matrix as only one of them will be used.
            arma::arma_rng::set_seed(random_sieve(mpicomm.rank() + kprimeoffset));
            MAT W = arma::randu<MAT>(this->m_globalm / mpicomm.size(), this->m_k);
            MAT H = arma::randu<MAT>(this->m_globaln / mpicomm.size(), this->m_k);
            sleep(10);
            MPI_Barrier(MPI_COMM_WORLD);
            memusage(mpicomm.rank(), "b4 constructor ");
            NMFTYPE nmfAlgorithm(Arows, Acols, W, H, mpicomm);
            sleep(10);
            memusage(mpicomm.rank(), "after constructor ");
            nmfAlgorithm.num_iterations(this->m_num_it);
            nmfAlgorithm.compute_error(this->m_compute_error);
            nmfAlgorithm.algorithm(this->m_nmfalgo);
            MPI_Barrier(MPI_COMM_WORLD);
            try {
            //nmfAlgorithm.computeNMFwithConv();
            nmfAlgorithm.computeNMF();
            } catch (std::exception &e) {
            printf("Failed rank %d: %s\n", mpicomm.rank(), e.what());
            MPI_Abort(MPI_COMM_WORLD, 1);
            }

            if (!m_outputfile_name.empty()) {
            dio.writeOutput(nmfAlgorithm.getLeftLowRankFactor(),
                            nmfAlgorithm.getRightLowRankFactor(), m_outputfile_name);
            }
            Beta.zeros();
            Arows_new.zeros();
            Acols_new.zeros();
            Arows_new = Arows + Arows_new;
            Acols_new = Acols_new + Acols;
            dio.randBetaMatrix("uniform", mpicomm.rank() + kPrimeOffset, &Beta);
            MPI_Barrier(MPI_COMM_WORLD);
        }// End for loop 'iter'
    } //End of for loop 'k (rank)'
  }

template <class NMFTYPE>
void nmfK2D() {
  std::string rand_prefix("rand_");
  MPICommunicator mpicomm(this->m_argc, this->m_argv, this->m_pr, this->m_pc);
  // #ifdef BUILD_CUDA
  //         if (mpicomm.rank()==0){
  //             gpuQuery();
  //         }
  // #endif
#ifdef USE_PACOSS
  std::string dim_part_file_name = this->m_Afile_name;
  dim_part_file_name += ".dpart.part" + std::to_string(mpicomm.rank());
  this->m_Afile_name += ".part" + std::to_string(mpicomm.rank());
  INFO << mpicomm.rank() << ":: part_file_name::" << dim_part_file_name
        << "::m_Afile_name::" << this->m_Afile_name << std::endl;
  Pacoss_SparseStruct<double> ss;
  ss.load(m_Afile_name.c_str());
  std::vector<std::vector<Pacoss_IntPair> > dim_part;
  Pacoss_Communicator<double>::loadDistributedDimPart(
      dim_part_file_name.c_str(), dim_part);
  Pacoss_Communicator<double> *rowcomm = new Pacoss_Communicator<double>(
      MPI_COMM_WORLD, ss._idx[0], dim_part[0]);
  Pacoss_Communicator<double> *colcomm = new Pacoss_Communicator<double>(
      MPI_COMM_WORLD, ss._idx[1], dim_part[1]);
  this->m_globalm = ss._dimSize[0];
  this->m_globaln = ss._dimSize[1];
  arma::umat locations(2, ss._idx[0].size());

  for (Pacoss_Int i = 0; i < ss._idx[0].size(); i++) {
    locations(0, i) = ss._idx[0][i];
    locations(1, i) = ss._idx[1][i];
  }
  arma::vec values(ss._idx[0].size());

  for (Pacoss_Int i = 0; i < values.size(); i++) values[i] = ss._val[i];
  SP_MAT A(locations, values);
  A.resize(rowcomm->localRowCount(), colcomm->localRowCount());
#else  // ifdef USE_PACOSS
  if ((this->m_pr > 0) && (this->m_pc > 0) &&
      (this->m_pr * this->m_pc != mpicomm.size())) {
    ERR << "pr*pc is not MPI_SIZE" << std::endl;
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
#ifdef BUILD_SPARSE
  UWORD nnz;
  DistIO<SP_MAT> dio(mpicomm, m_distio);

  if (mpicomm.rank() == 0) {
    INFO << "sparse case" << std::endl;
  }
#else   // ifdef BUILD_SPARSE
  DistIO<MAT> dio(mpicomm, m_distio);
#endif  // ifdef BUILD_SPARSE. One outstanding PACOSS
  if (m_Afile_name.compare(0, rand_prefix.size(), rand_prefix) == 0) {
    dio.readInput(m_Afile_name, this->m_globalm, this->m_globaln, this->m_k,
                  this->m_sparsity, this->m_pr, this->m_pc,
                  this->m_input_normalization);
  } else {
    dio.readInput(m_Afile_name);
  }
#ifdef BUILD_SPARSE
  // SP_MAT A(dio.A().row_indices, dio.A().col_ptrs, dio.A().values,
  // dio.A().n_rows, dio.A().n_cols);
  SP_MAT A(dio.A());
#else   // ifdef BUILD_SPARSE
  MAT A(dio.A());
#endif  // ifdef BUILD_SPARSE. One outstanding PACOSS
  if (m_Afile_name.compare(0, rand_prefix.size(), rand_prefix) != 0) {
    UWORD localm = A.n_rows;
    UWORD localn = A.n_cols;

    /*MPI_Allreduce(&localm, &(this->m_globalm), 1, MPI_INT,
      *            MPI_SUM, mpicomm.commSubs()[0]);
      * MPI_Allreduce(&localn, &(this->m_globaln), 1, MPI_INT,
      *            MPI_SUM, mpicomm.commSubs()[1]);*/
    this->m_globalm = localm * m_pr;
    this->m_globaln = localn * m_pc;
  }
#ifdef WRITE_RAND_INPUT
  dio.writeRandInput();
#endif  // ifdef WRITE_RAND_INPUT
#endif  // ifdef USE_PACOSS. Everything over. No more outstanding ifdef's.
  MAT A_new(A.n_rows, A.n_cols);
  MAT Beta(A.n_rows, A.n_cols);
  // A_new.zeros();
  // Beta.ones();
  // A_new = ( A + A_new ) % (Beta);

  // nmfk main loop -- Iterate over k=2:100 (you can get it from cmd later)
  // For each 'k', run nmf for 30 iter with a small perturbation in A.
  //for(int k=2; k < this->m_globaln/2; k++) {
  for(int k = this->m_k; k <= this->upper_k; k=k+1) {
    int runs = this->num_perturbs;
    int curr_k = k;
    CUBE Wall(this->m_globalm / mpicomm.size(), curr_k, runs);
    CUBE Hall(this->m_globaln / mpicomm.size(), curr_k, runs);
    Wall.zeros();
    Hall.zeros();
    MAT Wout(this->m_globalm, curr_k);
    Wout.zeros();
    //if(mpicomm.rank() <= mpicomm.size())  {
    for(int iter = 0; iter < runs; iter++)   {
      // don't worry about initializing with the
      // same matrix as only one of them will be used.
      arma::arma_rng::set_seed(mpicomm.rank() + k + iter);
      //Perturbations with an error rate
      Beta.zeros();
      A_new.zeros();
      dio.randBetaMatrix("uniform", mpicomm.rank() +  kPrimeOffset + k + iter, &Beta);
      A_new = ( A + A_new ) % (Beta);
      // Print A_new
      //dio.writeRandInput(); // Need to change this function
#ifdef USE_PACOSS
      MAT W = arma::randu<MAT>(rowcomm->localOwnedRowCount(), curr_k);
      MAT H = arma::randu<MAT>(colcomm->localOwnedRowCount(), curr_k);
#else   // ifdef USE_PACOSS
      MAT W = arma::randu<MAT>(this->m_globalm / mpicomm.size(), curr_k);
      MAT H = arma::randu<MAT>(this->m_globaln / mpicomm.size(), curr_k);
#endif  // ifdef USE_PACOSS
      //dio.writeOutput(W, H, iter, curr_k, m_outputfile_name+"_INIT_");
      // sometimes for really very large matrices starting w/
      // rand initialization hurts ANLS BPP running time. For a better
      // initializer we run couple of iterations of HALS.
#ifndef USE_PACOSS
#ifdef BUILD_SPARSE
      if (m_nmfalgo == ANLSBPP) {
        DistHALS<SP_MAT> lrinitializer(A_new + Beta, W, H, mpicomm, this->m_num_k_blocks, this->m_outputfile_sils);
        lrinitializer.num_iterations(4);
        lrinitializer.algorithm(HALS);
        lrinitializer.computeNMFwithConv();
        W = lrinitializer.getLeftLowRankFactor();
        H = lrinitializer.getRightLowRankFactor();
      }
#endif  // ifdef BUILD_SPARSE
#endif  // ifndef USE_PACOSS 

#ifdef MPI_VERBOSE
      INFO << mpicomm.rank() << "::" << __PRETTY_FUNCTION__
          << "::" << PRINTMATINFO(W) << std::endl;
      INFO << mpicomm.rank() << "::" << __PRETTY_FUNCTION__
          << "::" << PRINTMATINFO(H) << std::endl;
#endif  // ifdef MPI_VERBOSE
      // MPI_Barrier(MPI_COMM_WORLD);
      memusage(mpicomm.rank(), "b4 constructor ");
      NMFTYPE nmfAlgorithm(A_new, W, H, mpicomm, this->m_num_k_blocks, this->m_outputfile_sils);
#ifdef USE_PACOSS
      nmfAlgorithm.set_rowcomm(rowcomm);
      nmfAlgorithm.set_colcomm(colcomm);
#endif  // ifdef USE_PACOSS
      memusage(mpicomm.rank(), "after constructor ");
      nmfAlgorithm.num_iterations(this->m_num_it);
      nmfAlgorithm.compute_error(this->m_compute_error);
      nmfAlgorithm.algorithm(this->m_nmfalgo);
      nmfAlgorithm.regW(this->m_regW);
      nmfAlgorithm.regH(this->m_regH);
      // MPI_Barrier(MPI_COMM_WORLD);
      try {
          mpitic();
          //if (this->m_nmfalgo == BCD)  {
          //  nmfAlgorithm.computeNMFBCD();
          //} else {
            // nmfAlgorithm.computeNMFwithConv();
            nmfAlgorithm.computeNMF();
          //}
          double temp = mpitoc();
          if (mpicomm.rank() == 0) printf("NMF took %.3lf secs.\n", temp);
      } catch (std::exception &e) {
          printf("Failed rank %d: %s\n", mpicomm.rank(), e.what());
          MPI_Abort(MPI_COMM_WORLD, 1);
      }
      // Beta.zeros();
      // A_new.zeros();
      // dio.randBetaMatrix("uniform", mpicomm.rank() +  kPrimeOffset + k + iter, &Beta);
      // A_new = ( A + A_new ) % (Beta);
 
      //Store all the Ws across perturbations
      Wall.slice(iter) = nmfAlgorithm.getLeftLowRankFactor();
      Hall.slice(iter) = nmfAlgorithm.getRightLowRankFactor();
//      nmfAlgorithm.computeObjectiveError();
//     if(mpicomm.rank() == 0) printf("Objective error %.3lf \n", this->objective_err);
//     if(mpicomm.rank() == 0) printf("Relative error %.3lf \n", this->objective_err/nmfAlgorithm.globalsqnorma());
//     std::string outfullName = m_outputfile_sils+"relerr_at_k"+std::to_string(k);
//     std::ofstream outfile;
//      if (mpicomm.rank() == 0) {
//        outfile.open(outfullName.c_str(), std::ios_base::app);
//        nmfAlgorithm.computeObjectiveError();
 //       outfile <<  sqrt(this->objective_err / nmfAlgorithm.globalsqnorma()) <<"\n";
//        outfile.close();
//      }

//if(mpicomm.rank() == 0) printf("Objective error %.3lf \n", this->objective_err);
//if(mpicomm.rank() == 0) printf("Relative error %.3lf \n", this->objective_err/nmfAlgorithm.globalsqnorma());
      // For testing purposes only
      // dio.writeOutput(Wall.slice(iter), Hall.slice(iter), iter, curr_k, m_outputfile_name);

      //MPI_Barrier(MPI_COMM_WORLD);
    } // End for loop 'iter'
    /*
    if(mpicomm.rank()== 0)  {
        Wall.print("Wall in rank 0 = ");
        Hall.print("Hall in rank 0 = ");
    } 
    */
   DistClust<MAT> dc(Wall, Hall, mpicomm, curr_k);
   mpitic();
   dc.distReorder();
   double temp1 = mpitoc();
   if(mpicomm.rank() == 0) printf("DistClust took %.3lf secs.\n", temp1);
   //dc.distMedian();
#ifndef USE_PACOSS
  if (!m_outputfile_name.empty()) {
    dio.writeOutput(dc.getLeftLowRankFactor(), dc.getRightLowRankFactor(), runs, curr_k, m_outputfile_name);
    /*
    MPI_Allgather(dc.getLeftLowRankFactor().memptr(), (this->m_globalm / mpicomm.size()*curr_k), MPI_DOUBLE, Wout.memptr(), (this->m_globalm / mpicomm.size()*curr_k), MPI_DOUBLE, MPI_COMM_WORLD);
    if(mpicomm.rank() == 0)	{
    	std::stringstream sw;
    	sw << m_outputfile_name <<"W_K" << k ;
    	Wout.save(sw, arma::raw_ascii);
    	Wout.print("Wout = ");
    }
    */
  }
  // cout <<"Cube sizes "<<size(Wall)<<" "<<size(Hall)<<" "<<size(dro.getLeftLowRankFactor())<<" "<<size(dro.getRightLowRankFactor())<<endl;
#endif  // ifndef USE_PACOSS
   mpitic();
   dc.distClustStability(); 
   double temp2 = mpitoc();
   if(mpicomm.rank() == 0) printf("DistClustStability took %.3lf secs.\n", temp2);

   if(!m_outputfile_sils.empty()) {
      dio.writeSilhouettes(dc.getSilhouettes(), curr_k, m_outputfile_sils);
    }

   //Clear the W/Hall data after all the computations at that 'k'
   Wall.clear();
   Hall.clear();
  } //End of for loop 'k (rank)'
 }

void parseCommandLine() {
    ParseCommandLine pc(this->m_argc, this->m_argv);
    //pc.parseplancopts();
    pc.parseNMFkOpts();
    this->m_nmfalgo = pc.lucalgo();
    this->m_k = pc.lowrankk();
    this->m_Afile_name = pc.input_file_name();
    this->m_pr = pc.pr();
    this->m_pc = pc.pc();
    this->m_sparsity = pc.sparsity();
    this->m_num_it = pc.iterations();
    this->m_distio = TWOD;
    this->m_regW = pc.regW();
    this->m_regH = pc.regH();
    this->m_num_k_blocks = 1;
    this->m_globalm = pc.globalm();
    this->m_globaln = pc.globaln();
    this->m_compute_error = pc.compute_error();
    this->m_outputfile_name = pc.output_file_name();
    this->upper_k = pc.upper_limit_k();
    this->num_perturbs = pc.perturbs();
    this->m_outputfile_sils = pc.output_silhouettes();
   // this->objective_err = pc.objective_error();
    if (this->m_nmfalgo == NAIVEANLSBPP) {
      this->m_distio = ONED_DOUBLE;
    } else {
      this->m_distio = TWOD;
    }
    this->m_input_normalization = pc.input_normalization();
    //pc.printConfig();
    pc.printNMFkConfig();
    switch (this->m_nmfalgo) {
      case MU:
#ifdef BUILD_SPARSE
        nmfK2D<DistMU<SP_MAT> >();
#else   // ifdef BUILD_SPARSE
        nmfK2D<DistMU<MAT> >();
#endif  // ifdef BUILD_SPARSE
        break;
      case HALS:
#ifdef BUILD_SPARSE
        nmfK2D<DistHALS<SP_MAT> >();
#else   // ifdef BUILD_SPARSE
        nmfK2D<DistHALS<MAT> >();
#endif  // ifdef BUILD_SPARSE
        break;
      case ANLSBPP:
#ifdef BUILD_SPARSE
        nmfK2D<DistANLSBPP<SP_MAT> >();
#else   // ifdef BUILD_SPARSE
        nmfK2D<DistANLSBPP<MAT> >();
#endif  // ifdef BUILD_SPARSE
        break;
      case NAIVEANLSBPP:
#ifdef BUILD_SPARSE
        nmfk1D<DistNaiveANLSBPP<SP_MAT> >();
#else   // ifdef BUILD_SPARSE
        nmfk1D<DistNaiveANLSBPP<MAT> >();
#endif  // ifdef BUILD_SPARSE
        break;
      case AOADMM:
#ifdef BUILD_SPARSE
        nmfK2D<DistAOADMM<SP_MAT> >();
#else   // ifdef BUILD_SPARSE
        nmfK2D<DistAOADMM<MAT> >();
#endif  // ifdef BUILD_SPARSE
      case CPALS:
#ifdef BUILD_SPARSE
        nmfK2D<DistALS<SP_MAT> >();
#else   // ifdef BUILD_SPARSE
        nmfK2D<DistALS<MAT> >();
#endif  // ifdef BUILD_SPARSE
//      case BCD:
//#ifdef BUILD_SPARSE
//        nmfK2D<DistBCD<SP_MAT> >();
//#else   // ifdef BUILD_SPARSE
//        nmfK2D<DistBCD<MAT> >();
//#endif  // ifdef BUILD_SPARSE
    }
  }

 public:
  DistNMFk(int argc, char *argv[]) {
    this->m_argc = argc;
    this->m_argv = argv;
    this->parseCommandLine();
  }
};

}  // namespace planc

int main(int argc, char *argv[]) {
  try {
    planc::DistNMFk dNMFk(argc, argv);
    fflush(stdout);
  } catch (const std::exception &e) {
    INFO << "Exception with stack trace " << std::endl;
    INFO << e.what();
  }
}
