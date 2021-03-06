// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2008 Gael Guennebaud <gael.guennebaud@inria.fr>
//
// Eigen is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
//
// Alternatively, you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of
// the License, or (at your option) any later version.
//
// Eigen is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License or the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License and a copy of the GNU General Public License along with
// Eigen. If not, see <http://www.gnu.org/licenses/>.

#ifndef EIGEN_LLT_H
#define EIGEN_LLT_H

template<typename MatrixType, int UpLo> struct LLT_Traits;

/** \ingroup cholesky_Module
  *
  * \class LLT
  *
  * \brief Standard Cholesky decomposition (LL^T) of a matrix and associated features
  *
  * \param MatrixType the type of the matrix of which we are computing the LL^T Cholesky decomposition
  *
  * This class performs a LL^T Cholesky decomposition of a symmetric, positive definite
  * matrix A such that A = LL^* = U^*U, where L is lower triangular.
  *
  * While the Cholesky decomposition is particularly useful to solve selfadjoint problems like  D^*D x = b,
  * for that purpose, we recommend the Cholesky decomposition without square root which is more stable
  * and even faster. Nevertheless, this standard Cholesky decomposition remains useful in many other
  * situations like generalised eigen problems with hermitian matrices.
  *
  * Remember that Cholesky decompositions are not rank-revealing. This LLT decomposition is only stable on positive definite matrices,
  * use LDLT instead for the semidefinite case. Also, do not use a Cholesky decomposition to determine whether a system of equations
  * has a solution.
  *
  * \sa MatrixBase::llt(), class LDLT
  */
 /* HEY THIS DOX IS DISABLED BECAUSE THERE's A BUG EITHER HERE OR IN LDLT ABOUT THAT (OR BOTH)
  * Note that during the decomposition, only the upper triangular part of A is considered. Therefore,
  * the strict lower part does not have to store correct values.
  */
template<typename _MatrixType, int _UpLo> class LLT
{
  public:
    typedef _MatrixType MatrixType;
    enum {
      RowsAtCompileTime = MatrixType::RowsAtCompileTime,
      ColsAtCompileTime = MatrixType::ColsAtCompileTime,
      Options = MatrixType::Options,
      MaxColsAtCompileTime = MatrixType::MaxColsAtCompileTime
    };
    typedef typename MatrixType::Scalar Scalar;
    typedef typename NumTraits<typename MatrixType::Scalar>::Real RealScalar;
    typedef typename MatrixType::Index Index;

    enum {
      PacketSize = ei_packet_traits<Scalar>::size,
      AlignmentMask = int(PacketSize)-1,
      UpLo = _UpLo
    };

    typedef LLT_Traits<MatrixType,UpLo> Traits;

    /**
    * \brief Default Constructor.
    *
    * The default constructor is useful in cases in which the user intends to
    * perform decompositions via LLT::compute(const MatrixType&).
    */
    LLT() : m_matrix(), m_isInitialized(false) {}

    /** \brief Default Constructor with memory preallocation
      *
      * Like the default constructor but with preallocation of the internal data
      * according to the specified problem \a size.
      * \sa LLT()
      */
    LLT(Index size) : m_matrix(size, size),
                    m_isInitialized(false) {}

    LLT(const MatrixType& matrix)
      : m_matrix(matrix.rows(), matrix.cols()),
        m_isInitialized(false)
    {
      compute(matrix);
    }

    /** \returns a view of the upper triangular matrix U */
    inline typename Traits::MatrixU matrixU() const
    {
      ei_assert(m_isInitialized && "LLT is not initialized.");
      return Traits::getU(m_matrix);
    }

    /** \returns a view of the lower triangular matrix L */
    inline typename Traits::MatrixL matrixL() const
    {
      ei_assert(m_isInitialized && "LLT is not initialized.");
      return Traits::getL(m_matrix);
    }

    /** \returns the solution x of \f$ A x = b \f$ using the current decomposition of A.
      *
      * Since this LLT class assumes anyway that the matrix A is invertible, the solution
      * theoretically exists and is unique regardless of b.
      *
      * Example: \include LLT_solve.cpp
      * Output: \verbinclude LLT_solve.out
      *
      * \sa solveInPlace(), MatrixBase::llt()
      */
    template<typename Rhs>
    inline const ei_solve_retval<LLT, Rhs>
    solve(const MatrixBase<Rhs>& b) const
    {
      ei_assert(m_isInitialized && "LLT is not initialized.");
      ei_assert(m_matrix.rows()==b.rows()
                && "LLT::solve(): invalid number of rows of the right hand side matrix b");
      return ei_solve_retval<LLT, Rhs>(*this, b.derived());
    }

    template<typename Derived>
    bool solveInPlace(MatrixBase<Derived> &bAndX) const;

    LLT& compute(const MatrixType& matrix);

    /** \returns the LLT decomposition matrix
      *
      * TODO: document the storage layout
      */
    inline const MatrixType& matrixLLT() const
    {
      ei_assert(m_isInitialized && "LLT is not initialized.");
      return m_matrix;
    }

    MatrixType reconstructedMatrix() const;


    /** \brief Reports whether previous computation was successful.
      *
      * \returns \c Success if computation was succesful,
      *          \c NumericalIssue if the matrix.appears to be negative.
      */
    ComputationInfo info() const
    {
      ei_assert(m_isInitialized && "LLT is not initialized.");
      return m_info;
    }

    inline Index rows() const { return m_matrix.rows(); }
    inline Index cols() const { return m_matrix.cols(); }

  protected:
    /** \internal
      * Used to compute and store L
      * The strict upper part is not used and even not initialized.
      */
    MatrixType m_matrix;
    bool m_isInitialized;
    ComputationInfo m_info;
};

