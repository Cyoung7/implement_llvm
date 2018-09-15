# LLVM Tutorial

[原文链接](http://llvm.org/docs/tutorial/)

[TOC]

## 1.Tutorial Introduction and the Lexer

### 1.1 Tutorial Introduction

欢迎使用“使用LLVM实现语言”教程。 本教程将介绍一种简单语言的实现，展示它的乐趣和简单性。 本教程将帮助您了解并开始构建可扩展到其他语言的框架。 本教程中的代码也可以用到LLVM其他的特定场景。

本教程的目标是逐步推出我们的语言，描述它是如何随着时间的推移而构建的。 这将让我们涵盖相当广泛的语言设计和LLVM特定的使用问题，一直展示和解释它的代码，而不会让您事先需要知道大量细节。

有必要提前指出本教程实际上是专门教授编译器技术和LLVM，而不是教授现代和智能的软件工程原理。 实际上，这意味着我们将采用一些快捷方式来简化说明。 例如，代码在整个地方使用全局变量，不使用访问者等漂亮的设计模式......但它非常简单。 如果您深入挖掘并使用代码作为未来项目的基础，那么解决这些缺陷并不难。

我已经尝试将这个教程放在一起，如果你已经熟悉或者对各个部分不感兴趣，那么这些章节很容易被跳过。 本教程的结构是：

- 第1章：Kaleidoscope语言简介及其Lexer的定义 - 这显示了我们的目标以及我们希望它执行的基本功能。 为了使本教程最容易理解和掌握，我们选择用C ++实现所有内容，而不是使用词法分析器和解析器生成器。 LLVM显然可以很好地使用这些工具，如果您愿意，可以随意使用它。
- 第2章：实现解析器和AST - 使用词法分析器，我们可以讨论解析技术和基本的AST构造。 本教程描述了递归下降解析和运算符优先级解析。 第1章或第2章中没有任何内容是特定于LLVM的，此时代码甚至不在LLVM中链接.
- 第3章：LLVM IR的代码生成 - 在AST就绪的情况下，我们可以展示LLVM IR的生成是多么容易。
- 第4章：添加JIT和优化器支持 - 因为很多人都有兴趣将LLVM用作JIT，我们将直接进入它并向您展示添加JIT支持所需的3行代码。 LLVM在许多其他方面也很有用，但这是展示其强大功能的一种简单而“性感”的方式。
- 第5章：语言扩展：控制流 - 随着语言的启动和运行，我们将展示如何使用控制流操作扩展它（if / then / else和'for'循环）。 这让我们有机会谈论简单的SSA构造和控制流程。
- 第6章：语言扩展：用户定义的操作符 - 这是一个愚蠢但有趣的章节，讨论扩展语言以让用户程序定义自己的任意一元和二元运算符（具有可赋值的优先级！）。 这让我们可以构建一个重要的“语言”作为库例程。
- 第7章：语言扩展：可变变量 - 本章讨论添加用户定义的局部变量以及赋值运算符。 关于这一点的有趣部分是在LLVM中构建SSA表单是多么简单和微不足道：不，LLVM不需要您的前端构建SSA表单！
- 第8章：编译到目标文件 - 本章介绍如何获取LLVM IR并将其编译为目标文件。
- 第9章：语言扩展：调试信息 - 使用控制流，函数和可变变量构建了一个不错的编程语言，我们考虑将调试信息添加到独立可执行文件所需的内容。 此调试信息将允许您在Kaleidoscope函数中设置断点，打印参数变量和调用函数 - 所有这些都来自调试器！
- 第10章：结论和其他有用的LLVM花絮 - 本章通过讨论扩展语言的可能方法来讨论本系列，还包括一系列关于“特殊主题”信息的指针，如添加垃圾收集支持，异常，调试 ，支持“层层堆叠”，以及一堆其他提示和技巧。

在本教程结束时，我们将编写少于1000行非注释，非空白的代码行。 有了这么少的代码，我们就可以为不平凡的语言构建一个非常合理的编译器，包括手写的词法分析器，解析器，AST，以及使用JIT编译器的代码生成支持。 虽然其他系统可能有一些有趣的“hello world”教程，但我认为本教程的广泛性是对LLVM优势的一个很好的证明，以及为什么你应该考虑它，如果你对语言或编译器设计感兴趣。

关于本教程的说明：我们希望您扩展这门语言并自行使用它。 用代码疯狂的玩，编译器一点都不可怕 - 玩它可以很有趣！

### 1.2 基础语言

本教程将使用我们称之为“Kaleidoscope”的玩具语言进行说明（源自“意为美丽，形式和视图”）。 Kaleidoscope是一种过程语言，允许您定义函数，使用条件，数学等。在本教程中，我们将扩展Kaleidoscope以支持if / then / else构造，for循环，用户定义的运算符，JIT 使用简单的命令行界面进行编译等

因为我们希望保持简单，所以Kaleidoscope中唯一的数据类型是64位浮点类型（在C语言中也称为“double”）。 因此，所有值都是隐式双精度，并且语言不需要类型声明。 这为语言提供了非常好的简单语法。 例如，以下简单示例计算Fibonacci数：

```
# Compute the x'th fibonacci number.
def fib(x)
  if x < 3 then
    1
  else
    fib(x-1)+fib(x-2)

# This expression will compute the 40th number.
fib(40)
```

我们还允许Kaleidoscope调用标准库函数（LLVM JIT使这完全无关紧要）。 这意味着您可以在使用之前使用'extern'关键字来定义函数（这对于相互递归函数也很有用）。 例如：

```
xtern sin(arg);
extern cos(arg);
extern atan2(arg1 arg2);

atan2(sin(.4), cos(42))
```

第6章中包含了一个更有趣的例子，我们编写了一个小的Kaleidoscope应用程序，它以不同的放大倍数显示Mandelbrot Set。

让我们深入探讨这种语言的实现！

### 1.3  词法分析器

在实现一种语言时，首先需要的是能够处理文本文件并识别它所说的内容。 传统的方法是使用“词法分析器”（又名“扫描仪”）将输入分解为“符号”。 词法分析器返回的每个符号包括符号代码和可能的一些元数据（例如，数字的数值）。 首先，我们定义了可能用到的符号：

```c++
// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.
enum Token {
  tok_eof = -1,

  // commands
  tok_def = -2,
  tok_extern = -3,

  // primary
  tok_identifier = -4,
  tok_number = -5,
};

static std::string IdentifierStr; // Filled in if tok_identifier
static double NumVal;             // Filled in if tok_number
```

我们的词法分析器返回的每个标记将是Token枚举值之一，或者它将是一个'未知'字符，如'+'，它将作为ASCII值返回。 如果当前符号是标识符，则IdentifierStr全局变量保存标识符的名称。 如果当前符号是数字（如1.0），则NumVal保存其值。 请注意，为简单起见，我们使用全局变量，这不是真正的语言实现的最佳选择:)。

