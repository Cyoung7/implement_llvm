#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <iostream>

static llvm::LLVMContext theContext;
static llvm::IRBuilder<> builder(theContext);
static std::unique_ptr<llvm::Module> theModule;
static std::map<std::string,llvm::Value *> nameValues;

int main(){
    theModule = llvm::make_unique<llvm::Module>("hello,llvm",theContext);
    theModule->dump();
    std::cout << "hello" << std ::endl;
    return 0;

}