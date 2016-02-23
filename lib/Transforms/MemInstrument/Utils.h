#ifndef MEMINSTRUMENT_UTILS_H
#define MEMINSTRUMENT_UTILS_H

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/CallSite.h"

#include <cxxabi.h>
#include <set>

using namespace llvm;
namespace MemInstrument {

  bool isPointerToPointer(const Value* V);

  bool endsWith(const std::string &str, const std::string &suffix);

  bool startsWith(const std::string &bigString, const std::string &smallString);
  
  std::string getTypeAsString(Type* T);

  std::string getFieldName(Value *Addr);

  std::string getSourceInfoAsString(Instruction *I, std::string name);

  std::string getDirName(Instruction *I);

  std::string getFileName(Instruction *I);

  bool shouldInstrumentFunction(std::string name, std::set<std::string> whiteList);

  bool shouldInstrumentFunction(Function* F, std::string name, std::set<std::string> skippedDirs);

  bool shouldInstrumentDirectory(std::string name);

  bool shouldInstrumentFile(std::string name);

  std::string demangleFunctionName(std::string functionName); 

  std::map<std::string, std::string> getDebugInformation(Module &M);

  bool isCallSite(const llvm::Instruction* inst);

  llvm::CallSite getLLVMCallSite(const llvm::Instruction* inst);

  inline const llvm::Function* getCallee(const llvm::CallSite cs);
  
  inline const llvm::Function* getCalleeDirect(const llvm::CallSite cs);

  const llvm::StringRef getCalleeName(const llvm::CallSite cs);


}
#endif
