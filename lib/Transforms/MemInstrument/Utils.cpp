#include "Utils.h"
#include <cstdlib>
#include <set>
#include <sstream>

namespace MemInstrument {

  std::set<std::string> FunctionBlacklist = {
    "_Znwj,"               // new(unsigned int)
    "_ZnwjRKSt9nothrow_t", // new(unsigned int, nothrow)
    "_Znwm",               // new(unsigned long)
    "_ZnwmRKSt9nothrow_t", // new(unsigned long, nothrow)
    "_Znaj",               // new[](unsigned int)
    "_ZnajRKSt9nothrow_t", // new[](unsigned int, nothrow)
    "_Znam",               // new[](unsigned long)
    "_ZnamRKSt9nothrow_t",  // new[](unsigned long, nothrow)
    "malloc",
    "valloc",
    "realloc",
    "calloc",
    "_ZdlPv", // operator delete(void*)
    "_ZdaPv", // operator delete[](void*)
    "free",
    "_ZdlPvRKSt9nothrow_t",  // delete(void*, nothrow)
    "ZdaPvRKSt9nothrow_t",   // delete[](void*, nothrow)
    "syslog",
    "rInstrument",
    "dispatchInstrument",
    "mopInstrument",
    "fInstrument",
    "mopDealloc",
    "mopAlloc",
    "utrie_get32_52",
    "_Z15NS_IsMainThreadv",
    "_ZN8JSString8rootKindEv",
    "_ZN7mozilla6detail7BarrierILNS_14MemoryOrderingE2EE9afterLoadEv",
    "_ZN7mozilla6detail7BarrierILNS_14MemoryOrderingE2EE10beforeLoadEv",
    "_ZN11nsXPConnect9XPConnectEv",
    "_ZN8JSObject8rootKindEv",
    "_ZN12XPCJSRuntime3GetEv",
    "_ZN7mozilla6detail7BarrierILNS_14MemoryOrderingE1EE9afterLoadEv",
    "_ZN7mozilla6detail7BarrierILNS_14MemoryOrderingE1EE10beforeLoadEv",
    "_ZN7mozilla6detail7BarrierILNS_14MemoryOrderingE2EE9afterLoadEv",
    "_ZN7mozilla6detail7BarrierILNS_14MemoryOrderingE2EE10beforeLoadEv",
    "mozPoisonValue",
    "_ZN7mozilla3dom17FragmentOrElement15cycleCollection14GetParticipantEv",
    "_ZN8JSString8rootKindEv",
    "_ZN20nsGenericDOMDataNode15cycleCollection14GetParticipantEv",
    "_ZN12nsXULElement15cycleCollection14GetParticipantEv",
    "_ZN7mozilla6detail7BarrierILNS_14MemoryOrderingE0EE9afterLoadEv",
    "_ZN7mozilla6detail7BarrierILNS_14MemoryOrderingE0EE10beforeLoadEv",
    "_ZN10nsNodeInfo15cycleCollection14GetParticipantEv",
    "_ZN8JSString8rootKindEv"
  };

  bool endsWith(const std::string &str, const std::string &suffix){
    return str.size() >= suffix.size() &&
      str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
  }

  bool startsWith(const std::string &bigString, const std::string &smallString){
    return (bigString.compare(0, smallString.length(), smallString) == 0);
  }

  bool isPointerToPointer(const Value* V) {
    const Type* T = V->getType();
    return T->isPointerTy() && T->getContainedType(0)->isPointerTy();
  }

