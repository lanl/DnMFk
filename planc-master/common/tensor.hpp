/* Copyright 2017 Ramakrishnan Kannan */

#ifndef COMMON_TENSOR_HPP_
#define COMMON_TENSOR_HPP_

#include <cblas.h>
#include <armadillo>
#include <fstream>
#include <ios>
#include <random>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include "common/utils.h"

namespace planc {
/**
 * Data is stored such that the unfolding \f$Y_0\f$ is column
 * major.  This means the flattening \f$Y_{N-1}\f$ is row-major,
 * and any other flattening \f$Y_n\f$ can be represented as a set
 * of \f$\prod\limits_{k=n+1}^{N-1}I_k\f$ row major matrices, each
 * of which is \f$I_n \times \prod\limits_{k=0}^{n-1}I_k\f$.
 */

// sgemm (TRANSA, TRANSB, M, N, K, ALPHA, A, LDA, B, LDB, BETA, C, LDC)
// extern "C" void dgemm_(const char*, const char*, const int*,
//                      const int*, const int*, const double*, const double*,
//                      const int*, const double*, const int*, const double*,
//                      double*, const int*);

class Tensor {
 private:
  int m_modes;
  UVEC m_dimensions;
  UWORD m_numel;
  UVEC m_global_idx;
  unsigned int rand_seed;

  // for the time being it is used for debugging purposes
  size_t sub2ind(UVEC sub, UVEC dimensions) {
    assert(sub.n_rows == dimensions.n_rows);
    UVEC cumprod_dims = arma::cumprod(dimensions);
    UVEC cumprod_dims_shifted = arma::shift(cumprod_dims, 1);
    cumprod_dims_shifted(0) = 1;
    size_t idx = arma::dot(cumprod_dims_shifted, sub);
    return idx;
  }
  UVEC ind2sub(UVEC dimensions, size_t idx) {
    //   k = [1 cumprod(siz(1 : end - 1))];
    //   n = length(siz);
    //   idx = idx - 1;
    // for i = n : -1 : 1
    //   div = floor(idx / k(i));
    //   subs( :, i) = div + 1;
    //   idx = idx - k(i) * div;
    // end
    UVEC cumprod_dims = arma::cumprod(dimensions);
    UVEC cumprod_dims_shifted = arma::shift(cumprod_dims, 1);
    cumprod_dims_shifted(0) = 1;
    int modes = dimensions.n_elem;
    UVEC sub = arma::zeros<UVEC>(modes);
    float temp;
    for (int i = modes - 1; i >= 0; i--) {
      temp = std::floor((idx * 1.0) / (cumprod_dims_shifted(i) * 1.0));
      sub(i) = temp;
      idx = idx - cumprod_dims_shifted(i) * temp;
    }
    return sub;
  }

 public:
  std::vector<double> m_data;

  Tensor() {
    this->m_modes = 0;
    this->m_numel = 0;
  }
  /**
   * Constructor that takes only dimensions of every mode as a vector
   */
  explicit Tensor(const UVEC &i_dimensions)
      : m_modes(i_dimensions.n_rows),
        m_dimensions(i_dimensions),
        m_numel(arma::prod(i_dimensions)),
        rand_seed(103) {
    m_data.resize(m_numel);
    randu();
  }
  Tensor(const UVEC &i_dimensions, const UVEC &i_start_idx)
      : m_modes(i_dimensions.n_rows),
        m_dimensions(i_dimensions),
        m_numel(arma::prod(i_dimensions)),
        m_global_idx(i_start_idx),
        rand_seed(103) {
    m_data.resize(m_numel);
    randu();
  }
  /**
   * Need when copying from matrix to Tensor. otherwise copy constructor
   * will be called. The data will be passed in row major order
   *
   */
  Tensor(const UVEC &i_dimensions, double *i_data)
      : m_modes(i_dimensions.n_rows),
        m_dimensions(i_dimensions),
        m_numel(arma::prod(i_dimensions)),
        rand_seed(103) {
    m_data.resize(m_numel);
    memcpy(&this->m_data[0], i_data, sizeof(double) * this->m_numel);
  }
  ~Tensor() {}

