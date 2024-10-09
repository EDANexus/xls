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

#include <utility>
#include <vector>

#include "absl/cleanup/cleanup.h"
#include "absl/status/status.h"
#include "absl/strings/str_format.h"
#include "llvm/include/llvm/ADT/ArrayRef.h"
#include "llvm/include/llvm/ADT/DenseMap.h"
#include "llvm/include/llvm/ADT/STLExtras.h"
#include "llvm/include/llvm/ADT/SmallString.h"
#include "llvm/include/llvm/ADT/SmallVector.h"
#include "llvm/include/llvm/ADT/StringRef.h"
#include "llvm/include/llvm/ADT/StringSet.h"
#include "mlir/include/mlir/IR/Builders.h"
#include "mlir/include/mlir/IR/BuiltinAttributes.h"
#include "mlir/include/mlir/IR/BuiltinOps.h"
#include "mlir/include/mlir/IR/IRMapping.h"
#include "mlir/include/mlir/IR/PatternMatch.h"
#include "mlir/include/mlir/IR/SymbolTable.h"
#include "mlir/include/mlir/IR/Value.h"
#include "mlir/include/mlir/IR/Visitors.h"
#include "mlir/include/mlir/Support/LLVM.h"
#include "xls/common/status/status_macros.h"
#include "xls/contrib/mlir/IR/xls_ops.h"
#include "xls/contrib/mlir/transforms/passes.h"  // IWYU pragma: keep
#include "xls/contrib/mlir/util/interpreter.h"

