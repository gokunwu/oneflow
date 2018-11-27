#include "oneflow/core/common/data_type.h"
#include "oneflow/core/common/preprocessor.h"
#include "oneflow/core/ndarray/ndarray_reduce_impl.h"
#include "oneflow/core/ndarray/binary_func.h"

namespace oneflow {

#define SPECIALIZE_CPU_NDARRAY_REDUCE_IMPL(struct_name)                                            \
  template<typename T, const T (*binary_func)(const T, const T)>                                   \
  struct struct_name<DeviceType::kCPU, T, binary_func> final {                                     \
    static bool Matched(const XpuVarNdarray<T>& y, const XpuVarNdarray<const T>& x) {              \
      return false;                                                                                \
    }                                                                                              \
    static void Reduce(DeviceCtx* ctx, const XpuVarNdarray<T>& y, const XpuVarNdarray<const T>& x, \
                       const XpuVarNdarray<T>& tmp_storage) {                                      \
      UNIMPLEMENTED();                                                                             \
    }                                                                                              \
  }
SPECIALIZE_CPU_NDARRAY_REDUCE_IMPL(NdarrayScalarReduce);
SPECIALIZE_CPU_NDARRAY_REDUCE_IMPL(NdarrayMatrixRowReduce);
SPECIALIZE_CPU_NDARRAY_REDUCE_IMPL(NdarrayMatrixColReduce);
#undef SPECIALIZE_CPU_NDARRAY_REDUCE_IMPL

#define INSTANTIATE_NDARRAY_REDUCE_IMPL(dtype, binary_func)                                       \
  template struct NdarrayScalarReduce<DeviceType::kCPU, OF_PP_PAIR_FIRST(dtype), binary_func>;    \
  template struct NdarrayMatrixRowReduce<DeviceType::kCPU, OF_PP_PAIR_FIRST(dtype), binary_func>; \
  template struct NdarrayMatrixColReduce<DeviceType::kCPU, OF_PP_PAIR_FIRST(dtype), binary_func>;
OF_PP_SEQ_PRODUCT_FOR_EACH_TUPLE(INSTANTIATE_NDARRAY_REDUCE_IMPL, ARITHMETIC_DATA_TYPE_SEQ,
                                 REDUCE_BINARY_FUNC_SEQ);

}  // namespace oneflow
