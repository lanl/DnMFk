/* Copyright 2016 Ramakrishnan Kannan */

#ifndef NMF_HALS_HPP_
#define NMF_HALS_HPP_

#include "common/nmf.hpp"

namespace planc {

template <class T>
class HALSNMF : public NMF<T> {
 private:
  // Not happy with this design. However to avoid computing At again and again
  // making this as private variable.
  T At;
  MAT WtW;
  MAT HtH;
  MAT WtA;
  MAT AH;

  /*
   * Collected statistics are
   * iteration Htime Wtime totaltime normH normW densityH densityW relError
   */
  void allocateMatrices() {
    WtW = arma::zeros<MAT>(this->k, this->k);
    HtH = arma::zeros<MAT>(this->k, this->k);
    WtA = arma::zeros<MAT>(this->n, this->k);
    AH = arma::zeros<MAT>(this->m, this->k);
  }
  void freeMatrices() {
    this->At.clear();
    WtW.clear();
    HtH.clear();
    WtA.clear();
    AH.clear();
  }

 public:
  HALSNMF(const T &A, int lowrank) : NMF<T>(A, lowrank) {
    this->normalize_by_W();
    allocateMatrices();
    this->At = this->A.t();
  }
  HALSNMF(const T &A, const MAT &llf, const MAT &rlf) : NMF<T>(A, llf, rlf) {
    this->normalize_by_W();
    allocateMatrices();
    this->At = this->A.t();
  }
  void computeNMF() {
    unsigned int currentIteration = 0;
    INFO << "computed transpose At=" << PRINTMATINFO(this->At) << std::endl;
    while (currentIteration < this->num_iterations()) {
      tic();
      // update H
      tic();
      WtA = this->W.t() * this->A;
      WtW = this->W.t() * this->W;
      this->applyReg(this->regH(), &this->WtW);
      INFO << "starting H Prereq for "
           << " took=" << toc() << PRINTMATINFO(WtW) << PRINTMATINFO(WtA)
           << std::endl;
      // to avoid divide by zero error.
      tic();
      double normConst;
      VEC Hx;
      for (unsigned int x = 0; x < this->k; x++) {
        // H(i,:) = max(H(i,:) + WtA(i,:) - WtW_reg(i,:) * H,epsilon);
        Hx = this->H.col(x) + (((WtA.row(x)).t()) - (this->H * (WtW.col(x))));
        fixNumericalError<VEC>(&Hx, EPSILON_1EMINUS16, EPSILON_1EMINUS16);
        normConst = norm(Hx);
        if (normConst != 0) {
          this->H.col(x) = Hx;
        }
      }
      INFO << "Completed H (" << currentIteration << "/"
           << this->num_iterations() << ")"
           << " time =" << toc() << std::endl;
      // update W;
      tic();
      AH = this->A * this->H;
      HtH = this->H.t() * this->H;
      this->applyReg(this->regW(), &this->HtH);
      INFO << "starting W Prereq for "
           << " took=" << toc() << PRINTMATINFO(HtH) << PRINTMATINFO(AH)
           << std::endl;
      tic();
      VEC Wx;
      for (unsigned int x = 0; x < this->k; x++) {
        // FVEC Wx = W(:,x) + (AHt(:,x)-W*HHt(:,x))/HHtDiag(x);

        // W(:,i) = max(W(:,i) * HHt_reg(i,i) + AHt(:,i) - W * HHt_reg(:,i),
        //              epsilon);
        Wx = (this->W.col(x) * HtH(x, x)) +
             (((AH.col(x))) - (this->W * (HtH.col(x))));
        fixNumericalError<VEC>(&Wx, EPSILON_1EMINUS16, EPSILON_1EMINUS16);
        normConst = norm(Wx);
        if (normConst != 0) {
          Wx = Wx / normConst;
          this->W.col(x) = Wx;
        }
      }
      this->normalize_by_W();

      INFO << "Completed W (" << currentIteration << "/"
           << this->num_iterations() << ")"
           << " time =" << toc() << std::endl;

      INFO << "Completed It (" << currentIteration << "/"
           << this->num_iterations() << ")"
           << " time =" << toc() << std::endl;
      this->computeObjectiveError();
      INFO << "Completed it = " << currentIteration
           << " HALSERR=" << sqrt(this->objective_err) / this->normA
           << std::endl;
      currentIteration++;
    }
  }
  ~HALSNMF() {}
};

}  // namespace planc

#endif  // NMF_HALS_HPP_
