#ifndef __KOKKOSBATCHED_INNER_GEMM_FIX_A_DECL_HPP__
#define __KOKKOSBATCHED_INNER_GEMM_FIX_A_DECL_HPP__


/// \author Kyungjoo Kim (kyukim@sandia.gov)


namespace KokkosBatched {
  namespace Experimental {

    template<int mb, int nb>
    struct InnerGemmFixA {
      const int _as0, _as1, _bs0, _bs1, _cs0, _cs1;
    
      KOKKOS_INLINE_FUNCTION
      InnerGemmFixA(const int as0, const int as1, 
                    const int bs0, const int bs1,
                    const int cs0, const int cs1)
        : _as0(as0), _as1(as1), 
          _bs0(bs0), _bs1(bs1), 
          _cs0(cs0), _cs1(cs1) {}
    
      // serial rank update
      template<typename ValueType>
      KOKKOS_INLINE_FUNCTION
      int serial_invoke(const ValueType alpha,
                        const ValueType *__restrict__ A,
                        const ValueType *__restrict__ B,
                        const int n,
                        /**/  ValueType *__restrict__ C);
    
      // serial rank update for remainder
      template<typename ValueType>
      KOKKOS_INLINE_FUNCTION
      int serial_invoke(const ValueType alpha,
                        const ValueType *__restrict__ A,
                        const ValueType *__restrict__ B,
                        const int m, const int n, const int k,
                        /**/  ValueType *__restrict__ C);
    };
  }
}


#endif