词法分析器的实际实现是一个名为gettok的函数。 调用gettok函数以从标准输入返回下一个符号。 其定义开始于：

```c++
/// gettok - Return the next token from standard input.
static int gettok() {
  static int LastChar = ' ';

  // Skip any whitespace.
  while (isspace(LastChar))
    LastChar = getchar();
```

`gettok`通过调用C `getchar()`函数从标准输入一次读取一个字符来工作。 它在识别它们时会吃掉它们，并在`LastChar`中存储读取但未处理的最后一个字符。 它要做的第一件事就是忽略符号之间的空格。 这是通过上面的循环完成的。

`gettok`需要做的下一件事是识别标识符和特定关键字，如“def”。 Kaleidoscope通过这个简单的循环实现了这一点：

```c++
if (isalpha(LastChar)) { // identifier: [a-zA-Z][a-zA-Z0-9]*
  IdentifierStr = LastChar;
  while (isalnum((LastChar = getchar())))
    IdentifierStr += LastChar;

  if (IdentifierStr == "def")
    return tok_def;
  if (IdentifierStr == "extern")
    return tok_extern;
  return tok_identifier;
}
```

请注意，此代码在设置标识符时“IdentifierStr”设置为全局。 此外，由于语言关键字由相同的循环匹配，我们在这里处理它们。 数值类似：

```c++
if (isdigit(LastChar) || LastChar == '.') {   // Number: [0-9.]+
  std::string NumStr;
  do {
    NumStr += LastChar;
    LastChar = getchar();
  } while (isdigit(LastChar) || LastChar == '.');

  NumVal = strtod(NumStr.c_str(), 0);
  return tok_number;
}
```

这是处理输入的非常简单的代码。 当从输入读取数值时，我们使用C strtod函数将其转换为我们存储在NumVal中的数值。 请注意，这里没有进行足够的错误检查：它将错误地读取“1.23.45.67”并像处理“1.23”一样处理它。 随意扩展它:)。 接下来我们处理注释：

```c++
if (LastChar == '#') {
  // Comment until end of line.
  do
    LastChar = getchar();
  while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

  if (LastChar != EOF)
    return gettok();
}
```

我们通过跳到行尾来处理注释，然后返回下一个符号。 最后，如果输入与上述情况之一不匹配，则它是像“+”这样的运算符字符或文件的结尾。 这些代码使用以下代码处理：

```c++
// Check for end of file.  Don't eat the EOF.
  if (LastChar == EOF)
    return tok_eof;

  // Otherwise, just return the character as its ascii value.
  int ThisChar = LastChar;
  LastChar = getchar();
  return ThisChar;
}
```

有了这个，我们有了基本的`Kaleidoscope`语言的完整词法分析器（Lexer的完整代码清单可以在本教程的下一章中找到）。 接下来，我们将构建一个简单的解析器，使用它来构建抽象语法树(AST)。 当我们有这个时，我们将包含一个驱动程序，以便您可以一起使用词法分析器和解析器。

## 2. Implementing a Parser and AST

### 2.1 第二章简介

欢迎阅读“使用LLVM实现语言”教程的第2章。 本章介绍如何使用第1章中构建的词法分析器为我们的Kaleidoscope语言构建完整的解析器。 一旦我们有了解析器，我们将定义并构建一个抽象语法树（AST）。

我们将构建的解析器使用递归下降解析和操作符优先解析的组合来解析Kaleidoscope语言（后者用于二元表达式，前者用于其他所有内容）。 在我们解析之前，让我们来谈谈解析器的输出：抽象语法树。

### 2.3 The Abstract Syntax Tree (AST,抽象语法树)

程序的`AST`以这样的方式捕获其行为，即编译器的后续阶段（例如代码生成）很容易解释。 我们基本上希望语言中每个构件都有一个对象，AST应该对语言进行密切建模。 在Kaleidoscope中，我们有表达式，原型和函数对象。 我们先从表达式开始：

```c++
/// ExprAST - Base class for all expression nodes.
class ExprAST {
public:
  virtual ~ExprAST() {}
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
  double Val;

public:
  NumberExprAST(double Val) : Val(Val) {}
};
```

上面的代码显示了基本`ExprAST`类的定义和我们用于数字文字的一个子类。 关于此代码的重要注意事项是`NumberExprAST`类将文字的数值捕获为实例变量。 这允许编译器的后续阶段知道存储的数值是什么。

现在我们只创建了一个`AST`，因此它们还没有有用的访问方法。 例如，简单的添加虚拟方法来非常容易地打印代码。 以下是我们将在Kaleidoscope语言的基本形式中使用的其他表达式AST节点定义：

```c++
// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
  std::string Name;

public:
  VariableExprAST(const std::string &Name) : Name(Name) {}
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
  char Op;
  std::unique_ptr<ExprAST> LHS, RHS;

public:
  BinaryExprAST(char op, std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS)
    : Op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
  std::string Callee;
  std::vector<std::unique_ptr<ExprAST>> Args;

public:
  CallExprAST(const std::string &Callee,
              std::vector<std::unique_ptr<ExprAST>> Args)
    : Callee(Callee), Args(std::move(Args)) {}
};
```

这里的所有（有意而为）都相当直接：变量捕获变量名称，二元运算符捕获它们的操作符（例如'+'），还有捕获调用函数名称以及任何参数表达式的列表。 关于我们的AST的一个好处是它捕获语言特性而不关心语言的语法。 请注意，这里没有讨论二元运算符的优先级，词法结构等。

对于我们的基础语言，这些是我们将定义的所有表达式节点。 因为它还没有条件控制流程，所以它不是图灵完备的; 我们将在以后的文章中解决这个问题。 接下来我们需要的两件事是讨论函数接口的方法，以及讨论函数本身的方法：

