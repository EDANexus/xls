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

#ifndef XLS_IR_NODES_H_
#define XLS_IR_NODES_H_

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "xls/ir/format_strings.h"
#include "xls/ir/lsb_or_msb.h"
#include "xls/ir/node.h"
#include "xls/ir/op.h"
#include "xls/ir/register.h"
#include "xls/ir/source_location.h"
#include "xls/ir/type.h"
#include "xls/ir/value.h"

// TODO(meheff): Add comments to classes and methods.

namespace xls {

class Function;
class Instantiation;

struct SliceData {
  int64_t start;
  int64_t width;
};

class AfterAll final : public Node {
 public:
  AfterAll(const SourceInfo& loc, absl::Span<Node* const> dependencies,
           std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
};

class ArithOp final : public Node {
 public:
  static constexpr int64_t kLhsOperand = 0;
  static constexpr int64_t kRhsOperand = 1;

  ArithOp(const SourceInfo& loc, Node* lhs, Node* rhs, int64_t width, Op op,
          std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  int64_t width() const { return width_; }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  int64_t width_;
};

class Array final : public Node {
 public:
  Array(const SourceInfo& loc, absl::Span<Node* const> elements,
        Type* element_type, std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  Type* element_type() const { return element_type_; }

  int64_t size() const { return operand_count(); }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  Type* element_type_;
};

class ArrayConcat final : public Node {
 public:
  ArrayConcat(const SourceInfo& loc, absl::Span<Node* const> args,
              std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
};

class ArrayIndex final : public Node {
 public:
  static constexpr int64_t kArgOperand = 0;

  ArrayIndex(const SourceInfo& loc, Node* arg, absl::Span<Node* const> indices,
             std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  Node* array() const { return operand(0); }

  absl::Span<Node* const> indices() const { return operands().subspan(1); }
};

class ArraySlice final : public Node {
 public:
  static constexpr int64_t kArrayOperand = 0;
  static constexpr int64_t kStartOperand = 1;

  ArraySlice(const SourceInfo& loc, Node* array, Node* start, int64_t width,
             std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  int64_t width() const { return width_; }

  Node* array() const { return operand(0); }

  Node* start() const { return operand(1); }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  int64_t width_;
};

class ArrayUpdate final : public Node {
 public:
  static constexpr int64_t kArgOperand = 0;
  static constexpr int64_t kUpdateValueOperand = 1;

  ArrayUpdate(const SourceInfo& loc, Node* arg, Node* update_value,
              absl::Span<Node* const> indices, std::string_view name,
              FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  Node* array_to_update() const { return operand(0); }

  absl::Span<Node* const> indices() const { return operands().subspan(2); }

  Node* update_value() const { return operand(1); }
};

class Assert final : public Node {
 public:
  static constexpr int64_t kTokenOperand = 0;
  static constexpr int64_t kConditionOperand = 1;

  Assert(const SourceInfo& loc, Node* token, Node* condition,
         std::string_view message, std::optional<std::string> label,
         std::optional<std::string> original_label, std::string_view name,
         FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  const std::string& message() const { return message_; }

  std::optional<std::string> label() const { return label_; }

  std::optional<std::string> original_label() const { return original_label_; }

  Node* token() const { return operand(0); }

  Node* condition() const { return operand(1); }

  void set_label(std::string new_label) { label_ = std::move(new_label); }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  std::string message_;
  std::optional<std::string> label_;
  std::optional<std::string> original_label_;
};

class BinOp final : public Node {
 public:
  static constexpr int64_t kLhsOperand = 0;
  static constexpr int64_t kRhsOperand = 1;

  BinOp(const SourceInfo& loc, Node* lhs, Node* rhs, Op op,
        std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
};

class BitSlice final : public Node {
 public:
  static constexpr int64_t kArgOperand = 0;

