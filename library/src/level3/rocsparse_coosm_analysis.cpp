/*! \file */
/* ************************************************************************
 * Copyright (C) 2021-2023 Advanced Micro Devices, Inc. All rights Reserved.
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
#include "definitions.h"
#include "rocsparse_coosm.hpp"
#include "rocsparse_csrsm.hpp"
#include "utility.h"
#include <rocprim/rocprim.hpp>

#include "../conversion/rocsparse_coo2csr.hpp"

template <typename I, typename T>
static rocsparse_status rocsparse_coosm_analysis_core(rocsparse_handle          handle,
                                                      rocsparse_operation       trans_A,
                                                      rocsparse_operation       trans_B,
                                                      I                         m,
                                                      I                         nrhs,
                                                      int64_t                   nnz,
                                                      const T*                  alpha_device_host,
                                                      const rocsparse_mat_descr descr,
                                                      const T*                  coo_val,
                                                      const I*                  coo_row_ind,
                                                      const I*                  coo_col_ind,
                                                      const T*                  B,
                                                      int64_t                   ldb,
                                                      rocsparse_mat_info        info,
                                                      rocsparse_analysis_policy analysis,
                                                      rocsparse_solve_policy    solve,
                                                      void*                     temp_buffer)
{

    // Buffer
    char* ptr = reinterpret_cast<char*>(temp_buffer);

    if(std::is_same<I, int32_t>() && nnz < std::numeric_limits<int32_t>::max())
    {
        // convert to csr
        int32_t* csr_row_ptr = reinterpret_cast<int32_t*>(ptr);
        ptr += sizeof(int32_t) * (m / 256 + 1) * 256;

        const I* csr_col_ind = coo_col_ind;
        const T* csr_val     = coo_val;

        // Create column pointers
        RETURN_IF_ROCSPARSE_ERROR(rocsparse_coo2csr_template(
            handle, coo_row_ind, (int32_t)nnz, m, csr_row_ptr, descr->base));

        // Call CSR analysis
        RETURN_IF_ROCSPARSE_ERROR(rocsparse_csrsm_analysis_template(handle,
                                                                    trans_A,
                                                                    trans_B,
                                                                    m,
                                                                    nrhs,
                                                                    (int32_t)nnz,
                                                                    alpha_device_host,
                                                                    descr,
                                                                    csr_val,
                                                                    csr_row_ptr,
                                                                    csr_col_ind,
                                                                    B,
                                                                    ldb,
                                                                    info,
                                                                    analysis,
                                                                    solve,
                                                                    ptr));
    }
    else
    {
        // convert to csr
        int64_t* csr_row_ptr = reinterpret_cast<int64_t*>(ptr);
        ptr += sizeof(int64_t) * (m / 256 + 1) * 256;

        const I* csr_col_ind = coo_col_ind;
        const T* csr_val     = coo_val;

        // Create column pointers
        RETURN_IF_ROCSPARSE_ERROR(
            rocsparse_coo2csr_template(handle, coo_row_ind, nnz, m, csr_row_ptr, descr->base));

        // Call CSR analysis
        RETURN_IF_ROCSPARSE_ERROR(rocsparse_csrsm_analysis_template(handle,
                                                                    trans_A,
                                                                    trans_B,
                                                                    m,
                                                                    nrhs,
                                                                    nnz,
                                                                    alpha_device_host,
                                                                    descr,
                                                                    csr_val,
                                                                    csr_row_ptr,
                                                                    csr_col_ind,
                                                                    B,
                                                                    ldb,
                                                                    info,
                                                                    analysis,
                                                                    solve,
                                                                    ptr));
    }

    return rocsparse_status_success;
}

static rocsparse_status rocsparse_coosm_analysis_quickreturn(rocsparse_handle    handle,
                                                             rocsparse_operation trans_A,
                                                             rocsparse_operation trans_B,
                                                             int64_t             m,
                                                             int64_t             nrhs,
                                                             int64_t             nnz,
                                                             const void*         alpha_device_host,
                                                             const rocsparse_mat_descr descr,
                                                             const void*               coo_val,
                                                             const void*               coo_row_ind,
                                                             const void*               coo_col_ind,
                                                             const void*               B,
                                                             int64_t                   ldb,
                                                             rocsparse_mat_info        info,
                                                             rocsparse_analysis_policy analysis,
                                                             rocsparse_solve_policy    solve,
                                                             void*                     temp_buffer)
{
    if(m == 0 || nrhs == 0)
    {
        return rocsparse_status_success;
    }
    return rocsparse_status_continue;
}

static rocsparse_status rocsparse_coosm_analysis_checkarg(rocsparse_handle    handle, //0
                                                          rocsparse_operation trans_A, //1
                                                          rocsparse_operation trans_B, //2
                                                          int64_t             m, //3
                                                          int64_t             nrhs, //4
                                                          int64_t             nnz, //5
                                                          const void*         alpha_device_host, //6
                                                          const rocsparse_mat_descr descr, //7
                                                          const void*               coo_val, //8
                                                          const void*               coo_row_ind, //9
                                                          const void*        coo_col_ind, //10
                                                          const void*        B, //11
                                                          int64_t            ldb, //12
                                                          rocsparse_mat_info info, //13
                                                          rocsparse_analysis_policy analysis, //14
                                                          rocsparse_solve_policy    solve, //15
                                                          void* temp_buffer) //16
{
    ROCSPARSE_CHECKARG_HANDLE(0, handle);
    ROCSPARSE_CHECKARG_ENUM(1, trans_A);
    ROCSPARSE_CHECKARG_ENUM(2, trans_B);
    ROCSPARSE_CHECKARG_SIZE(3, m);
    ROCSPARSE_CHECKARG_SIZE(4, nrhs);
    ROCSPARSE_CHECKARG_SIZE(5, nnz);

    ROCSPARSE_CHECKARG_POINTER(7, descr);
    ROCSPARSE_CHECKARG(
        7, descr, (descr->type != rocsparse_matrix_type_general), rocsparse_status_not_implemented);
    ROCSPARSE_CHECKARG(7,
                       descr,
                       (descr->storage_mode != rocsparse_storage_mode_sorted),
                       rocsparse_status_not_implemented);

    ROCSPARSE_CHECKARG_POINTER(13, info);
    ROCSPARSE_CHECKARG_ENUM(14, analysis);
    ROCSPARSE_CHECKARG_ENUM(15, solve);

    const rocsparse_status status = rocsparse_coosm_analysis_quickreturn(handle,
                                                                         trans_A,
                                                                         trans_B,
                                                                         m,
                                                                         nrhs,
                                                                         nnz,
                                                                         alpha_device_host,
                                                                         descr,
                                                                         coo_val,
                                                                         coo_row_ind,
                                                                         coo_col_ind,
                                                                         B,
                                                                         ldb,
                                                                         info,
                                                                         analysis,
                                                                         solve,
                                                                         temp_buffer);
    if(status != rocsparse_status_continue)
    {
        RETURN_IF_ROCSPARSE_ERROR(status);
        return rocsparse_status_success;
    }

    ROCSPARSE_CHECKARG_POINTER(6, alpha_device_host);
    ROCSPARSE_CHECKARG_ARRAY(8, nnz, coo_val);
    ROCSPARSE_CHECKARG_ARRAY(9, nnz, coo_row_ind);
    ROCSPARSE_CHECKARG_ARRAY(10, nnz, coo_col_ind);
    ROCSPARSE_CHECKARG_POINTER(11, B);
    ROCSPARSE_CHECKARG_POINTER(16, temp_buffer);
    return rocsparse_status_continue;
}

template <typename I, typename T>
rocsparse_status rocsparse_coosm_analysis_template(rocsparse_handle          handle,
                                                   rocsparse_operation       trans_A,
                                                   rocsparse_operation       trans_B,
                                                   I                         m,
                                                   I                         nrhs,
                                                   int64_t                   nnz,
                                                   const T*                  alpha_device_host,
                                                   const rocsparse_mat_descr descr,
                                                   const T*                  coo_val,
                                                   const I*                  coo_row_ind,
                                                   const I*                  coo_col_ind,
                                                   const T*                  B,
                                                   int64_t                   ldb,
                                                   rocsparse_mat_info        info,
                                                   rocsparse_analysis_policy analysis,
                                                   rocsparse_solve_policy    solve,
                                                   void*                     temp_buffer)
{

    const rocsparse_status status = rocsparse_coosm_analysis_checkarg(handle,
                                                                      trans_A,
                                                                      trans_B,
                                                                      m,
                                                                      nrhs,
                                                                      nnz,
                                                                      alpha_device_host,
                                                                      descr,
                                                                      coo_val,
                                                                      coo_row_ind,
                                                                      coo_col_ind,
                                                                      B,
                                                                      ldb,
                                                                      info,
                                                                      analysis,
                                                                      solve,
                                                                      temp_buffer);
    if(status != rocsparse_status_continue)
    {
        RETURN_IF_ROCSPARSE_ERROR(status);
        return rocsparse_status_success;
    }

    RETURN_IF_ROCSPARSE_ERROR(rocsparse_coosm_analysis_core(handle,
                                                            trans_A,
                                                            trans_B,
                                                            m,
                                                            nrhs,
                                                            nnz,
                                                            alpha_device_host,
                                                            descr,
                                                            coo_val,
                                                            coo_row_ind,
                                                            coo_col_ind,
                                                            B,
                                                            ldb,
                                                            info,
                                                            analysis,
                                                            solve,
                                                            temp_buffer));

    return rocsparse_status_success;
}

#define INSTANTIATE(ITYPE, TTYPE)                                \
    template rocsparse_status rocsparse_coosm_analysis_template( \
        rocsparse_handle          handle,                        \
        rocsparse_operation       trans_A,                       \
        rocsparse_operation       trans_B,                       \
        ITYPE                     m,                             \
        ITYPE                     nrhs,                          \
        int64_t                   nnz,                           \
        const TTYPE*              alpha_device_host,             \
        const rocsparse_mat_descr descr,                         \
        const TTYPE*              coo_val,                       \
        const ITYPE*              coo_row_ind,                   \
        const ITYPE*              coo_col_ind,                   \
        const TTYPE*              B,                             \
        int64_t                   ldb,                           \
        rocsparse_mat_info        info,                          \
        rocsparse_analysis_policy analysis,                      \
        rocsparse_solve_policy    solve,                         \
        void*                     temp_buffer);

INSTANTIATE(int32_t, float);
INSTANTIATE(int32_t, double);
INSTANTIATE(int32_t, rocsparse_float_complex);
INSTANTIATE(int32_t, rocsparse_double_complex);
INSTANTIATE(int64_t, float);
INSTANTIATE(int64_t, double);
INSTANTIATE(int64_t, rocsparse_float_complex);
INSTANTIATE(int64_t, rocsparse_double_complex);
#undef INSTANTIATE