```c++
/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;

public:
  PrototypeAST(const std::string &name, std::vector<std::string> Args)
    : Name(name), Args(std::move(Args)) {}

  const std::string &getName() const { return Name; }
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST {
  std::unique_ptr<PrototypeAST> Proto;
  std::unique_ptr<ExprAST> Body;

public:
  FunctionAST(std::unique_ptr<PrototypeAST> Proto,
              std::unique_ptr<ExprAST> Body)
    : Proto(std::move(Proto)), Body(std::move(Body)) {}
};
```

在Kaleidoscope中，只使用一个或多个参数来作为函数输入。 由于所有值都是双精度浮点数，因此每个参数的类型不需要任何存储空间。 在更进一步和现实性的语言中，“ExprAST”类可能具有类型字段。

有了这个框架，我们现在可以讨论在Kaleidoscope中解析表达式和函数体。

### 2.3 基础解析(Parser Basics)

现在我们要构建一个AST，我们需要定义解析器代码来构建它。 这里的想法是我们要解析类似“x + y”（由词法分析器返回的三个标记）到AST中，这可以通过这样的调用生成：

```c++
auto LHS = llvm::make_unique<VariableExprAST>("x");
auto RHS = llvm::make_unique<VariableExprAST>("y");
auto Result = std::make_unique<BinaryExprAST>('+', std::move(LHS),
                                              std::move(RHS));
```

为此，我们首先定义一些基本的辅助程序：

```c++
/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.
static int CurTok;
static int getNextToken() {
  return CurTok = gettok();
}
```

这在词法分析器里实现了一个简单的标记缓冲区。 这允许我们在词法分析器返回时提前查看下一个标识符。 我们解析器中的每个函数都假定CurTok是需要解析的当前标识符 。

```c++
/// LogError* - These are little helper functions for error handling.
std::unique_ptr<ExprAST> LogError(const char *Str) {
  fprintf(stderr, "LogError: %s\n", Str);
  return nullptr;
}
std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
  LogError(Str);
  return nullptr;
}
```

LogError事务是我们的解析器将用于处理错误的简单帮助程序。 我们的解析器中的错误恢复不是最好的，并且对用户不是特别友好的，但它对我们的教程来说已经足够了。 这些事务使得在具有各种返回类型的事务中更容易处理错误：它们全部返回null。

有了这些基本的辅助函数，我们就可以实现我们语法的第一部分：数值解析。

### 2.4 基础表达式解析(Basic Expression Parsing)

我们从数值开始，因为它们处理起来最简单。 对于语法中的每个作业，我们将定义一个解析该作业的函数。 对于数值，我们有：

```c++
/// numberexpr ::= number
static std::unique_ptr<ExprAST> ParseNumberExpr() {
  auto Result = llvm::make_unique<NumberExprAST>(NumVal);
  getNextToken(); // consume the number
  return std::move(Result);
}
```

这个事务非常简单：当前符号是tok_number符号时，它希望被调用。 它获取当前数字值，创建NumberExprAST节点，将词法分析器前进到下一个符号，最后返回。

这有一些有趣的地方。 最重要的一点是，这个事务会占用与其相关的所有符号，并返回带有下一个符号（它不是语法生成的一部分）的词法分析器缓冲区(buffer)。 这是递归下降解析器的一种相当标准的方法。 有关更好的示例，括号运算符的定义如下：

```c++
/// parenexpr ::= '(' expression ')'
static std::unique_ptr<ExprAST> ParseParenExpr() {
  getNextToken(); // eat (.
  auto V = ParseExpression();
  if (!V)
    return nullptr;

  if (CurTok != ')')
    return LogError("expected ')'");
  getNextToken(); // eat ).
  return V;
}
```

这个函数说明了解析器中一些有趣的东西：

- 1）它显示了我们如何使用LogError事务。调用时，此函数需要当前符号为`(`标记，但在解析子表达式后，可能没有等待到`)`。例如，如果用户输入`（4 x`而不是`（4）`，解析器应该发出错误。因为错误可能发生，解析器需要一种方式来指明它们发生错误：在我们的解析器中，我们返回null错误。
- 2）这个函数的另一个有趣的方面是它通过调用ParseExpression使用递归（我们很快就会看到ParseExpression可以调用ParseParenExpr）。这很强大，因为它允许我们处理递归 语法，并使每个作业变得非常简单。请注意，括号本身不会导致AST节点的建立。虽然我们可以这样做，但括号最重要的作用是引导解析器并提供分组。解析器构造AST后，不需要括号。

下一个简单的作业是用于处理变量引用和函数调用：

```c++
/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
  std::string IdName = IdentifierStr;

  getNextToken();  // eat identifier.

  if (CurTok != '(') // Simple variable ref.
    return llvm::make_unique<VariableExprAST>(IdName);

  // Call.
  getNextToken();  // eat (
  std::vector<std::unique_ptr<ExprAST>> Args;
  if (CurTok != ')') {
    while (1) {
      if (auto Arg = ParseExpression())
        Args.push_back(std::move(Arg));
      else
        return nullptr;

      if (CurTok == ')')
        break;

      if (CurTok != ',')
        return LogError("Expected ')' or ',' in argument list");
      getNextToken();
    }
  }

  // Eat the ')'.
  getNextToken();

  return llvm::make_unique<CallExprAST>(IdName, std::move(Args));
}
```

此事务遵循与其他事务相同的样式。 （如果当前符号是tok_identifier符号，则期望被调用）。 它还有递归和错误处理。 一个有趣的方面是它使用预测来确定当前标识符是独立变量引用还是函数调用表达式。 它通过检查标识符后面的标记是否为`(`标记，根据需要构造VariableExprAST或CallExprAST节点来处理此问题。

现在我们已经拥有了所有简单的表达式解析逻辑，我们可以定义一个辅助函数将它们组合成一个入口点。 我们称这类表达式为“主”表达式，使得本教程后面更加清晰。 为了解析任意的主表达式，我们需要确定它是什么类型的表达式：

```c++
/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
static std::unique_ptr<ExprAST> ParsePrimary() {
  switch (CurTok) {
  default:
    return LogError("unknown token when expecting an expression");
  case tok_identifier:
    return ParseIdentifierExpr();
  case tok_number:
    return ParseNumberExpr();
  case '(':
    return ParseParenExpr();
  }
}
```

