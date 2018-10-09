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

- 1）它显示了我们如何使用LogError事务。调用时，此函数需要当前符号为`(`标记，但在解析子表达式后，可能没有等待到`)`。例如，如果用户输入`（4 x`而不是`（4）`，解析器应该emitted错误。因为错误可能发生，解析器需要一种方式来指明它们发生错误：在我们的解析器中，我们返回null错误。
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

`codegen`方法表示为该`AST`节点emitted`IR`及其依赖的所有内容，并且它们都返回一个`LLVM Value`对象。 `Value`是用于表示LLVM中的[静态单一分配（SSA）寄存器](http://en.wikipedia.org/wiki/Static_single_assignment_form)或`SSA值`的类。 `SSA值`的最独特之处在于它们的值是在相关指令执行时计算，并且在指令重新执行之前（执行中）它不会获得新值。换句话说，没有办法“改变”SSA值。欲了解更多信息，请阅读[静态单一作业](http://en.wikipedia.org/wiki/Static_single_assignment_form) :一旦你理解它们，这些概念就非常自然了。

请注意，不是将虚方法添加到ExprAST类层次结构中，而是使用访问模式或其他方式对此进行建模也是有意义的。同样，本教程将不再讨论良好的软件工程实践：出于我们的目的，添加虚拟方法是最简单的。

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

NamedValues map会跟踪当前作用域中定义的值以及它们的LLVM表示形式。 （换句话说，它是代码的符号表）。在这种形式的语言中，唯一可以引用的是函数参数。因此，在为其函数体生成代码时，函数参数将在此映射中。

有了这些基础知识，我们就可以开始讨论如何为每个表达式生成代码。请注意，这假设已将Builder设置为生成代码。现在，我们假设已经完成了，我们只是用它来emitted代码。

### 3.3 中间表示代码生成

为表达式节点生成LLVM代码非常简单：所有四个表达式节点的非注释代码少于45行。 首先，我们将做数值表示：

```c++
Value *NumberExprAST::codegen() {
  return ConstantFP::get(TheContext, APFloat(Val));
}
```

在LLVM IR中，数字常量用`ConstantFP`类表示，它在内部保存`APFloat`中的数值（`APFloat`具有保持任意精度的浮点常数的能力）。 这段代码基本上只是创建并返回一个`ConstantFP`。 请注意，在LLVM IR中，常量都是唯一的并且共享。 出于这个原因，API使用`foo :: get（...）`而不是`new foo（..）`或`foo :: Create（..）`。

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

在上面的示例中，LLVM builder 类开始显示其价值。 IRBuilder知道在哪里插入新创建的指令，您所要做的就是指定要创建的指令（例如使用CreateFAdd），使用哪些操作数（此处为L和R），并可选择为生成的指令提供名称。

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

对`FunctionType :: get`的调用创建了应该用于给定Prototype的FunctionType。由于Kaleidoscope中的所有函数参数都是double类型，因此第一行创建了一个“N”个LLVM double类型值的向量。然后使用Functiontype :: get方法创建一个函数类型，该函数将“N"个double数作为参数，结果返回一个double值，而不是vararg（false参数表示这一点）。请注意，LLVM中的类型与常量一样是唯一的，所以你不需要“new”一个类型，你直接“get”它。

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

一旦设置了插入点并填充了NamedValues映射，我们就会调用`codegen`方法来获取函数的根表达式。 如果没有发生错误，则会emitted代码以将表达式计算到基础块中并返回计算的值。 假设没有错误，我们然后创建一个LLVM ret指令( [ret instruction](http://llvm.org/docs/LangRef.html#ret-instruction))，完成该函数。 构建函数后，我们调用LLVM提供的verifyFunction。 此函数对生成的代码执行各种一致性检查，以确定我们的编译器是否正在执行所有操作。 使用它很重要：它可以捕获很多错误。 函数执行完成并验证后，我们将其返回。

```c++
 // Error reading body, remove function.
  TheFunction->eraseFromParent();
  return nullptr;
}
```

这里留下的唯一一件事是处理错误情形。 为简单起见，我们仅通过删除函数(由eraseFromParent方法生成)来处理此问题。 这允许用户重新定义之前错误输入的函数：如果我们没有删除它，它将存在于带有context的符号表中，从而阻止将来重新定义。

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

## 4. Kaleidoscope: Adding JIT and Optimizer Support

### 4.1 第4章介绍

欢迎阅读“使用LLVM实现语言”教程的第4章。 第1-3章描述了简单语言的实现，并增加了对生成LLVM IR的支持。 本章介绍两种新技术：为您的语言添加优化器支持，以及添加JIT编译器支持。 这些新增内容将演示如何为Kaleidoscope语言提供优质，高效的代码。

### 4.2 细小常数折叠

我们在第3章的演示优雅且易于扩展。 不幸的是，它不会产生很棒的代码。 但是，在编译简单代码时，IRBuilder确实给了我们明显的优化：

```shell
ready> def test(x) 1+2+x;
Read function definition:
define double @test(double %x) {
entry:
        %addtmp = fadd double 3.000000e+00, %x
        ret double %addtmp
}
```

此代码如果不通过解析输入构建的AST的文字转录。 那将是：

```shell
ready> def test(x) 1+2+x;
Read function definition:
define double @test(double %x) {
entry:
        %addtmp = fadd double 2.000000e+00, 1.000000e+00
        %addtmp1 = fadd double %addtmp, %x
        ret double %addtmp1
}
```

如上所述，常量折叠是一种非常常见且非常重要的优化：以至于许多语言实现者在其AST表示中实现常量折叠支持。

使用LLVM，您不需要在AST中支持。由于构建LLVM IR的所有调用都通过LLVM IR构建器，因此构建器本身会在调用它时检查是否存在常量折叠机会。如果是这样，它只是执行常量折叠并返回常量而不是创建指令。

嗯，这很容易:)。在实践中，我们建议在生成这样的代码时始终使用IRBuilder。它的使用没有“语法开销”（你不必在任何地方使用常量检查来编译你的编译器）并且它可以大大减少在某些情况下生成的LLVM IR的数量（特别是对于具有宏预处理器的语言或使用很多常量）。

另一方面，IRBuilder受到以下事实的限制：它在构建代码时将所有分析内联到代码中。下面是一个稍微复杂的例子：

```
ready> def test(x) (1+2+x)*(x+(1+2));
ready> Read function definition:
define double @test(double %x) {
entry:
        %addtmp = fadd double 3.000000e+00, %x
        %addtmp1 = fadd double %x, 3.000000e+00
        %multmp = fmul double %addtmp, %addtmp1
        ret double %multmp
}
```

在这种情况下，乘法的LHS和RHS是相同的值。 我们真的很想看到这产生“tmp = x + 3; result = tmp * tmp;“而不是两次计算”x + 3“。

不幸的是，没有多少本地分析能够检测并纠正这一点。 这需要两个转换：表达式的重新关联（使add的词法相同）和Common Subexpression Elimination（CSE）删除冗余的add指令。 幸运的是，LLVM以“passes”的形式提供了您可以使用的各种优化。

### 4.3 LLVM Optimization Passes

LLVM提供了许多优化过程，它们可以执行许多不同类型的操作并具有不同的权衡。与其他系统不同，LLVM并不认为一组优化适用于所有语言和所有情况的错误概念。 LLVM允许编译器实现者就 要使用的优化，按什么顺序以及在什么情况下做出完整的决策。

作为一个具体的例子，LLVM支持“whole module”passes，它们可以查看尽可能大的代码体（通常是整个文件，但如果在链接时运行，这可能是整个程序的重要部分） 。它还支持并包含“per-function”passes，它一次只能在一个函数上运行，而不需要查看其他函数。有关传递及其运行方式的更多信息，请参阅[如何编写Pass]( [How to Write a Pass](http://llvm.org/docs/WritingAnLLVMPass.html))文档和[LLVM Pass列表](http://llvm.org/docs/Passes.html)。

对于Kaleidoscope，我们目前正在生成函数，一次一个，当用户输入它们时。我们不会在这个设置中瞄准最优优化，但我们也想要尽可能抓住简单快捷的东西。因此，当用户键入函数时，我们将选择运行一些按函数优化。如果我们想要创建一个“静态Kaleidoscope编译器”，我们将使用现在拥有的代码，除非我们推迟运行优化器直到整个文件被解析。

为了实现每个函数的优化，我们需要设置一个FunctionPassManager来保存和组织我们想要运行的LLVM优化。完成后，我们可以添加一组优化来运行。对于我们想要优化的每个模块，我们需要一个新的FunctionPassManager，因此我们将编写一个函数来为我们创建和初始化module 和 pass manager：

```c++
void InitializeModuleAndPassManager(void) {
  // Open a new module.
  TheModule = llvm::make_unique<Module>("my cool jit", TheContext);

  // Create a new pass manager attached to it.
  TheFPM = llvm::make_unique<FunctionPassManager>(TheModule.get());

  // Do simple "peephole" optimizations and bit-twiddling optzns.
  TheFPM->add(createInstructionCombiningPass());
  // Reassociate expressions.
  TheFPM->add(createReassociatePass());
  // Eliminate Common SubExpressions.
  TheFPM->add(createGVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  TheFPM->add(createCFGSimplificationPass());

  TheFPM->doInitialization();
}
```

此代码初始化全局模块TheModule，以及函数pass manager  `TheFPM`，它附加到TheModule。 一旦设置了pass manager ，我们就会使用一系列“add”调用来添加一堆LLVM Passes。

在这种情况下，我们选择添加四个优化过程。 我们在这里选择的通道是一组非常标准的“cleanup”优化，可用于各种代码。 我不会深入研究他们的所作所为，但相信我，他们是一个很好的起点:)。

一旦设置了PassManager，我们就需要使用它。 我们通过在构造新创建的函数（在FunctionAST :: codegen（）中）之后运行它，但在它返回到客户端之前执行此操作：

```c++
if (Value *RetVal = Body->codegen()) {
  // Finish off the function.
  Builder.CreateRet(RetVal);

  // Validate the generated code, checking for consistency.
  verifyFunction(*TheFunction);

  // Optimize the function.
  TheFPM->run(*TheFunction);

  return TheFunction;
}
```

如您所见，这非常简单。 FunctionPassManager优化并更新LLVM功能*，改进（但愿）它的主体。 有了这个，我们可以再次尝试我们的测试：

```
ready> def test(x) (1+2+x)*(x+(1+2));
ready> Read function definition:
define double @test(double %x) {
entry:
        %addtmp = fadd double %x, 3.000000e+00
        %multmp = fmul double %addtmp, %addtmp
        ret double %multmp
}
```

正如预期的那样，我们现在可以获得优化的代码，从每次执行此函数时都可以保存浮点加法指令。

LLVM提供了各种可在某些情况下使用的优化。 有关各种Passes的一些文档可用，但它不是很完整。 另一个好的想法来源可以来自于看看Clang开始运行的Passes。 “opt”工具允许您尝试从命令行传递，以便您可以查看它们是否执行任何操作。

现在我们已经从前端获得了合理的代码，让我们谈谈关于它的执行！

### 4.4 Adding a JIT Compiler

LLVM IR中提供的代码可以应用各种各样的工具。例如，您可以对其运行优化（如上所述），您可以将其以文本或二进制形式转储，您可以将代码编译为某个目标的汇编文件（.s），或者您可以JIT编译它。 LLVM IR表示的好处在于它是编译器的许多不同部分之间的“通用currency(货币)”。

在本节中，我们将向我们的解释器添加JIT编译器支持。我们想要Kaleidoscope的基本思想是让用户像现在一样输入函数体，但是立即评估他们输入的顶级表达式。例如，如果他们输入“1 + 2;”，我们应该评估如果他们定义了一个函数，他们应该可以从命令行调用它。

为此，我们首先准备环境以为当前本机目标创建代码并声明和初始化JIT。这是通过调用一些InitializeNativeTarget \ *函数并添加一个全局变量TheJIT，并在main中初始化它来完成的：

```c++
static std::unique_ptr<KaleidoscopeJIT> TheJIT;
...
int main() {
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();

  // Install standard binary operators.
  // 1 is lowest precedence.
  BinopPrecedence['<'] = 10;
  BinopPrecedence['+'] = 20;
  BinopPrecedence['-'] = 20;
  BinopPrecedence['*'] = 40; // highest.

  // Prime the first token.
  fprintf(stderr, "ready> ");
  getNextToken();

  TheJIT = llvm::make_unique<KaleidoscopeJIT>();

  // Run the main "interpreter loop" now.
  MainLoop();

  return 0;
}
```

我们还需要为JIT设置数据布局：

```c++
void InitializeModuleAndPassManager(void) {
  // Open a new module.
  TheModule = llvm::make_unique<Module>("my cool jit", TheContext);
  TheModule->setDataLayout(TheJIT->getTargetMachine().createDataLayout());

  // Create a new pass manager attached to it.
  TheFPM = llvm::make_unique<FunctionPassManager>(TheModule.get());
  ...
```

KaleidoscopeJIT类是一个专门为这些教程构建的简单JIT，可以在llvm-src / examples / Kaleidoscope / include / KaleidoscopeJIT.h的LLVM源代码中找到。 在后面的章节中，我们将看看它是如何工作的并使用新功能扩展它，但是现在我们将把它当作给定的。 它的API非常简单：addModule将一个LLVM IR模块添加到JIT中，使其功能可用于执行; removeModule删除一个模块，释放与该模块中的代码相关的任何内存; 和findSymbol允许我们查找编译代码的指针。

我们可以使用这个简单的API并更改我们解析顶级表达式的代码，如下所示：

```c++
static void HandleTopLevelExpression() {
  // Evaluate a top-level expression into an anonymous function.
  if (auto FnAST = ParseTopLevelExpr()) {
    if (FnAST->codegen()) {

      // JIT the module containing the anonymous expression, keeping a handle so
      // we can free it later.
      auto H = TheJIT->addModule(std::move(TheModule));
      InitializeModuleAndPassManager();

      // Search the JIT for the __anon_expr symbol.
      auto ExprSymbol = TheJIT->findSymbol("__anon_expr");
      assert(ExprSymbol && "Function not found");

      // Get the symbol's address and cast it to the right type (takes no
      // arguments, returns a double) so we can call it as a native function.
      double (*FP)() = (double (*)())(intptr_t)ExprSymbol.getAddress();
      fprintf(stderr, "Evaluated to %f\n", FP());

      // Delete the anonymous expression module from the JIT.
      TheJIT->removeModule(H);
    }
```

如果解析和codegen成功，则下一步是将包含顶层表达式的模块添加到JIT。我们通过调用addModule来实现这一点，addModule触发模块中所有函数的代码生成，并返回一个句柄，可以用来稍后从JIT中删除模块。一旦将模块添加到JIT，就不能再修改它，因此我们还打开一个新模块，通过调用InitializeModuleAndPassManager（）来保存后续代码。

一旦我们将模块添加到JIT，我们需要获得指向最终生成代码的指针。我们通过调用JIT的findSymbol方法并传递顶级表达式函数的名称来执行此操作：__ anon_expr。由于我们刚刚添加了这个函数，我们断言findSymbol返回了一个结果。

接下来，我们通过在符号上调用getAddress（）来获取__anon_expr函数的内存中地址。回想一下，我们将顶层表达式编译为一个自包含的LLVM函数，该函数不带参数并返回计算的double。因为LLVM JIT编译器与本机平台ABI匹配，这意味着您可以将结果指针强制转换为该类型的函数指针并直接调用它。这意味着，JIT编译代码与静态链接到应用程序的本机机器代码之间没有区别。

最后，由于我们不支持对顶层表达式进行重新评估，因此当我们完成释放相关内存时，我们会从JIT中删除该模块。但是，回想一下，我们之前创建的几行模块（通过InitializeModuleAndPassManager）仍处于打开状态，等待添加新代码。

只需这两个变化，让我们看看Kaleidoscope现在如何运作！

```
ready> 4+5;
Read top-level expression:
define double @0() {
entry:
  ret double 9.000000e+00
}

Evaluated to 9.000000
```

好吧，这看起来基本上是有效的。 该函数的转储显示了“无参数函数总是返回double”，我们为每个键入的顶层表达式合成。这演示了非常基本的功能，但我们可以做更多吗？

```
ready> def testfunc(x y) x + y*2;
Read function definition:
define double @testfunc(double %x, double %y) {
entry:
  %multmp = fmul double %y, 2.000000e+00
  %addtmp = fadd double %multmp, %x
  ret double %addtmp
}

ready> testfunc(4, 10);
Read top-level expression:
define double @1() {
entry:
  %calltmp = call double @testfunc(double 4.000000e+00, double 1.000000e+01)
  ret double %calltmp
}

Evaluated to 24.000000

ready> testfunc(5, 10);
ready> LLVM ERROR: Program used external function 'testfunc' which could not be resolved!
```

函数定义和调用也有效，但最后一行出了问题。call看起来有效，发生了什么？正如您可能从API中猜到的那样，模块是JIT的分配单元，而testfunc是包含匿名表达式的同一模块的一部分。当我们从JIT中删除该模块以释放匿名表达式的内存时，我们删除了testfunc的定义。然后，当我们第二次尝试调用testfunc时，JIT再也找不到它了。

解决此问题的最简单方法是将匿名表达式放在与其余函数定义不同的模块中。只要每个被调用的函数都有一个原型，JIT就会愉快地解决跨模块边界的函数调用，并在调用之前添加到JIT中。通过将匿名表达式放在不同的模块中，我们可以删除它而不影响其余的功能。

事实上，我们将更进一步，将每个函数都放在自己的模块中。这样做可以让我们利用KaleidoscopeJIT的有用属性，使我们的环境更像REPL：函数可以多次添加到JIT（与每个函数必须具有唯一定义的模块不同）。当您在KaleidoscopeJIT中查找符号时，它将始终返回最新的定义：

```
ready> def foo(x) x + 1;
Read function definition:
define double @foo(double %x) {
entry:
  %addtmp = fadd double %x, 1.000000e+00
  ret double %addtmp
}

ready> foo(2);
Evaluated to 3.000000

ready> def foo(x) x + 2;
define double @foo(double %x) {
entry:
  %addtmp = fadd double %x, 2.000000e+00
  ret double %addtmp
}

ready> foo(2);
Evaluated to 4.000000
```

为了让每个函数都存在于自己的模块中，我们需要一种方法来重新生成我们打开的每个新模块中的预先函数声明：

```c++
static std::unique_ptr<KaleidoscopeJIT> TheJIT;

...

Function *getFunction(std::string Name) {
  // First, see if the function has already been added to the current module.
  if (auto *F = TheModule->getFunction(Name))
    return F;

  // If not, check whether we can codegen the declaration from some existing
  // prototype.
  auto FI = FunctionProtos.find(Name);
  if (FI != FunctionProtos.end())
    return FI->second->codegen();

  // If no existing prototype exists, return null.
  return nullptr;
}

...

Value *CallExprAST::codegen() {
  // Look up the name in the global module table.
  Function *CalleeF = getFunction(Callee);

...

Function *FunctionAST::codegen() {
  // Transfer ownership of the prototype to the FunctionProtos map, but keep a
  // reference to it for use below.
  auto &P = *Proto;
  FunctionProtos[Proto->getName()] = std::move(Proto);
  Function *TheFunction = getFunction(P.getName());
  if (!TheFunction)
    return nullptr;
```

为了实现这一点，我们首先添加一个新的全局函数FunctionProtos，它包含每个函数的最新原型。 我们还将添加一个方便的方法getFunction（）来替换对TheModule-> getFunction（）的调用。 我们的便捷方法在TheModule中搜索现有的函数声明，如果没有找到，则回退到从FunctionProtos生成新的声明。 在CallExprAST :: codegen（）中，我们只需要替换对TheModule-> getFunction（）的调用。 在FunctionAST :: codegen（）中，我们需要先更新FunctionProtos映射，然后调用getFunction（）。 完成此操作后，我们总是可以在当前模块中获取任何先前声明的函数的函数声明。

我们还需要更新HandleDefinition和HandleExtern：

```c++
static void HandleDefinition() {
  if (auto FnAST = ParseDefinition()) {
    if (auto *FnIR = FnAST->codegen()) {
      fprintf(stderr, "Read function definition:");
      FnIR->print(errs());
      fprintf(stderr, "\n");
      TheJIT->addModule(std::move(TheModule));
      InitializeModuleAndPassManager();
    }
  } else {
    // Skip token for error recovery.
     getNextToken();
  }
}

static void HandleExtern() {
  if (auto ProtoAST = ParseExtern()) {
    if (auto *FnIR = ProtoAST->codegen()) {
      fprintf(stderr, "Read extern: ");
      FnIR->print(errs());
      fprintf(stderr, "\n");
      FunctionProtos[ProtoAST->getName()] = std::move(ProtoAST);
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}
```

在HandleDefinition中，我们添加两行来将新定义的函数传递给JIT并打开一个新模块。 在HandleExtern中，我们只需添加一行即可将原型添加到FunctionProtos中。

通过这些更改，让我们再次尝试我们的REPL（这次我删除了匿名函数的转储，你现在应该得到这个想法:)：

```
ready> def foo(x) x + 1;
ready> foo(2);
Evaluated to 3.000000

ready> def foo(x) x + 2;
ready> foo(2);
Evaluated to 4.000000
```

有用！

即使使用这个简单的代码，我们也会获得一些令人惊讶的强大功能 - 请查看：

```
ready> extern sin(x);
Read extern:
declare double @sin(double)

ready> extern cos(x);
Read extern:
declare double @cos(double)

ready> sin(1.0);
Read top-level expression:
define double @2() {
entry:
  ret double 0x3FEAED548F090CEE
}

Evaluated to 0.841471

ready> def foo(x) sin(x)*sin(x) + cos(x)*cos(x);
Read function definition:
define double @foo(double %x) {
entry:
  %calltmp = call double @sin(double %x)
  %multmp = fmul double %calltmp, %calltmp
  %calltmp2 = call double @cos(double %x)
  %multmp4 = fmul double %calltmp2, %calltmp2
  %addtmp = fadd double %multmp, %multmp4
  ret double %addtmp
}

ready> foo(4.0);
Read top-level expression:
define double @3() {
entry:
  %calltmp = call double @foo(double 4.000000e+00)
  ret double %calltmp
}

Evaluated to 1.000000
```

哇，JIT怎么知道sin和cos？答案非常简单：KaleidoscopeJIT有一个简单的符号解析规则，用于查找任何给定模块中不可用的符号：首先，它搜索已经添加到JIT的所有模块，从最近的到最古老的，找到最新的定义。如果在JIT中找不到定义，它将回退到在Kaleidoscope进程本身上调用“dlsym（”sin“）”。由于“sin”是在JIT的地址空间中定义的，它只是简单地修补模块中的调用以直接调用libm版本的sin。但在某些情况下，这甚至更进一步：因为sin和cos是标准数学函数的名称，当使用上面的“sin（1.0）”中的常量调用时，常量文件夹将直接评估函数调用正确的结果。

在未来，我们将看到如何调整此符号解析规则，以启用各种有用的功能，从安全性（限制JIT代码可用的符号集）到基于符号名称的动态代码生成，甚至是惰性的编译

符号解析规则的一个直接好处是我们现在可以通过编写任意C ++代码来实现操作来扩展语言。例如，如果我们添加：

```
#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

/// putchard - putchar that takes a double and returns 0.
extern "C" DLLEXPORT double putchard(double X) {
  fputc((char)X, stderr);
  return 0;
}
```

注意，对于Windows，我们需要实际导出函数，因为动态符号加载器将使用GetProcAddress来查找符号。

现在我们可以使用以下内容为控制台生成简单的输出：“extern putchard（x）; putchard（120）;“，在控制台上打印小写的'x'（120是'x'的ASCII码）。 类似的代码可用于在Kaleidoscope中实现文件I / O，控制台输入和许多其他功能。

这完成了Kaleidoscope教程的JIT和优化器章节。 此时，我们可以编译非Turing完整的编程语言，优化和JIT以用户驱动的方式编译它。 接下来我们将研究用[控制流构造扩展语言](http://llvm.org/docs/tutorial/LangImpl05.html)，解决一些有趣的LLVM IR问题。

### 4.5 Full Code Listing

以下是我们的运行示例的完整代码清单，使用LLVM JIT和优化器进行了增强。 要构建此示例，请使用：

```shell
# Compile
clang++ -g toy.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core mcjit native` -O3 -o toy
# Run
./toy
```

如果您在Linux上编译它，请确保添加“-rdynamic”选项。 这可确保在运行时正确解析外部函数。

[Here is the code](https://github.com/llvm-mirror/llvm/blob/master/examples/Kaleidoscope/Chapter4/toy.cpp)

##  5.Kaleidoscope: Extending the Language: Control Flow

### 5.1 第5章介绍

欢迎阅读“使用LLVM实现语言”教程的第5章。 第1-4部分介绍了简单的Kaleidoscope语言的实现，并包括对生成LLVM IR的支持，然后是优化和JIT编译器。 不幸的是，正如所呈现的那样，Kaleidoscope几乎没用：它除了调用和返回之外没有其他控制流。 这意味着您不能在代码中使用条件分支，从而显着限制其功能。 在本期“构建编译器”中，我们将扩展Kaleidoscope，使其具有if / then / else表达式和一个简单的'for'循环。

### 5.2 If/Then/Else

扩展Kaleidoscope以支持if / then / else非常简单。 它基本上只需要为词法分析器，解析器，AST和LLVM代码发布器添加对这个“新”概念的支持。 这个例子很好，因为它显示了随着时间的推移“增强”一种语言是多么容易，随着新想法的发现逐渐扩展它。

在我们开始“如何”添加此扩展之前，让我们谈谈我们想要的“什么”。 基本的想法是我们希望能够写出这样的东西：

```python
def fib(x)
  if x < 3 then
    1
  else
    fib(x-1)+fib(x-2);
```

在Kaleidoscope中，每个构造都是一个表达式：没有语句。 因此，if / then / else表达式需要返回与其他任何值相同的值。 由于我们使用的几乎是函数形式，我们将对其进行评估，然后根据条件的解析方式返回“then”或“else”值。 这与C“？：”表达非常相似。

if / then / else表达式的语义是它将条件计算为布尔相等的值：0.0被认为是假，其他一切被认为是真。 如果条件为真，则计算并返回第一个子表达式，如果条件为假，则计算并返回第二个子表达式。 由于Kaleidoscope允许单方面有效性，因此这种行为对于确定是非常重要的。

现在我们知道了我们“想要”的东西，让我们把它分解成它的组成部分。

#### 5.2.1. Lexer Extensions for If/Then/Else

词法分析器扩展很简单。 首先，我们为相关标记添加新的枚举值：

```c++
// control
tok_if = -6,
tok_then = -7,
tok_else = -8,
```

一旦我们有了这个，我们就会识别词法分析器中的新关键字。 这很简单：

```c++
...
if (IdentifierStr == "def")
  return tok_def;
if (IdentifierStr == "extern")
  return tok_extern;
if (IdentifierStr == "if")
  return tok_if;
if (IdentifierStr == "then")
  return tok_then;
if (IdentifierStr == "else")
  return tok_else;
return tok_identifier;
```

#### 5.2.2. AST Extensions for If/Then/Else

为了表示新表达式，我们为它添加了一个新的AST节点：

```c++
/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Cond, Then, Else;

public:
  IfExprAST(std::unique_ptr<ExprAST> Cond, std::unique_ptr<ExprAST> Then,
            std::unique_ptr<ExprAST> Else)
    : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}

  Value *codegen() override;
};
```

AST节点只有指向各种子表达式的指针。

#### 5.2.3. Parser Extensions for If/Then/Else

现在我们有来自词法分析器的相关标记，并且我们要构建AST节点，我们的解析逻辑相对简单。 首先我们定义一个新的解析函数：

```c++
/// ifexpr ::= 'if' expression 'then' expression 'else' expression
static std::unique_ptr<ExprAST> ParseIfExpr() {
  getNextToken();  // eat the if.

  // condition.
  auto Cond = ParseExpression();
  if (!Cond)
    return nullptr;

  if (CurTok != tok_then)
    return LogError("expected then");
  getNextToken();  // eat the then

  auto Then = ParseExpression();
  if (!Then)
    return nullptr;

  if (CurTok != tok_else)
    return LogError("expected else");

  getNextToken();

  auto Else = ParseExpression();
  if (!Else)
    return nullptr;

  return llvm::make_unique<IfExprAST>(std::move(Cond), std::move(Then),
                                      std::move(Else));
}
```

接下来我们将它作为主要表达式连接起来：

```c++
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
  case tok_if:
    return ParseIfExpr();
  }
}
```

#### 5.2.4. LLVM IR for If/Then/Else

现在我们已经解析并构建了AST，最后一部分是添加LLVM代码生成支持。 这是if / then / else示例中最有趣的部分，因为这是它开始引入新概念的地方。 上面的所有代码都已在前面的章节中详细介绍过。

为了激发我们想要生成的代码，让我们看一个简单的例子。 考虑：

```
extern foo();
extern bar();
def baz(x) if x then foo() else bar();
```

如果您不优化，您（很快）从Kaleidoscope获取的代码如下所示：

```
declare double @foo()