  BitSlice(const SourceInfo& loc, Node* arg, int64_t start, int64_t width,
           std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  int64_t start() const { return start_; }

  int64_t width() const { return width_; }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  int64_t start_;
  int64_t width_;
};

class BitSliceUpdate final : public Node {
 public:
  static constexpr int64_t kArgOperand = 0;
  static constexpr int64_t kStartOperand = 1;
  static constexpr int64_t kValueOperand = 2;

  BitSliceUpdate(const SourceInfo& loc, Node* arg, Node* start, Node* value,
                 std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  Node* to_update() const { return operand(0); }

  Node* start() const { return operand(1); }

  Node* update_value() const { return operand(2); }
};

class BitwiseReductionOp final : public Node {
 public:
  static constexpr int64_t kOperandOperand = 0;

  BitwiseReductionOp(const SourceInfo& loc, Node* operand, Op op,
                     std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
};

class CompareOp final : public Node {
 public:
  static constexpr int64_t kLhsOperand = 0;
  static constexpr int64_t kRhsOperand = 1;

  CompareOp(const SourceInfo& loc, Node* lhs, Node* rhs, Op op,
            std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
};

class Concat final : public Node {
 public:
  Concat(const SourceInfo& loc, absl::Span<Node* const> args,
         std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  SliceData GetOperandSliceData(int64_t operandno) const;
};

class CountedFor final : public Node {
 public:
  static constexpr int64_t kInitialValueOperand = 0;

  CountedFor(const SourceInfo& loc, Node* initial_value,
             absl::Span<Node* const> invariant_args, int64_t trip_count,
             int64_t stride, Function* body, std::string_view name,
             FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  int64_t trip_count() const { return trip_count_; }

  int64_t stride() const { return stride_; }

  Function* body() const { return body_; }

  Node* initial_value() const { return operand(0); }

  absl::Span<Node* const> invariant_args() const {
    return operands().subspan(1);
  }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  int64_t trip_count_;
  int64_t stride_;
  Function* body_;
};

class Cover final : public Node {
 public:
  static constexpr int64_t kConditionOperand = 0;

  Cover(const SourceInfo& loc, Node* condition, std::string_view label,
        std::optional<std::string> original_label, std::string_view name,
        FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  const std::string& label() const { return label_; }

  std::optional<std::string> original_label() const { return original_label_; }

  Node* condition() const { return operand(0); }

  void set_label(std::string new_label) { label_ = std::move(new_label); }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  std::string label_;
  std::optional<std::string> original_label_;
};

class Decode final : public Node {
 public:
  static constexpr int64_t kArgOperand = 0;

  Decode(const SourceInfo& loc, Node* arg, int64_t width, std::string_view name,
         FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  int64_t width() const { return width_; }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  int64_t width_;
};

class DynamicBitSlice final : public Node {
 public:
  static constexpr int64_t kArgOperand = 0;
  static constexpr int64_t kStartOperand = 1;

  DynamicBitSlice(const SourceInfo& loc, Node* arg, Node* start, int64_t width,
                  std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  int64_t width() const { return width_; }

  Node* to_slice() const { return operand(0); }

  Node* start() const { return operand(1); }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  int64_t width_;
};

class DynamicCountedFor final : public Node {
 public:
  static constexpr int64_t kInitialValueOperand = 0;
  static constexpr int64_t kTripCountOperand = 1;
  static constexpr int64_t kStrideOperand = 2;

  DynamicCountedFor(const SourceInfo& loc, Node* initial_value,
                    Node* trip_count, Node* stride,
                    absl::Span<Node* const> invariant_args, Function* body,
                    std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  Function* body() const { return body_; }

  Node* initial_value() const { return operand(0); }

  Node* trip_count() const { return operand(1); }

  Node* stride() const { return operand(2); }

  absl::Span<Node* const> invariant_args() const {
    return operands().subspan(3);
  }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  Function* body_;
};

class Encode final : public Node {
 public:
  static constexpr int64_t kArgOperand = 0;

  Encode(const SourceInfo& loc, Node* arg, std::string_view name,
         FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
};

class ExtendOp final : public Node {
 public:
  static constexpr int64_t kArgOperand = 0;

