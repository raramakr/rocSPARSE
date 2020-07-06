/* ************************************************************************
 * Copyright (c) 2019 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * ************************************************************************ */

/*! \file
 *  \brief flops.hpp provides floating point counts of Sparse Linear Algebra Subprograms
 *  of Level 1, 2 and 3.
 */

#pragma once
#ifndef FLOPS_HPP
#define FLOPS_HPP

#include <rocsparse.h>
#include <vector>

/*
 * ===========================================================================
 *    level 1 SPARSE
 * ===========================================================================
 */
template <typename T>
constexpr double axpyi_gflop_count(rocsparse_int nnz)
{
    return (2.0 * nnz) / 1e9;
}

template <typename T>
constexpr double doti_gflop_count(rocsparse_int nnz)
{
    return (2.0 * nnz) / 1e9;
}

template <typename T>
constexpr double roti_gflop_count(rocsparse_int nnz)
{
    return (6.0 * nnz) / 1e9;
}

/*
 * ===========================================================================
 *    level 2 SPARSE
 * ===========================================================================
 */
template <typename T>
constexpr double spmv_gflop_count(rocsparse_int M, rocsparse_int nnz, bool beta = false)
{
    return (2.0 * nnz + (beta ? M : 0)) / 1e9;
}

template <typename T>
constexpr double csrsv_gflop_count(rocsparse_int M, rocsparse_int nnz, rocsparse_diag_type diag)
{
    return (2.0 * nnz + M + (diag == rocsparse_diag_type_non_unit ? M : 0)) / 1e9;
}

/*
 * ===========================================================================
 *    level 3 SPARSE
 * ===========================================================================
 */
template <typename T>
constexpr double bsrmm_gflop_count(rocsparse_int N,
                                   rocsparse_int nnzb,
                                   rocsparse_int block_dim,
                                   rocsparse_int nnz_C,
                                   bool          beta = false)
{
    return (3.0 * nnzb * block_dim * block_dim * N + (beta ? nnz_C : 0)) / 1e9;
}

template <typename T>
constexpr double
    csrmm_gflop_count(rocsparse_int N, rocsparse_int nnz_A, rocsparse_int nnz_C, bool beta = false)
{
    return (3.0 * nnz_A * N + (beta ? nnz_C : 0)) / 1e9;
}

/*
 * ===========================================================================
 *    extra SPARSE
 * ===========================================================================
 */
template <typename T>
constexpr double csrgeam_gflop_count(
    rocsparse_int nnz_A, rocsparse_int nnz_B, rocsparse_int nnz_C, const T* alpha, const T* beta)
{
    // Flop counter
    double flops = 0.0;

    if(alpha && beta)
    {
        // Count alpha * A
        flops += static_cast<double>(nnz_A);

        // Count beta * B
        flops += static_cast<double>(nnz_B);

        // Count A + B
        flops += static_cast<double>(nnz_C);
    }
    else if(!alpha)
    {
        // Count beta * B
        flops += static_cast<double>(nnz_B);
    }
    else
    {
        // Count alpha * A
        flops += static_cast<double>(nnz_A);
    }

    return flops / 1e9;
}

template <typename T>
constexpr double csrgemm_gflop_count(rocsparse_int                     M,
                                     const T*                          alpha,
                                     const std::vector<rocsparse_int>& csr_row_ptr_A,
                                     const std::vector<rocsparse_int>& csr_col_ind_A,
                                     const std::vector<rocsparse_int>& csr_row_ptr_B,
                                     const T*                          beta,
                                     const std::vector<rocsparse_int>& csr_row_ptr_D,
                                     rocsparse_index_base              baseA)
{
    // Flop counter
    double flops = 0.0;

    // Loop over rows of A
    for(rocsparse_int i = 0; i < M; ++i)
    {
        if(alpha)
        {
            rocsparse_int row_begin_A = csr_row_ptr_A[i] - baseA;
            rocsparse_int row_end_A   = csr_row_ptr_A[i + 1] - baseA;

            // Loop over columns of A
            for(rocsparse_int j = row_begin_A; j < row_end_A; ++j)
            {
                // Current column of A
                rocsparse_int col_A = csr_col_ind_A[j] - baseA;

                // Count flops generated by alpha * A * B
                flops += 2.0 * (csr_row_ptr_B[col_A + 1] - csr_row_ptr_B[col_A]) + 1.0;
            }
        }

        if(beta)
        {
            // Count flops generated by beta * D
            flops += (csr_row_ptr_D[i + 1] - csr_row_ptr_D[i]);
        }
    }

    return flops / 1e9;
}

#endif // FLOPS_HPP