declare double @bar()

define double @baz(double %x) {
entry:
  %ifcond = fcmp one double %x, 0.000000e+00
  br i1 %ifcond, label %then, label %else

then:       ; preds = %entry
  %calltmp = call double @foo()
  br label %ifcont

else:       ; preds = %entry
  %calltmp1 = call double @bar()
  br label %ifcont

ifcont:     ; preds = %else, %then
  %iftmp = phi double [ %calltmp, %then ], [ %calltmp1, %else ]
  ret double %iftmp
}
```

要可视化控制流图，您可以使用LLVM“opt”工具的一个漂亮功能。 如果将此LLVM IR放入“t.ll”并运行“llvm-as <t.ll |” opt -analyze -view-cfg“，会弹出一个窗口，你会看到这个图：

![LangImpl05-cfg](https://github.com/Cyoung7/implement_llvm/blob/master/doc/image/LangImpl05-cfg.png)

另一种方法是通过将实际调用插入代码并重新编译或通过调用这些来调用“F-> viewCFG（）”或“F-> viewCFGOnly（）”（其中F是“函数*”）。调试器。 LLVM具有许多用于可视化各种图形的很好的功能。

回到生成的代码，它非常简单：条件语句评估条件表达式（在我们的例子中为“x”），并将结果与“fcmp one”指令进行比较（'one'是“Ordered and Not”等于”）。根据此表达式的结果，代码跳转到“then”或“else”块，其中包含true / false情况的表达式。

一旦then / else块完成执行，它们都会回到'ifcont'块以执行在if / then / else之后发生的代码。在这种情况下，唯一要做的就是返回函数的调用者。那么问题就变成了：代码如何知道返回哪个表达式？

这个问题的答案涉及一个重要的SSA操作：Phi操作。如果你不熟悉SSA，[维基百科的文章](http://en.wikipedia.org/wiki/Static_single_assignment_form)是一个很好的介绍，你最喜欢的搜索引擎上有各种其他介绍。简短的版本是Phi操作的“执行”需要“记住”哪个块控制来自哪里。 Phi操作采用与输入控制块相对应的值。在这种情况下，如果控制来自“then”块，它将获得“calltmp”的值。如果控制来自“else”块，则它获得“calltmp1”的值。

在这一点上，你可能开始想“哦不！这意味着我简单优雅的前端必须开始生成SSA表单才能使用LLVM！“幸运的是，事实并非如此，我们强烈建议不要在前端实施SSA构造算法，除非有一个非常好的理由这样做。实际上，在为可能需要Phi节点的普通命令式编程语言编写的代码中，有两种值可以浮动：

- 涉及用户变量的代码：x = 1; x = x + 1;
- AST结构中隐含的值，例如本例中的Phi节点。

在本教程的第7章（“可变变量”）中，我们将深入讨论＃1。 现在，请相信我，你不需要SSA构造来处理这种情况。 对于＃2，您可以选择使用我们将为＃1描述的技术，或者如果方便的话，您可以直接插入Phi节点。 在这种情况下，生成Phi节点非常容易，因此我们选择直接开始。

好的，足够的动力和概述，让我们生成代码！

#### 5.2.5. Code Generation for If/Then/Else

为了生成代码，我们为IfExprAST实现了codegen方法：

```c++
Value *IfExprAST::codegen() {
  Value *CondV = Cond->codegen();
  if (!CondV)
    return nullptr;

  // Convert condition to a bool by comparing non-equal to 0.0.
  CondV = Builder.CreateFCmpONE(
      CondV, ConstantFP::get(TheContext, APFloat(0.0)), "ifcond");
```

这段代码很简单，与我们之前看到的类似。 我们为条件emitted表达式，然后将该值与零进行比较，以获得真值作为1位（bool）值。

```c++
Function *TheFunction = Builder.GetInsertBlock()->getParent();

// Create blocks for the then and else cases.  Insert the 'then' block at the
// end of the function.
BasicBlock *ThenBB =
    BasicBlock::Create(TheContext, "then", TheFunction);
BasicBlock *ElseBB = BasicBlock::Create(TheContext, "else");
BasicBlock *MergeBB = BasicBlock::Create(TheContext, "ifcont");

Builder.CreateCondBr(CondV, ThenBB, ElseBB);
```

此代码创建与if / then / else语句相关的基本块，并直接对应于上例中的块。第一行获取正在构建的当前Function对象。它通过询问构建器当前的BasicBlock，并询问该块的“父”（它当前嵌入的函数）来获得这一点。

一旦有了，就会创建三个块。请注意，它将“TheFunction”传递给“then”块的构造函数。这会导致构造函数自动将新块插入指定函数的末尾。创建了另外两个块，但尚未插入到该函数中。

一旦创建了块，我们就可以emitted在它们之间选择的条件分支。请注意，创建新块不会隐式影响IRBuilder，因此它仍然会插入条件进入的块中。另请注意，它正在创建“then”块和“else”块的分支，即使“else”块尚未插入到函数中。这一切都很好：它是LLVM支持前向引用的标准方式。

```c++
// Emit then value.
Builder.SetInsertPoint(ThenBB);

Value *ThenV = Then->codegen();
if (!ThenV)
  return nullptr;

Builder.CreateBr(MergeBB);
// Codegen of 'Then' can change the current block, update ThenBB for the PHI.
ThenBB = Builder.GetInsertBlock();
```

插入条件分支后，我们移动构建器以开始插入“then”块。严格地说，此调用将插入点移动到指定块的末尾。但是，由于“then”块是空的，它也可以通过在块的开头插入来开始。 :)

一旦设置了插入点，我们递归地代码生成来自AST的“then”表达式。为了完成“then”块，我们为合并块创建了一个无条件分支。 LLVM IR的一个有趣（也是非常重要）方面是它需要使用控制流指令（例如return或branch）“终止”所有基本块。这意味着必须在LLVM IR中明确指出所有控制流。如果违反此规则，验证程序将emitted错误。

这里的最后一行非常微妙，但非常重要。基本问题是当我们在合并块中创建Phi节点时，我们需要设置块/值对，以指示Phi将如何工作。重要的是，Phi节点期望在CFG中具有块的每个前任的条目。那么，为什么我们在将它们设置为ThenBB 5行以上时获取当前块？问题在于“Then”表达式实际上可能会改变Builderemitted的块，例如，如果它包含嵌套的“if / then / else”表达式。因为以递归方式调用codegen（）可以任意改变当前块的概念，所以我们需要获得将设置Phi节点的代码的最新值。

```c++
// Emit else block.
TheFunction->getBasicBlockList().push_back(ElseBB);
Builder.SetInsertPoint(ElseBB);

Value *ElseV = Else->codegen();
if (!ElseV)
  return nullptr;

Builder.CreateBr(MergeBB);
// codegen of 'Else' can change the current block, update ElseBB for the PHI.
ElseBB = Builder.GetInsertBlock();
```

'else'块的代码生成与'then'块的codegen基本相同。 唯一显着的区别是第一行，它将'else'块添加到函数中。 先前回想一下，'else'块已创建，但未添加到该函数中。 既然emitted了'then'和'else'块，我们可以使用合并代码完成：

```c++
  // Emit merge block.
  TheFunction->getBasicBlockList().push_back(MergeBB);
  Builder.SetInsertPoint(MergeBB);
  PHINode *PN =
    Builder.CreatePHI(Type::getDoubleTy(TheContext), 2, "iftmp");

  PN->addIncoming(ThenV, ThenBB);
  PN->addIncoming(ElseV, ElseBB);
  return PN;
}
```

这里的前两行现在很熟悉：第一行将“merge”块添加到Function对象（它之前是浮动的，就像上面的else块一样）。 第二个更改插入点，以便新创建的代码将进入“合并”块。 完成后，我们需要创建PHI节点并为PHI设置块/值对。

最后，CodeGen函数返回phi节点作为if / then / else表达式计算的值。 在上面的示例中，此返回值将提供给顶层函数的代码，该函数将创建返回指令。

总的来说，我们现在能够在Kaleidoscope中执行条件代码。 通过此扩展，Kaleidoscope是一种相当完整的语言，可以计算各种数字函数。 接下来我们将添加另一个非函数语言熟悉的有用表达式...

### 5.3. ‘for’ Loop Expression

现在我们知道如何向语言添加基本控制流构造，我们有了添加更多功能的工具。 让我们添加更有意义的东西，`for`循环表达式：

```
extern putchard(char);
def printstar(n)
  for i = 1, i < n, 1.0 in
    putchard(42);  # ascii 42 = '*'

