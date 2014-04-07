#include "Utils.h"

namespace MemInstrument {

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

  std::string getSourceInfoAsString(Instruction *I, std::string name){
    if (MDNode *N = I->getMetadata("dbg")) {  // Here I is an LLVM instruction
      DILocation Loc(N);                      // DILocation is in DebugInfo.h
      std::string Line = std::to_string(Loc.getLineNumber());
      std::string File = Loc.getFilename().str();
      std::string Dir = Loc.getDirectory().str();
      return (Dir + "/" + File + ":" + Line + "(" + name + ")");
    }
    return name;
  }

  bool shouldInstrument(std::string name, std::set<std::string> whiteList){
    if(whiteList.find(name) != whiteList.end())
      return true;
    return false;
  }

  std::string demangleFunctionName(std::string func) {
    std::string ret(func);
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
      size_t endpos = demangled.find("(");

      // Templated functions will have type information first, so skip to the                           
      // first space.                                                                                   
      size_t startpos = demangled.find(" ");
      if(startpos < endpos) {
	// skip until after the space                                                                   
	++startpos;
	// also modify endpos to the first '<' to remove template info                                  
	endpos = demangled.find("<") - startpos;
      } else {
	// regular C++ function, no template info to remove                                             
	startpos = 0;
      }

      ret = demangled.substr(startpos,endpos);
    }

    return ret;
  }

}
