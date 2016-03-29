/* Copyright (c) 2016, the Cap authors.
 *
 * This file is subject to the Modified BSD License and may not be distributed
 * without copyright and license information. Please refer to the file LICENSE
 * for the text and further information on this license.
 */

#include <cap/dof_extractor.h>

namespace cap
{

template <typename T>
dealii::FullMatrix<T>
DoFExtractor::extract_matrix(dealii::FullMatrix<T> const &matrix_in) const
{
  dealii::FullMatrix<T> matrix_out(this->row_indices.size(),
                                   this->column_indices.size());
  matrix_out.extract_submatrix_from(matrix_in, this->row_indices,
                                    this->column_indices);
  return matrix_out;
}

template <typename T>
dealii::Vector<T>
DoFExtractor::extract_vector(dealii::Vector<T> const &vector_in) const
{
  unsigned int const n_dofs = this->row_indices.size();
  dealii::Vector<T> vector_out(n_dofs);
  for (unsigned int dof = 0; dof < n_dofs; ++dof)
  {
    vector_out[dof] = vector_in[this->row_indices[dof]];
  } // end for dof
  return vector_out;
}

} // end namespace cap