  /**
   *  copy constructor
   */
  Tensor(const Tensor &src) {
    clear();
    this->m_numel = src.numel();
    this->m_modes = src.modes();
    this->m_dimensions = src.dimensions();
    this->m_global_idx = src.global_idx();
    this->rand_seed = 103;
    this->m_data = src.m_data;
  }

  Tensor &operator=(const Tensor &other) {  // copy assignment
    if (this != &other) {                   // self-assignment check expected
      clear();
      // this->m_data = new double[other.numel()];  // create storage in this
      this->m_numel = other.numel();
      this->m_modes = other.modes();
      this->m_dimensions = other.dimensions();
      this->m_global_idx = other.global_idx();
      this->m_data = other.m_data;
    }
    return *this;
  }

  void swap(Tensor &in) {
    using std::swap;
    swap(m_numel, in.m_numel);
    swap(m_modes, in.m_modes);
    swap(m_dimensions, in.m_dimensions);
    swap(m_global_idx, in.m_global_idx);
    swap(rand_seed, in.rand_seed);
    swap(m_data, in.m_data);
  }
  /**
   * Clears the data and also destroys the storage
   */

  void clear() {
    this->m_numel = 0;
    this->m_data.clear();
  }

  /// Return the number of modes. It is a scalar value.
  int modes() const { return m_modes; }
  /// Returns a vector of dimensions on every mode
  UVEC dimensions() const { return m_dimensions; }
  UVEC global_idx() const { return m_global_idx; }

  /**
   * Returns the dimension of the input mode.
   * @param[in] a mode of the tensor.
   * @return dimension of ith mode
   */

  int dimension(int i) const { return m_dimensions[i]; }
  /// Returns total number of elements
  UWORD numel() const { return m_numel; }

  void set_idx(const UVEC &i_start_idx) { m_global_idx = i_start_idx; }
  /**
   * Return the product of dimensions except mode i
   * @param[in] mode i
   * @return product of dimensions except mode i
   */
  UWORD dimensions_leave_out_one(int i) const {
    UWORD rc = arma::prod(this->m_dimensions);
    return rc - this->m_dimensions(i);
  }
  /// Zeros out the entire tensor
  void zeros() {
    for (UWORD i = 0; i < this->m_numel; i++) {
      this->m_data[i] = 0;
    }
  }
  /// set the tensor with uniform random.
  void rand() {
#pragma omp parallel for
    for (UWORD i = 0; i < this->m_numel; i++) {
      unsigned int *temp = const_cast<unsigned int *>(&rand_seed);
      this->m_data[i] =
          static_cast<double>(rand_r(temp)) / static_cast<double>(RAND_MAX);
    }
  }
  /// set the tensor with uniform int values
  void randi() {
    std::random_device rd;
    std::mt19937 gen(rd());
    int max_randi = this->m_numel;
    std::uniform_int_distribution<> dis(0, max_randi);
#pragma omp parallel for
    for (UWORD i = 0; i < this->m_numel; i++) {
      this->m_data[i] = dis(gen);
    }
  }
  /**
   * set the tensor with uniform random values starting with a seed.
   * can be used to generate the same tensor again and again.
   * @param[in] an integer seed value preferrably a relative prime number
   */
  void randu(const int i_seed = -1) {
    std::random_device rd;
    std::uniform_real_distribution<> dis(0, 1);
    if (i_seed == -1) {
      std::mt19937 gen(rand_seed);
#pragma omp parallel for
      for (unsigned int i = 0; i < this->m_numel; i++) {
        m_data[i] = dis(gen);
      }
    } else {
      std::mt19937 gen(i_seed);
#pragma omp parallel for
      for (unsigned int i = 0; i < this->m_numel; i++) {
        m_data[i] = dis(gen);
      }
    }
  }

