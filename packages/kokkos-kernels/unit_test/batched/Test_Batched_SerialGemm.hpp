/// \author Kyungjoo Kim (kyukim@sandia.gov)

#include "gtest/gtest.h"
#include "Kokkos_Core.hpp"
#include "Kokkos_Random.hpp"

//#include "KokkosBatched_Vector.hpp"

#include "KokkosBatched_Gemm_Decl.hpp"
#include "KokkosBatched_Gemm_Serial_Impl.hpp"

#include "KokkosKernels_TestUtils.hpp"

using namespace KokkosBatched::Experimental;

namespace Test {

  template<typename TA, typename TB>
  struct ParamTag { 
    typedef TA transA;
    typedef TB transB;
  };
 
  template<typename DeviceType,
           typename ViewType,
           typename ScalarType,
           typename ParamTagType, 
           typename AlgoTagType>
  struct Functor {
    ViewType _a, _b, _c;
    
    ScalarType _alpha, _beta;
    
    KOKKOS_INLINE_FUNCTION
    Functor(const ScalarType alpha, 
            const ViewType &a,
            const ViewType &b,
            const ScalarType beta,
            const ViewType &c)
      : _a(a), _b(b), _c(c), _alpha(alpha), _beta(beta) {}
    
    KOKKOS_INLINE_FUNCTION
    void operator()(const ParamTagType &, const int k) const {
      auto aa = Kokkos::subview(_a, k, Kokkos::ALL(), Kokkos::ALL());
      auto bb = Kokkos::subview(_b, k, Kokkos::ALL(), Kokkos::ALL());
      auto cc = Kokkos::subview(_c, k, Kokkos::ALL(), Kokkos::ALL());
      
      SerialGemm<typename ParamTagType::transA,
        typename ParamTagType::transB,
        AlgoTagType>::
        invoke(1.0, aa, bb, 1.0, cc);
    }
    
    inline
    void run() {
      Kokkos::RangePolicy<DeviceType,ParamTagType> policy(0, _c.dimension_0());
      Kokkos::parallel_for(policy, *this);            
    }
  };
    
  template<typename DeviceType,
           typename ViewType,
           typename ScalarType,
           typename ParamTagType, 
           typename AlgoTagType>
  void impl_test_batched_gemm(const int N, const int BlkSize) {
    typedef typename ViewType::value_type value_type;
    typedef Kokkos::Details::ArithTraits<value_type> ats;

    /// randomized input testing views
    ScalarType alpha = 1.5, beta = 3.0;

    ViewType
      a0("a0", N, BlkSize,BlkSize), a1("a1", N, BlkSize, BlkSize),
      b0("b0", N, BlkSize,BlkSize), b1("b1", N, BlkSize, BlkSize),
      c0("c0", N, BlkSize,BlkSize), c1("c1", N, BlkSize, BlkSize);

    Kokkos::Random_XorShift64_Pool<typename DeviceType::execution_space> random(13718);
    Kokkos::fill_random(a0, random, value_type(1.0));
    Kokkos::fill_random(b0, random, value_type(1.0));
    Kokkos::fill_random(c0, random, value_type(1.0));

    Kokkos::deep_copy(a1, a0);
    Kokkos::deep_copy(b1, b0);
    Kokkos::deep_copy(c1, c0);

    /// test body
    Functor<DeviceType,ViewType,ScalarType,
      ParamTagType,Algo::Gemm::Unblocked>(alpha, a0, b0, beta, c0).run();
    Functor<DeviceType,ViewType,ScalarType,
      ParamTagType,AlgoTagType>(alpha, a1, b1, beta, c1).run();

    /// for comparison send it to host
    typename ViewType::HostMirror c0_host = Kokkos::create_mirror_view(c0);
    typename ViewType::HostMirror c1_host = Kokkos::create_mirror_view(c1);

    Kokkos::deep_copy(c0_host, c0);
    Kokkos::deep_copy(c1_host, c1);

    /// check c0 = c1 ; this eps is about 10^-14
    typedef typename ats::mag_type mag_type;
    mag_type sum(1), diff(0);
    const mag_type eps = 1.0e3 * ats::epsilon();

    for (int k=0;k<N;++k) 
      for (int i=0;i<BlkSize;++i) 
        for (int j=0;j<BlkSize;++j) {
          sum  += ats::abs(c0_host(k,i,j));
          diff += ats::abs(c0_host(k,i,j)-c1_host(k,i,j));
        }
    EXPECT_NEAR_KK( diff/sum, 0, eps);
  }
}

