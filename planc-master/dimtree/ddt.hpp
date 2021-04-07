/* Copyright Koby Hayashi 2018 */

#ifndef DIMTREE_DDT_HPP_
#define DIMTREE_DDT_HPP_

#include "common/ncpfactors.hpp"
#include "common/tensor.hpp"
#include "dimtree/ddttensor.hpp"
#include "dimtree/dimtrees.hpp"

class DenseDimensionTree {
  ktensor *m_local_Y;
  tensor *m_local_T;
  tensor *projection_Tensor;
  tensor *buffer_Tensor;
  ktensor *projection_Ktensor;
  double *col_MTTKRP;
  long int num_threads;
  long int s;
  long int ldp;
  long int rdp;

 public:
  DenseDimensionTree(const planc::Tensor &i_input_tensor,
                     const planc::NCPFactors &i_ncp_factors,
                     long int split_mode) {
    m_local_T = reinterpret_cast<tensor *>(malloc(sizeof *m_local_T));
    m_local_Y = reinterpret_cast<ktensor *>(malloc(sizeof *m_local_Y));
    projection_Tensor =
        reinterpret_cast<tensor *>(malloc(sizeof *projection_Tensor));
    buffer_Tensor = reinterpret_cast<tensor *>(malloc(sizeof *buffer_Tensor));
    projection_Ktensor =
        reinterpret_cast<ktensor *>(malloc(sizeof *projection_Ktensor));
    m_local_T->data = const_cast<double *>(&i_input_tensor.m_data[0]);
    m_local_T->nmodes = i_input_tensor.modes();
    m_local_T->dims_product = i_input_tensor.numel();
    m_local_Y->factors = reinterpret_cast<double **>(
        malloc(sizeof(double *) * m_local_T->nmodes));
    m_local_Y->dims = (long int *)malloc(sizeof(long int) * m_local_T->nmodes);
    m_local_T->dims = (long int *)malloc(sizeof(long int) * m_local_T->nmodes);
    m_local_Y->nmodes = i_ncp_factors.modes();
    m_local_Y->rank = i_ncp_factors.rank();
    VEC temp_vec = i_ncp_factors.lambda();
    m_local_Y->lambdas = temp_vec.memptr();
    m_local_Y->dims_product = arma::prod(i_ncp_factors.dimensions());

    for (long int i = 0; i < m_local_T->nmodes; i++) {
      m_local_T->dims[i] = i_input_tensor.dimensions()[i];
      m_local_Y->dims[i] = i_input_tensor.dimensions()[i];
      m_local_Y->factors[i] = reinterpret_cast<double *>(
          malloc(sizeof(double) * i_ncp_factors.rank() * m_local_T->dims[i]));
    }
    num_threads = 16;
    s = split_mode;
    // Allocate memory for the larger of two partial MTTKRP
    set_left_right_product(s);
    if (ldp <= rdp) {
      projection_Tensor->data = reinterpret_cast<double *>(
          malloc(sizeof(double) * rdp * m_local_Y->rank));
      buffer_Tensor->data = reinterpret_cast<double *>(
          malloc(sizeof(double) * m_local_Y->rank * rdp));
    } else {
      projection_Tensor->data = reinterpret_cast<double *>(
          malloc(sizeof(double) * ldp * m_local_Y->rank));
      buffer_Tensor->data = reinterpret_cast<double *>(
          malloc(sizeof(double) * m_local_Y->rank * ldp));
    }

    projection_Tensor->nmodes = m_local_T->nmodes;
    buffer_Tensor->nmodes = m_local_T->nmodes;

    projection_Tensor->dims =
        (long int *)malloc(sizeof(long int) * m_local_T->nmodes);
    buffer_Tensor->dims =
        (long int *)malloc(sizeof(long int) * m_local_T->nmodes);
    for (long int i = 0; i < m_local_T->nmodes; i++) {
      projection_Tensor->dims[i] = m_local_T->dims[i];
      buffer_Tensor->dims[i] = m_local_T->dims[i];
    }

    projection_Tensor->dims_product = m_local_T->dims_product;
    buffer_Tensor->dims_product = m_local_T->dims_product;
    ktensor_copy_constructor(m_local_Y, projection_Ktensor);
    for (long int i = 0; i < i_ncp_factors.modes(); i++) {
      MAT temp = i_ncp_factors.factor(i).t();
      std::memcpy(m_local_Y->factors[i], temp.memptr(),
                  sizeof(double) * temp.n_rows * temp.n_cols);
    }
    // col_MTTKRP = (double*)malloc(sizeof(double) * m_local_Y->rank *
    // max_mode);
  }

  void set_factor(const double *arma_factor_ptr, const long int mode) {
    // TransposeM(arma_factor_ptr, m_local_Y->factors[mode],
    // m_local_Y->dims[mode], m_local_Y->rank);
    std::memcpy(m_local_Y->factors[mode], arma_factor_ptr,
                sizeof(double) * m_local_Y->dims[mode] * m_local_Y->rank);
  }