template<int UpLo> struct ei_llt_inplace;

template<> struct ei_llt_inplace<Lower>
{
  template<typename MatrixType>
  static bool unblocked(MatrixType& mat)
  {
    typedef typename MatrixType::Scalar Scalar;
    typedef typename MatrixType::RealScalar RealScalar;
    typedef typename MatrixType::Index Index;
    ei_assert(mat.rows()==mat.cols());
    const Index size = mat.rows();
    for(Index k = 0; k < size; ++k)
    {
      Index rs = size-k-1; // remaining size

      Block<MatrixType,Dynamic,1> A21(mat,k+1,k,rs,1);
      Block<MatrixType,1,Dynamic> A10(mat,k,0,1,k);
      Block<MatrixType,Dynamic,Dynamic> A20(mat,k+1,0,rs,k);

      RealScalar x = ei_real(mat.coeff(k,k));
      if (k>0) x -= mat.row(k).head(k).squaredNorm();
      if (x<=RealScalar(0))
        return false;
      mat.coeffRef(k,k) = x = ei_sqrt(x);
      if (k>0 && rs>0) A21.noalias() -= A20 * A10.adjoint();
      if (rs>0) A21 *= RealScalar(1)/x;
    }
    return true;
  }

  template<typename MatrixType>
  static bool blocked(MatrixType& m)
  {
    typedef typename MatrixType::Index Index;
    ei_assert(m.rows()==m.cols());
    Index size = m.rows();
    if(size<32)
      return unblocked(m);

    Index blockSize = size/8;
    blockSize = (blockSize/16)*16;
    blockSize = std::min(std::max(blockSize,Index(8)), Index(128));

    for (Index k=0; k<size; k+=blockSize)
    {
      // partition the matrix:
      //       A00 |  -  |  -
      // lu  = A10 | A11 |  -
      //       A20 | A21 | A22
      Index bs = std::min(blockSize, size-k);
      Index rs = size - k - bs;
      Block<MatrixType,Dynamic,Dynamic> A11(m,k,   k,   bs,bs);
      Block<MatrixType,Dynamic,Dynamic> A21(m,k+bs,k,   rs,bs);
      Block<MatrixType,Dynamic,Dynamic> A22(m,k+bs,k+bs,rs,rs);

      if(!unblocked(A11)) return false;
      if(rs>0) A11.adjoint().template triangularView<Upper>().template solveInPlace<OnTheRight>(A21);
      if(rs>0) A22.template selfadjointView<Lower>().rankUpdate(A21,-1); // bottleneck
    }
    return true;
  }
};

template<> struct ei_llt_inplace<Upper>
{
  template<typename MatrixType>
  static EIGEN_STRONG_INLINE bool unblocked(MatrixType& mat)
  {
    Transpose<MatrixType> matt(mat);
    return ei_llt_inplace<Lower>::unblocked(matt);
  }
  template<typename MatrixType>
  static EIGEN_STRONG_INLINE bool blocked(MatrixType& mat)
  {
    Transpose<MatrixType> matt(mat);
    return ei_llt_inplace<Lower>::blocked(matt);
  }
};