# print 100 '*' characters
printstar(100);
```

该表达式定义了一个新变量（在这种情况下为“i”），它从起始值迭代，而条件（在这种情况下为“i <n”）为真，递增一个可选步长值（在这种情况下为“1.0”））。 如果省略步长值，则默认为1.0。 循环为true时，它会执行其循环体。 因为我们没有更好的办法来处理，所以我们只需将循环定义为始终返回0.0。 将来当我们有可变变量时，它会变得更有用。

和以前一样，让我们来谈谈如何使得Kaleidoscope来支持这一功能。

#### 5.3.1. Lexer Extensions for the ‘for’ Loop

词法分析器扩展与if / then / else相同：

```c++
... in enum Token ...
// control
tok_if = -6, tok_then = -7, tok_else = -8,
tok_for = -9, tok_in = -10

... in gettok ...
if (IdentifierStr == "def")
  return tok_def;
if (IdentifierStr == "extern")
  return tok_extern;
if (IdentifierStr == "if")
  return tok_if;
if (IdentifierStr == "then")
  return tok_then;
if (IdentifierStr == "else")
  return tok_else;
if (IdentifierStr == "for")
  return tok_for;
if (IdentifierStr == "in")
  return tok_in;
return tok_identifier;
```

#### 5.3.2. AST Extensions for the ‘for’ Loop

AST节点也很简单。 它基本上归结为捕获节点中的变量名称和组成表达式。

```c++
/// ForExprAST - Expression class for for/in.
class ForExprAST : public ExprAST {
  std::string VarName;
  std::unique_ptr<ExprAST> Start, End, Step, Body;

public:
  ForExprAST(const std::string &VarName, std::unique_ptr<ExprAST> Start,
             std::unique_ptr<ExprAST> End, std::unique_ptr<ExprAST> Step,
             std::unique_ptr<ExprAST> Body)
    : VarName(VarName), Start(std::move(Start)), End(std::move(End)),
      Step(std::move(Step)), Body(std::move(Body)) {}