  /**
   * size of krp must be product of all dimensions leaving out nxk.
   * o_mttkrp will be of size dimension[n]xk.
   * Memory must be allocated and freed by the caller
   * @param[in]  i_n mode number
   * @param[in]  i_krp Khatri-rao product matrix leaving out mode i_n
   * @param[out] o_mttkrp pointer to the mttkrp matrix.
   */

  void mttkrp(const int i_n, const MAT &i_krp, MAT *o_mttkrp) const {
    (*o_mttkrp).zeros();
    if (i_n == 0) {
      // if n == 1
      // Ur = khatrirao(U{2: N}, 'r');
      // Y = reshape(X.data, szn, szr);
      // V =  Y * Ur;
      // Compute number of columns of Y_n
      // Technically, we could divide the total number of entries by n,
      // but that seems like a bad decision
      // size_t ncols = arma::prod(this->m_dimensions);
      // ncols /= this->m_dimensions[0];
      // Call matrix matrix multiply
      // call dgemm (TRANSA, TRANSB, M, N, K, ALPHA, A, LDA, B, LDB, BETA, C,
      // LDC) C := alpha*op( A )*op( B ) + beta*C A, B and C are matrices, with
      // op( A ) an m by k matrix, op( B ) a k by n matrix and C an m by n
      // matrix. matricized tensor is m x k is in column major format krp is k x
      // n is in column major format output is m x n in column major format
      // char transa = 'N';
      // char transb = 'N';
      int m = this->m_dimensions[0];
      int n = i_krp.n_cols;
      int k = i_krp.n_rows;
      // int lda = m;
      // int ldb = k;
      // int ldc = m;
      double alpha = 1;
      double beta = 0;
      // sgemm (TRANSA, TRANSB, M, N, K, ALPHA, A, LDA, B, LDB, BETA, C, LDC)
      // dgemm_(&transa, &transb, &m, &n, &k, &alpha, this->m_data,
      // &lda, i_krp.memptr(), &ldb, &beta, o_mttkrp->memptr() , &ldc);
      // printf("mode=%d,i=%d,m=%d,n=%d,k=%d,alpha=%lf,T_stride=%d,lda=%d,krp_stride=%d,ldb=%d,beat=%lf,mttkrp=!!!,ldc=%d\n",0,
      // 0, m, n, k,alpha,0*k*m,m,0*n*k,i_krp.n_rows,beta,n);
      cblas_dgemm(CblasRowMajor, CblasTrans, CblasTrans, m, n, k, alpha,
                  &this->m_data[0], m, i_krp.memptr(), k, beta,
                  o_mttkrp->memptr(), n);

    } else {
      int ncols = 1;
      int nmats = 1;
      int lowrankk = i_krp.n_cols;

      // Count the number of columns
      for (int i = 0; i < i_n; i++) {
        ncols *= this->m_dimensions[i];
      }

      // Count the number of matrices
      for (int i = i_n + 1; i < this->m_modes; i++) {
        nmats *= this->m_dimensions[i];
      }
      // For each matrix...
      for (int i = 0; i < nmats; i++) {
        // char transa = 'T';
        // char transb = 'N';
        int m = this->m_dimensions[i_n];
        int n = lowrankk;
        int k = ncols;
        // int lda = k;  // not sure. could be m. higher confidence on k.
        // int ldb = i_krp.n_rows;
        // int ldc = m;
        double alpha = 1;
        double beta = (i == 0) ? 0 : 1;
        // double *A = this->m_data + i * k * m;
        // double *B = const_cast<double *>(i_krp.memptr()) + i * k;

        // for KRP move ncols*lowrankk
        // for tensor X move as n_cols*blas_n
        // For output matrix don't move anything as beta=1;
        // for reference from gram while moving input tensor like
        // Y->data()+i*nrows*ncols sgemm (TRANSA, TRANSB, M, N, K, ALPHA, A,
        // LDA, B, LDB, BETA, C, LDC)
        // dgemm_(&transa, &transb, &m, &n, &k, &alpha, A,
        // &lda, B , &ldb, &beta, (*o_mttkrp).memptr() , &ldc);
        // printf("mode=%d,i=%d,m=%d,n=%d,k=%d,alpha=%lf,T_stride=%d,lda=%d,krp_stride=%d,ldb=%d,beat=%lf,mttkrp=!!!,ldc=%d\n",i_n,
        // i, m, n, k,alpha,i*k*m,k,i*n*k,nmats*ncols,beta,n);
        cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasTrans, m, n, k, alpha,
                    &this->m_data[0] + i * k * m, ncols, i_krp.memptr() + i * k,
                    ncols * nmats, beta, o_mttkrp->memptr(), n);
      }
    }
  }
  /// prints the value of the tensor.
  void print() const {
    INFO << "Dimensions: " << this->m_dimensions;
    for (unsigned int i = 0; i < this->m_numel; i++) {
      std::cout << i << " : " << this->m_data[i] << std::endl;
    }
  }

