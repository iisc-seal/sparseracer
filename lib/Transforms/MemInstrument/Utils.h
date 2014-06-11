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


#include <cxxabi.h>
#include <set>

using namespace llvm;
namespace MemInstrument {
  
  std::string getTypeAsString(Type* T);

  std::string getFieldName(Value *Addr);

  std::string getSourceInfoAsString(Instruction *I, std::string name);

  std::string getDirName(Instruction *I);

  bool shouldInstrumentFunction(std::string name, std::set<std::string> whiteList);

  bool shouldInstrumentDirectory(std::string name);

  std::string demangleFunctionName(std::string functionName); 

  std::map<std::string, std::string> getDebugInformation(Module &M);

}
#endif