  Value *codegen() override;
};
```

#### 5.3.3. Parser Extensions for the ‘for’ Loop

解析器代码也是相当标准的。 这里唯一有趣的是处理可选的步骤值。 解析器代码通过检查第二个逗号是否存在来处理它。 如果不是，则在AST节点中将步长值设置为null：

```c++
/// forexpr ::= 'for' identifier '=' expr ',' expr (',' expr)? 'in' expression
static std::unique_ptr<ExprAST> ParseForExpr() {
  getNextToken();  // eat the for.

  if (CurTok != tok_identifier)
    return LogError("expected identifier after for");

  std::string IdName = IdentifierStr;
  getNextToken();  // eat identifier.

  if (CurTok != '=')
    return LogError("expected '=' after for");
  getNextToken();  // eat '='.


  auto Start = ParseExpression();
  if (!Start)
    return nullptr;
  if (CurTok != ',')
    return LogError("expected ',' after for start value");
  getNextToken();

  auto End = ParseExpression();
  if (!End)
    return nullptr;

  // The step value is optional.
  std::unique_ptr<ExprAST> Step;
  if (CurTok == ',') {
    getNextToken();
    Step = ParseExpression();
    if (!Step)
      return nullptr;
  }

  if (CurTok != tok_in)
    return LogError("expected 'in' after for");
  getNextToken();  // eat 'in'.

  auto Body = ParseExpression();
  if (!Body)
    return nullptr;

  return llvm::make_unique<ForExprAST>(IdName, std::move(Start),
                                       std::move(End), std::move(Step),
                                       std::move(Body));
}
```

 我们再次把它作为主要表达方式连接起来：

```c++
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
  case tok_if:
    return ParseIfExpr();
  case tok_for:
    return ParseForExpr();
  }
}
```

#### 5.3.4. LLVM IR for the ‘for’ Loop

现在我们得到了很好的部分：我们想为这件事生成的LLVM IR。 通过上面的简单示例，我们获得了这个LLVM IR（请注意，为了清楚起见，生成此转储并禁用优化）：

```
declare double @putchard(double)

