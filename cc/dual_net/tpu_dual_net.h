// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef CC_DUAL_NET_TPU_DUAL_NET_H_
#define CC_DUAL_NET_TPU_DUAL_NET_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "cc/constants.h"
#include "cc/dual_net/dual_net.h"
#include "cc/thread_safe_queue.h"
#include "tensorflow/core/framework/graph.pb.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/public/session.h"

namespace minigo {

class TpuDualNet : public DualNet {
 public:
  // graph_path: A frozen GraphDef proto suitable for running on TPU, e.g.
  //             generated by freeze_graph.py --use_tpu
  // tpu_name: The gRPC address of the VM's TPU, e.g. "grpc://10.240.2.10:8470".
  TpuDualNet(const std::string& graph_path, const std::string& tpu_name);
  ~TpuDualNet() override;

  void RunMany(std::vector<const BoardFeatures*> features,
               std::vector<Output*> outputs, std::string* model) override;

  int GetBufferCount() const override;

 private:
  class Worker {
   public:
    Worker(const tensorflow::GraphDef& graph_def, const std::string& tpu_name,
           int num_replicas);
    ~Worker();

    void RunMany(std::vector<const DualNet::BoardFeatures*> features,
                 std::vector<DualNet::Output*> outputs);

    void InitializeTpu();
    void ShutdownTpu();

   private:
    void Reserve(size_t capacity);

    std::unique_ptr<tensorflow::Session> session_;
    std::vector<std::pair<std::string, tensorflow::Tensor>> inputs_;
    std::vector<std::string> output_names_;
    std::vector<tensorflow::Tensor> outputs_;
    const int num_replicas_;
    size_t batch_capacity_;
  };

  ThreadSafeQueue<std::unique_ptr<Worker>> workers_;
  std::string graph_path_;
};

}  // namespace minigo

#endif  // CC_DUAL_NET_TPU_DUAL_NET_H_