现在你看到了这个函数的定义，可以更明显的看到为什么我们在各种函数中需要假设CurTok的状态。 这使得可以采用预测的方式来确定正在检查的表达式的类型，然后使用函数调用对其进行解析。

现在处理了基本表达式，我们需要处理二进制表达式。 它们有点复杂。

### 2.5 二元表达式解析(Binary Expression Parsing)

二元表达式很难解析，因为它们通常是模糊的。 例如，当给定字符串`x + y * z`时，解析器可以选择将其解析为`（x + y）* z`或`x +（y * z）`。 对于数学中的常见定义，我们期望后面的解析，因为`*`（乘法）具有比`+`（加法）更高的优先级。

有很多方法可以解决这个问题，但优雅而有效的方法是使用[Operator-Precedence Parsing](http://en.wikipedia.org/wiki/Operator-precedence_parser)。 此解析技术使用二元运算符的优先级来指导递归。 首先，我们需要一个优先级表：

```c++
/// BinopPrecedence - This holds the precedence for each binary operator that is
/// defined.
static std::map<char, int> BinopPrecedence;

/// GetTokPrecedence - Get the precedence of the pending binary operator token.
static int GetTokPrecedence() {
  if (!isascii(CurTok))
    return -1;

  // Make sure it's a declared binop.
  int TokPrec = BinopPrecedence[CurTok];
  if (TokPrec <= 0) return -1;
  return TokPrec;
}

int main() {
  // Install standard binary operators.
  // 1 is lowest precedence.
  BinopPrecedence['<'] = 10;
  BinopPrecedence['+'] = 20;
  BinopPrecedence['-'] = 20;
  BinopPrecedence['*'] = 40;  // highest.
  ...
}
```

对于Kaleidoscope的基本形式，我们只支持4个二元运算符（这显然可以由你，我们的读者大胆的扩展）。 GetTokPrecedence函数返回当前标记的优先级，如果标记不是二元运算符，则返回-1。使用映射可以轻松添加新运算符，并清楚地表明算法不依赖于所涉及的特定运算符，但是很容易消除映射并在GetTokPrecedence函数中进行比较。 （或者只使用固定大小的数组）。

通过上面定义的辅助程序，我们现在可以开始解析二元表达式。运算符优先级解析的基本思想是将具有可能不明确的二元运算符的表达式分解为多个部分。例如，考虑表达式`a + b +（c + d）* e * f + g`。运算符优先级解析将此视为由二元运算符分隔的主表达式流。因此，它将首先解析主要的主要表达式“a”，然后它将看到对\[ +，b \]\[+，（c + d）\]\[\*，e\]\[\*，f\]和[+，g]。请注意，因为括号是主表达式，所以二元表达式解析器根本不需要担心嵌套的子表达式，如（c + d）。

```c++
/// expression
///   ::= primary binoprhs
///
static std::unique_ptr<ExprAST> ParseExpression() {
  auto LHS = ParsePrimary();
  if (!LHS)
    return nullptr;

  return ParseBinOpRHS(0, std::move(LHS));
}
```

`ParseBinOpRHS`是为我们解析对序列的函数。 它需要一个优先级和一个指向目前已解析的部分的表达式的指针。 请注意，“x”是一个完全有效的表达式：因此，“binoprhs”被允许为空，在这种情况下，它返回传递给它的表达式。 在上面的示例中，代码将“a”的表达式传递给ParseBinOpRHS，当前标识符为“+”。

传递给`ParseBinOpRHS`的优先级值表示允许该函数吃进的最小运算符优先级。 例如，如果当前对流是[+，x]并且`ParseBinOpRHS`以优先级40传递，则它不会消耗任何标识符（因为'+'的优先级仅为20）。 考虑到这一点，`ParseBinOpRHS`以：

```c++
/// binoprhs
///   ::= ('+' primary)*
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                              std::unique_ptr<ExprAST> LHS) {
  // If this is a binop, find its precedence.
  while (1) {
    int TokPrec = GetTokPrecedence();

    // If this is a binop that binds at least as tightly as the current binop,
    // consume it, otherwise we are done.
    if (TokPrec < ExprPrec)
      return LHS;
```

此代码获取当前符号的优先级，并检查是否过低。 因为我们将无效标记优先级定义为-1，所以此检查隐式地知道当标记流用完二元运算符时，对流(pair-stream)结束。 如果此检查成功，我们知道该符号是二元运算符，并且它将包含在此表达式中：

```c++
// Okay, we know this is a binop.
int BinOp = CurTok;
getNextToken();  // eat binop

// Parse the primary expression after the binary operator.
auto RHS = ParsePrimary();
if (!RHS)
  return nullptr;
```

因此，此代码吃掉（并记住）二元运算符，然后解析后面的主表达式。 这就构建了整个对，第一个是运行示例的[+，b]。

现在我们解析了表达式的左侧和一对RHS序列，我们必须确定表达式关联的方式。 特别是，我们可以有`（a + b）binop(二元符) unparsed(未解析的符号)`或`a +（b binop unparsed）`。 为了确定这一点，我们展望“binop”以确定其优先级，并将其与BinOp的优先级（在当前情况下为“+”）进行比较：

```c++
// If BinOp binds less tightly with RHS than the operator after RHS, let
// the pending operator take RHS as its LHS.
int NextPrec = GetTokPrecedence();
if (TokPrec < NextPrec) {
```

如果`binop`在“RHS”右边的优先级低于或等于我们当前运算符的优先级，那么我们知道括号关联为“（a + b）binop ...”。 在我们的示例中，当前运算符为“+”，下一个运算符为“+”，我们知道它们具有相同的优先级。 在这种情况下，我们将为“a + b”创建AST节点，然后继续解析：

```c++
      ... if body omitted ...
    }

    // Merge LHS/RHS.
    LHS = llvm::make_unique<BinaryExprAST>(BinOp, std::move(LHS),
                                           std::move(RHS));
  }  // loop around to the top of the while loop.
}
```

在上面的例子中，这将把`a + b +`变成`（a + b）`并执行循环的下一次迭代，其中`+`作为当前符号。 上面的代码将吃掉+，记住并解析“（c + d）”作为主表达式，这使得当前对等于[+，（c + d）]。 然后它将使用`*`作为主要右侧的binop来评估上面的“if”条件。 在这种情况下，`*`的优先级高于`+`的优先级，因此将执行if条件。

这里留下的关键问题是`if条件如何完全解析右边表达式?`特别是，要为我们的示例正确构建AST，它需要将所有“（c + d）* e * f”作为RHS表达式变量。 执行此操作的代码非常简单（上面两个块的代码重复上下文）：

```c++
    // If BinOp binds less tightly with RHS than the operator after RHS, let
    // the pending operator take RHS as its LHS.
    int NextPrec = GetTokPrecedence();
    if (TokPrec < NextPrec) {
      RHS = ParseBinOpRHS(TokPrec+1, std::move(RHS));
      if (!RHS)
        return nullptr;
    }
    // Merge LHS/RHS.
    LHS = llvm::make_unique<BinaryExprAST>(BinOp, std::move(LHS),
                                           std::move(RHS));
  }  // loop around to the top of the while loop.
}
```

此时，我们知道主要的RHS二元运算符优先于当前正在解析的binop。因此，我们知道任何运算符都优先于“+”的对的序列应该被一起解析并返回为“RHS”。为此，我们递归调用ParseBinOpRHS函数，指定“TokPrec + 1”作为它继续所需的最小优先级。在上面的例子中，这将导致它将`（c + d）* e * f`的AST节点作为RHS返回，然后将其设置为“+”表达式的RHS。

最后，在while循环的下一次迭代中，解析“+ g”片段并将其添加到AST。使用这一小段代码（14行代码），我们以非常优雅的方式正确处理完全通用的二元表达式解析。这是对这段代码的旋风之旅，它有点微妙。我建议通过几个棘手的例子来看看它是如何工作的。

这包含了表达式的处理。此时，我们可以将解析器指向任意标记流并从中构建表达式，停止在不属于表达式的第一个标记处。接下来我们需要处理函数定义等。

### 2.6 解析其余部分(Parsing the Rest)

接下来还缺少函数原型的处理。 在Kaleidoscope中，这些用于'extern'函数声明以及函数体定义。 执行此操作的代码是直截了当的，并且不是很有趣（一旦表达式幸存下来）： 

```c++
/// prototype
///   ::= id '(' id* ')'
static std::unique_ptr<PrototypeAST> ParsePrototype() {
  if (CurTok != tok_identifier)
    return LogErrorP("Expected function name in prototype");

  std::string FnName = IdentifierStr;
  getNextToken();

  if (CurTok != '(')
    return LogErrorP("Expected '(' in prototype");

  // Read the list of argument names.
  std::vector<std::string> ArgNames;
  while (getNextToken() == tok_identifier)
    ArgNames.push_back(IdentifierStr);
  if (CurTok != ')')
    return LogErrorP("Expected ')' in prototype");

  // success.
  getNextToken();  // eat ')'.

  return llvm::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}
```

鉴于此，函数定义非常简单，只是一个原型加上一个表达式来实现正文：

```c++
/// definition ::= 'def' prototype expression
static std::unique_ptr<FunctionAST> ParseDefinition() {
  getNextToken();  // eat def.
  auto Proto = ParsePrototype();
  if (!Proto) return nullptr;

  if (auto E = ParseExpression())
    return llvm::make_unique<FunctionAST>(std::move(Proto), std::move(E));
  return nullptr;
}
```

另外，我们支持`extern`来声明`sin`和`cos`之类的函数，以及支持用户函数的前向声明。 这些`extern`只是没有实现的原型：

```c++
/// external ::= 'extern' prototype
static std::unique_ptr<PrototypeAST> ParseExtern() {
  getNextToken();  // eat extern.
  return ParsePrototype();
}
```

最后，我们还让用户输入任意顶层表达式并动态评估它们。 我们将通过为它们定义匿名的nullary（零参数）函数来处理这个问题：

```c++
/// toplevelexpr ::= expression
static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
  if (auto E = ParseExpression()) {
    // Make an anonymous proto.
    auto Proto = llvm::make_unique<PrototypeAST>("", std::vector<std::string>());
    return llvm::make_unique<FunctionAST>(std::move(Proto), std::move(E));
  }
  return nullptr;
}
```

现在我们已经完成了所有部分，让我们构建一个小驱动程序，让我们实际执行我们构建的代码！

### 2.7 驱动(The Driver)

这个驱动程序只是通过顶层调度循环调用所有解析部分。 这里没什么有趣的，所以我只包括顶层循环。 请参阅下面的“顶层解析”部分中的完整代码。

```c++
/// top ::= definition | external | expression | ';'
static void MainLoop() {
  while (1) {
    fprintf(stderr, "ready> ");
    switch (CurTok) {
    case tok_eof:
      return;
    case ';': // ignore top-level semicolons.
      getNextToken();
      break;
    case tok_def:
      HandleDefinition();
      break;
    case tok_extern:
      HandleExtern();
      break;
    default:
      HandleTopLevelExpression();
      break;
    }
  }
}
```

最有趣的部分是我们忽略顶层分号。 你问，这是为什么？ 基本原因是，如果在命令行中键入“4 + 5”，则解析器不知道这是否是您要键入的结尾。 例如，在下一行，您可以键入“def foo ...”，在这种情况下，4 + 5是顶级表达式的结尾。 或者，您可以键入“* 6”，这将继续表达式。 使用顶级分号允许您键入“4 + 5;”，解析器将知道您已完成。

### 2.8 小结

只有不到400行的注释代码（240行非注释，非空行代码），我们完全定义了我们的最小语言，包括词法分析器，解析器和AST构建器。 完成此操作后，可执行文件将验证Kaleidoscope代码并告诉我们它是否在语法上无效。 例如，这是一个示例交互：

```shell
$ ./a.out
ready> def foo(x y) x+foo(y, 4.0);
Parsed a function definition.
ready> def foo(x y) x+y y;
Parsed a function definition.
Parsed a top-level expr
ready> def foo(x y) x+y );
Parsed a function definition.
Error: unknown token when expecting an expression
ready> extern sin(a);
ready> Parsed an extern
ready> ^D
$
```

这里有很大的扩展空间。 您可以定义新的AST节点，以多种方式扩展语言等。在下一部分中，我们将介绍如何从AST生成LLVM中间表示（IR）。

### 2.9 Full Code Listing

以下是我们正在运行的示例的完整代码清单。 因为它使用LLVM库，我们需要将它们链接起来。为此，我们使用[llvm-config](http://llvm.org/cmds/llvm-config.html)工具通知makefile /命令行有关使用哪些选项：

```shell
# Compile
clang++ -g -O3 toy.cpp `llvm-config --cxxflags`
# Run
./a.out
```

[Here is the code](https://github.com/llvm-mirror/llvm/blob/master/examples/Kaleidoscope/Chapter2/toy.cpp)

## 3.  Code generation to LLVM IR

### 3.1 第三章介绍

欢迎阅读“使用LLVM实现语言”教程的第3章。 本章介绍如何将第2章中构建的抽象语法树转换为LLVM IR。 这将告诉你一些LLVM是如何工作的，以及演示它的易用性。 构建词法分析器和解析器要比生成LLVM IR代码要多得多。:)

请注意：本章和后面的代码需要LLVM 3.7或更高版本。 LLVM 3.6及之前的版本不适用。 另请注意，您需要使用与LLVM版本匹配的本教程版本：如果您使用的是正式的LLVM版本，请使用您的版本附带的文档版本或[llvm.org](http://llvm.org/releases/)版本页面。

### 3.2 代码生成设置

为了生成LLVM IR，我们需要一些简单的设置才能开始。 首先，我们在每个AST类中定义虚拟代码生成`codegen`方法：

```c++
/// ExprAST - Base class for all expression nodes.
class ExprAST {
public:
  virtual ~ExprAST() {}
  virtual Value *codegen() = 0;
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
  double Val;

public:
  NumberExprAST(double Val) : Val(Val) {}
  virtual Value *codegen();
};
...
```

`codegen`方法表示为该`AST`节点发出`IR`及其依赖的所有内容，并且它们都返回一个`LLVM Value`对象。 `Value`是用于表示LLVM中的[静态单一分配（SSA）寄存器](http://en.wikipedia.org/wiki/Static_single_assignment_form)或`SSA值`的类。 `SSA值`的最独特之处在于它们的值是在相关指令执行时计算，并且在指令重新执行之前（执行中）它不会获得新值。换句话说，没有办法“改变”SSA值。欲了解更多信息，请阅读[静态单一作业](http://en.wikipedia.org/wiki/Static_single_assignment_form) :一旦你理解它们，这些概念就非常自然了。

请注意，不是将虚方法添加到ExprAST类层次结构中，而是使用访问者模式或其他方式对此进行建模也是有意义的。同样，本教程将不再讨论良好的软件工程实践：出于我们的目的，添加虚拟方法是最简单的。

第二件事我们想要一个如同用于解析器的“LogError”方法 ，它将用于报告在代码生成期间发现的错误（例如，使用未声明的参数）：

```c++
static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule;
static std::map<std::string, Value *> NamedValues;

Value *LogErrorV(const char *Str) {
  LogError(Str);
  return nullptr;
}
```

静态变量将在代码生成期间使用。 `TheContext`是一个不透明的对象，拥有许多核心的LLVM数据结构，例如类型和常量值表。我们不需要详细了解它，我们只需要一个实例就可以传递到需要它的API中。

`Builder`对象是一个辅助对象，可以轻松生成LLVM指令。 `IRBuilder`类模板的实例跟踪插入指令的当前位置，并具有创建新指令的方法。

`TheModule`是一个包含函数和全局变量的LLVM结构。在许多方面，它是LLVM IR用于包含代码的顶层结构。它将存储我们生成的所有IR，这就是`codegen`方法返回原始值*而不是`unique_ptr <Value>`的原因。

NamedValues映射会跟踪当前作用域中定义的值以及它们的LLVM表示形式。 （换句话说，它是代码的符号表）。在这种形式的万花筒中，唯一可以引用的是功能参数。因此，在为其函数体生成代码时，函数参数将在此映射中。

有了这些基础知识，我们就可以开始讨论如何为每个表达式生成代码。请注意，这假设已将Builder设置为生成代码。现在，我们假设已经完成了，我们只是用它来发出代码。

### 3.3 中间表示代码生成

为表达式节点生成LLVM代码非常简单：所有四个表达式节点的注释代码少于45行。 首先，我们将做数字表示：

```c++
Value *NumberExprAST::codegen() {
  return ConstantFP::get(TheContext, APFloat(Val));
}
```

在LLVM IR中，数字常量用`ConstantFP`类表示，它在内部保存`APFloat`中的数值（`APFloat`具有保持任意精度的浮点常数的能力）。 这段代码基本上只是创建并返回一个`ConstantFP`。 请注意，在LLVM IR中，常量都是唯一的并且共享。 出于这个原因，API使用`oo :: get（...）`而不是`new foo（..）`或`foo :: Create（..）`。

```c++
Value *VariableExprAST::codegen() {
  // Look this variable up in the function.
  Value *V = NamedValues[Name];
  if (!V)
    LogErrorV("Unknown variable name");
  return V;
}
```

使用LLVM对变量的引用也非常简单。 在简单版的Kaleidoscope中，我们假设变量已经在某处定义并且其值可用。 实际上，NamedValues映射中唯一的值是函数参数。 此代码只是检查指定的名称是否在映射中（如果没有，则引用未知变量）并返回该值。 在以后的章节中，我们将在符号表和局部变量中添加对循环归纳变量的支持。

```c++
Value *BinaryExprAST::codegen() {
  Value *L = LHS->codegen();
  Value *R = RHS->codegen();
  if (!L || !R)
    return nullptr;

  switch (Op) {
  case '+':
    return Builder.CreateFAdd(L, R, "addtmp");
  case '-':
    return Builder.CreateFSub(L, R, "subtmp");
  case '*':
    return Builder.CreateFMul(L, R, "multmp");
  case '<':
    L = Builder.CreateFCmpULT(L, R, "cmptmp");
    // Convert bool 0/1 to double 0.0 or 1.0
    return Builder.CreateUIToFP(L, Type::getDoubleTy(TheContext),
                                "booltmp");
  default:
    return LogErrorV("invalid binary operator");
  }
}
```

二元运算符更有意思。这里的基本思想是递归地为表达式的左侧散发(emits)代码，然后是右侧，然后我们计算二元表达式的结果。在此代码中，我们对操作码进行了简单的切换，以创建正确的LLVM指令。

在上面的示例中，LLVM builder 类开始显示其值。 IRBuilder知道在哪里插入新创建的指令，您所要做的就是指定要创建的指令（例如使用CreateFAdd），使用哪些操作数（此处为L和R），并可选择为生成的指令提供名称。

LLVM的一个好处是名称只是一个提示。例如，如果上面的代码散发多个“addtmp”变量，LLVM将自动为每个变量提供一个增加的唯一数字后缀。指令的本地值名称纯粹是可选的，但它使读取IR转储更加容易。

LLVM指令受严格规则的约束：例如，add指令的Left和Right运算符必须具有相同的类型，add的结果类型必须与操作数类型匹配。因为Kaleidoscope中的所有值都是双精度的，所以这为add，sub和mul提供了非常简单的代码。

另一方面，LLVM指定fcmp指令始终返回'i1'值（一位整数）。这个问题是Kaleidoscope希望值为0.0或1.0。为了获得这些语义，我们将fcmp指令与uitofp指令结合起来。该指令通过将输入视为无符号值将其输入整数转换为浮点值。相反，如果我们使用sitofp指令，Kaleidoscope'<'运算符将返回0.0和-1.0，具体取决于输入值。

```c++
Value *CallExprAST::codegen() {
  // Look up the name in the global module table.
  Function *CalleeF = TheModule->getFunction(Callee);
  if (!CalleeF)
    return LogErrorV("Unknown function referenced");

  // If argument mismatch error.
  if (CalleeF->arg_size() != Args.size())
    return LogErrorV("Incorrect # arguments passed");

  std::vector<Value *> ArgsV;
  for (unsigned i = 0, e = Args.size(); i != e; ++i) {
    ArgsV.push_back(Args[i]->codegen());
    if (!ArgsV.back())
      return nullptr;
  }

  return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}
```

使用LLVM时，函数调用的代码生成非常简单。 上面的代码最初在LLVM Module 的符号表中执行函数名称查找。 回想一下，LLVM Module 是包含我们JIT的functions的容器。 通过为每个函数指定与用户指定的名称相同的名称，我们可以使用LLVM symbol表来解析我们的函数名称。

一旦我们有了要调用的函数，我们递归地对每个要传入的参数进行编码，并创建一个[LLVM调用指令](http://llvm.org/docs/LangRef.html#call-instruction)。 请注意，LLVM默认调用native C，允许调用标准库函数，如“sin”和“cos”，无需额外写。

这包含了我们对Kaleidoscope中迄今为止的四个基本表达式的处理。 随意进入并添加更多。 例如，通过浏览[LLVM language reference](http://llvm.org/docs/LangRef.html)，您将找到其他一些非常容易插入基本框架的有趣指令。

### 3.4 函数代码生成

原型(prototypes)和函数的代码生成必须处理许多细节，这使得它们的代码不如表达式代码生成美观，但我们可以指出一些重点。 首先，我们来谈谈原型的代码生成：它们既用于函数体，也用于外部函数声明。 代码以：

```c++
Function *PrototypeAST::codegen() {
  // Make the function type:  double(double,double) etc.
  std::vector<Type*> Doubles(Args.size(),
                             Type::getDoubleTy(TheContext));
  FunctionType *FT =
    FunctionType::get(Type::getDoubleTy(TheContext), Doubles, false);

  Function *F =
    Function::Create(FT, Function::ExternalLinkage, Name, TheModule);
```

这段代码将很多功能集成到几行中。首先请注意，此函数返回`Function *`而不是`Value *`。因为“原型”实际上是关于函数的外部接口（不是由表达式计算的值），所以它返回与codegen时相对应的LLVM函数是有意义的。

对`FunctionType :: get`的调用创建了应该用于给定Prototype的FunctionType。由于Kaleidoscope中的所有函数参数都是double类型，因此第一行创建了一个“N”个LLVM double类型值的向量。然后使用Functiontype :: get方法创建一个函数类型，该函数类型将“N"个double数作为参数，结果返回一个double值，而不是vararg（false参数表示这一点）。请注意，LLVM中的类型与常量一样是唯一的，所以你不需要“new”一个类型，你直接“get”它。

上面的最后一行实际上创建了与Prototype相对应的IR函数。表示要使用的类型，链接和名称，以及要插入的模块(module)。 “[外部链接](http://llvm.org/docs/LangRef.html#linkage)”意味着该函数可以在当前模块外部定义和/或可以由模块外部的函数调用。传入的名称是用户指定的名称：由于指定了“TheModule”，因此该名称在“TheModule”的符号(symbol)表中注册。

```c++
// Set names for all arguments.
unsigned Idx = 0;
for (auto &Arg : F->args())
  Arg.setName(Args[Idx++]);

return F;
```

最后，我们根据Prototype中给出的名称设置每个函数参数的名称。 此步骤并非严格必要，但保持名称一致会使IR更具可读性，并允许后续代码直接引用其名称的参数，而不必在Prototype AST中查找它们。

在这一点上，我们有一个没有函数体的函数原型。 这是LLVM IR表示函数声明的方式。 对于Kaleidoscope中的外部声明，这是我们需要的。 但是对于函数定义，我们需要codegen并附加一个函数体。

```c++
Function *FunctionAST::codegen() {
    // First, check for an existing function from a previous 'extern' declaration.
  Function *TheFunction = TheModule->getFunction(Proto->getName());

  if (!TheFunction)
    TheFunction = Proto->codegen();

  if (!TheFunction)
    return nullptr;

  if (!TheFunction->empty())
    return (Function*)LogErrorV("Function cannot be redefined.");
```

对于函数定义，我们首先在TheModule的符号表中搜索此函数的现有版本，以防已经使用'extern'语句创建了一个。 如果Module :: getFunction返回null，则不存在先前版本，因此我们将从Prototype中编译一个。 在任何一种情况下，我们都希望在开始之前断言函数是空的（即还没有定义）。

```c++
// Create a new basic block to start insertion into.
BasicBlock *BB = BasicBlock::Create(TheContext, "entry", TheFunction);
Builder.SetInsertPoint(BB);

// Record the function arguments in the NamedValues map.
NamedValues.clear();
for (auto &Arg : TheFunction->args())
  NamedValues[Arg.getName()] = &Arg;
```

现在我们到了建立Builder的步骤。 第一行创建一个新的基础块([basic block](http://en.wikipedia.org/wiki/Basic_block))（名为“entry”），它插入到TheFunction中。 然后第二行告诉构建器应该将新指令插入到新基础块的末尾。 LLVM中的基础块是定义控制流图([Control Flow Graph](http://en.wikipedia.org/wiki/Control_flow_graph))中函数的重要部分。 由于我们没有任何控制流，因此我们的函数此时只包含一个块。 我们将在第5章修复此问题:)。

接下来，我们将函数参数添加到NamedValues map（首先清除它之后），以便它们可以被VariableExprAST节点访问。

```c++
if (Value *RetVal = Body->codegen()) {
  // Finish off the function.
  Builder.CreateRet(RetVal);

  // Validate the generated code, checking for consistency.
  verifyFunction(*TheFunction);

  return TheFunction;
}
```

一旦设置了插入点并填充了NamedValues映射，我们就会调用`codegen`方法来获取函数的根表达式。 如果没有发生错误，则会发出代码以将表达式计算到条目块中并返回计算的值。 假设没有错误，我们然后创建一个LLVM ret指令( [ret instruction](http://llvm.org/docs/LangRef.html#ret-instruction))，完成该函数。 构建函数后，我们调用LLVM提供的verifyFunction。 此函数对生成的代码执行各种一致性检查，以确定我们的编译器是否正在执行所有操作。 使用它很重要：它可以捕获很多错误。 函数执行完成并验证后，我们将其返回。

```c++
 // Error reading body, remove function.
  TheFunction->eraseFromParent();
  return nullptr;
}
```

这里留下的唯一一件事是处理错误情形。 为简单起见，我们仅通过删除函数(由eraseFromParent方法生成)来处理此问题。 这允许用户重新定义之前错误输入的函数：如果我们没有删除它，它将存在于带有正文的符号表中，从而阻止将来重新定义。

但是，此代码确实存在错误：如果FunctionAST :: codegen（）方法找到现有的IR函数，则它不会根据定义自己的原型验证其签名。 这意味着较早的'extern'声明将优先于函数定义的签名，这可能导致codegen失败，例如，如果函数参数的名称不同。 有很多方法可以解决这个问题，看看你能想出什么！ 这是一个测试用例：

```c++
extern foo(a);     # ok, defines foo.
def foo(b) b;      # Error: Unknown variable name. (decl using 'a' takes precedence).
```

### 3.5. 驱动更改及结束思考

目前，除了我们可以查看漂亮的IR调用之外，LLVM的代码生成并没有给我们带来太多帮助。 示例代码将对codegen的调用插入到“HandleDefinition”，“HandleExtern”等函数中，然后转储出LLVM IR。 这为查看简单函数的LLVM IR提供了一种很好的方法。 例如：

```shell
ready> 4+5;
Read top-level expression:
define double @0() {
entry:
  ret double 9.000000e+00
}
```

请注意解析器如何将顶层表达式转换为我们的匿名函数。 当我们在下一章中添加[JIT支持](http://llvm.org/docs/tutorial/LangImpl04.html#adding-a-jit-compiler)时，这将非常方便。 另请注意，代码非常精确地转录，除了IRBuilder完成的简单常量折叠之外，不会执行任何优化。 我们将在下一章中明确添加优化。

```shell
ready> def foo(a b) a*a + 2*a*b + b*b;
Read function definition:
define double @foo(double %a, double %b) {
entry:
  %multmp = fmul double %a, %a
  %multmp1 = fmul double 2.000000e+00, %a
  %multmp2 = fmul double %multmp1, %b
  %addtmp = fadd double %multmp, %multmp2
  %multmp3 = fmul double %b, %b
  %addtmp4 = fadd double %addtmp, %multmp3
  ret double %addtmp4
}
```

这显示了一些简单的算术。 请注意与我们用于创建指令的LLVM构建器调用具有惊人的相似性。

```shell
ready> def bar(a) foo(a, 4.0) + bar(31337);
Read function definition:
define double @bar(double %a) {
entry:
  %calltmp = call double @foo(double %a, double 4.000000e+00)
  %calltmp1 = call double @bar(double 3.133700e+04)
  %addtmp = fadd double %calltmp, %calltmp1
  ret double %addtmp
}
```

这显示了一些函数调用。 请注意，如果您调用此函数将需要很长时间才能执行。 在未来我们将添加条件控制流以实际使递归有用:)。

```c++
ready> extern cos(x);
Read extern:
declare double @cos(double)

ready> cos(1.234);
Read top-level expression:
define double @1() {
entry:
  %calltmp = call double @cos(double 1.234000e+00)
  ret double %calltmp
}
```

这显示了libm“cos”函数的extern，以及对它的调用。

```c++
ready> ^D
; ModuleID = 'my cool jit'

define double @0() {
entry:
  %addtmp = fadd double 4.000000e+00, 5.000000e+00
  ret double %addtmp
}

define double @foo(double %a, double %b) {
entry:
  %multmp = fmul double %a, %a
  %multmp1 = fmul double 2.000000e+00, %a
  %multmp2 = fmul double %multmp1, %b
  %addtmp = fadd double %multmp, %multmp2
  %multmp3 = fmul double %b, %b
  %addtmp4 = fadd double %addtmp, %multmp3
  ret double %addtmp4
}

define double @bar(double %a) {
entry:
  %calltmp = call double @foo(double %a, double 4.000000e+00)
  %calltmp1 = call double @bar(double 3.133700e+04)
  %addtmp = fadd double %calltmp, %calltmp1
  ret double %addtmp
}

declare double @cos(double)

define double @1() {
entry:
  %calltmp = call double @cos(double 1.234000e+00)
  ret double %calltmp
}
```

当您退出当前演示时（通过在`Linux`上通过`CTRL + D`发送`EOF`或在Windows上按`CTRL + Z`和`ENTER`发送`EOF`），它会为生成的整个模块转储IR。 在这里，您可以看到所有功能相互引用的大图。

这包含了Kaleidoscope教程的第三章。 接下来，我们将描述如何为此添加JIT codegen和优化器支持，以便我们实际上可以开始运行代码！

### 3.6. Full Code Listing

以下是我们运行示例的完整代码清单，使用LLVM代码生成器进行了增强。 因为它使用LLVM库，我们需要将它们链接起来。为此，我们使用llvm-config工具通知makefile /命令行有关使用哪些选项：

```shell
# Compile
clang++ -g -O3 toy.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core` -o toy
# Run
./toy
```

 [Here is the code](https://github.com/llvm-mirror/llvm/blob/master/examples/Kaleidoscope/Chapter3/toy.cpp)