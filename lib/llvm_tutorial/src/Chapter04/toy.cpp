//
// Created by cyoung on 18-9-17.
//

#include "../../include/KaleidoscopeJIT.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <llvm/ExecutionEngine/Orc/Core.h>


using namespace llvm;
using namespace llvm::orc;

//====--------------------------------------------------------====//
// Lexer
//====--------------------------------------------------------====//
//其他字符返回其对应的ascii
enum Token{
    tok_eof = -1,
    tok_def = -2,
    tok_extern = -3,
    tok_identifier = -4,
    tok_number = -5
};

static std::string IdentifierStr;
static double  NumVal;

static int gettok(){
    //首先将读取的char设为space
    static int LastChar = ' ';
    //跳过所有的whitespace
    while (isspace(LastChar)){
        LastChar = getchar();
    }
    //跳出循环已经读取了非空格的第一个字符

    //识别identifier(标识符),首字母不能为数字
    if(isalpha(LastChar)){
        IdentifierStr = LastChar;
        while (isalnum((LastChar = getchar()))){
            IdentifierStr += LastChar;
        }

        if(IdentifierStr == "def"){
            return tok_def;
        }
        if(IdentifierStr == "extern"){
            return tok_extern;
        }
        return tok_identifier;
    }

    //识别数字,添加flag,只能出现一个小数点
    if(isdigit(LastChar) || LastChar == '.'){
        bool flag = true;
        std::string NumStr;
        do{
            if(LastChar == '.'){
                flag = false;
            }
            NumStr += LastChar;
            LastChar = getchar();
        }while (isdigit(LastChar) || (LastChar=='.' && flag));

        //将字符串转换为浮点数,
        //string.c_str()返回指向字符串的指针
        NumVal = strtod(NumStr.c_str(), nullptr);
        return tok_number;
    }

    //识别注释,跳过本行注释,进行下一行有效字符的识别
    if(LastChar == '#'){
        do{
            LastChar = getchar();
        }while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if(LastChar != EOF){
            return gettok();
        }
    }

    //文件结尾
    if(LastChar == EOF){
        return tok_eof;
    }

    //否则返回该字符的ascii码
    int ThisChar = LastChar;
    //这里是什么意思?需要读入下一个字符
    LastChar = getchar();
    return ThisChar;
}
