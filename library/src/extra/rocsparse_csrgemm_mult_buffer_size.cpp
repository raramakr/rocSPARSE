/*! \file */
/* ************************************************************************
 * Copyright (C) 2023 Advanced Micro Devices, Inc. All rights Reserved.
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

#include "../conversion/rocsparse_identity.hpp"
#include "csrgemm_device.h"
#include "definitions.h"
#include "internal/extra/rocsparse_csrgemm.h"
#include "rocsparse_csrgemm.hpp"
#include "utility.h"

#include "rocsparse_csrgemm_mult.hpp"
#include <rocprim/rocprim.hpp>

#define INSTANTIATE(I, J, T)                                           \
    template rocsparse_status rocsparse_csrgemm_mult_buffer_size_core( \
        rocsparse_handle          handle,                              \
        rocsparse_operation       trans_A,                             \
        rocsparse_operation       trans_B,                             \
        J                         m,                                   \
        J                         n,                                   \
        J                         k,                                   \
        const T*                  alpha,                               \
        const rocsparse_mat_descr descr_A,                             \
        I                         nnz_A,                               \
        const I*                  csr_row_ptr_A,                       \
        const J*                  csr_col_ind_A,                       \
        const rocsparse_mat_descr descr_B,                             \
        I                         nnz_B,                               \
        const I*                  csr_row_ptr_B,                       \
        const J*                  csr_col_ind_B,                       \
        rocsparse_mat_info        info_C,                              \
        size_t*                   buffer_size)

INSTANTIATE(int32_t, int32_t, float);
INSTANTIATE(int32_t, int32_t, double);
INSTANTIATE(int32_t, int32_t, rocsparse_float_complex);
INSTANTIATE(int32_t, int32_t, rocsparse_double_complex);

INSTANTIATE(int64_t, int32_t, float);
INSTANTIATE(int64_t, int32_t, double);
INSTANTIATE(int64_t, int32_t, rocsparse_float_complex);
INSTANTIATE(int64_t, int32_t, rocsparse_double_complex);

INSTANTIATE(int32_t, int64_t, float);
INSTANTIATE(int32_t, int64_t, double);
INSTANTIATE(int32_t, int64_t, rocsparse_float_complex);
INSTANTIATE(int32_t, int64_t, rocsparse_double_complex);

INSTANTIATE(int64_t, int64_t, float);
INSTANTIATE(int64_t, int64_t, double);
INSTANTIATE(int64_t, int64_t, rocsparse_float_complex);
INSTANTIATE(int64_t, int64_t, rocsparse_double_complex);

#undef INSTANTIATE

template <typename I, typename J, typename T>
rocsparse_status rocsparse_csrgemm_mult_buffer_size_core(rocsparse_handle          handle,
                                                         rocsparse_operation       trans_A,
                                                         rocsparse_operation       trans_B,
                                                         J                         m,
                                                         J                         n,
                                                         J                         k,
                                                         const T*                  alpha,
                                                         const rocsparse_mat_descr descr_A,
                                                         I                         nnz_A,
                                                         const I*                  csr_row_ptr_A,
                                                         const J*                  csr_col_ind_A,
                                                         const rocsparse_mat_descr descr_B,
                                                         I                         nnz_B,
                                                         const I*                  csr_row_ptr_B,
                                                         const J*                  csr_col_ind_B,
                                                         rocsparse_mat_info        info_C,
                                                         size_t*                   buffer_size)
{
    // Stream
    hipStream_t stream = handle->stream;

    // rocprim buffer
    size_t rocprim_size;
    size_t rocprim_max = 0;

    // rocprim::reduce
    RETURN_IF_HIP_ERROR(rocprim::reduce(
        nullptr, rocprim_size, csr_row_ptr_A, &nnz_A, 0, m, rocprim::maximum<I>(), stream));
    rocprim_max = std::max(rocprim_max, rocprim_size);

    // rocprim exclusive scan
    RETURN_IF_HIP_ERROR(rocprim::exclusive_scan(
        nullptr, rocprim_size, csr_row_ptr_A, &nnz_A, 0, m + 1, rocprim::plus<I>(), stream));
    rocprim_max = std::max(rocprim_max, rocprim_size);

    // rocprim::radix_sort_pairs
    rocprim::double_buffer<I> buf1(&nnz_A, &nnz_B);
    rocprim::double_buffer<J> buf2(&n, &k);
    RETURN_IF_HIP_ERROR(
        rocprim::radix_sort_pairs(nullptr, rocprim_size, buf1, buf2, m, 0, 3, stream));
    rocprim_max = std::max(rocprim_max, rocprim_size);

    *buffer_size = ((rocprim_max - 1) / 256 + 1) * 256;

    // Group arrays
    *buffer_size += sizeof(J) * 256 * CSRGEMM_MAXGROUPS;
    *buffer_size += sizeof(J) * 256;
    *buffer_size += ((sizeof(J) * m - 1) / 256 + 1) * 256;

    // Permutation arrays
    *buffer_size += ((sizeof(J) * m - 1) / 256 + 1) * 256;
    *buffer_size += ((sizeof(J) * m - 1) / 256 + 1) * 256;
    *buffer_size += ((sizeof(I) * m - 1) / 256 + 1) * 256;

    info_C->csrgemm_info->buffer_size    = buffer_size[0];
    info_C->csrgemm_info->is_initialized = true;
    return rocsparse_status_success;
}

rocsparse_status rocsparse_csrgemm_mult_buffer_size_quickreturn(rocsparse_handle          handle,
                                                                rocsparse_operation       trans_A,
                                                                rocsparse_operation       trans_B,
                                                                int64_t                   m,
                                                                int64_t                   n,
                                                                int64_t                   k,
                                                                const void*               alpha,
                                                                const rocsparse_mat_descr descr_A,
                                                                int64_t                   nnz_A,
                                                                const void* csr_row_ptr_A,
                                                                const void* csr_col_ind_A,
                                                                const rocsparse_mat_descr descr_B,
                                                                int64_t                   nnz_B,
                                                                const void*        csr_row_ptr_B,
                                                                const void*        csr_col_ind_B,
                                                                rocsparse_mat_info info_C,
                                                                size_t*            buffer_size)
{
    if(m == 0 || n == 0 || k == 0 || nnz_A == 0 || nnz_B == 0)
    {
        *buffer_size                         = 0;
        info_C->csrgemm_info->buffer_size    = buffer_size[0];
        info_C->csrgemm_info->is_initialized = true;
        return rocsparse_status_success;
    }

    return rocsparse_status_continue;
}
