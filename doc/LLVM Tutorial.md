# LLVM Tutorial

[链接](http://llvm.org/docs/tutorial/)

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

这里的所有（有意而为）都相当直接：变量捕获变量名称，二元运算符捕获它们的操作符（例如'+'），并且调用捕获函数名称以及任何参数表达式的列表。 关于我们的AST的一个好处是它捕获语言特性而不关心语言的语法。 请注意，这里没有讨论二元运算符的优先级，词法结构等。

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

在Kaleidoscope中，只使用参数的个数来作为函数输入。 由于所有值都是双精度浮点数，因此每个参数的类型不需要任何存储空间。 在更进一步和现实性的语言中，“ExprAST”类可能具有类型字段。

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

这在词法分析器里实现了一个简单的标记缓冲区。 这允许我们在词法分析器返回时提前查看下一个符号。 我们解析器中的每个函数都假定CurTok是需要解析的当前符号。

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

LogError事务是我们的解析器将用于处理错误的简单帮助程序事务。 我们的解析器中的错误恢复不是最好的，并且对用户不是特别友好的，但它对我们的教程来说已经足够了。 这些事务使得在具有各种返回类型的事务中更容易处理错误：它们全部返回null。

有了这些基本的辅助函数，我们就可以实现我们语法的第一部分：数字文字。

### 2.4 基础表达式解析(Basic Expression Parsing)

我们从数字开始，因为它们处理起来最简单。 对于语法中的每个作业，我们将定义一个解析该作业的函数。 对于数字，我们有：

```c++
/// numberexpr ::= number
static std::unique_ptr<ExprAST> ParseNumberExpr() {
  auto Result = llvm::make_unique<NumberExprAST>(NumVal);
  getNextToken(); // consume the number
  return std::move(Result);
}
```

这个事务非常简单：当前符号是tok_number符号时，它希望被调用。 它获取当前数字值，创建NumberExprAST节点，将词法分析器前进到下一个符号，最后返回。

这有一些有趣的方面。 最重要的一点是，这个例事务会占用与其相关的所有符号，并返回带有下一个符号（它不是语法生成的一部分）的词法分析器缓冲区(buffer)。 这是递归下降解析器的一种相当标准的方法。 有关更好的示例，括号运算符的定义如下：

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
- 2）这个函数的另一个有趣的方面是它通过调用ParseExpression使用递归（我们很快就会看到ParseExpression可以调用ParseParenExpr）。这很强大，因为它允许我们处理递归语法，并使每个作业变得非常简单。请注意，括号本身不会导致AST节点的建立。虽然我们可以这样做，但括号最重要的作用是引导解析器并提供分组。解析器构造AST后，不需要括号。

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