template<typename DeviceType, 
         typename ValueType, 
         typename ScalarType,
         typename ParamTagType,
         typename AlgoTagType>
int test_batched_gemm() {
#if defined(KOKKOSKERNELS_INST_LAYOUTLEFT) 
  {
    typedef Kokkos::View<ValueType***,Kokkos::LayoutLeft,DeviceType> ViewType;
    Test::impl_test_batched_gemm<DeviceType,ViewType,ScalarType,ParamTagType,AlgoTagType>(     0, 10);
    Test::impl_test_batched_gemm<DeviceType,ViewType,ScalarType,ParamTagType,AlgoTagType>(    10, 15);
    Test::impl_test_batched_gemm<DeviceType,ViewType,ScalarType,ParamTagType,AlgoTagType>(  1024,  9);
    Test::impl_test_batched_gemm<DeviceType,ViewType,ScalarType,ParamTagType,AlgoTagType>(132231,  3);
  }
#endif
#if defined(KOKKOSKERNELS_INST_LAYOUTRIGHT) 
  {
    typedef Kokkos::View<ValueType***,Kokkos::LayoutRight,DeviceType> ViewType;
    Test::impl_test_batched_gemm<DeviceType,ViewType,ScalarType,ParamTagType,AlgoTagType>(     0, 10);
    Test::impl_test_batched_gemm<DeviceType,ViewType,ScalarType,ParamTagType,AlgoTagType>(    10, 15);
    Test::impl_test_batched_gemm<DeviceType,ViewType,ScalarType,ParamTagType,AlgoTagType>(  1024,  9);
    Test::impl_test_batched_gemm<DeviceType,ViewType,ScalarType,ParamTagType,AlgoTagType>(132231,  3);
  }
#endif
  
  return 0;
}

#if defined(KOKKOSKERNELS_INST_FLOAT)
TEST_F( TestCategory, batched_scalar_gemm_nt_nt_float_float ) {
  typedef ::Test::ParamTag<Trans::NoTranspose,Trans::NoTranspose> param_tag_type;
  typedef Algo::Gemm::Blocked algo_tag_type;
  test_batched_gemm<TestExecSpace,float,float,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_gemm_t_nt_float_float ) {
  typedef ::Test::ParamTag<Trans::Transpose,Trans::NoTranspose> param_tag_type;
  typedef Algo::Gemm::Blocked algo_tag_type;
  test_batched_gemm<TestExecSpace,float,float,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_gemm_nt_t_float_float ) {
  typedef ::Test::ParamTag<Trans::NoTranspose,Trans::Transpose> param_tag_type;
  typedef Algo::Gemm::Blocked algo_tag_type;
  test_batched_gemm<TestExecSpace,float,float,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_gemm_t_t_float_float ) {
  typedef ::Test::ParamTag<Trans::Transpose,Trans::Transpose> param_tag_type;
  typedef Algo::Gemm::Blocked algo_tag_type;
  test_batched_gemm<TestExecSpace,float,float,param_tag_type,algo_tag_type>();
}
#endif

#if defined(KOKKOSKERNELS_INST_DOUBLE)
TEST_F( TestCategory, batched_scalar_gemm_nt_nt_double_double ) {
  typedef ::Test::ParamTag<Trans::NoTranspose,Trans::NoTranspose> param_tag_type;
  typedef Algo::Gemm::Blocked algo_tag_type;
  test_batched_gemm<TestExecSpace,double,double,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_gemm_t_nt_double_double ) {
  typedef ::Test::ParamTag<Trans::Transpose,Trans::NoTranspose> param_tag_type;
  typedef Algo::Gemm::Blocked algo_tag_type;
  test_batched_gemm<TestExecSpace,double,double,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_gemm_nt_t_double_double ) {
  typedef ::Test::ParamTag<Trans::NoTranspose,Trans::Transpose> param_tag_type;
  typedef Algo::Gemm::Blocked algo_tag_type;
  test_batched_gemm<TestExecSpace,double,double,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_gemm_t_t_double_double ) {
  typedef ::Test::ParamTag<Trans::Transpose,Trans::Transpose> param_tag_type;
  typedef Algo::Gemm::Blocked algo_tag_type;
  test_batched_gemm<TestExecSpace,double,double,param_tag_type,algo_tag_type>();
}
#endif


#if defined(KOKKOSKERNELS_INST_COMPLEX_DOUBLE)

/// dcomplex, dcomplex

