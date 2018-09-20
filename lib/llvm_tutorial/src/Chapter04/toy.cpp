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
#include <utility> #include <vector>
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


//=========----------------------------========//
// AST
//=========----------------------------========//

namespace {
    class ExprAST{
    public:
        //virtual:基类的指针可以执行派生类,virtual告诉编译器调用派生类的相应函数
        virtual ~ExprAST() = default;

        virtual Value *codegen() = 0;
    };

    //数值节点
    class NumberExprAST:public ExprAST{
        double Val;
    public:
        NumberExprAST(double Val):Val(Val){}

        Value *codegen() override ;
    };

    //变量节点
    class VariableExprAST:public ExprAST{
        std::string Name;
    public:
        VariableExprAST(std::string name):Name(std::move(name)){}

        Value *codegen() override ;
    };

    //二元表达式节点
    class BinaryExprAST:public ExprAST{
        char Op;
        std::unique_ptr<ExprAST> LHS,RHS;

    public:
        BinaryExprAST(char op,std::unique_ptr<ExprAST>lsh,std::unique_ptr<ExprAST>rsh)
                :Op(op),LHS(std::move(lsh)),RHS(std::move(rsh)){}

        Value *codegen() override;

    };


    //函数调用节点
    class CallExprAST : public ExprAST {
        std::string Callee;
        //每一个参数可以是一个表达式,所以这里参数是一系列节点
        std::vector<std::unique_ptr<ExprAST>> Args;

    public:
        CallExprAST(std::string Callee,
                    std::vector<std::unique_ptr<ExprAST>> Args)
                : Callee(std::move(Callee)), Args(std::move(Args)) {}

        Value *codegen() override;
    };

    //函数声明节点

    class PrototypeAST {
        std::string Name;
        //每个参数就是一个变量名
        std::vector<std::string> Args;

    public:
        PrototypeAST(std::string Name, std::vector<std::string> Args)
                : Name(std::move(Name)), Args(std::move(Args)) {}

        Function *codegen();
        const std::string &getName() const { return Name; }
    };

    //函数定义节点
    class FunctionAST {
        std::unique_ptr<PrototypeAST> Proto;
        //函数体就是一个表达式
        std::unique_ptr<ExprAST> Body;
    public:
        FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                    std::unique_ptr<ExprAST> Body)
                : Proto(std::move(Proto)), Body(std::move(Body)) {}

        Function *codegen();
    };
}


//==========-------------------------======================//
// 解析
//==========-------------------------======================//

//但前标识符的标记
static int CurTok;
//读入下一个标识符,也可以理解为吃掉当前标识符
static int getNextToken(){
    return CurTok = gettok();
}

//二元操作符的优先级
static std::map<char, int> BinopPrecedence;

static int GetTokPrecedence(){
    //该标识符的标记不再ascii范围内,一定是上面几种特殊标识符
    if(!isascii(CurTok)){
        return -1;
    }

    int TokPrec = BinopPrecedence[CurTok];
    //没有该二元操作符
    if (TokPrec <= 0)
        return -1;
    return TokPrec;
}

std::unique_ptr<ExprAST> LogError(const char *str){
    fprintf(stderr, "Error: %s\n", str);
    return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
    LogError(Str);
    return nullptr;
}


//声明解析表达式的函数
static std::unique_ptr<ExprAST> ParseExpression();

//数值解析
static std::unique_ptr<ExprAST> ParseNumberExpr(){

//    std::unique_ptr<NumberExprAST> result = llvm::make_unique<NumberExprAST>(NumVal);
    //auto:根据变量初始值的类型自动为此变量选择匹配的类型,必须在定义是初始化
    auto result = llvm::make_unique<NumberExprAST>(NumVal);
    //读入下一个标识符
    getNextToken();
    //这里是创建一个result节点,然后将result的指针返回
    return std::move(result);
}

//解析()中的表达式
static std::unique_ptr<ExprAST> ParseParentExpr(){
    //吃掉 (
    getNextToken();
    //在解析表达式时,统一标准是要吃掉表达式的所有标识符,并移动到下一个标识符
    auto V = ParseExpression();
    if(!V){
        return nullptr;
    }
    //这里的下一个标识符必须是 )
    if(CurTok != ')'){
        return LogError("expected ')'");
    }
    return V;
}