  ~DenseDimensionTree() {
    for (long int i = 0; i < m_local_T->nmodes; i++) {
      free(m_local_Y->factors[i]);
    }
    free(m_local_Y->factors);
    free(m_local_Y->dims);
    free(m_local_T->dims);
    destruct_Tensor(projection_Tensor);
    destruct_Tensor(buffer_Tensor);
    destruct_Ktensor(projection_Ktensor, 0);
    free(m_local_Y);
    free(m_local_T);
    free(projection_Ktensor);
    free(buffer_Tensor);
    free(projection_Tensor);
    // free(col_MTTKRP);
  }

  void set_left_right_product(const long int i_split_mode) {
    ldp = 1;
    rdp = 1;
    for (long int i = 0; i < m_local_T->nmodes; i++) {
      if (i <= i_split_mode) {
        ldp *= m_local_T->dims[i];
      } else {
        rdp *= m_local_T->dims[i];
      }
    }
  }

  /*
   * Return the col major ordered mttkrp
   */

  void in_order_reuse_MTTKRP(long int n, double *out, bool colmajor,
                             double &multittv_time, double &mttkrp_time) {
    // ktensor *Y = m_local_Y;
    direction D;
    multittv_time = 0;
    mttkrp_time = 0;

    if (n == 0) {
      /**
          Updating the first factor matrix, do a partial MTTKRP
      */
      D = ::direction::right;  //   Contracting over the right modes
      LR_Ktensor_Reordering_existingY(
          m_local_Y, projection_Ktensor, s + 1,
          opposite_direction(D));  // s+1 because s is included in the left

      if (projection_Ktensor->nmodes == 1) {  // factor matrix output PM
        /**
            PM is a factor matrix update
            D) right, tells the function that 0 is being updated
            m_local_Y) the original ktensor
            T) the original data tensor
            num_threads)
        */
        tic();
        partial_MTTKRP_with_KRP_output_FM(D, m_local_Y, m_local_T, num_threads);
        mttkrp_time += toc();

      } else {  // tensor output PM
        /**
            PM outputs an intermediate tensor
            T) the original tensor, PM always takes the original tensor
            m_local_Y) the original ktensor,
            D)
            s) overall split point
            projection_Tensor)
            num_threads)
        */
        tic();
        partial_MTTKRP_with_KRP_output_T(s, D, m_local_Y, m_local_T,
                                         projection_Tensor, num_threads);
        mttkrp_time += toc();
        tic();
        multi_TTV_with_KRP_output_FM(D, projection_Tensor, projection_Ktensor,
                                     num_threads);
        multittv_time += toc();
      }
    } else if (n == s + 1) {
      /**
          Updating the first left side, do a partial MTTKRP
      */
      D = ::direction::left;  //    Contracting over the left modes
      LR_Ktensor_Reordering_existingY(
          m_local_Y, projection_Ktensor, s,
          opposite_direction(D));  // s because s is not included on the right
      if (projection_Ktensor->nmodes == 1) {  // factor matrix output PM
        tic();
        partial_MTTKRP_with_KRP_output_FM(D, m_local_Y, m_local_T, num_threads);
        mttkrp_time += toc();
      } else {
        tic();
        partial_MTTKRP_with_KRP_output_T(s, D, m_local_Y, m_local_T,
                                         projection_Tensor, num_threads);
        mttkrp_time += toc();

        // op(D) because multi ttvs in this tree structure are always right
        tic();
        multi_TTV_with_KRP_output_FM(opposite_direction(D), projection_Tensor,
                                     projection_Ktensor, num_threads);
        multittv_time += toc();
      }
    } else {
      D = ::direction::left;
      if (projection_Ktensor->nmodes == 2) {
        // A single left contraction to output a factor martix
        tic();
        multi_TTV_with_KRP_output_FM(D, projection_Tensor, projection_Ktensor,
                                     num_threads);
        multittv_time += toc();
      } else {
        /**
            1) A single left contraction to updage the projection_tensor
            2) A right contraction to get the desired MTTKRP
        */
        tic();
        multi_TTV_with_KRP_output_T(0, D, projection_Tensor, projection_Ktensor,
                                    buffer_Tensor, num_threads);
        multittv_time += toc();

        LR_tensor_Reduction(buffer_Tensor, projection_Tensor,
                            buffer_Tensor->nmodes, D);
        tensor_data_swap(projection_Tensor, buffer_Tensor);

        remove_mode_Ktensor(projection_Ktensor,
                            0);  // remove the 0th factor matrix and mode from
                                 // the projection_Ktensor
        tic();
        multi_TTV_with_KRP_output_FM(::direction::right, projection_Tensor,
                                     projection_Ktensor, num_threads);
        multittv_time += toc();
      }
    }
    if (colmajor) {
      TransposeM(m_local_Y->factors[n], out, m_local_Y->rank,
                 m_local_Y->dims[n]);
    } else {
      std::memcpy(out, m_local_Y->factors[n],
                  sizeof(double) * m_local_Y->rank * m_local_Y->dims[n]);
    }
  }
};

#endif  // DIMTREE_DDT_HPP_