TEST_F( TestCategory, batched_scalar_gemm_nt_nt_dcomplex_dcomplex ) {
  typedef ::Test::ParamTag<Trans::NoTranspose,Trans::NoTranspose> param_tag_type;
  typedef Algo::Gemm::Blocked algo_tag_type;
  test_batched_gemm<TestExecSpace,Kokkos::complex<double>,Kokkos::complex<double>,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_gemm_t_nt_dcomplex_dcomplex ) {
  typedef ::Test::ParamTag<Trans::Transpose,Trans::NoTranspose> param_tag_type;
  typedef Algo::Gemm::Blocked algo_tag_type;
  test_batched_gemm<TestExecSpace,Kokkos::complex<double>,Kokkos::complex<double>,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_gemm_nt_t_dcomplex_dcomplex ) {
  typedef ::Test::ParamTag<Trans::NoTranspose,Trans::Transpose> param_tag_type;
  typedef Algo::Gemm::Blocked algo_tag_type;
  test_batched_gemm<TestExecSpace,Kokkos::complex<double>,Kokkos::complex<double>,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_gemm_t_t_dcomplex_dcomplex ) {
  typedef ::Test::ParamTag<Trans::Transpose,Trans::Transpose> param_tag_type;
  typedef Algo::Gemm::Blocked algo_tag_type;
  test_batched_gemm<TestExecSpace,Kokkos::complex<double>,Kokkos::complex<double>,param_tag_type,algo_tag_type>();
}
// TEST_F( TestCategory, batched_scalar_gemm_ct_nt_dcomplex_dcomplex ) {
//   typedef ::Test::ParamTag<Trans::ConjTranspose,Trans::NoTranspose> param_tag_type;
//   typedef Algo::Gemm::Blocked algo_tag_type;
//   test_batched_gemm<TestExecSpace,Kokkos::complex<double>,Kokkos::complex<double>,param_tag_type,algo_tag_type>();
// }
// TEST_F( TestCategory, batched_scalar_gemm_nt_ct_dcomplex_dcomplex ) {
//   typedef ::Test::ParamTag<Trans::NoTranspose,Trans::ConjTranspose> param_tag_type;
//   typedef Algo::Gemm::Blocked algo_tag_type;
//   test_batched_gemm<TestExecSpace,Kokkos::complex<double>,Kokkos::complex<double>,param_tag_type,algo_tag_type>();
// }

/// dcomplex, double

TEST_F( TestCategory, batched_scalar_gemm_nt_nt_dcomplex_double ) {
  typedef ::Test::ParamTag<Trans::NoTranspose,Trans::NoTranspose> param_tag_type;
  typedef Algo::Gemm::Blocked algo_tag_type;
  test_batched_gemm<TestExecSpace,Kokkos::complex<double>,double,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_gemm_t_nt_dcomplex_double ) {
  typedef ::Test::ParamTag<Trans::Transpose,Trans::NoTranspose> param_tag_type;
  typedef Algo::Gemm::Blocked algo_tag_type;
  test_batched_gemm<TestExecSpace,Kokkos::complex<double>,double,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_gemm_nt_t_dcomplex_double ) {
  typedef ::Test::ParamTag<Trans::NoTranspose,Trans::Transpose> param_tag_type;
  typedef Algo::Gemm::Blocked algo_tag_type;
  test_batched_gemm<TestExecSpace,Kokkos::complex<double>,double,param_tag_type,algo_tag_type>();
}
TEST_F( TestCategory, batched_scalar_gemm_t_t_dcomplex_double ) {
  typedef ::Test::ParamTag<Trans::Transpose,Trans::Transpose> param_tag_type;
  typedef Algo::Gemm::Blocked algo_tag_type;
  test_batched_gemm<TestExecSpace,Kokkos::complex<double>,double,param_tag_type,algo_tag_type>();
}
// TEST_F( TestCategory, batched_scalar_gemm_ct_nt_dcomplex_double ) {
//   typedef ::Test::ParamTag<Trans::ConjTranspose,Trans::NoTranspose> param_tag_type;
//   typedef Algo::Gemm::Blocked algo_tag_type;
//   test_batched_gemm<TestExecSpace,Kokkos::complex<double>,double,param_tag_type,algo_tag_type>();
// }
// TEST_F( TestCategory, batched_scalar_gemm_nt_ct_dcomplex_double ) {
//   typedef ::Test::ParamTag<Trans::NoTranspose,Trans::ConjTranspose> param_tag_type;
//   typedef Algo::Gemm::Blocked algo_tag_type;
//   test_batched_gemm<TestExecSpace,Kokkos::complex<double>,double,param_tag_type,algo_tag_type>();
// }

#endif
