// Copyright 2021 The XLS Authors
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

#include "xls/codegen/codegen_wrapper_pass.h"

#include "xls/passes/optimization_pass.h"
#include "xls/passes/pass_base.h"

namespace xls::verilog {

absl::StatusOr<bool> CodegenWrapperPass::RunInternal(
    CodegenPassUnit* unit, const CodegenPassOptions& options,
    PassResults* results) const {
  return wrapped_pass_->RunOnFunctionBase(
      unit->block, OptimizationPassOptions(options), results);
}

}  // namespace xls::verilog
