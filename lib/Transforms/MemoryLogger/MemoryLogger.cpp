//===- MemoryLogger.cpp - Instrument all loads and stores in a program ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements two versions of the LLVM "Hello World" pass described
// in docs/WritingAnLLVMPass.html
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "hello"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"

#include <algorithm>

using namespace llvm;

STATISTIC(HelloCounter, "Counts number of functions greeted");

namespace {
  // Hello - The first implementation, without getAnalysisUsage.
  struct MemoryLog : public FunctionPass {
    static char ID; // Pass identification, replacement for typeid
    MemoryLog() : FunctionPass(ID) {}

    virtual bool runOnFunction(Function &F) {
      for(Function::iterator BB = F.begin(); BB != F.end(); ++BB) {
	for(BasicBlock::iterator II = BB->begin(); II != BB->end(); ++II) {
	  errs() << "==========***===========\n" <<  *II << "\t" << II->getOpcodeName() <<  "\n";
	  
	  if (StoreInst * SI = dyn_cast<StoreInst>(II)) {
	    SI->dump();
	    SI->getValueOperand()->dump();
	    errs() << "\n";
	    const Value *Addr = SI->getPointerOperand();
	    Addr->dump();
	    Type *OrigPtrTy = Addr->getType();
	    OrigPtrTy->dump();
	    Type *OrigTy = cast<PointerType>(OrigPtrTy)->getElementType();
	    OrigTy->dump();
	    IRBuilder<> IRB(II);
	    // Value *AddrLong = IRB.CreatePointerCast(Addr, IntptrTy);
	    errs() << "\n";
	  }
	
	  if (CallInst * CI = dyn_cast<CallInst>(II)) {
	    if (Function * CalledFunc = CI->getCalledFunction()) {
	      errs() << "Call Instruction found!" ;
	      CalledFunc->dump();
	      std::string name = CalledFunc->getName();
	      std::string delPrefix("_Zdl"); 
	      errs() << name <<"\n";
	      std::pair<std::string::const_iterator, std::string::const_iterator> res;
	      res = std::mismatch(delPrefix.begin(), delPrefix.end(), name.begin());
	      if (res.first == delPrefix.end()) {
		errs() << "Delete Instruction found!\n" ;
		if(BitCastInst *castInstr = dyn_cast<BitCastInst>(CI->getOperand(0))){
		  errs() << "\n" ;
		  LoadInst *load = dyn_cast<LoadInst>(castInstr->stripPointerCasts());
		  if(load != NULL) {
		    load->getPointerOperand()->getType()->dump();
		  }
		  errs() << "\n";
		}
		errs() << "\n";
	      }
	    }
	  }

	  if(BitCastInst *castInstr = dyn_cast<BitCastInst>(II)){
	    errs() << "Cast Instruction found!" ;
	    LoadInst *load = dyn_cast<LoadInst>(castInstr->stripPointerCasts());
	    if(load != NULL) {
	      load->getPointerOperand()->getType()->dump();
	    }
	    errs() << "\n";
	  }

	}
      }
      return false;
    }
  };
}

char MemoryLog::ID = 0;
static RegisterPass<MemoryLog> X("memlog", "Log reads and writes");

// namespace {
//   // Hello2 - The second implementation with getAnalysisUsage implemented.
//   struct Hello2 : public FunctionPass {
//     static char ID; // Pass identification, replacement for typeid
//     Hello2() : FunctionPass(ID) {}

//     virtual bool runOnFunction(Function &F) {
//       ++HelloCounter;
//       errs() << "Hello: ";
//       errs().write_escaped(F.getName()) << '\n';
//       return false;
//     }

//     // We don't modify the program, so we preserve all analyses
//     virtual void getAnalysisUsage(AnalysisUsage &AU) const {
//       AU.setPreservesAll();
//     }
//   };
// }

// char Hello2::ID = 0;
// static RegisterPass<Hello2>
// Y("hello2", "Hello World Pass (with getAnalysisUsage implemented)");