  ExtendOp(const SourceInfo& loc, Node* arg, int64_t new_bit_count, Op op,
           std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  int64_t new_bit_count() const { return new_bit_count_; }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  int64_t new_bit_count_;
};

class Gate final : public Node {
 public:
  static constexpr int64_t kConditionOperand = 0;
  static constexpr int64_t kDataOperand = 1;

  Gate(const SourceInfo& loc, Node* condition, Node* data,
       std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  Node* condition() const { return operand(0); }

  Node* data() const { return operand(1); }
};
class InputPort final : public Node {
 public:
  InputPort(const SourceInfo& loc, std::string_view name, Type* type,
            FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  std::string_view name() const { return name_; }
};

class InstantiationInput final : public Node {
 public:
  static constexpr int64_t kDataOperand = 0;

  InstantiationInput(const SourceInfo& loc, Node* data,
                     Instantiation* instantiation, std::string_view port_name,
                     std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  Instantiation* instantiation() const { return instantiation_; }

  const std::string& port_name() const { return port_name_; }

  Node* data() const { return operand(0); }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  Instantiation* instantiation_;
  std::string port_name_;
};

class InstantiationOutput final : public Node {
 public:
  InstantiationOutput(const SourceInfo& loc, Instantiation* instantiation,
                      std::string_view port_name, std::string_view name,
                      FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  Instantiation* instantiation() const { return instantiation_; }

  const std::string& port_name() const { return port_name_; }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  Instantiation* instantiation_;
  std::string port_name_;
};

class Invoke final : public Node {
 public:
  Invoke(const SourceInfo& loc, absl::Span<Node* const> args,
         Function* to_apply, std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  Function* to_apply() const { return to_apply_; }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  Function* to_apply_;
};

class Literal final : public Node {
 public:
  Literal(const SourceInfo& loc, Value value, std::string_view name,
          FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  const Value& value() const { return value_; }

  bool IsZero() const { return value().IsBits() && value().bits().IsZero(); }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  Value value_;
};

class Map final : public Node {
 public:
  static constexpr int64_t kArgOperand = 0;

  Map(const SourceInfo& loc, Node* arg, Function* to_apply,
      std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  Function* to_apply() const { return to_apply_; }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  Function* to_apply_;
};

class MinDelay final : public Node {
 public:
  static constexpr int64_t kTokenOperand = 0;

  MinDelay(const SourceInfo& loc, Node* token, int64_t delay,
           std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  int64_t delay() const { return delay_; }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  int64_t delay_;
};

class NaryOp final : public Node {
 public:
  NaryOp(const SourceInfo& loc, absl::Span<Node* const> args, Op op,
         std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
};

class Next final : public Node {
 public:
  static constexpr int64_t kParamOperand = 0;
  static constexpr int64_t kValueOperand = 1;

  Next(const SourceInfo& loc, Node* param, Node* value,
       std::optional<Node*> predicate, std::string_view name,
       FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  Node* param() const { return operand(0); }

  Node* value() const { return operand(1); }

  std::optional<Node*> predicate() const {
    return predicate_operand_number().ok()
               ? std::optional<Node*>(operand(*predicate_operand_number()))
               : std::nullopt;
  }

  absl::StatusOr<int64_t> predicate_operand_number() const {
    if (!has_predicate_) {
      return absl::InternalError("predicate is not present");
    }
    int64_t ret = 2;

    return ret;
  }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  bool has_predicate_;
};

class OneHot final : public Node {
 public:
  static constexpr int64_t kInputOperand = 0;

  OneHot(const SourceInfo& loc, Node* input, LsbOrMsb priority,
         std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  LsbOrMsb priority() const { return priority_; }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  LsbOrMsb priority_;
};

class OneHotSelect final : public Node {
 public:
  static constexpr int64_t kSelectorOperand = 0;

  OneHotSelect(const SourceInfo& loc, Node* selector,
               absl::Span<Node* const> cases, std::string_view name,
               FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  Node* selector() const { return operand(0); }

  absl::Span<Node* const> cases() const { return operands().subspan(1); }

  Node* get_case(int64_t case_no) const { return cases().at(case_no); }
};

class OutputPort final : public Node {
 public:
  static constexpr int64_t kOperandOperand = 0;

