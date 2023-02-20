//===- ConvertRoot.cpp - Slang AST to MLIR conversion ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ImportVerilogInternals.h"
#include "circt/Dialect/LLHD/IR/LLHDOps.h"

using namespace circt;
using namespace slang::syntax;
using namespace ImportVerilog;

using slang::SourceLocation;
using ConvertLocationFn = std::function<Location(SourceLocation)>;

namespace {
class RootConverter : public SyntaxVisitor<RootConverter> {
public:
  ModuleOp module;
  OpBuilder builder;
  ConvertLocationFn convertLocation;
  LogicalResult result = success();

  RootConverter(ModuleOp module, ConvertLocationFn convertLocation)
      : module(module), builder(module->getContext()),
        convertLocation(convertLocation) {}

  void handle(const ModuleDeclarationSyntax &decl);

  llhd::EntityOp handle(ModuleHeaderSyntax *header);
};
} // namespace

void RootConverter::handle(const ModuleDeclarationSyntax &decl) {
  builder.setInsertionPointToEnd(module.getBody());

  auto entityOp = handle(decl.header);
  if (!entityOp)
    return;
}

llhd::EntityOp RootConverter::handle(ModuleHeaderSyntax *header) {
  auto location = convertLocation(header->name.location());
  auto entityType = builder.getFunctionType({}, {});
  size_t ins = 0;

  auto entity = builder.create<llhd::EntityOp>(location, entityType, ins,
                                               ArrayAttr(), ArrayAttr());

  entity.setName(header->name.valueText());
  entity.addEntryBlock();
  return entity;
}

LogicalResult circt::ImportVerilog::convertSyntaxTree(
    slang::syntax::SyntaxTree &syntax, ModuleOp module,
    llvm::function_ref<StringRef(slang::BufferID)> getBufferFilePath) {
  RootConverter visitor(module, [&](SourceLocation loc) {
    return convertLocation(module.getContext(), syntax.sourceManager(),
                           getBufferFilePath, loc);
  });
  syntax.root().visit(visitor);
  return visitor.result;
}