define double @printstar(double %n) {
entry:
  ; initial value = 1.0 (inlined into phi)
  br label %loop

loop:       ; preds = %loop, %entry
  %i = phi double [ 1.000000e+00, %entry ], [ %nextvar, %loop ]
  ; body
  %calltmp = call double @putchard(double 4.200000e+01)
  ; increment
  %nextvar = fadd double %i, 1.000000e+00

  ; termination test
  %cmptmp = fcmp ult double %i, %n
  %booltmp = uitofp i1 %cmptmp to double
  %loopcond = fcmp one double %booltmp, 0.000000e+00
  br i1 %loopcond, label %loop, label %afterloop

afterloop:      ; preds = %loop
  ; loop always returns 0.0
  ret double 0.000000e+00
}
```

这个循环包含我们之前看到的所有相同结构：一个phi节点，几个表达式和一些基本块。 让我们看看这是如何组合在一起的。

#### 5.3.5. Code Generation for the ‘for’ Loop

codegen的第一部分非常简单：我们只输出循环值的起始表达式：

```c++
Value *ForExprAST::codegen() {
  // Emit the start code first, without 'variable' in scope.
  Value *StartVal = Start->codegen();
  if (!StartVal)
    return nullptr;
```

完成此操作后，下一步是为循环体的启动设置LLVM基本块。 在上面的例子中，整个循环体是一个块，但请记住，循环体代码本身可以由多个块组成（例如，如果它包含if / then / else或for / in表达式）。

```c++
// Make the new basic block for the loop header, inserting after current
// block.
Function *TheFunction = Builder.GetInsertBlock()->getParent();
BasicBlock *PreheaderBB = Builder.GetInsertBlock();
BasicBlock *LoopBB =
    BasicBlock::Create(TheContext, "loop", TheFunction);

// Insert an explicit fall through from the current block to the LoopBB.
Builder.CreateBr(LoopBB);
```

这段代码与我们在if / then / else中看到的类似。 因为我们需要它来创建Phi节点，所以我们记住进入循环的块。 一旦我们有了这个块，我们需要创建实际的块来执行循环,并为这两个块之间的连接创建一个无条件分支。

```c++
// Start insertion in LoopBB.
Builder.SetInsertPoint(LoopBB);

// Start the PHI node with an entry for Start.
PHINode *Variable = Builder.CreatePHI(Type::getDoubleTy(TheContext),
                                      2, VarName.c_str());
Variable->addIncoming(StartVal, PreheaderBB);
```

现在已经设置了循环的“preheader”，我们切换到循环体的发布代码。 首先，我们移动插入点并为循环变量创建PHI节点。 由于我们已经知道起始值的传入值，因此我们将其添加到Phi节点。 请注意，Phi最终将获得备份的第二个值，但我们还无法设置它（因为它不存在！）。

```c++
// Within the loop, the variable is defined equal to the PHI node.  If it
// shadows an existing variable, we have to restore it, so save it now.
Value *OldVal = NamedValues[VarName];
NamedValues[VarName] = Variable;

// Emit the body of the loop.  This, like any other expr, can change the
// current BB.  Note that we ignore the value computed by the body, but don't
// allow an error.
if (!Body->codegen())
  return nullptr;
```

现在代码开始变得更有趣了。 我们的'for'循环为符号表引入了一个新变量。 这意味着我们的符号表 现在可以包含函数参数或循环变量。 为了解决这个问题，在我们编写循环体之前，我们将循环变量添加为其名称的当前值。 请注意，外部作用域中可能存在同名变量。 很容易使这个错误（emitted错误并返回null，如果已经存在VarName的条目）但我们选择允许影子变量。 为了正确处理这个问题，我们记住了我们可能在OldVal中隐藏的值（如果没有影子变量，它将为null)

一旦将循环变量设置到符号表中，代码就会递归为body生成代码。 它允许body使用循环变量：在body的任意地方需要用到循环变量都可以在符号表中找到.

```c++
// Emit the step value.
Value *StepVal = nullptr;
if (Step) {
  StepVal = Step->codegen();
  if (!StepVal)
    return nullptr;
} else {
  // If not specified, use 1.0.
  StepVal = ConstantFP::get(TheContext, APFloat(1.0));
}

Value *NextVar = Builder.CreateFAdd(Variable, StepVal, "nextvar");
```

现在body被发布，我们通过步长值计算循环变量的下一个值，如果不存在则初始化为1.0。 'NextVar'将是循环下一次迭代时循环变量的值。

```c++
// Compute the end condition.
Value *EndCond = End->codegen();
if (!EndCond)
  return nullptr;

// Convert condition to a bool by comparing non-equal to 0.0.
EndCond = Builder.CreateFCmpONE(
    EndCond, ConstantFP::get(TheContext, APFloat(0.0)), "loopcond");
```

最后，我们评估循环的退出值，以确定循环是否应该退出。 这与if / then / else语句的条件判断相对应。

```c++
// Create the "after loop" block and insert it.
BasicBlock *LoopEndBB = Builder.GetInsertBlock();
BasicBlock *AfterBB =
    BasicBlock::Create(TheContext, "afterloop", TheFunction);

// Insert the conditional branch into the end of LoopEndBB.
Builder.CreateCondBr(EndCond, LoopBB, AfterBB);

// Any new code will be inserted in AfterBB.
Builder.SetInsertPoint(AfterBB);
```

随着循环body的代码完成，我们只需要为它完成控制流。 此代码记录结束块（对于phi节点），然后为循环退出创建一个块（“afterloop”）。 根据退出条件的value，它创建一个条件分支，选择再次执行循环或是退出循环。 任何将来的代码都会在“afterloop”块中emitted，因此它会为其设置插入位置。

```c++
  // Add a new entry to the PHI node for the backedge.
  Variable->addIncoming(NextVar, LoopEndBB);

  // Restore the unshadowed variable.
  if (OldVal)
    NamedValues[VarName] = OldVal;
  else
    NamedValues.erase(VarName);

  // for expr always returns 0.0.
  return Constant::getNullValue(Type::getDoubleTy(TheContext));
}
```

最终代码处理各种清理：现在我们有“NextVar”值，我们可以将传入值添加到循环PHI节点。 之后，我们从符号表中删除循环变量，使其在for循环后不在范围内。 最后，for循环的代码生成总是返回0.0，这就是我们从ForExprAST :: codegen（）返回的内容。

有了这个，我们总结了本教程中的“向Kaleidoscope添加控制流”一章。 在本章中，我们添加了两个控制流构造，并使用它们来激发LLVM IR的几个方面，这些方面对于前端实现者来说非常重要。 在我们的传奇的下一章中，我们会变得有点疯狂，并为我们可怜的无辜语言添加用户定义的运算符。

### 5.4. Full Code Listing

以下是我们正在运行的示例的完整代码清单，使用if / then / else和for表达式进行了增强。 要构建此示例，请使用：

```shell
# Compile
clang++ -g toy.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core mcjit native` -O3 -o toy
# Run
./toy
```