  OutputPort(const SourceInfo& loc, Node* operand, std::string_view name,
             FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  std::string_view name() const { return name_; }
};

class Param final : public Node {
 public:
  Param(const SourceInfo& loc, Type* type, std::string_view name,
        FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  std::string_view name() const { return name_; }
};

class PartialProductOp final : public Node {
 public:
  static constexpr int64_t kLhsOperand = 0;
  static constexpr int64_t kRhsOperand = 1;

  PartialProductOp(const SourceInfo& loc, Node* lhs, Node* rhs, int64_t width,
                   Op op, std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  int64_t width() const { return width_; }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  int64_t width_;
};

class PrioritySelect final : public Node {
 public:
  static constexpr int64_t kSelectorOperand = 0;

  PrioritySelect(const SourceInfo& loc, Node* selector,
                 absl::Span<Node* const> cases, Node* default_value,
                 std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  Node* selector() const { return operand(0); }

  absl::Span<Node* const> cases() const {
    return operands().subspan(1, cases_size_);
  }

  Node* get_case(int64_t case_no) const { return cases().at(case_no); }

  Node* default_value() const { return operand(1 + cases_size_); }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  int64_t cases_size_;
};

class Receive final : public Node {
 public:
  static constexpr int64_t kTokenOperand = 0;

  Receive(const SourceInfo& loc, Node* token, std::optional<Node*> predicate,
          std::string_view channel_name, bool is_blocking,
          std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  const std::string& channel_name() const { return channel_name_; }

  bool is_blocking() const { return is_blocking_; }

  Node* token() const { return operand(0); }

  std::optional<Node*> predicate() const {
    return predicate_operand_number().ok()
               ? std::optional<Node*>(operand(*predicate_operand_number()))
               : std::nullopt;
  }

  Type* GetPayloadType() const;
  void ReplaceChannel(std::string_view new_channel_name);

  absl::StatusOr<int64_t> predicate_operand_number() const {
    if (!has_predicate_) {
      return absl::InternalError("predicate is not present");
    }
    int64_t ret = 1;

    return ret;
  }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  std::string channel_name_;
  bool is_blocking_;
  bool has_predicate_;
};

class RegisterRead final : public Node {
 public:
  RegisterRead(const SourceInfo& loc, Register* reg, std::string_view name,
               FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  Register* GetRegister() const { return reg_; }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  Register* reg_;
};

class RegisterWrite final : public Node {
 public:
  static constexpr int64_t kDataOperand = 0;

  RegisterWrite(const SourceInfo& loc, Node* data,
                std::optional<Node*> load_enable, std::optional<Node*> reset,
                Register* reg, std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  Node* data() const { return operand(0); }

  std::optional<Node*> load_enable() const {
    return load_enable_operand_number().ok()
               ? std::optional<Node*>(operand(*load_enable_operand_number()))
               : std::nullopt;
  }

  std::optional<Node*> reset() const {
    return reset_operand_number().ok()
               ? std::optional<Node*>(operand(*reset_operand_number()))
               : std::nullopt;
  }

  Register* GetRegister() const { return reg_; }

  absl::Status ReplaceExistingLoadEnable(Node* new_operand) {
    return has_load_enable_
               ? ReplaceOperandNumber(*load_enable_operand_number(),
                                      new_operand)
               : absl::InternalError(
                     "Unable to replace load enable on RegisterWrite -- "
                     "register does not have an existing load enable operand.");
  }

  absl::Status AddOrReplaceReset(Node* new_reset_node, Reset new_reset_info) {
    reg_->UpdateReset(new_reset_info);
    if (!has_reset_) {
      AddOperand(new_reset_node);
      has_reset_ = true;
      return absl::OkStatus();
    }
    return ReplaceOperandNumber(*reset_operand_number(), new_reset_node);
  }

  absl::StatusOr<int64_t> load_enable_operand_number() const {
    if (!has_load_enable_) {
      return absl::InternalError("load_enable is not present");
    }
    int64_t ret = 1;

    return ret;
  }

  absl::StatusOr<int64_t> reset_operand_number() const {
    if (!has_reset_) {
      return absl::InternalError("reset is not present");
    }
    int64_t ret = 2;

    if (!has_load_enable_) {
      ret--;
    }

    return ret;
  }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  Register* reg_;
  bool has_load_enable_;
  bool has_reset_;
};

class Select final : public Node {
 public:
  static constexpr int64_t kSelectorOperand = 0;