  void print(const UVEC &global_dims, const UVEC &global_start_sub) {
    UVEC local_sub = arma::zeros<UVEC>(global_dims.n_elem);
    // UVEC global_start_sub = ind2sub(global_dims, global_start_idx);
    UVEC global_sub = arma::zeros<UVEC>(global_dims.n_elem);
    int global_idx;
    for (unsigned int i = 0; i < this->m_numel; i++) {
      local_sub = ind2sub(this->m_dimensions, i);
      global_sub = global_start_sub + local_sub;
      global_idx = sub2ind(global_sub, global_dims);
      std::cout << i << " : " << global_idx << " : " << this->m_data[i]
                << std::endl;
    }
  }
  /// returns the frobenius norm of the tensor
  double norm() const {
    double norm_fro = 0;
    for (unsigned int i = 0; i < this->m_numel; i++) {
      norm_fro += (this->m_data[i] * this->m_data[i]);
    }
    return norm_fro;
  }
  /**
   * Computes the squared error with the input tensor
   * @param[in] b an input tensor
   */
  double err(const Tensor &b) const {
    double norm_fro = 0;
    double err_diff;
    for (unsigned int i = 0; i < this->m_numel; i++) {
      err_diff = this->m_data[i] - b.m_data[i];
      norm_fro += err_diff * err_diff;
    }
    return norm_fro;
  }
  /**
   * Scales the tensor with the constant value.
   * @param[in] scale can be an int, float or double value
   */

  template <typename NumericType>
  void scale(NumericType scale) {
    // static_assert(std::is_arithmetic<NumericType>::value,
    //               "NumericType for scale operation must be numeric");
    for (unsigned int i = 0; i < this->m_numel; i++) {
      this->m_data[i] = this->m_data[i] * scale;
    }
  }
  /**
   * Shifts (add or subtract) the tensor with the constant value.
   * If the i_shift is negative it subtracts, otherwise it adds.
   * @param[in] i_shift can be an int, float or double value
   */
  template <typename NumericType>
  void shift(NumericType i_shift) {
    // static_assert(std::is_arithmetic<NumericType>::value,
    //               "NumericType for shift operation must be numeric");
    for (unsigned int i = 0; i < this->m_numel; i++) {
      this->m_data[i] = this->m_data[i] + i_shift;
    }
  }
  /**
   * Truncate all the value between min and max. Any value beyond
   * the min and max will be truncated to min and max.
   * @param[in] min - any value less than min will be set to min
   * @param[in] max - any value greater than max will be set to max
   */
  template <typename NumericType>
  void bound(NumericType min, NumericType max) {
    // static_assert(std::is_arithmetic<NumericType>::value,
    //               "NumericType for bound operation must be numeric");
    for (unsigned int i = 0; i < this->m_numel; i++) {
      if (this->m_data[i] < min) this->m_data[i] = min;
      if (this->m_data[i] > max) this->m_data[i] = max;
    }
  }
  /**
   * Sets only the lower bound
   * @param[in] min - any value less than min will be set to min
   */

