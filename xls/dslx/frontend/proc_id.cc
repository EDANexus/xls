// Copyright 2024 The XLS Authors
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

#include "xls/dslx/frontend/proc_id.h"

#include <utility>
#include <vector>

#include "xls/dslx/frontend/ast.h"
#include "xls/dslx/frontend/proc.h"

namespace xls::dslx {

ProcId ProcIdFactory::CreateProcId(const ProcId& parent, Proc* spawnee,
                                   bool count_as_new_instance) {
  std::vector<std::pair<Proc*, int>> new_stack = parent.proc_instance_stack;
  int& instance_count =
      instance_counts_[std::make_pair(parent, spawnee->identifier())];
  new_stack.push_back(std::make_pair(spawnee, instance_count));
  if (count_as_new_instance) {
    ++instance_count;
  }
  return ProcId{.proc_instance_stack = std::move(new_stack)};
}

}  // namespace xls::dslx