  Select(const SourceInfo& loc, Node* selector, absl::Span<Node* const> cases,
         std::optional<Node*> default_value, std::string_view name,
         FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  Node* selector() const { return operand(0); }

  absl::Span<Node* const> cases() const {
    return operands().subspan(1, cases_size_);
  }

  Node* get_case(int64_t case_no) const { return cases().at(case_no); }

  std::optional<Node*> default_value() const {
    return has_default_value_ ? std::optional<Node*>(operands().back())
                              : std::nullopt;
  }

  bool AllCases(const std::function<bool(Node*)>& p) const;
  Node* any_case() const {
    return !cases().empty()              ? cases().front()
           : default_value().has_value() ? default_value().value()
                                         : nullptr;
  }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  int64_t cases_size_;
  bool has_default_value_;
};

class Send final : public Node {
 public:
  static constexpr int64_t kTokenOperand = 0;
  static constexpr int64_t kDataOperand = 1;

  Send(const SourceInfo& loc, Node* token, Node* data,
       std::optional<Node*> predicate, std::string_view channel_name,
       std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  const std::string& channel_name() const { return channel_name_; }

  Node* token() const { return operand(0); }

  Node* data() const { return operand(1); }

  std::optional<Node*> predicate() const {
    return predicate_operand_number().ok()
               ? std::optional<Node*>(operand(*predicate_operand_number()))
               : std::nullopt;
  }

  void ReplaceChannel(std::string_view new_channel_name);

  absl::StatusOr<int64_t> predicate_operand_number() const {
    if (!has_predicate_) {
      return absl::InternalError("predicate is not present");
    }
    int64_t ret = 2;

    return ret;
  }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  std::string channel_name_;
  bool has_predicate_;
};

class Trace final : public Node {
 public:
  static constexpr int64_t kTokenOperand = 0;
  static constexpr int64_t kConditionOperand = 1;

  Trace(const SourceInfo& loc, Node* token, Node* condition,
        absl::Span<Node* const> args, absl::Span<FormatStep const> format,
        int64_t verbosity, std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  absl::Span<FormatStep const> format() const { return format_; }

  int64_t verbosity() const { return verbosity_; }

  Node* token() const { return operand(0); }

  Node* condition() const { return operand(1); }

  absl::Span<Node* const> args() const { return operands().subspan(2); }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  std::vector<FormatStep> format_;
  int64_t verbosity_;
};

class Tuple final : public Node {
 public:
  Tuple(const SourceInfo& loc, absl::Span<Node* const> elements,
        std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  int64_t size() const { return operand_count(); }
};

class TupleIndex final : public Node {
 public:
  static constexpr int64_t kArgOperand = 0;

  TupleIndex(const SourceInfo& loc, Node* arg, int64_t index,
             std::string_view name, FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
  int64_t index() const { return index_; }

  bool IsDefinitelyEqualTo(const Node* other) const final;

 private:
  int64_t index_;
};

class UnOp final : public Node {
 public:
  static constexpr int64_t kArgOperand = 0;

  UnOp(const SourceInfo& loc, Node* arg, Op op, std::string_view name,
       FunctionBase* function);

  absl::StatusOr<Node*> CloneInNewFunction(
      absl::Span<Node* const> new_operands,
      FunctionBase* new_function) const final;
};

}  // namespace xls

#endif  // XLS_IR_NODES_H_