  template <typename NumericType>
  void lower_bound(NumericType min) {
    // static_assert(std::is_arithmetic<NumericType>::value,
    //               "NumericType for bound operation must be numeric");
    for (unsigned int i = 0; i < this->m_numel; i++) {
      if (this->m_data[i] < min) this->m_data[i] = min;
    }
  }

  /**
   * Write the tensor to the given filename. Compile with
   * -D_FILE_OFFSET_BITS=64 to support large files greater than 2GB
   * @param[in] filename as std::string
   */
  void write(std::string filename,
             std::ios_base::openmode mode = std::ios_base::out) {
    std::string filename_no_extension =
        filename.substr(0, filename.find_last_of("."));
    // info file always in text mode
    filename_no_extension.append(".info");
    std::ofstream ofs;
    ofs.open(filename_no_extension, std::ios_base::out);
    // write modes
    ofs << this->m_modes << std::endl;
    // dimension of modes
    for (int i = 0; i < this->m_modes; i++) {
      ofs << this->m_dimensions[i] << std::endl;
    }
    ofs << std::endl;
    ofs.close();
    FILE *fp = fopen(filename.c_str(), "wb");
    INFO << "size of the outputfile in GB "
         << (this->m_numel * 8.0) / (1024 * 1024 * 1024) << std::endl;
    size_t nwrite =
        fwrite(&this->m_data[0], sizeof(std::vector<double>::value_type),
               this->numel(), fp);
    if (nwrite != this->numel()) {
      WARN << "something wrong ::write::" << nwrite
           << "::numel::" << this->numel() << std::endl;
    }
    fclose(fp);
  }
  /**
   * Reads a tensor from the file. Compile with
   * -D_FILE_OFFSET_BITS=64 to support large files greater than 2GB
   * @param[in] filename as std::string
   */

  void read(std::string filename,
            std::ios_base::openmode mode = std::ios_base::in) {
    // clear existing tensor
    if (this->m_numel > 0) {
      this->m_data.clear();  // destroy storage in this
      this->m_numel = 0;
    }
    std::string filename_no_extension =
        filename.substr(0, filename.find_last_of("."));
    filename_no_extension.append(".info");

    std::ifstream ifs;
    // info file always in text mode
    ifs.open(filename_no_extension, std::ios_base::in);
    // write modes
    ifs >> this->m_modes;
    // dimension of modes
    this->m_dimensions = arma::zeros<UVEC>(this->m_modes);
    for (int i = 0; i < this->m_modes; i++) {
      ifs >> this->m_dimensions[i];
    }
    ifs.close();
    // ifs.open(filename, mode);
    FILE *fp = fopen(filename.c_str(), "rb");
    this->m_numel = arma::prod(this->m_dimensions);
    this->m_data.resize(this->m_numel);
    // for (int i = 0; i < this->m_numel; i++) {
    //   ifs >> this->m_data[i];
    // }
    // ifs.read(reinterpret_cast<char *>(this->m_data), sizeof(this->m_data));
    size_t nread =
        fread(&this->m_data[0], sizeof(std::vector<double>::value_type),
              this->numel(), fp);
    if (nread != this->numel()) {
      WARN << "something wrong ::write::" << nread
           << "::numel::" << this->numel() << std::endl;
    }
    // Close the file
    fclose(fp);
  }

  /**
   * Given a vector of subscripts, it return the linear index
   * in the tensor.
   * @param[in] vector of subscript
   * @return linear index within the tensor
   */

  UWORD sub2ind(UVEC sub) {
    assert(sub.n_cols == this->m_dimensions.n_cols);
    UVEC cumprod_dims = arma::cumprod(this->m_dimensions);
    UVEC cumprod_dims_shifted = arma::shift(cumprod_dims, 1);
    cumprod_dims_shifted(0) = 1;
    size_t idx = arma::dot(cumprod_dims_shifted, sub);
    return idx;
  }
  double at(UVEC sub) { return m_data[sub2ind(sub)]; }
};  // class Tensor
}  // namespace planc

void swap(planc::Tensor &x, planc::Tensor &y) { x.swap(y); }

#endif  // COMMON_TENSOR_HPP_