namespace mlir::xls {

#define GEN_PASS_DEF_PROCELABORATIONPASS
#include "xls/contrib/mlir/transforms/passes.h.inc"

namespace {

using llvm::ArrayRef;
using llvm::SmallString;
using llvm::StringRef;
using llvm::StringSet;

// Replaces all structured channel ops in a region with the corresponding
// unstructured channel op (ssend -> send, etc). The channel map is a mapping
// from the channel value in the structured op to the symbol ref of the
// corresponding chan op.
void replaceStructuredChannelOps(Region& region,
                                 llvm::DenseMap<Value, SymbolRefAttr> chanMap) {
  mlir::IRRewriter rewriter(region.getContext());
  region.walk([&](SBlockingReceiveOp srecv) {
    rewriter.setInsertionPoint(srecv);
    rewriter.replaceOpWithNewOp<BlockingReceiveOp>(
        srecv, srecv->getResultTypes(), srecv.getTkn(), srecv.getPredicate(),
        chanMap[srecv.getChannel()]);
  });
  region.walk([&](SNonblockingReceiveOp srecv) {
    rewriter.setInsertionPoint(srecv);
    rewriter.replaceOpWithNewOp<NonblockingReceiveOp>(
        srecv, srecv->getResultTypes(), srecv.getTkn(), srecv.getPredicate(),
        chanMap[srecv.getChannel()]);
  });
  region.walk([&](SSendOp ssend) {
    rewriter.setInsertionPoint(ssend);
    rewriter.replaceOpWithNewOp<SendOp>(ssend, ssend.getTkn(), ssend.getData(),
                                        ssend.getPredicate(),
                                        chanMap[ssend.getChannel()]);
  });
}

class ProcElaborationPass
    : public impl::ProcElaborationPassBase<ProcElaborationPass> {
 public:
  void runOnOperation() override;
};

class ElaborationContext
    : public InterpreterContext<ElaborationContext, ChanOp> {
 public:
  explicit ElaborationContext(OpBuilder& builder, ModuleOp module)
      : builder(builder), symbolTable(module) {}

  OpBuilder& getBuilder() { return builder; }

  StringAttr makeUniqueSymbol(StringRef name) {
    auto existsAlready = [this](StringRef name) {
      return symbolTable.lookup(name) != nullptr ||
             addedSymbols.contains(name.str());
    };
    if (!existsAlready(name)) {
      addedSymbols.insert(name.str());
      return builder.getStringAttr(name);
    }

    unsigned counter = 0;
    // Note the template parameter is the length of the returned SmallString,
    // not the number of probes to do!
    SmallString<32> str =
        symbolTable.generateSymbolName<32>(name, existsAlready, counter);
    addedSymbols.insert(str.str().str());
    return builder.getStringAttr(str);
  }

  void createEproc(SprocOp sproc, std::vector<value_type> channels) {
    StringAttr symbol = makeUniqueSymbol(sproc.getSymName());

    EprocOp eproc = builder.create<EprocOp>(sproc.getLoc(), symbol);

    IRMapping mapping;
    sproc.getNext().cloneInto(&eproc.getBody(), mapping);
    llvm::DenseMap<Value, SymbolRefAttr> chanMap;
    for (auto [arg, chan] : llvm::zip(sproc.getNextChannels(), channels)) {
      chanMap[mapping.lookup(arg)] = SymbolRefAttr::get(chan.getSymNameAttr());
    }
    replaceStructuredChannelOps(eproc.getBody(), chanMap);
    eproc.getBody().front().eraseArguments(0, sproc.getNextChannels().size());
  }

 private:
  OpBuilder& builder;
  SymbolTable symbolTable;
  StringSet<> addedSymbols;
};

class ElaborationInterpreter
    : public Interpreter<ElaborationInterpreter, ElaborationContext, ChanOp,
                         SchanOp, YieldOp, SpawnOp> {
 public:
  using Interpreter::Interpret;

  template <typename... T>
  absl::Status InterpretTop(SprocOp sproc,
                            ArrayRef<ChanOp> boundaryChannels = {},
                            T&&... contextArgs) {  // NOLINT
    ElaborationContext ctx(std::forward<T>(contextArgs)...);
    ctx.PushLiveness(GetOrCreateLiveness(sproc));
    return Interpret(sproc, ctx, boundaryChannels);
  }

  absl::Status Interpret(SchanOp op, ElaborationContext& ctx) {
    StringAttr symbol = ctx.makeUniqueSymbol(op.getName());
    ChanOp chan =
        ctx.getBuilder().create<ChanOp>(op.getLoc(), symbol, op.getType());
    ctx.Set(op.getResult(0), chan);
    ctx.Set(op.getResult(1), chan);

    return absl::OkStatus();
  }

  absl::Status Interpret(YieldOp, ElaborationContext&) {
    return absl::OkStatus();
  }

  absl::Status Interpret(SpawnOp op, ElaborationContext& ctx) {
    SprocOp sproc = op.resolveCallee();
    if (!sproc) {
      return absl::InvalidArgumentError("failed to resolve callee");
    }

    ctx.PushLiveness(GetOrCreateLiveness(sproc));
    absl::Cleanup popper = [&] { ctx.PopLiveness(); };

    XLS_ASSIGN_OR_RETURN(auto arguments, ctx.Get(op.getChannels()));
    if (arguments.size() != sproc.getChannelArguments().size()) {
      return absl::InternalError(absl::StrFormat(
          "Call to %s requires %d arguments but got %d",
          op.getCallee().getLeafReference().str(),
          sproc.getChannelArguments().size(), arguments.size()));
    }
    XLS_ASSIGN_OR_RETURN(auto results,
                         Interpret(sproc.getSpawns(), arguments, ctx));
    ctx.createEproc(sproc, results);
    return absl::OkStatus();
  }

  absl::Status Interpret(SprocOp op, ElaborationContext& ctx,
                         ArrayRef<ChanOp> boundaryChannels = {}) {
    XLS_ASSIGN_OR_RETURN(auto results,
                         Interpret(op.getSpawns(), boundaryChannels, ctx));
    ctx.createEproc(op, results);
    return absl::OkStatus();
  }
};

}  // namespace

void ProcElaborationPass::runOnOperation() {
  ModuleOp module = getOperation();
  // Elaborate all sprocs marked "top". Elaboration traverses a potentially
  // cyclical graph of sprocs, so we delay removing the sprocs until the end.
  for (auto sproc : module.getOps<SprocOp>()) {
    if (!sproc.getIsTop()) {
      continue;
    }

    OpBuilder builder(sproc);
    SmallVector<ChanOp> boundaryChannels;
    if (sproc.getBoundaryChannelNames().has_value()) {
      for (auto [arg, name] : llvm::zip(sproc.getChannelArguments(),
                                        *sproc.getBoundaryChannelNames())) {
        SchanType schan = cast<SchanType>(arg.getType());
        auto nameAttr = cast<StringAttr>(name);
        auto echan = builder.create<ChanOp>(sproc.getLoc(), nameAttr,
                                            schan.getElementType());
        if (schan.getIsInput()) {
          echan.setSendSupported(false);
        } else {
          echan.setRecvSupported(false);
        }
        boundaryChannels.push_back(echan);
      }
    }

    ElaborationInterpreter interpreter;
    auto result =
        interpreter.InterpretTop(sproc, boundaryChannels, builder, module);
    if (!result.ok()) {
      sproc.emitError() << "failed to elaborate: " << result.message();
    }
  }
  module.walk([&](SprocOp sproc) { sproc.erase(); });
}

}  // namespace mlir::xls
