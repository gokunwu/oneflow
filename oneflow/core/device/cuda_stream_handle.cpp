#include "oneflow/core/device/cuda_stream_handle.h"
#include "oneflow/core/device/cuda_util.h"
#include "oneflow/core/job/machine_context.h"

namespace oneflow {

#ifdef WITH_CUDA

const cudaStream_t* CudaStreamHandle::cuda_stream() {
  if (!cuda_stream_) {
    cuda_stream_.reset(new cudaStream_t);
    CudaCheck(cudaStreamCreateWithPriority(cuda_stream_.get(), cudaStreamDefault, priority_));
  }
  return cuda_stream_.get();
}

const cublasHandle_t* CudaStreamHandle::cublas_pmh_handle() {
  if (!cublas_pmh_handle_) {
    cublas_pmh_handle_.reset(new cublasHandle_t);
    CudaCheck(cublasCreate(cublas_pmh_handle_.get()));
    CudaCheck(cublasSetStream(*cublas_pmh_handle_, *cuda_stream()));
  }
  return cublas_pmh_handle_.get();
}

const cublasHandle_t* CudaStreamHandle::cublas_pmd_handle() {
  if (!cublas_pmd_handle_) {
    cublas_pmd_handle_.reset(new cublasHandle_t);
    CudaCheck(cublasCreate(cublas_pmd_handle_.get()));
    CudaCheck(cublasSetStream(*cublas_pmd_handle_, *cuda_stream()));
    CudaCheck(cublasSetPointerMode(*cublas_pmd_handle_, CUBLAS_POINTER_MODE_DEVICE));
  }
  return cublas_pmd_handle_.get();
}

const cublasHandle_t* CudaStreamHandle::cublas_tensor_op_math_handle() {
  if (!cublas_tensor_op_math_handle_) {
    cublas_tensor_op_math_handle_.reset(new cublasHandle_t);
    CudaCheck(cublasCreate(cublas_tensor_op_math_handle_.get()));
    CudaCheck(cublasSetStream(*cublas_tensor_op_math_handle_, *cuda_stream()));
    CudaCheck(cublasSetMathMode(*cublas_tensor_op_math_handle_, CUBLAS_TENSOR_OP_MATH));
  }
  return cublas_tensor_op_math_handle_.get();
}

const cudnnHandle_t* CudaStreamHandle::cudnn_handle() {
  if (!cudnn_handle_) {
    cudnn_handle_.reset(new cudnnHandle_t);
    CudaCheck(cudnnCreate(cudnn_handle_.get()));
    CudaCheck(cudnnSetStream(*cudnn_handle_, *cuda_stream()));
  }
  return cudnn_handle_.get();
}

void CudaStreamHandle::AddCallBack(std::function<void()> callback) {
  CudaCBEvent cb_event;
  cb_event.callback = callback;
  cb_event.event = GetCudaEvent();
  cb_event.cuda_stream_handle = this;
  CudaCheck(cudaEventRecord(cb_event.event, *cuda_stream()));
  cb_event_chan_->Send(cb_event);
}

cudaEvent_t CudaStreamHandle::GetCudaEvent() {
  cudaEvent_t ret;
  bool from_cache = false;
  while (cuda_event_pool_mutex_.test_and_set(std::memory_order_acquire)) {}
  if (!cuda_event_pool_.empty()) {
    ret = cuda_event_pool_.front();
    cuda_event_pool_.pop_front();
    from_cache = true;
  }
  cuda_event_pool_mutex_.clear(std::memory_order_release);
  if (!from_cache) {
    CudaCheck(cudaEventCreateWithFlags(&ret, cudaEventBlockingSync | cudaEventDisableTiming));
  }
  return ret;
}

void CudaStreamHandle::PutCudaEvent(cudaEvent_t event) {
  while (cuda_event_pool_mutex_.test_and_set(std::memory_order_acquire)) {}
  cuda_event_pool_.emplace_back(event);
  cuda_event_pool_mutex_.clear(std::memory_order_release);
}

CudaStreamHandle::~CudaStreamHandle() {
  for (cudaEvent_t event : cuda_event_pool_) { CudaCheck(cudaEventDestroy(event)); }
  if (cudnn_handle_) { CudaCheck(cudnnDestroy(*cudnn_handle_)); }
  if (cublas_pmh_handle_) { CudaCheck(cublasDestroy(*cublas_pmh_handle_)); }
  if (cublas_pmd_handle_) { CudaCheck(cublasDestroy(*cublas_pmd_handle_)); }
  if (cuda_stream_) { CudaCheck(cudaStreamDestroy(*cuda_stream_)); }
}

#endif  // WITH_CUDA

}  // namespace oneflow