[Here is the code](https://github.com/llvm-mirror/llvm/blob/master/examples/Kaleidoscope/Chapter5/toy.cpp)

## 6. Kaleidoscope: Extending the Language: User-defined Operators

### 6.1. Chapter 6 Introduction

欢迎阅读“使用LLVM实现语言”教程的第6章。在我们的教程中，我们现在拥有一个功能完备的语言，它非常简单，但也很有用。然而，它仍然存在一个大问题。我们的语言没有很多有用的运算符（比如除法，逻辑否定，甚至除了小于之外的任何比较）。

本教程的这一章非常简单地将用户定义的运算符添加到简单而美观的Kaleidoscope语言中。这种插话现在在某些方面给我们一种简单而丑陋的语言，同时也是一种强大的语言。创建自己的语言的一个好处是你可以决定什么是好的或坏的。在本教程中，我们假设可以使用它作为一种方法来展示一些有趣的解析技术。

在本教程结束时，我们将运行一个示例[呈现the Mandelbrot set](http://llvm.org/docs/tutorial/LangImpl06.html#kicking-the-tires)的Kaleidoscope应用程序。这给出了一个使用Kaleidoscope及其功能集构建的示例。

### 6.2. User-defined Operators: the Idea

我们将添加到Kaleidoscope的“运算符重载”比C ++等语言更通用。在C++中，您只能重新定义现有运算符：您无法以编程的方式更改语法，引入新运算符，更改优先级等。在本章中，我们将此功能添加到Kaleidoscope中，这将使用户完善支持的运算符集。

在这个教程中允许用户定义的运算符,其展示了使用手写解析器的强大功能和灵活性。到目前为止，我们实现的解析器,大部分语法使用递归下降法,和表达式采用运算符优先级解析。详细信息请参见[第2章](http://llvm.org/docs/tutorial/LangImpl02.html)。通过使用运算符优先级解析，很容易让程序员在语法中引入新的运算符：语法在JIT运行时可以动态扩展。

我们将添加的两个特定功能是可编程的一元运算符（现在，Kaleidoscope完全没有一元运算符）以及二元运算符。一个例子是：

```
# Logical unary not.
def unary!(v)
  if v then
    0
  else
    1;

# Define > with the same precedence as <.
def binary> 10 (LHS RHS)
  RHS < LHS;

# Binary "logical or", (note that it does not "short circuit")
def binary| 5 (LHS RHS)
  if LHS then
    1
  else if RHS then
    1
  else
    0;

# Define = with slightly lower precedence than relationals.
def binary= 9 (LHS RHS)
  !(LHS < RHS | LHS > RHS);
```

许多语言都希望能够在语言本身中实现其标准运行时库。 在Kaleidoscope中，我们可以在库中实现该语言的重要部分！

我们将这些功能的实现分解为两部分：实现支持用户自定义的二元运算符以及添加一元运算符。

### 6.3. User-defined Binary Operators

使用我们当前的框架，添加对用户定义的二元运算符的支持非常简单。 我们首先添加对一元/二元关键字的支持：

```c++
enum Token {
  ...
  // operators
  tok_binary = -11,
  tok_unary = -12
};
...
static int gettok() {
...
    if (IdentifierStr == "for")
      return tok_for;
    if (IdentifierStr == "in")
      return tok_in;
    if (IdentifierStr == "binary")
      return tok_binary;
    if (IdentifierStr == "unary")
      return tok_unary;
    return tok_identifier;
```

这只是增加了对一元和二元关键字的词法分析器支持，就像我们在前几章中所做的那样。 关于我们当前AST的一个好处是，我们通过使用它们的ASCII代码作为操作码来表示完全泛化的二元运算符。 对于我们的扩展运算符，我们将使用相同的表示，因此我们不需要任何新的AST或解析器支持。

另一方面，我们必须能够在“def binary |”中表示这些新运算符的定义 5“功能定义的一部分。 到目前为止，在我们的语法中，函数定义的“名称”被解析为“原型”生成并进入PrototypeAST AST节点。 为了将我们新的用户定义的运算符表示为原型，我们必须扩展PrototypeAST AST节点，如下所示：

```c++
/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its argument names as well as if it is an operator.
class PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;
  bool IsOperator;
  unsigned Precedence;  // Precedence if a binary op.

public:
  PrototypeAST(const std::string &name, std::vector<std::string> Args,
               bool IsOperator = false, unsigned Prec = 0)
  : Name(name), Args(std::move(Args)), IsOperator(IsOperator),
    Precedence(Prec) {}

  Function *codegen();
  const std::string &getName() const { return Name; }

  bool isUnaryOp() const { return IsOperator && Args.size() == 1; }
  bool isBinaryOp() const { return IsOperator && Args.size() == 2; }

  char getOperatorName() const {
    assert(isUnaryOp() || isBinaryOp());
    return Name[Name.size() - 1];
  }

  unsigned getBinaryPrecedence() const { return Precedence; }
};
```

基本上，除了知道原型的名称之外，我们现在还要跟踪它是否是运算符，如果是，运算符的优先级是什么。 优先级仅用于二元运算符（如下所示，它不适用于一元运算符）。 既然我们有办法为用户定义的运算符表示原型，我们需要解析它：

```c++
/// prototype
///   ::= id '(' id* ')'
///   ::= binary LETTER number? (id, id)
static std::unique_ptr<PrototypeAST> ParsePrototype() {
  std::string FnName;

  unsigned Kind = 0;  // 0 = identifier, 1 = unary, 2 = binary.
  unsigned BinaryPrecedence = 30;

  switch (CurTok) {
  default:
    return LogErrorP("Expected function name in prototype");
  case tok_identifier:
    FnName = IdentifierStr;
    Kind = 0;
    getNextToken();
    break;
  case tok_binary:
    getNextToken();
    if (!isascii(CurTok))
      return LogErrorP("Expected binary operator");
    FnName = "binary";
    FnName += (char)CurTok;
    Kind = 2;
    getNextToken();

    // Read the precedence if present.
    if (CurTok == tok_number) {
      if (NumVal < 1 || NumVal > 100)
        return LogErrorP("Invalid precedence: must be 1..100");
      BinaryPrecedence = (unsigned)NumVal;
      getNextToken();
    }
    break;
  }

  if (CurTok != '(')
    return LogErrorP("Expected '(' in prototype");

  std::vector<std::string> ArgNames;
  while (getNextToken() == tok_identifier)
    ArgNames.push_back(IdentifierStr);
  if (CurTok != ')')
    return LogErrorP("Expected ')' in prototype");

  // success.
  getNextToken();  // eat ')'.

  // Verify right number of names for operator.
  if (Kind && ArgNames.size() != Kind)
    return LogErrorP("Invalid number of operands for operator");

  return llvm::make_unique<PrototypeAST>(FnName, std::move(ArgNames), Kind != 0,
                                         BinaryPrecedence);
}
```

这都是相当简单的解析代码，我们在过去已经看过很多类似的代码。 关于上面代码的一个有趣的部分是为二元运算符设置FnName的几行。 这为新定义的“@”运算符构建了类似“binary @”的名称。 然后，它利用了LLVM符号表中的符号名称允许包含任何字符的事实，包括嵌入的nul字符。

下一个有趣的事情是codegen支持这些二元运算符。 鉴于我们当前的结构，这是我们现有二元运算符节点的默认情况的简单添加：

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
    break;
  }

  // If it wasn't a builtin binary operator, it must be a user defined one. Emit
  // a call to it.
  Function *F = getFunction(std::string("binary") + Op);
  assert(F && "binary operator not found!");

  Value *Ops[2] = { L, R };
  return Builder.CreateCall(F, Ops, "binop");
}
```

如您所见，新代码实际上非常简单。 它只是在符号表中查找适当的运算符并生成对它的函数调用。 由于用户定义的运算符只是作为普通函数构建的（因为“原型”归结为具有正确名称的函数）所有内容都已落实到位。

我们遗漏的最后一段代码是一些顶级魔法：

```c++
Function *FunctionAST::codegen() {
  // Transfer ownership of the prototype to the FunctionProtos map, but keep a
  // reference to it for use below.
  auto &P = *Proto;
  FunctionProtos[Proto->getName()] = std::move(Proto);
  Function *TheFunction = getFunction(P.getName());
  if (!TheFunction)
    return nullptr;

  // If this is an operator, install it.
  if (P.isBinaryOp())
    BinopPrecedence[P.getOperatorName()] = P.getBinaryPrecedence();

  // Create a new basic block to start insertion into.
  BasicBlock *BB = BasicBlock::Create(TheContext, "entry", TheFunction);
```

基本上，在对代码进行代码化之前，如果它是用户定义的运算符，我们会在优先级表中注册它。 这允许二元运算符解析我们已有的逻辑来处理它。 由于我们正在研究一个完全通用的运算符优先级解析器，所以我们需要做的就是“扩展语法”。

现在我们有了有用的用户定义二元运算符。 这构建了我们为其他运营商构建的先前框架。 添加一元运算符更具挑战性，因为我们还没有任何框架 - 让我们看看它需要什么。

### 6.4. User-defined Unary Operators

由于我们目前不支持Kaleidoscope语言中的一元运算符，因此我们需要添加所有内容来支持它们。 上面，我们为词法分析器添加了对“一元”关键字的简单支持。 除此之外，我们需要一个AST节点：

```c++
/// UnaryExprAST - Expression class for a unary operator.
class UnaryExprAST : public ExprAST {
  char Opcode;
  std::unique_ptr<ExprAST> Operand;

public:
  UnaryExprAST(char Opcode, std::unique_ptr<ExprAST> Operand)
    : Opcode(Opcode), Operand(std::move(Operand)) {}

  Value *codegen() override;
};
```

到目前为止，这个AST节点非常简单明了。 它直接镜像二元运算符AST节点，除了它只有一个子节点。 有了这个，我们需要添加解析逻辑。 解析一元运算符非常简单：我们将添加一个新函数来执行此操作：

```c++
/// unary
///   ::= primary
///   ::= '!' unary
static std::unique_ptr<ExprAST> ParseUnary() {
  // If the current token is not an operator, it must be a primary expr.
  if (!isascii(CurTok) || CurTok == '(' || CurTok == ',')
    return ParsePrimary();

  // If this is a unary operator, read it.
  int Opc = CurTok;
  getNextToken();
  if (auto Operand = ParseUnary())
    return llvm::make_unique<UnaryExprAST>(Opc, std::move(Operand));
  return nullptr;
}
```

我们添加的语法非常简单。 如果我们在解析主运算符时看到一元运算符，我们将运算符作为前缀并将剩余的块解析为另一个一元运算符。 这允许我们处理多个一元运算符（例如“!! x”）。 请注意，一元运算符不能像二元运算符那样具有模糊的解析，因此不需要优先级信息。

这个函数的问题是我们需要从某个地方调用ParseUnary。 为此，我们改变ParsePrimary的先前调用者来调用ParseUnary：

```c++
/// binoprhs
///   ::= ('+' unary)*
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                              std::unique_ptr<ExprAST> LHS) {
  ...
    // Parse the unary expression after the binary operator.
    auto RHS = ParseUnary();
    if (!RHS)
      return nullptr;
  ...
}
/// expression
///   ::= unary binoprhs
///
static std::unique_ptr<ExprAST> ParseExpression() {
  auto LHS = ParseUnary();
  if (!LHS)
    return nullptr;

  return ParseBinOpRHS(0, std::move(LHS));
}
```

 通过这两个简单的更改，我们现在能够解析一元运算符并为它们构建AST。 接下来，我们需要为原型添加解析器支持，以解析一元运算符原型。 我们将上面的二元运算符代码扩展为：

```c++
/// prototype
///   ::= id '(' id* ')'
///   ::= binary LETTER number? (id, id)
///   ::= unary LETTER (id)
static std::unique_ptr<PrototypeAST> ParsePrototype() {
  std::string FnName;

  unsigned Kind = 0;  // 0 = identifier, 1 = unary, 2 = binary.
  unsigned BinaryPrecedence = 30;

  switch (CurTok) {
  default:
    return LogErrorP("Expected function name in prototype");
  case tok_identifier:
    FnName = IdentifierStr;
    Kind = 0;
    getNextToken();
    break;
  case tok_unary:
    getNextToken();
    if (!isascii(CurTok))
      return LogErrorP("Expected unary operator");
    FnName = "unary";
    FnName += (char)CurTok;
    Kind = 1;
    getNextToken();
    break;
  case tok_binary:
    ...
```

与二元运算符一样，我们将一元运算符命名为包含运算符字符的名称。 这有助于我们在代码生成时间。 说到，我们需要添加的最后一部分是对一元运算符的codegen支持。 它看起来像这样：

```c++
Value *UnaryExprAST::codegen() {
  Value *OperandV = Operand->codegen();
  if (!OperandV)
    return nullptr;

  Function *F = getFunction(std::string("unary") + Opcode);
  if (!F)
    return LogErrorV("Unknown unary operator");

  return Builder.CreateCall(F, OperandV, "unop");
}
```

此代码与二进制运算符的代码类似，但更简单。 它更简单，主要是因为它不需要处理任何预定义的运算符。

### 6.5. Kicking the Tires

这有点难以置信，但是我们已经在最后几章中介绍了一些简单的扩展，我们已经成长为一种真正的语言。 有了这个，我们可以做很多有趣的事情，包括I / O，数学和其他一些东西。 例如，我们现在可以添加一个很好的排序运算符（printd被定义为打印出指定的值和换行符）：

```
ready> extern printd(x);
Read extern:
declare double @printd(double)

ready> def binary : 1 (x y) 0;  # Low-precedence operator that ignores operands.
...
ready> printd(123) : printd(456) : printd(789);
123.000000
456.000000
789.000000
Evaluated to 0.000000
```

我们还可以定义一堆其他“原始”操作，例如：

```
# Logical unary not.
def unary!(v)
  if v then
    0
  else
    1;

# Unary negate.
def unary-(v)
  0-v;

# Define > with the same precedence as <.
def binary> 10 (LHS RHS)
  RHS < LHS;

# Binary logical or, which does not short circuit.
def binary| 5 (LHS RHS)
  if LHS then
    1
  else if RHS then
    1
  else
    0;

# Binary logical and, which does not short circuit.
def binary& 6 (LHS RHS)
  if !LHS then
    0
  else
    !!RHS;

# Define = with slightly lower precedence than relationals.
def binary = 9 (LHS RHS)
  !(LHS < RHS | LHS > RHS);

# Define ':' for sequencing: as a low-precedence operator that ignores operands
# and just returns the RHS.
def binary : 1 (x y) y;
```

鉴于之前的if / then / else支持，我们还可以为I / O定义有趣的函数。 例如，下面打印出一个字符，其“密度”反映传入的值：值越低，字符越密集：

```
ready> extern putchard(char);
...
ready> def printdensity(d)
  if d > 8 then
    putchard(32)  # ' '
  else if d > 4 then
    putchard(46)  # '.'
  else if d > 2 then
    putchard(43)  # '+'
  else
    putchard(42); # '*'
...
ready> printdensity(1): printdensity(2): printdensity(3):
       printdensity(4): printdensity(5): printdensity(9):
       putchard(10);
**++.
Evaluated to 0.000000
```

基于这些简单的原始操作，我们可以开始定义更有趣的事情。 例如，这是一个小函数，它确定复平面中某个函数发散所需的迭代次数：

```
# Determine whether the specific location diverges.
# Solve for z = z^2 + c in the complex plane.
def mandelconverger(real imag iters creal cimag)
  if iters > 255 | (real*real + imag*imag > 4) then
    iters
  else
    mandelconverger(real*real - imag*imag + creal,
                    2*real*imag + cimag,
                    iters+1, creal, cimag);

# Return the number of iterations required for the iteration to escape
def mandelconverge(real imag)
  mandelconverger(real, imag, 0, real, imag);
```

这个“z = z2 + c”函数是一个漂亮的小生物，是计算Mandelbrot集的基础。 我们的mandelconverge函数返回复杂轨道逃逸所需的迭代次数，饱和到255.这本身并不是一个非常有用的函数，但是如果你在二维平面上绘制它的值，你可以看到Mandelbrot 组。 鉴于我们仅限于在这里使用putchard，我们惊人的图形输出是有限的，但我们可以使用上面的密度绘图仪鞭打一些东西：

```c++
# Compute and plot the mandelbrot set with the specified 2 dimensional range
# info.
def mandelhelp(xmin xmax xstep   ymin ymax ystep)
  for y = ymin, y < ymax, ystep in (
    (for x = xmin, x < xmax, xstep in
       printdensity(mandelconverge(x,y)))
    : putchard(10)
  )

# mandel - This is a convenient helper function for plotting the mandelbrot set
# from the specified position with the specified Magnification.
def mandel(realstart imagstart realmag imagmag)
  mandelhelp(realstart, realstart+realmag*78, realmag,
             imagstart, imagstart+imagmag*40, imagmag);
```

鉴于此，我们可以尝试绘制出mandelbrot集！ 让我们尝试一下：

```
ready> mandel(-2.3, -1.3, 0.05, 0.07);
ready> mandel(-2, -1, 0.02, 0.04);
ready> mandel(-0.9, -1.4, 0.02, 0.03);
```

此时，您可能已经开始意识到Kaleidoscope是一种真实而强大的语言。 它可能不是自相似的:)，但它可以用于绘制事物！

有了这个，我们总结了本教程的“添加用户定义的运算符”一章。 我们已经成功地扩充了我们的语言，添加了扩展库中语言的能力，并且我们已经展示了如何在Kaleidoscope中构建一个简单但有趣的最终用户应用程序。 此时，Kaleidoscope可以构建各种功能性的应用程序，并且可以调用具有副作用的函数，但它实际上无法定义和变异变量本身。

引人注目的是，变量变异是某些语言的一个重要特征，如何在不必向前端添加“SSA构造”阶段的情况下添加对[可变变量的支持](http://llvm.org/docs/tutorial/LangImpl07.html)并不是很明显。 在下一章中，我们将介绍如何在不在前端构建SSA的情况下添加变量变异。

### 6.6. Full Code Listing

以下是我们运行示例的完整代码清单，增强了对用户定义运算符的支持。 要构建此示例，请使用：

```
# Compile
clang++ -g toy.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core mcjit native` -O3 -o toy
# Run
./toy
```

在某些平台上，您需要在链接时指定-rdynamic或-Wl，-export-dynamic。 这可确保主可执行文件中定义的符号导出到动态链接器，因此可在运行时进行符号解析。 如果将支持代码编译到共享库中，则不需要这样做，但这样做会导致Windows出现问题。

[Here is the code](https://github.com/llvm-mirror/llvm/blob/master/examples/Kaleidoscope/Chapter6/toy.cpp)