  std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
      elems.push_back(item);
    }
    return elems;
  }

  std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
  }

  std::string getTypeAsString(Type* T){
    std::string type_str;
    llvm::raw_string_ostream rso(type_str);
    T->print(rso);
    return rso.str();
  }

  std::string getFieldName(Value *Addr){
    if (GEPOperator *GEP = dyn_cast<GEPOperator>(Addr))
      return getTypeAsString(GEP->getPointerOperand()->getType());

    return "";
  }

  std::string getDirName(Instruction *I){
    if (MDNode *N = I->getMetadata("dbg")) {  // Here I is an LLVM instruction
      DILocation Loc(N);                      // DILocation is in DebugInfo.h
      //std::string File = Loc.getFilename().str();
      std::string Dir = Loc.getDirectory().str();
      return Dir;
    }
    return "";
  }

  std::string getFileName(Instruction *I){
    if (MDNode *N = I->getMetadata("dbg")) {  // Here I is an LLVM instruction
      DILocation Loc(N);                      // DILocation is in DebugInfo.h
      //std::string File = Loc.getFilename().str();
      std::string fileName = Loc.getFilename().str();
      return fileName;
    }
    return "";
  }

  std::string getSourceInfoAsString(Instruction *I, std::string name){
    if (MDNode *N = I->getMetadata("dbg")) {  // Here I is an LLVM instruction
      DILocation Loc(N);                      // DILocation is in DebugInfo.h
      std::string Line = std::to_string(Loc.getLineNumber());
      std::string File = Loc.getFilename().str();
      std::string Dir = Loc.getDirectory().str();
      return (Dir + ':' + File + ":" + Line + "(" + name + ")");
    }
    return name;
  }

  bool shouldInstrumentFunction(std::string name, std::set<std::string> whiteList){
    if(whiteList.find(name) != whiteList.end())
      return true;
    return false;
  }

  bool shouldInstrumentFunction(Function* F, std::string name, std::set<std::string> skippedDirs){
    if(FunctionBlacklist.find(name) != FunctionBlacklist.end()){
      llvm::outs() << "Skipping " << name << "\n";
      return false;
    }

    if(name.find("__cxx") != std::string::npos || name.find("PR_") != std::string::npos){
      llvm::outs() << "Skipping " << name << "\n";
      return false;
    }
    //    char *funcs = getenv("INSTRUMENTFUNCS");
    BasicBlock &Entry = F->getEntryBlock();
    Instruction *First = Entry.begin();
    std::string location = ""; // = getSourceInfoAsString(First, "");
    for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I){
      location = getSourceInfoAsString(&*I, "");
      if(location != "")
	break;
    }

    bool found = false;
    for (std::set<std::string>::iterator it = skippedDirs.begin(); it != skippedDirs.end(); ++it){
      if(location.find(*it) != std::string::npos){
	found= true;
	break;
      }
    }
    if(found){
      llvm::outs() << "Skipping " << name << "\n";
      return false;
    }
    
    // Avoid 'framework' directories
    // if(location.find("nsprpub") != std::string::npos ||
    //    location.find("xpcom") != std::string::npos ||
    //    location.find("mfbt") != std::string::npos ||
    //    location.find("mozglue") != std::string::npos ||
    //    location.find("js/") != std::string::npos ||
    //    location.find("db/") != std::string::npos ||
    //    location.find("memory/") != std::string::npos ||
    //    location.find("ipc") != std::string::npos ||
    //    location.find("gfx/") != std::string::npos ||
    //    location.find("layout/style") != std::string::npos ||
    //    location.find("security") != std::string::npos
    //    ){
    //   llvm::outs() << "Skipping " << name << "\n";
    //   return false;
    // }
      
    if(location == ""){
      llvm::outs() << "No location for " << name << "\n";
      //return false;
    }

    if(name.find("PR_") != std::string::npos)
      llvm::outs() << "Missed" << name << " at " << location << "\n";
    
    llvm::outs() << "Instrumenting " << name << " \n";
    return true;
    // if(funcs == NULL)
    //   return false;
    // std::string instrFuncs(funcs);
    // std::vector<std::string> wList = split(instrFuncs, ':');

    // for(std::vector<std::string>::iterator it = wList.begin(); it != wList.end(); ++it) {
    //   std::size_t found = name.find(*it);
    //   if (found != std::string::npos){
    // 	return true;
    //   }
    // }
    // return false;
  }

  bool shouldInstrumentDirectory(std::string name){    
    char *dirs = getenv("INSTRUMENTDIRS");
    if(dirs == NULL)
      return false;
    std::string instrDirs(dirs);
    std::vector<std::string> wList = split(instrDirs, ':');

    for(std::vector<std::string>::iterator it = wList.begin(); it != wList.end(); ++it) {
      std::size_t found = name.find(*it);
      if (found != std::string::npos){
	return true;
      }
    }
    
    return false;
  }

  bool shouldInstrumentFile(std::string name){
    char *files = getenv("INSTRUMENTFILES");
    if(files == NULL)
      return false;
    std::string instrFiles(files);
    std::vector<std::string> wList = split(instrFiles, ':');

    for(std::vector<std::string>::iterator it = wList.begin(); it != wList.end(); ++it) {
      std::size_t found = name.find(*it);
      // llvm::outs() << name << " : " << *it << "\n";
      if (found != std::string::npos){
	// llvm::outs() << "Instrumenting " << name << "\n";
	return true;
      }
    }
    
    return false;
  }

  std::map<std::string, std::string> getDebugInformation(Module &M){
    DebugInfoFinder Finder;
    std::map<std::string, std::string> funcToDebugInfo;
    Finder.processModule(M);
    for (DebugInfoFinder::iterator i = Finder.subprogram_begin(),
	   e = Finder.subprogram_end();
	 i != e; ++i) {
      DISubprogram S(*i);
      Function *F = S.getFunction();
      if(F){
	for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I){
	  if (MDNode *N = I->getMetadata("dbg")) {
	    DILocation Loc(N);                     
	    std::string Dir = Loc.getDirectory().str();
	    funcToDebugInfo[F->getName().str()] = Dir;
	    break;
	  }
	}
      }
    }
    return funcToDebugInfo;
  }

  std::string demangleFunctionName(std::string func) {
    // std::string ret(func);
    // Check for name mangling. C++ functions will always start with _Z                                 
    // Demangled form is processed to remove type information.                                          

    if(func.length() >= 2 && func[0] == '_' && func[1] == 'Z') {
      int stat;

      char *test = abi::__cxa_demangle(func.c_str(), NULL, NULL, &stat);
      if(NULL == test)
	return "Demangling Failed on " + func;


      std::string demangled = test;
      free(test);

      // Select up to the first ( to only insert function name                                          
      // size_t endpos = demangled.find("(");

      // Templated functions will have type information first, so skip to the                           
      // first space.                                                                                   
      // size_t startpos = demangled.find(" ");
      // if(startpos < endpos) {
      // 	// skip until after the space                                                                   
      // 	++startpos;
      // 	// also modify endpos to the first '<' to remove template info                                  
      // 	endpos = demangled.find("<") - startpos;
      // } else {
      // 	// regular C++ function, no template info to remove                                             
      // 	startpos = 0;
      // }

      //return demangled.substr(startpos,endpos);
      return demangled;
    }

    return func;
  }

}