//解析一个标识符,后边如果有(),就是函数函数调用
static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
    //当前读入的标识符
    std::string IdName = IdentifierStr;
    //吃掉标识符
    getNextToken();

    //简单的变量标识符
    if (CurTok != '('){
        return std::move(llvm::make_unique<VariableExprAST>(IdName));
    }

    //函数调用
    //吃掉 (
    getNextToken();
    std::vector<std::unique_ptr<ExprAST>> Args;

    //函数带参数
    if(CurTok != ')'){
        while (true){
            if (auto Arg = ParseExpression()){
                Args.push_back(std::move(Arg));
            } else{
                return nullptr;
            }

            //所有参数解析完毕
            if(CurTok == ')'){
                break;
            }
            //表达式解析之后既不是,又不是) ,输入有误
            if (CurTok != ','){
                return LogError("Expected ')' or ',' in argument list");
            }
            //吃掉,
            getNextToken();
        }
    }
    //吃掉 )
    getNextToken();
    return llvm::make_unique<CallExprAST>(IdName,std::move(Args));
}

//解析主表达式,作为解析各种表达式的入口
static std::unique_ptr<ExprAST> ParsePrimary() {
    switch (CurTok){
        default:
            return LogError("unknown token when expecting an expression");
        //当前是一个标识符,其可能是一个普通标识符,也可能是一个函数调用标识符
        case tok_identifier:
            return ParseIdentifierExpr();
        case tok_number:
            return ParseNumberExpr();
        case '(':
            return ParseParentExpr();
    }
}

//解析二元操作符,形式 (op + 主表达式)*

static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                              std::unique_ptr<ExprAST>LHS){
    while (true){
        int TokPrec = GetTokPrecedence();
        //当前op优先级小于之前所有op的优先级,返回当前op之前的表达式
        if(TokPrec < ExprPrec){
            return LHS;
        }
        //拿到当前操作符
        int BinOp = CurTok;
        getNextToken();

        //解析op右边的主表达式
        auto RHS = ParsePrimary();

        if (!RHS){
            return nullptr;
        }
        //到此CurTok已经来到下一个op
        //拿到下一个op的优先级
        int NextPrec = GetTokPrecedence();
        if(TokPrec < NextPrec){
            //进入递归，
            RHS = ParseBinOpRHS(TokPrec+1,std::move(RHS));
            if (!RHS){
                return nullptr;
            }
        }

        //合并LHS/RHS
        LHS = llvm::make_unique<BinaryExprAST>(BinOp,std::move(LHS),std::move(RHS));

    }

}


//解析整表达式
static std::unique_ptr<ExprAST> ParseExpression() {
    auto LHS = ParsePrimary();
    if(!LHS){
        return nullptr;
    }
    return ParseBinOpRHS(0,std::move(LHS));
}

//解析函数原型
static std::unique_ptr<PrototypeAST> ParsePrototype(){
    if(CurTok != tok_identifier){
        return LogErrorP("Expected function name in prototype");
    }
    std::string FnName = IdentifierStr;
    //吃掉函数名
    getNextToken();

    if (CurTok != '(')
        return LogErrorP("Expected '(' in prototype");
    std::vector<std::string> ArgNames;
    //这里为了简单起见，参数之间以空格间隔，而不是,
    while (getNextToken() == tok_identifier)
        ArgNames.push_back(IdentifierStr);

    if (CurTok != ')')
        return LogErrorP("Expected ')' in prototype");

    // 吃掉)
    getNextToken();

    return llvm::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

//函数定义解析
static std::unique_ptr<FunctionAST> ParseDefinition() {
    //吃掉def
    getNextToken();
    auto Proto = ParsePrototype();
    if(!Proto){
        return nullptr;
    }
    if (auto E = ParseExpression()){
        return llvm::make_unique<FunctionAST>(std::move(Proto),std::move(E));
    }
    return nullptr;
}



static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
    if (auto E = ParseExpression()) {
        // Make an anonymous proto.
        auto Proto = llvm::make_unique<PrototypeAST>("__anon_expr",
                                                     std::vector<std::string>());
        return llvm::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}

static std::unique_ptr<PrototypeAST> ParseExtern() {
    getNextToken(); // eat extern.
    return ParsePrototype();
}