template<typename MatrixType> struct LLT_Traits<MatrixType,Lower>
{
  typedef TriangularView<MatrixType, Lower> MatrixL;
  typedef TriangularView<typename MatrixType::AdjointReturnType, Upper> MatrixU;
  inline static MatrixL getL(const MatrixType& m) { return m; }
  inline static MatrixU getU(const MatrixType& m) { return m.adjoint(); }
  static bool inplace_decomposition(MatrixType& m)
  { return ei_llt_inplace<Lower>::blocked(m); }
};

template<typename MatrixType> struct LLT_Traits<MatrixType,Upper>
{
  typedef TriangularView<typename MatrixType::AdjointReturnType, Lower> MatrixL;
  typedef TriangularView<MatrixType, Upper> MatrixU;
  inline static MatrixL getL(const MatrixType& m) { return m.adjoint(); }
  inline static MatrixU getU(const MatrixType& m) { return m; }
  static bool inplace_decomposition(MatrixType& m)
  { return ei_llt_inplace<Upper>::blocked(m); }
};

/** Computes / recomputes the Cholesky decomposition A = LL^* = U^*U of \a matrix
  *
  *
  * \returns a reference to *this
  */
template<typename MatrixType, int _UpLo>
LLT<MatrixType,_UpLo>& LLT<MatrixType,_UpLo>::compute(const MatrixType& a)
{
  assert(a.rows()==a.cols());
  const Index size = a.rows();
  m_matrix.resize(size, size);
  m_matrix = a;

  m_isInitialized = true;
  bool ok = Traits::inplace_decomposition(m_matrix);
  m_info = ok ? Success : NumericalIssue;

  return *this;
}

template<typename _MatrixType, int UpLo, typename Rhs>
struct ei_solve_retval<LLT<_MatrixType, UpLo>, Rhs>
  : ei_solve_retval_base<LLT<_MatrixType, UpLo>, Rhs>
{
  typedef LLT<_MatrixType,UpLo> LLTType;
  EIGEN_MAKE_SOLVE_HELPERS(LLTType,Rhs)

  template<typename Dest> void evalTo(Dest& dst) const
  {
    dst = rhs();
    dec().solveInPlace(dst);
  }
};

/** This is the \em in-place version of solve().
  *
  * \param bAndX represents both the right-hand side matrix b and result x.
  *
  * \returns true always! If you need to check for existence of solutions, use another decomposition like LU, QR, or SVD.
  *
  * This version avoids a copy when the right hand side matrix b is not
  * needed anymore.
  *
  * \sa LLT::solve(), MatrixBase::llt()
  */
template<typename MatrixType, int _UpLo>
template<typename Derived>
bool LLT<MatrixType,_UpLo>::solveInPlace(MatrixBase<Derived> &bAndX) const
{
  ei_assert(m_isInitialized && "LLT is not initialized.");
  ei_assert(m_matrix.rows()==bAndX.rows());
  matrixL().solveInPlace(bAndX);
  matrixU().solveInPlace(bAndX);
  return true;
}

/** \returns the matrix represented by the decomposition,
 * i.e., it returns the product: L L^*.
 * This function is provided for debug purpose. */
template<typename MatrixType, int _UpLo>
MatrixType LLT<MatrixType,_UpLo>::reconstructedMatrix() const
{
  ei_assert(m_isInitialized && "LLT is not initialized.");
  return matrixL() * matrixL().adjoint().toDenseMatrix();
}

/** \cholesky_module
  * \returns the LLT decomposition of \c *this
  */
template<typename Derived>
inline const LLT<typename MatrixBase<Derived>::PlainObject>
MatrixBase<Derived>::llt() const
{
  return LLT<PlainObject>(derived());
}

/** \cholesky_module
  * \returns the LLT decomposition of \c *this
  */
template<typename MatrixType, unsigned int UpLo>
inline const LLT<typename SelfAdjointView<MatrixType, UpLo>::PlainObject, UpLo>
SelfAdjointView<MatrixType, UpLo>::llt() const
{
  return LLT<PlainObject,UpLo>(m_matrix);
}

#endif // EIGEN_LLT_H
