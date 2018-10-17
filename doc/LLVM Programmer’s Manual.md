# LLVM Programmer’s Manual

[原文链接](http://llvm.org/docs/ProgrammersManual.html#the-c-standard-template-library)

目录生成指令:

./gh-md-toc /home/cyoung/CLionProjects/implement_llvm/doc/LLVM\ Programmer’s\ Manual.md 

[TOC]

## Introduction

本文档旨在强调LLVM源代码库中可用的一些重要类和接口。本手册无意解释什么是LLVM，工作原理以及LLVM代码的外在。它假设您了解LLVM的基础知识，并且有兴趣编写转换(transformations)或以其他方式(analyzing)分析或操作代码。

本文档应该让您了解，在LLVM基础结构的不断增长的源代码中,以便找到自己的学习方式。请注意，本手册并非用于替代源代码阅读，因此如果您认为其中一个类中应该有一个方法可以执行某些操作但未列出，请检查源代码。提供了与[doxygen](http://llvm.org/doxygen/)源的链接，以使其尽可能方便。

本文档的第一部分描述了在LLVM基础结构中工作时有用的一般信息，第二部分描述了Core LLVM类。将来本手册将扩展，其中包含描述如何使用扩展库的信息，例如支配者(dominator)信息，CFG遍历过程以及InstVisitor（doxygen）模板等有用的实用程序。

## General Information

本节包含在LLVM源代码库中工作时非常有用的一般信息，但这并非针对任何特定API。

### The C++ Standard Template Library

LLVM大量使用C ++标准模板库（STL），可能比您以前或之前看到的要多得多。 因此，您可能希望对所使用的技术和库的功能进行一些背景阅读。 有许多讨论STL的好网页，以及关于这个主题的几本书，它将不会在本文中讨论。

以下是一些有用的链接：

- [cppreference.com](http://en.cppreference.com/w/) - STL和标准C ++库其他部分的出色参考。
- [C++ In a Nutshell](http://www.tempest-sw.com/cpp/) - 这是一本正在制作的O'Reilly书。 它有一个体面的标准库参考，可以与Dinkumware相媲美，但遗憾的是，自该书出版以来，它已不再免费。
- [C++ Frequently Asked Questions](http://www.parashift.com/c++-faq-lite/)
- [SGI’s STL Programmer’s Guide](http://www.sgi.com/tech/stl/) - 包含有用的[STL简介](http://www.sgi.com/tech/stl/stl_introduction.html)。
- [Bjarne Stroustrup’s C++ Page](http://www.research.att.com/~bs/C++.html)
- [Bruce Eckel’s Thinking in C++, 2nd ed. Volume 2 Revision 4.0 (even better, get the book)](http://www.mindview.net/Books/TICPP/ThinkingInCPP2e.html)

我们还鼓励您查看[LLVM编码标准指南](http://llvm.org/docs/CodingStandards.html)，该指南侧重于如何编写可维护代码而不是如何放置花括号的位置。

### Other useful references

[跨平台使用静态和共享库](http://www.fortran-2000.com/ArnaudRecipes/sharedlib.html)

## Important and useful LLVM APIs

在这里，我们重点介绍一些LLVM API，这些API通常很有用，并且在编写转换时很有用。

### The isa<>, cast<> and dyn_cast<> templates

LLVM源代码库广泛使用自定义形式的RTTI。 这些模板与C ++ dynamic_cast <>运算符有许多相似之处，但它们避免了一些缺点（主要因为dynamic_cast <>仅适用于具有v-table的类）。 因为它们经常被使用，所以你必须知道它们的作用以及它们的工作方式。 所有这些模板都在`llvm / Support / Casting.h`[doxygen](http://llvm.org/doxygen/Casting_8h_source.html)文件中定义（请注意，您很少需要直接包含此文件）。

``isa<>``:

- isa <>运算符的工作方式与Java“instanceof”运算符完全相同。 它返回true或false，具体取决于引用或指针是否指向指定类的实例。 这对于各种排序的约束检查非常有用（例如下面的例子）。

``cast<>``:

- cast <>运算符是“已检查的强转换”操作。 它将指针或引用从基类转换为派生类，如果它实际不是正确类型的实例，则会导致断言失败。 这应该用于您有一些信息让您相信某些东西属于正确类型的情况。 isa <>和cast <>模板的一个示例是：

```c++
static bool isLoopInvariant(const Value *V, const Loop *L) {
  if (isa<Constant>(V) || isa<Argument>(V) || isa<GlobalValue>(V))
    return true;

  // Otherwise, it must be an instruction...
  return !L->contains(cast<Instruction>(V)->getParent());
}
```

请注意，您最好不要使用isa <>测试后跟一个cast <>的这种方式，从而使用dyn_cast <>运算符更好的替代

`dyn_cast<>`:

- dyn_cast <>运算符是“检查强制转换”操作。 它检查操作数是否属于指定类型，如果是，则返回指向它的指针（此运算符不适用于引用）。 如果操作数的类型不正确，则返回空指针。 因此，这非常类似于C ++中的dynamic_cast <>运算符，并且应该在相同的情况下使用。 通常，dyn_cast <>运算符用于if语句或其他一些流控制语句，如下所示：

```c++
if (auto *AI = dyn_cast<AllocationInst>(Val)) {
  // ...
}
```

- 这种形式的if语句有效地将对isa <>的调用和对cast <>的调用组合在一起，这非常方便。

  请注意，dyn_cast <>运算符（如C ++的dynamic_cast <>或Java的instanceof运算符）可能会被滥用。 特别是，您不应该使用大型链接if / then / else块来检查许多不同的类变体。 如果您发现自己想要这样做，那么使用InstVisitor类直接调度指令类型会更清晰，更有效。

`cast_or_null<>`:

- cast_or_null <>运算符与cast <>运算符的工作方式类似，不同之处在于它允许空指针作为参数（然后传播）。 这有时很有用，允许您将多个空检查合并为一个。

`dyn_cast_or_null<>`:

- dyn_cast_or_null <>运算符的工作原理与dyn_cast <>运算符类似，不同之处在于它允许空指针作为参数（然后传播）。 这有时很有用，允许您将多个空检查合并为一个。

这五个模板可以用于任何类，无论它们是否具有v表。 如果要添加对这些模板的支持，请参阅文档[如何为类层次结构设置LLVM样式的RTTI](http://llvm.org/docs/HowToSetUpLLVMStyleRTTI.html)

### Passing strings (the StringRef and Twine classes)

虽然LLVM通常不会进行太多字符串操作，但我们确实有几个重要的API需要字符串。 两个重要的例子是`Value`类 - 它具有指令，函数等的名称 - 以及在LLVM和Clang中广泛使用的`StringMap`类。

这些是泛型类，它们需要能够接受可能嵌入空字符的字符串。 因此，他们不能简单地使用`const char *`，并且使用const std :: string并要求客户端执行通常不必要的堆分配。 相反，许多LLVM API使用`StringRef`或`const Twine＆`来有效地传递字符串。

#### The StringRef class

`StringRef`数据类型表示对常量字符串（字符数组和长度）的引用，并支持std :: string上可用的常见操作，但不需要堆分配。

它可以使用C样式的以null结尾的字符串，`std :: string`或显式的字符指针和长度来隐式构造。 例如，`StringRef find`函数声明为：

```c++
iterator find(StringRef Key);
```

并且用户可以使用以下任何一种方式调用它：

```c++
Map.find("foo");                 // Lookup "foo"
Map.find(std::string("bar"));    // Lookup "bar"
Map.find(StringRef("\0baz", 4)); // Lookup "\0baz"
```

类似地，需要返回字符串的API可以返回StringRef实例，该实例可以使用str成员函数直接使用或转换为std :: string。 有关更多信息，请参阅`llvm / ADT / StringRef.h`（[doxygen](http://llvm.org/doxygen/StringRef_8h_source.html)）。

您应该很少直接使用StringRef类，因为它包含指向外部存储器的指针，通常不能安全存储类的实例（除非您知道不会释放外部存储器）。 StringRef在LLVM中很小且非常普遍，它应该始终按值传递。

#### The Twine class

Twine（[doxygen](http://llvm.org/doxygen/classllvm_1_1Twine.html)）类是API接受拼接(concat)字符串的有效方式。 例如，一个常见的LLVM范例是根据具有后缀的另一个指令的名称命名一条指令，例如：

```c++
New = CmpInst::Create(..., SO->getName() + ".cmp");
```

Twine类实际上是一个轻量级的[repo](http://en.wikipedia.org/wiki/Rope_(computer_science) )，指向临时（堆栈分配）对象。 可以隐式构造Twines作为应用于字符串的加号运算符的结果（即，C字符串，`std :: string`或`StringRef`）。 该字符串延迟了字符串的实际连接时间，直到实际需要它才链接，此时它可以直接有效地呈现到字符数组中。 这避免了构造字符串连接的临时结果所涉及不必要的堆分配(简单来说就是一个lazy机制)。 有关更多信息，请参阅`llvm / ADT / Twine.h`（[doxygen](http://llvm.org/doxygen/Twine_8h_source.html)）和[此处](http://llvm.org/docs/ProgrammersManual.html#dss-twine)。

与`StringRef`一样，Twine对象指向外部存储器，几乎不应直接存储或提及。 它们仅用于定义应该能够有效接受连接字符串的函数。

### Formatting strings (the formatv function)

虽然LLVM不一定会进行大量的字符串操作和解析，但它确实会进行大量的字符串格式化(formatting)。 从诊断消息到llvm工具输出（如llvm-readobj）到打印详细的反汇编列表和LLDB运行时日志记录，字符串格式化的需求是普遍存在的。

formatv在本质上与printf类似，但使用了不同的语法，这些语法大量借鉴了Python和C＃。 与printf不同，它推导出在编译时要格式化的类型，因此它不需要格式说明符，例如`％d`。 这减少了尝试构造可移植格式字符串的心理开销，尤其是对应于特定平台的类型（如size_t或指针类型）。 与printf和Python不同，如果LLVM不知道如何格式化类型，它还无法编译。 这两个属性确保函数比传统格式化方法（如printf函数系列）更安全，更简单。

#### Simple formatting

对`formatv`的调用涉及由0个或更多替换序列(**replacement sequences**)组成的单个格式字符串( **format string**)，后跟可变长度的替换值列表(**replacement values**)。替换序列是`{N [[，align]：style]}`形式的字符串。

N指的是替换值列表中参数的从0开始的索引。请注意，这意味着可以按任何顺序多次引用相同的参数，可能使用不同的样式和/或对齐选项。

`align`是一个可选字符串，指定要将值格式化的字段宽度，以及字段中值的对齐方式。它被指定为可选的对齐样式，后跟正整数字段宽度。对齐样式可以是字符之一 - （左对齐），=（中心对齐）或+（右对齐）。默认为右对齐。

`style`是一个可选字符串，由一个特定类型组成，用于控制值的格式。例如，要将浮点值格式化为百分比，可以使用样式选项P.

#### Custom formatting

有两种方法可以自定义类型的格式化行为。

1.使用适当的静态格式方法为类型T提供llvm :: format_provider <T>的模板特化

```c++
namespace llvm {
  template<>
  struct format_provider<MyFooBar> {
    static void format(const MyFooBar &V, raw_ostream &Stream, StringRef Style) {
      // Do whatever is necessary to format `V` into `Stream`
    }
  };
  void foo() {
    MyFooBar X;
    std::string S = formatv("{0}", X);
  }
}
```

这是一种有用的可扩展性机制，用于添加对使用您自己的自定义样式选项格式化自己的自定义类型的支持。 但是当你想扩展格式化库已经知道如何格式化的类型的机制时，它没有用。 为此，我们还需要其他东西。

2.提供继承自llvm :: FormatAdapter <T>的格式适配器。

```c++
namespace anything {
  struct format_int_custom : public llvm::FormatAdapter<int> {
    explicit format_int_custom(int N) : llvm::FormatAdapter<int>(N) {}
    void format(llvm::raw_ostream &Stream, StringRef Style) override {
      // Do whatever is necessary to format ``this->Item`` into ``Stream``
    }
  };
}
namespace llvm {
  void foo() {
    std::string S = formatv("{0}", anything::format_int_custom(42));
  }
}
```

如果检测到类型是从FormatAdapter <T>派生的，则formatv将在传递指定样式的参数上调用format方法。 这允许提供任何类型的自定义格式，包括已具有内置格式提供者的格式。

#### formatv Examples

下面的目的是提供一组不完整的示例，演示formatv的用法。 通过阅读doxygen文档或查看单元测试套件，可以找到更多信息。

```c++
std::string S;
// Simple formatting of basic types and implicit string conversion.
S = formatv("{0} ({1:P})", 7, 0.35);  // S == "7 (35.00%)"

// Out-of-order referencing and multi-referencing
outs() << formatv("{0} {2} {1} {0}", 1, "test", 3); // prints "1 3 test 1"

// Left, right, and center alignment
S = formatv("{0,7}",  'a');  // S == "      a";
S = formatv("{0,-7}", 'a');  // S == "a      ";
S = formatv("{0,=7}", 'a');  // S == "   a   ";
S = formatv("{0,+7}", 'a');  // S == "      a";

// Custom styles
S = formatv("{0:N} - {0:x} - {1:E}", 12345, 123908342); // S == "12,345 - 0x3039 - 1.24E8"

// Adapters
S = formatv("{0}", fmt_align(42, AlignStyle::Center, 7));  // S == "  42   "
S = formatv("{0}", fmt_repeat("hi", 3)); // S == "hihihi"
S = formatv("{0}", fmt_pad("hi", 2, 6)); // S == "  hi      "

// Ranges
std::vector<int> V = {8, 9, 10};
S = formatv("{0}", make_range(V.begin(), V.end())); // S == "8, 9, 10"
S = formatv("{0:$[+]}", make_range(V.begin(), V.end())); // S == "8+9+10"
S = formatv("{0:$[ + ]@[x]}", make_range(V.begin(), V.end())); // S == "0x8 + 0x9 + 0xA"
```

### Error handling

正确的错误处理有助于我们识别代码中的错误，并帮助最终用户了解其工具使用中的错误。 错误分为两大类：程序性和可恢复性，具有不同的处理和报告策略。

#### Programmatic Errors

程序错误是程序不变量或API契约的违反，并代表程序本身的错误。 我们的目标是记录不变量，并在运行时断开不变量时在故障点快速中止（提供一些基本诊断）。

处理程序错误的基本工具是断言和llvm_unreachable函数。 断言用于表示不变条件，并应包含描述不变量的消息：

```c++
assert(isPhysReg(R) && "All virt regs should have been allocated already.");
```

llvm_unreachable函数可用于记录在程序不变量持有时不应输入的控制流区域：

```c++
enum { Foo, Bar, Baz } X = foo();

switch (X) {
  case Foo: /* Handle Foo */; break;
  case Bar: /* Handle Bar */; break;
  default:
    llvm_unreachable("X should be Foo or Bar here");
}
```

#### Recoverable Errors

可恢复的错误表示程序环境中的错误，例如资源故障（丢失的文件，丢失的网络连接等）或格式错误的输入。 应检测这些错误并将其传达给程序级别，以便对其进行适当处理。 处理错误可能与向用户报告问题一样简单，也可能涉及恢复尝试。

> 注意:虽然在整个LLVM中使用这种错误处理方案是理想的，但有些地方可能无法应用。 在绝对必须发出非编程错误且错误模型不可行的情况下，您可以调用report_fatal_error，它将调用已安装的错误处理程序，打印消息并退出程序。

使用LLVM的错误方案对可恢复错误进行建模。 此方案使用函数返回值表示错误，类似于经典C整数错误代码或C ++的std :: error_code。 但是，Error类实际上是用户定义的错误类型的轻量级包装器，允许附加任意信息来描述错误。 这类似于C ++异常允许抛出用户定义类型的方式。

成功值是通过调用`Error :: success（）`创建的，例如：

```c++
Error foo() {
  // Do something.
  // Return success.
  return Error::success();
}
```

构建和返回成功值非常便宜 - 它们对程序性能的影响很小。

使用`make_error <T>`构造失败值，其中T是从`ErrorInfo`实用程序继承的任何类，例如：

```c++
class BadFileFormat : public ErrorInfo<BadFileFormat> {
public:
  static char ID;
  std::string Path;

  BadFileFormat(StringRef Path) : Path(Path.str()) {}

  void log(raw_ostream &OS) const override {
    OS << Path << " is malformed";
  }

  std::error_code convertToErrorCode() const override {
    return make_error_code(object_error::parse_failed);
  }
};

char BadFileFormat::ID; // This should be declared in the C++ file.

Error printFormattedFile(StringRef Path) {
  if (<check for valid format>)
    return make_error<BadFileFormat>(Path);
  // print file contents.
  return Error::success();
}
```

错误值可以隐式转换为bool：true表示错误，false表示成功，启用以下习语：

```c++
Error mayFail();

Error foo() {
  if (auto Err = mayFail())
    return Err;
  // Success! We can proceed.
  ...
```

 对于可能失败但需要返回值的函数，可以使用Expected <T>实用程序。 可以使用T或Error构造此类型的值。 预期的<T>值也可以隐式转换为布尔值，但使用与Error相反的约定：true表示成功，false表示错误。 如果成功，则可以通过解引用运算符访问T值。 如果失败，可以使用takeError（）方法提取Error值。 惯用法看起来像：

```c++
Expected<FormattedFile> openFormattedFile(StringRef Path) {
  // If badly formatted, return an error.
  if (auto Err = checkFormat(Path))
    return std::move(Err);
  // Otherwise return a FormattedFile instance.
  return FormattedFile(Path);
}

Error processFormattedFile(StringRef Path) {
  // Try to open a formatted file
  if (auto FileOrErr = openFormattedFile(Path)) {
    // On success, grab a reference to the file and continue.
    auto &File = *FileOrErr;
    ...
  } else
    // On error, extract the Error value and return it.
    return FileOrErr.takeError();
}
```

如果`Expected <T>`值处于成功模式，则`takeError（）`方法将返回成功值。 使用这个事实，上面的函数可以重写为：

```c++
Error processFormattedFile(StringRef Path) {
  // Try to open a formatted file
  auto FileOrErr = openFormattedFile(Path);
  if (auto Err = FileOrErr.takeError())
    // On error, extract the Error value and return it.
    return Err;
  // On success, grab a reference to the file and continue.
  auto &File = *FileOrErr;
  ...
}
```

对于涉及多个Expected <T>值的函数，第二种形式通常更具可读性，因为它限制了所需的缩进。

所有错误实例，无论是成功还是失败，必须在销毁之前检查或移动（通过std :: move或return）。 意外丢弃未经检查的错误将导致程序在运行未经检查的值的析构函数时中止，从而可以轻松识别并修复违反此规则的行为。

一旦测试成功值（通过调用布尔转换运算符），就会将其视为已检查：

```c++
if (auto Err = mayFail(...))
  return Err; // Failure value - move error to caller.

// Safe to continue: Err was checked.
```

相反，即使mayFail返回成功值，以下代码也总是会导致中止：

```c++
mayFail();
// Program will always abort here, even if mayFail() returns Success, since
// the value is not checked. 
```

一旦激活了错误类型的处理程序，就会考虑检查失败值：

```c++
handleErrors(
  processFormattedFile(...),
  [](const BadFileFormat &BFF) {
    report("Unable to process " + BFF.Path + ": bad format");
  },
  [](const FileNotFound &FNF) {
    report("File not found " + FNF.Path);
  });
```

`handleErrors`函数将错误作为其第一个参数，后跟可变列表“处理程序”，每个“处理程序”必须是带有一个参数的可调用类型（函数，`lambda`或带调用运算符的类）。 `handleErrors`函数将访问序列中的每个处理程序，并根据错误的动态类型检查其参数类型，运行匹配的第一个处理程序。 这与用于决定为C ++异常运行哪个catch子句的决策过程相同。

由于传递给`handleErrors`的处理程序列表可能未涵盖可能发生的每种错误类型，因此handleErrors函数还会返回必须检查或传播的Error值。 如果传递给`handleErrors`的错误值与任何处理程序都不匹配，则它将从`handleErrors`返回。 因此，惯用法使用`handleErrors`看起来像：

```c++
if (auto Err =
      handleErrors(
        processFormattedFile(...),
        [](const BadFileFormat &BFF) {
          report("Unable to process " + BFF.Path + ": bad format");
        },
        [](const FileNotFound &FNF) {
          report("File not found " + FNF.Path);
        }))
  return Err;
```

如果您确实知道处理程序列表是详尽的，则可以使用`handleAllErrors`函数。这与`handleErrors`相同，只是如果传入未处理的错误它将终止程序，因此可以返回void。通常应该避免使用`handleAllErrors`函数：在程序的其他地方引入新的错误类型可以很容易地将以前详尽的错误列表转换为非详尽的列表，从而有可能导致程序意外终止。在可能的情况下，使用`handleErrors`并将未知错误传播到堆栈中。

对于工具代码，可以通过打印错误消息然后退出错误代码来处理错误，`ExitOnError`实用程序可能是一个比`handleErrors`更好的选择，因为它在调用易错函数时简化了控制流程。

在已知对易错函数的特定调用将始终成功的情况下（例如，调用只能在已知安全的输入的输入子集上失败的函数），`cantFail`函数可以是用于删除错误类型，简化控制流程。

##### StringError

StringError许多类型的错误都没有恢复策略，可以采取的唯一操作是将它们报告给用户，以便用户可以尝试修复环境。 在这种情况下，将错误表示为字符串非常有意义。 LLVM为此提供StringError类。 它需要两个参数：字符串错误消息，以及用于互操作性的等效`std :: error_code`：

```c++
make_error<StringError>("Bad executable",
                        make_error_code(errc::executable_format_error"));
```

如果你确定你正在构建的错误永远不需要转换为std :: error_code，你可以使用inconvertibleErrorCode（）函数：

```c++
make_error<StringError>("Bad executable", inconvertibleErrorCode());
```

这只应在仔细考虑后才能完成。 如果尝试将此错误转换为`std :: error_code`，则会立即触发程序终止。 除非您确定您的错误不需要互操作性，否则您应该查找可以转换为的现有`std :: error_code`，甚至（尽管很痛苦）考虑引入一个新的作为权宜之计。

##### Interoperability with std::error_code and ErrorOr

许多现有的LLVM API使用`std :: error_code`及其合作伙伴`ErrorOr <T>`（它扮演与`Expected <T>`相同的角色，但包装了`std :: error_code`而不是Error）。 错误类型的传染性本质意味着尝试更改其中一个函数以返回Error或`Expected <T>`通常会导致对调用者，调用者的调用者等进行大量更改。 （第一次这样的尝试，从`MachOObjectFile`的构造函数返回一个错误，在差异达到3000行之后被放弃，影响了六个库，并且仍然在增长）。

为解决此问题，引入了`Error/std :: error_code`互操作性要求。 两对函数允许将任何Error值转换为std :: error_code，任何`Expected <T>`都将转换为`ErrorOr <T>`，反之亦然：

```c++
std::error_code errorToErrorCode(Error Err);
Error errorCodeToError(std::error_code EC);

template <typename T> ErrorOr<T> expectedToErrorOr(Expected<T> TOrErr);
template <typename T> Expected<T> errorOrToExpected(ErrorOr<T> TOrEC);
```

使用这些API可以很容易地制作外科补丁，将各个函数从`std :: error_code`更新为Error，从`ErrorOr <T>`更新为`Expected <T>`。

##### Returning Errors from error handlers

错误恢复尝试本身可能会失败。 出于这个原因，handleErrors实际上识别三种不同形式的处理程序签名：

```c++
// Error must be handled, no new errors produced:
void(UserDefinedError &E);

// Error must be handled, new errors can be produced:
Error(UserDefinedError &E);

// Original error can be inspected, then re-wrapped and returned (or a new
// error can be produced):
Error(std::unique_ptr<UserDefinedError> E);
```

 从处理程序返回的任何错误都将从handleErrors函数返回，以便可以自行处理，或者在堆栈中向上传播。

##### Using ExitOnError to simplify tool code

库代码永远不应该为可恢复的错误调用exit，但是在工具代码（尤其是命令行工具）中，这可能是一种合理的方法。 遇到错误时调用exit会大大简化控制流程，因为错误不再需要在堆栈中传播。 这允许代码以直线样式写入，只要每个错误的调用都包含在检查中并调用退出。 `ExitOnError`类通过提供检查Error值的调用操作符，在成功案例中删除错误并记录到stderr然后在失败情况下退出来支持此模式。

要使用此类，请在程序中声明一个全局ExitOnError变量：

```c++
ExitOnError ExitOnErr;
```

然后可以通过调用ExitOnErr将对错误函数的调用包装起来，将它们转换为非失败调用：

```c++
Error mayFail();
Expected<int> mayFail2();

void foo() {
  ExitOnErr(mayFail());
  int X = ExitOnErr(mayFail2());
}
```

失败时，错误的日志消息将写入stderr，可选地前面有一个字符串“banner”，可以通过调用setBanner方法来设置。 也可以使用setExitCodeMapper方法从Error值向退出代码提供映射：

```c++
int main(int argc, char *argv[]) {
  ExitOnErr.setBanner(std::string(argv[0]) + " error:");
  ExitOnErr.setExitCodeMapper(
    [](const Error &Err) {
      if (Err.isA<BadFileFormat>())
        return 2;
      return 1;
    });
```

尽可能在工具代码中使用`ExitOnError`，因为它可以极大地提高可读性。

##### Using cantFail to simplify safe callsites

某些功能可能仅对其输入的子集失败，因此可以假定使用已知安全输入的调用成功。

cantFail函数通过包装断言它们的参数是成功值来封装它，并且在`Expected <T>`的情况下，解包T值：

```c++
Error onlyFailsForSomeXValues(int X);
Expected<int> onlyFailsForSomeXValues2(int X);

void foo() {
  cantFail(onlyFailsForSomeXValues(KnownSafeValue));
  int Y = cantFail(onlyFailsForSomeXValues2(KnownSafeValue));
  ...
}
```

与ExitOnError实用程序一样，cantFail简化了控制流程。 然而，它们对错误情况的处理是非常不同的：在保证ExitOnError在错误输入上终止程序的情况下，cantFile只是断言结果是成功的。 在调试版本中，如果遇到错误，这将导致断言失败。 在发布版本中，未定义cantFail的失败值行为。 因此，必须注意cantFail的使用：客户必须确定cantFail包装的调用确实不能使用给定的参数失败。

在库代码中使用cantFail函数应该很少，但它们可能在工具和单元测试代码中更有用，其中输入和/或模拟类或函数可能是安全的。

##### Fallible constructors

易错构造函数有些类需要资源获取或其他在构造期间可能失败的复杂初始化。 不幸的是，构造函数不能返回错误，并且客户端在构造之后测试对象以确保它们有效是容易出错的，因为它很容易忘记测试。 要解决此问题，请使用命名的构造函数idiom并返回`Expected <T>`：

```c++
class Foo {
public:

  static Expected<Foo> Create(Resource R1, Resource R2) {
    Error Err;
    Foo F(R1, R2, Err);
    if (Err)
      return std::move(Err);
    return std::move(F);
  }

private:

  Foo(Resource R1, Resource R2, Error &Err) {
    ErrorAsOutParameter EAO(&Err);
    if (auto Err2 = R1.acquire()) {
      Err = std::move(Err2);
      return;
    }
    Err = R2.acquire();
  }
};
```

这里，命名的构造函数通过引用将Error传递给实际的构造函数，然后构造函数可以使用它来返回错误。 `ErrorAsOutParameter`实用程序在构造函数的入口处设置Error值的checked标志，以便可以分配错误，然后在退出时重置它以强制客户端（命名构造函数）检查错误。

通过使用这个习惯用法，尝试构造Foo的客户端接收格式良好的Foo或Error，而不是处于无效状态的对象。

##### Propagating and consuming errors based on types

在某些情况下，已知某些类型的错误是良性的。 例如，当走遍档案时，一些客户可能乐于跳过格式错误的目标文件，而不是立即终止行走。 使用复杂的处理程序方法可以实现跳过格式错误的对象，但Error.h标头提供了两个实用程序，使这个习惯用法更加清晰：类型检查方法isA和consumeError函数：

```c++
Error walkArchive(Archive A) {
  for (unsigned I = 0; I != A.numMembers(); ++I) {
    auto ChildOrErr = A.getMember(I);
    if (auto Err = ChildOrErr.takeError()) {
      if (Err.isA<BadFileFormat>())
        consumeError(std::move(Err))
      else
        return Err;
    }
    auto &Child = *ChildOrErr;
    // Use Child
    ...
  }
  return Error::success();
}
```

##### Concatenating Errors with joinErrors

在上面的归档步骤示例中，BadFileFormat错误被简单地使用和忽略。 如果客户端希望在完成遍历存档后报告这些错误，则可以使用joinErrors实用程序：

```c++
Error walkArchive(Archive A) {
  Error DeferredErrs = Error::success();
  for (unsigned I = 0; I != A.numMembers(); ++I) {
    auto ChildOrErr = A.getMember(I);
    if (auto Err = ChildOrErr.takeError())
      if (Err.isA<BadFileFormat>())
        DeferredErrs = joinErrors(std::move(DeferredErrs), std::move(Err));
      else
        return Err;
    auto &Child = *ChildOrErr;
    // Use Child
    ...
  }
  return DeferredErrs;
}
```

joinErrors例程构建一个名为ErrorList的特殊错误类型，它包含用户定义错误的列表。 handleErrors例程识别此类型，并将尝试按顺序处理每个包含的错误。 如果可以处理所有包含的错误，handleErrors将返回`Error :: success（）`，否则handleErrors将连接剩余的错误并返回生成的ErrorList。 

##### Building fallible iterators and iterator ranges

上面的归档步骤示例通过索引检索归档成员，但是这需要相当多的样板来进行迭代和错误检查。 我们可以通过使用带有“易错迭代器”模式的Error来清理它。 通常的C ++迭代器模式不允许增量失败，但我们可以通过让迭代器持有一个Error引用来通过它来报告失败来加入对它的支持。 在此模式中，如果增量操作失败，则通过Error参考记录故障，并将迭代器值设置为范围的结尾以终止循环。 这确保了解除引用操作在普通迭代器解除引用是安全的任何地方都是安全的（即，当迭代器不等于结束时）。 在遵循这种模式的地方（如在llvm :: object :: Archive类中），结果是更清晰的迭代习语：

```c++
Error Err;
for (auto &Child : Ar->children(Err)) {
  // Use Child - we only enter the loop when it's valid
  ...
}
// Check Err after the loop to ensure it didn't break due to an error.
if (Err)
  return Err;
```

有关错误及其相关实用程序的更多信息，请参见Error.h头文件。

### Passing functions and other callable objects

有时您可能希望函数传递回调对象。 为了支持lambda表达式和其他函数对象，您不应该使用传统的C方法来获取函数指针和不透明的cookie：

```c++
void takeCallback(bool (*Callback)(Function *, void *), void *Cookie);
```

相反，请使用以下方法之一：

#### Function template

如果您不介意将函数的定义放入头文件中，请将其设置为在可调用类型上模板化的函数模板。

```c++
template<typename Callable>
void takeCallback(Callable Callback) {
  Callback(1, 2, 3);
}
```

#### The `function_ref `class template

function_ref（[doxygen](http://llvm.org/doxygen/classllvm_1_1function__ref_3_01Ret_07Params_8_8_8_08_4.html)）类模板表示对可调用对象的引用，该可调用对象是在可调用类型上模板化的。 如果在函数返回后不需要保持回调，这是将回调传递给函数的一个很好的选择。 这样，function_ref是std :: function，因为StringRef是std :: string。

`function_ref <Ret（Param1，Param2，...)>`可以从任何可调用对象隐式构造，该对象可以使用Param1，Param2，...类型的参数调用，并返回一个可以转换为Ret类型的值。 例如：

```c++
void visitBasicBlocks(Function *F, function_ref<bool (BasicBlock*)> Callback) {
  for (BasicBlock &BB : *F)
    if (Callback(&BB))
      return;
}
```

可以使用：

```c++
visitBasicBlocks(F, [&](BasicBlock *BB) {
  if (process(BB))
    return isEmpty(BB);
  return false;
});
```

请注意，function_ref对象包含指向外部存储器的指针，因此存储类的实例通常不安全（除非您知道不会释放外部存储器）。 如果您需要此功能，请考虑使用`std :: function`。 function_ref足够小，应始终按值传递。

### The `LLVM_DEBUG()` macro and `-debug `option

通常在处理你的通行证时，你会在你的通行证中放入一堆调试打印输出和其他代码。 在你使它工作之后，你想要删除它，但是你将来可能会再次需要它（以解决你遇到的新bug）。

当然，正因为如此，您不希望删除调试打印输出，但您不希望它们总是嘈杂。 标准的妥协是将它们注释掉，允许您在将来需要时启用它们。

`llvm/Support/Debug.h`（[doxygen](http://llvm.org/doxygen/Debug_8h_source.html)）文件提供了一个名为`LLVM_DEBUG（）`的宏，它是解决此问题的更好的解决方案。 基本上，您可以将任意代码放入LLVM_DEBUG宏的参数中，并且只有在使用'-debug'命令行参数运行'opt'（或任何其他工具）时才会执行它：

```c++
LLVM_DEBUG(dbgs() << "I am here!\n");
```

然后你就可以像这样运行你的pass：

```
$ opt < a.bc > /dev/null -mypass
<no output>
$ opt < a.bc > /dev/null -mypass -debug
I am here!
```

使用`LLVM_DEBUG（）`宏而不是自制的解决方案，您无需为传递的调试输出创建“又一个”命令行选项。 请注意，对于非断言构建禁用`LLVM_DEBUG（）`宏，因此它们根本不会对性能产生影响（出于同样的原因，它们也不应包含副作用！）。

关于`LLVM_DEBUG（）`宏的另一个好处是你可以直接在gdb中启用或禁用它。 如果程序正在运行，只需使用gdb中的“set DebugFlag = 0”或“set DebugFlag = 1”。 如果程序尚未启动，您可以随时使用-debug运行它。

#### Fine grained debug info with `DEBUG_TYPE` and the `-debug-only` option

有时您可能会发现自己处于启用-debug只会打开太多信息（例如在处理代码生成器时）的情况。 如果要使用更细粒度的控件启用调试信息，则应定义`DEBUG_TYPE`宏并使用`-debug-only`选项，如下所示：

```c++
#define DEBUG_TYPE "foo"
LLVM_DEBUG(dbgs() << "'foo' debug type\n");
#undef  DEBUG_TYPE
#define DEBUG_TYPE "bar"
LLVM_DEBUG(dbgs() << "'bar' debug type\n");
#undef  DEBUG_TYPE
```

然后你就可以像这样运行你的pass：

```
$ opt < a.bc > /dev/null -mypass
<no output>
$ opt < a.bc > /dev/null -mypass -debug
'foo' debug type
'bar' debug type
$ opt < a.bc > /dev/null -mypass -debug-only=foo
'foo' debug type
$ opt < a.bc > /dev/null -mypass -debug-only=bar
'bar' debug type
$ opt < a.bc > /dev/null -mypass -debug-only=foo,bar
'foo' debug type
'bar' debug type
```

当然，实际上，您只应在文件顶部设置`DEBUG_TYPE`，以指定整个模块的调试类型。请注意，只有在包含`Debug.h`后才能执行此操作，而不是在任何`#include`的头文件中执行此操作。此外，您应该使用比“foo”和“bar”更有意义的名称，因为没有适当的系统来确保名称不会发生冲突。如果两个不同的模块使用相同的字符串，则在指定名称时将全部打开它们。例如，这允许使用`-debug-only = InstrSched`启用指令调度的所有调试信息，即使源存在于多个文件中也是如此。该名称不得包含逗号（，），因为它用于分隔`-debug-only`选项的参数。

出于性能原因，`-debug-only`在LLVM的优化构建（`--enable-optimized`）中不可用。

`DEBUG_WITH_TYPE`宏也可用于您要设置DEBUG_TYPE的情况，但仅适用于一个特定的DEBUG语句。它需要一个额外的第一个参数，这是要使用的类型。例如，前面的示例可以写为：

``` c++
DEBUG_WITH_TYPE("foo", dbgs() << "'foo' debug type\n");
DEBUG_WITH_TYPE("bar", dbgs() << "'bar' debug type\n");
```

### The `Statistic` class `& -stats` option

`Statistic`类`＆-stats`选项`llvm/ADT/Statistic.h`（[doxygen](http://llvm.org/doxygen/Statistic_8h_source.html)）文件提供了一个名为Statistic的类，它用作跟踪LLVM编译器正在执行的操作以及各种优化的有效性的统一方式。 有必要了解哪些优化有助于使特定程序更快地运行。

通常你可以在一些大型程序上运行你的传递，并且你有兴趣看看它进行了多少次转换。 虽然你可以通过手工检查或一些临时方法来做到这一点，但这对于大型程序来说真的很痛苦并不是很有用。 使用Statistic类可以很容易地跟踪这些信息，并且计算的信息以统一的方式呈现，其余的传递被执行。

统计使用有很多例子，但使用它的基础知识如下：

像这样定义你的统计数据：

```c++
#define DEBUG_TYPE "mypassname"   // This goes before any #includes.
STATISTIC(NumXForms, "The # of times I did stuff");
```

`STATISTIC`宏定义了一个静态变量，其名称由第一个参数指定。 传递名称取自`DEBUG_TYPE`宏，描述取自第二个参数。 定义的变量（在本例中为“`NumXForms`”）的作用类似于无符号整数。

每当你进行转换时，碰撞计数器：

```c++
++NumXForms;   // I did stuff!
```

这就是你要做的一切。 要“选择”打印收集的统计信息，请使用“-stats”选项：

```
$ opt -stats -mypassname < program.bc > /dev/null
... statistics output ...
```

请注意，为了使用'-stats'选项，必须在启用断言的情况下编译LLVM。

当从SPEC基准测试套件运行选择C文件时，它会提供如下所示的报告：

```
 7646 bitcodewriter   - Number of normal instructions
   725 bitcodewriter   - Number of oversized instructions
129996 bitcodewriter   - Number of bitcode bytes written
  2817 raise           - Number of insts DCEd or constprop'd
  3213 raise           - Number of cast-of-self removed
  5046 raise           - Number of expression trees converted
    75 raise           - Number of other getelementptr's formed
   138 raise           - Number of load/store peepholes
    42 deadtypeelim    - Number of unused typenames removed from symtab
   392 funcresolve     - Number of varargs functions resolved
    27 globaldce       - Number of global variables removed
     2 adce            - Number of basic blocks removed
   134 cee             - Number of branches revectored
    49 cee             - Number of setcc instruction eliminated
   532 gcse            - Number of loads removed
  2919 gcse            - Number of instructions removed
    86 indvars         - Number of canonical indvars added
    87 indvars         - Number of aux indvars removed
    25 instcombine     - Number of dead inst eliminate
   434 instcombine     - Number of insts combined
   248 licm            - Number of load insts hoisted
  1298 licm            - Number of insts hoisted to a loop pre-header
     3 licm            - Number of insts hoisted to multiple loop preds (bad, no loop pre-header)
    75 mem2reg         - Number of alloca's promoted
  1444 cfgsimplify     - Number of blocks simplified
```

 显然，有了这么多的优化，为这些东西建立一个统一的框架是非常好的。 使你的传递适合框架使其更易于维护和有用。

### Adding debug counters to aid in debugging your code

有时，在编写新的通行证或试图追踪错误时，能够控制通行证中是否发生某些事情是很有用的。 例如，有时最小化工具只能轻松为您提供大型测试用例。 您希望使用二分法自动将您的错误缩小到发生或不发生的特定转换。 这是调试计数器帮助的地方。 它们提供了一个框架，使您的代码部分只执行一定次数。

`llvm/Support/DebugCounter.h`（[doxygen](http://llvm.org/doxygen/DebugCounter_8h_source.html)）文件提供了一个名为DebugCounter的类，可用于创建控制代码部分执行的命令行计数器选项。

像这样定义DebugCounter：

```c++
DEBUG_COUNTER(DeleteAnInstruction, "passname-delete-instruction",
              "Controls which instructions get delete");
```

DEBUG_COUNTER宏定义一个静态变量，其名称由第一个参数指定。 计数器的名称（在命令行中使用）由第二个参数指定，帮助中使用的描述由第三个参数指定。

无论您想要哪种代码控制，都可以使用DebugCounter :: shouldExecute来控制它。

```c++
if (DebugCounter::shouldExecute(DeleteAnInstruction))
  I->eraseFromParent();
```

这就是你要做的一切。 现在，使用opt，您可以使用'--debug-counter'选项控制此代码何时触发。 提供了两个计数器，跳过和计数。 skip是跳过代码路径执行的次数。 count是跳过后执行代码路径的次数。

```
opt --debug-counter=passname-delete-instruction-skip=1,passname-delete-instruction-count=2 -passname
```

这将在我们第一次点击它时跳过上面的代码，然后执行两次，然后跳过其余的执行。

因此，如果在以下代码上执行：

```
%1 = add i32 %a, %b
%2 = add i32 %a, %b
%3 = add i32 %a, %b
%4 = add i32 %a, %b
```

它会删除数字％2和％3。

utils / bisect-skip-count中提供了一个实用程序，用于二进制搜索skip和count参数。 它可用于自动最小化调试计数器变量的跳过和计数。

### Viewing graphs while debugging code

LLVM中的一些重要数据结构是图形：例如，由LLVM [BasicBlocks](http://llvm.org/docs/ProgrammersManual.html#basicblock)构成的CFG，由LLVM [MachineBasicBlocks](http://llvm.org/docs/CodeGenerator.html#machinebasicblock)构成的CFG和[指令选择DAG](http://llvm.org/docs/CodeGenerator.html#selectiondag)。在许多情况下，在调试编译器的各个部分时，立即可视化这些图形是很好的。

LLVM提供了几个在调试版本中可用的回调来完成。例如，如果调用`Function :: viewCFG（）`方法，当前的LLVM工具将弹出一个包含该函数的CFG的窗口，其中每个基本块是图中的节点，并且每个节点包含块中的指令。类似地，还存在`Function :: viewCFGOnly（）`（不包括指令），`MachineFunction :: viewCFG（）`和`MachineFunction :: viewCFGOnly（）`以及`SelectionDAG :: viewGraph（）`方法。例如，在GDB中，您通常可以使用调用`DAG.viewGraph（）`之类的东西来弹出窗口。或者，您可以在要调试的位置的代码中调用这些函数。

要使其工作，需要进行少量设置。在使用X11的Unix系统上，安装graphviz工具包，并确保“dot”和“gv”在您的路径中。如果您在Mac OS X上运行，请下载并安装Mac OS X Graphviz程序，并将`/Applications/Graphviz.app/Contents/MacOS/`（或安装它的任何位置）添加到您的路径中。配置，构建或运行LLVM时，程序不需要存在，并且可以在活动的调试会话期间根据需要进行安装。

`SelectionDAG`已经扩展，可以更容易地在大型复杂图形中定位有趣的节点。从gdb，如果你调用`DAG.setGraphColor`（节点，“颜色”），那么下一个调用`DAG.viewGraph（）`将突出显示指定颜色的节点（可以在颜色中找到颜色的选择。）更复杂的节点属性可以提供调用`DAG.setGraphAttrs`（节点，“属性”）（可以在[Graph属性](http://www.graphviz.org/doc/info/attrs.html)中找到选项。)如果要重新启动并清除所有当前图形属性，则可以调用`DAG.clearGraphAttrs()`。

请注意，图形可视化功能是从发布版本中编译的，以减小文件大小。这意味着您需要`Debug + Asserts`或`Release + Asserts`构建才能使用这些功能。

## Picking the Right Data Structure for a Task(null)



## Debugging

## Helpful Hints for Common Operations

本节介绍如何执行一些非常简单的LLVM代码转换。 这是为了给出使用的常用习语的示例，显示LLVM转换的实际操作方面。

因为这是一个“操作方法”部分，所以您还应该阅读将要使用的主要类。 [核心LLVM类层次结构参考手册](http://llvm.org/docs/ProgrammersManual.html#coreclasses)包含您应了解的主要类的详细信息和说明。

### Basic Inspection and Traversal Routines

LLVM编译器基础结构具有许多可以遍历(traversed)的不同数据结构。 遵循C ++标准模板库的示例，用于遍历这些各种数据结构的技术基本相同。 对于可枚举的值序列，`XXXbegin（）`函数（或方法）返回一个迭代器指向序列的开头，`XXXend（）`函数返回一个迭代器，指向一个超过序列的最后一个有效元素的迭代器，并且有一些 两种操作之间通用的`XXXiterator`数据类型。

因为迭代模式在程序表示的许多不同方面是通用的，所以可以在它们之上使用标准模板库算法，并且更容易记住如何迭代。 首先，我们展示了一些需要遍历的数据结构的常见示例。 其他数据结构以非常类似的方式遍历。

#### Iterating over the BasicBlock in a Function

通长你拥有一个`Function`实例，你想通过某些方式来转换他们; 特别是，你想操纵它的BasicBlocks。 为此，您需要迭代构成函数的所有BasicBlock。 以下是打印BasicBlock名称及其包含的说明数的示例：

```c++
Function &Func = ...
for (BasicBlock &BB : Func)
  // Print out the name of the basic block if it has one, and then the
  // number of instructions that it contains
  errs() << "Basic block (name=" << BB.getName() << ") has "
             << BB.size() << " instructions.\n";
```

#### Iterating over the Instruction in a BasicBlock

迭代BasicBlock中的指令就像处理函数中的BasicBlocks一样，很容易迭代构成BasicBlocks的各个指令。 这是一个打印出BasicBlock中每条指令的代码片段：

```c++
BasicBlock& BB = ...
for (Instruction &I : BB)
   // The next statement works since operator<<(ostream&,...)
   // is overloaded for Instruction&
   errs() << I << "\n";
```

但是，这不是打印出BasicBlock内容的最佳方式！ 由于ostream操作符几乎可以满足你所关心的任何内容，你可以在基本块本身上调用print例程：`errs（）<< BB <<“\n”;`。

#### Iterating over the Instruction in a Function

迭代函数中的指令如果你发现通常先迭代`Function`的`BasicBlocks`然后是迭代`BasicBlock`的`InstIterator`，那么应该使用`InstIterator`。 您需要包含`llvm / IR / InstIterator.h`（[doxygen](http://llvm.org/doxygen/InstIterator_8h.html)），然后在代码中显式实例化`InstIterators`。 这是一个小例子，展示了如何将函数中的所有指令转储到标准错误流：

```c++
#include "llvm/IR/InstIterator.h"

// F is a pointer to a Function instance
for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I)
  errs() << *I << "\n";
```

容易，不是吗？ 您还可以使用InstIterators使用其初始内容填充工作列表。 例如，如果要初始化工作列表以包含函数F中的所有指令，那么您需要做的就是：

```c++
std::set<Instruction*> worklist;
// or better yet, SmallPtrSet<Instruction*, 64> worklist;

for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I)
  worklist.insert(&*I);
```

STL集工作列表现在将包含F指向的函数中的所有指令。

#### Turning an iterator into a class pointer (and vice-versa)

有时候，将迭代器转换为类指针（反之亦然），当你所有的东西都是一个迭代器时，抓取一个类实例的引用（或指针）会很有用。 好吧，从迭代器中提取引用或指针非常简单。 假设我是`BasicBlock :: iterator`，`j`是`BasicBlock :: const_iterator`：

```c++
Instruction& inst = *i;   // Grab reference to instruction reference
Instruction* pinst = &*i; // Grab pointer to instruction reference
const Instruction& inst = *j;
```

但是，您将在LLVM框架中使用的迭代器是特殊的：它们将在需要时自动转换为ptr-to-instance类型。 您可以简单地将迭代器分配给正确的指针类型，而不是取消引用迭代器然后获取结果的地址，并且在分配后得到取消引用和地址操作（在幕后，这是一个结果 超载铸造机制）。 因此，最后一个例子的第二行，

```c++
Instruction *pinst = &*i;
```

在语义上等同于:

```c++
Instruction *pinst = i;
```

也可以将类指针转换为相应的迭代器，这是一个恒定时间操作（非常有效）。 以下代码段说明了LLVM迭代器提供的转换构造函数的使用。 通过使用这些，您可以显式地获取某些东西的迭代器，而无需通过某种结构的迭代实际获取它：

```c++
void printNextInstruction(Instruction* inst) {
  BasicBlock::iterator it(inst);
  ++it; // After this line, it refers to the instruction after *inst
  if (it != inst->getParent()->end()) errs() << *it << "\n";
}
```

不幸的是，这些隐含的转换需要付出代价; 它们可以防止这些迭代器符合标准迭代器约定，从而可以与标准算法和容器一起使用。 例如，它们阻止以下代码（其中B是BasicBlock）来编译：

```c++
llvm::SmallVector<llvm::Instruction *, 16>(B->begin(), B->end());
```

因此，有些日子可能会删除这些隐式转换，并且operator *已更改为返回指针而不是引用。

#### Finding call sites: a slightly more complex example

查找调用站点：稍微复杂一点的例子，你正在编写一个FunctionPass，并希望计算整个模块中的所有位置（即，跨越每个函数），其中某个函数（即某些函数*）已经在 范围。 正如您稍后将要了解的那样，您可能希望使用InstVisitor以更直接的方式完成此任务，但是此示例将允许我们探索如果您没有InstVisitor，您将如何执行此操作。 在伪代码中，这是我们想要做的：

```
initialize callCounter to zero
for each Function f in the Module
  for each BasicBlock b in f
    for each Instruction i in b
      if (i is a CallInst and calls the given function)
        increment callCounter
```

 实际的代码是（记住，因为我们正在编写一个FunctionPass，我们的FunctionPass派生类只需要覆盖runOnFunction方法）：

```c++
Function* targetFunc = ...;

class OurFunctionPass : public FunctionPass {
  public:
    OurFunctionPass(): callCounter(0) { }

    virtual runOnFunction(Function& F) {
      for (BasicBlock &B : F) {
        for (Instruction &I: B) {
          if (auto *CallInst = dyn_cast<CallInst>(&I)) {
            // We know we've encountered a call instruction, so we
            // need to determine if it's a call to the
            // function pointed to by m_func or not.
            if (CallInst->getCalledFunction() == targetFunc)
              ++callCounter;
          }
        }
      }
    }

  private:
    unsigned callCounter;
};
```

#### Treating calls and invokes the same way

您可能已经注意到前面的示例有点过于简单，因为它没有处理“调用”指令生成的调用站点。 在这种情况下，在其他情况下，您可能会发现要以相同的方式处理CallInsts和InvokeInsts，即使它们最具体的公共基类是Instruction，其中包含许多不太密切相关的事物。 对于这些情况，LLVM提供了一个名为CallSite（doxygen）的方便的包装类。它本质上是一个包含指令指针的包装器，其中一些方法提供了CallInsts和InvokeInsts的通用功能。

此类具有“值语义”：它应该通过值传递，而不是通过引用传递，并且不应使用operator new或operator delete动态分配或取消分配。 它具有高效的可复制性，可分配性和可构造性，其成本与裸指针的成本相当。 如果你看它的定义，它只有一个指针成员。

#### Iterating over def-use & use-def chains

通常，我们可能有一个Value类（doxygen）的实例，我们想确定哪个用户使用Value。 特定值的所有用户列表称为def-use链。 例如，假设我们有一个名为F的Function *到一个特定的函数foo。 查找使用foo的所有指令就像迭代F的def-use链一样简单：

```c++
Function *F = ...;

for (User *U : F->users()) {
  if (Instruction *Inst = dyn_cast<Instruction>(U)) {
    errs() << "F is used in instruction:\n";
    errs() << *Inst << "\n";
  }
```

或者，通常有一个User Class（doxygen）的实例，并且需要知道它使用了什么值。 用户使用的所有值的列表称为use-def链。 类指令的实例是常见的用户，因此我们可能希望迭代特定指令使用的所有值（即特定指令的操作数）：

```c++
Instruction *pi = ...;

for (Use &U : pi->operands()) {
  Value *v = U.get();
  // ...
}
```

将对象声明为const是执行无变异算法（例如分析等）的重要工具。 为此，上面的迭代器以Value :: const_use_iterator和Value :: const_op_iterator的形式出现。 它们分别在const Value * s或const User * s上调用use / op_begin（）时自动出现。 取消引用后，它们返回const Use * s。 否则上述模式保持不变。

#### Iterating over predecessors & successors of blocks

使用“llvm / IR / CFG.h”中定义的例程可以很容易地迭代块的前驱和后继。 只需使用这样的代码迭代BB的所有前辈：

```c++
#include "llvm/IR/CFG.h"
BasicBlock *BB = ...;

for (BasicBlock *Pred : predecessors(BB)) {
  // ...
}
```

同样，迭代后继者使用后继者。

### Making simple changes

LLVM基础结构中存在一些值得了解的原始转换操作。 执行转换时，操作基本块的内容是相当常见的。 本节介绍了执行此操作的一些常用方法，并提供了示例代码。

#### Creating and inserting new Instructions

*Instantiating Instructions(实例化说明)*

创建指令很简单：只需调用构造函数以获得实例化的指令并提供必要的参数。 例如，AllocaInst只需要一个（const-ptr-to）类型。 从而：

```c++
auto *ai = new AllocaInst(Type::Int32Ty);
```

将创建一个AllocaInst实例，该实例表示在运行时在当前堆栈帧中分配一个整数。 每个指令子类可能都有不同的默认参数，这些参数会改变指令的语义，因此请参阅[doxygen文档](http://llvm.org/doxygen/classllvm_1_1Instruction.html)，了解您想要实例化的指令的子类。

*Naming values(命名值)*

在您能够命名指令的值时非常有用，因为这有助于调试转换。 如果您最终查看生成的LLVM机器代码，您肯定希望将逻辑名称与指令结果相关联！ 通过为指令构造函数的Name（默认）参数提供值，可以将逻辑名与运行时指令执行结果相关联。 例如，假设我正在编写一个动态为堆栈上的整数分配空间的转换，并且该整数将被某些其他代码用作某种索引。 为了实现这一点，我将AllocaInst置于某个Function的第一个BasicBlock的第一个点，并且我打算在同一个Function中使用它。 我可能会这样做：

```c++
auto *pa = new AllocaInst(Type::Int32Ty, 0, "indexLoc");
```

其中indexLoc现在是指令执行值的逻辑名，它是指向运行时堆栈上的整数的指针。

*Inserting instructions(插入说明)*

将指令插入到形成BasicBlock的现有指令序列中有三种方法：

- 插入显式指令列表

  给定`BasicBlock * pb`，`BasicBlock`中的`Instruction * pi`，以及我们希望在`* pi`之前插入的新创建的指令，我们执行以下操作：

  ```c++
  BasicBlock *pb = ...;
  Instruction *pi = ...;
  auto *newInst = new Instruction(...);
  
  pb->getInstList().insert(pi, newInst); // Inserts newInst before pi in pb
  ```

  附加到BasicBlock的末尾是如此常见，以至于Instruction类和指令派生类提供了构造函数，这些构造函数将指向要附加到BasicBlock的指针。 例如代码看起来像：

  ```c++
  BasicBlock *pb = ...;
  auto *newInst = new Instruction(...);
  
  pb->getInstList().push_back(newInst); // Appends newInst to pb
  ```

  变为：

  ```c++
  BasicBlock *pb = ...;
  auto *newInst = new Instruction(..., pb);
  ```

  哪个更干净，特别是如果你要创建长指令流。

- 插入隐式指令列表

  已经在`BasicBlock`中的指令实例隐含地与现有指令列表相关联：封闭基本块的指令列表。 因此，我们可以完成与上面的代码相同的事情而不通过执行以下方式给出`BasicBlock`：

  ```c++
  Instruction *pi = ...;
  auto *newInst = new Instruction(...);
  
  pi->getParent()->getInstList().insert(pi, newInst);
  ```

  实际上，这一系列步骤经常发生，指令类和指令派生类提供了构造函数，这些构造函数采用（作为默认参数）指向新创建的指令应该在其之前的指令的指针。 也就是说，指令构造函数能够在该指令之前立即将新创建的实例插入所提供指令的BasicBlock中。 使用带有insertBefore（默认）参数的指令构造函数，上面的代码变为：

  ```c++
  Instruction* pi = ...;
  auto *newInst = new Instruction(..., pi);
  ```

  更干净，特别是如果你创建了很多指令并将它们添加到`BasicBlocks`。

- 使用`IRBuilder`实例插入

  使用以前的方法插入几个指令可能非常费力。 IRBuilder是一个便利类，可用于在BasicBlock的末尾或特定指令之前添加多个指令。 它还支持常量折叠和重命名命名寄存器（请参阅IRBuilder的模板参数）。

  下面的例子演示了IRBuilder的一个非常简单的用法，其中在指令pi之前插入了三条指令。 前两个指令是调用指令，第三个指令将两个调用的返回值相乘。

  ```c++
  Instruction *pi = ...;
  IRBuilder<> Builder(pi);
  CallInst* callOne = Builder.CreateCall(...);
  CallInst* callTwo = Builder.CreateCall(...);
  Value* result = Builder.CreateMul(callOne, callTwo);
  ```

  下面的示例与上面的示例类似，只是创建的IRBuilder在BasicBlock pb的末尾插入指令。

  ```c++
  BasicBlock *pb = ...;
  IRBuilder<> Builder(pb);
  CallInst* callOne = Builder.CreateCall(...);
  CallInst* callTwo = Builder.CreateCall(...);
  Value* result = Builder.CreateMul(callOne, callTwo);
  ```

  有关IRBuilder的实际用途，请[参阅Kaleidoscope](http://llvm.org/docs/tutorial/LangImpl03.html)：代码生成到LLVM IR。

#### Deleting Instructions

从形成BasicBlock的现有指令序列中删除指令非常简单：只需调用指令的eraseFromParent（）方法即可。 例如：

```
Instruction *I = .. ;
I->eraseFromParent();
```

这将取消指令与其包含的基本块的链接并删除它。 如果您只想取消指令与其包含的基本块的链接但不删除它，则可以使用removeFromParent（）方法。

#### Replacing an Instruction with another Value

##### Replacing individual instructions

替换indiv包括“[llvm/Transforms/Utils/BasicBlockUtils.h](http://llvm.org/doxygen/BasicBlockUtils_8h_source.html)”允许使用两个非常有用的替换函数：`ReplaceInstWithValue`和`ReplaceInstWithInst`指令

##### Deleting Instructions

- `ReplaceInstWithValue`

  此函数用值替换给定指令的所有用法，然后删除原始指令。 以下示例说明了替换特定AllocaInst的结果，该结果为具有指向整数的空指针的单个整数分配内存。

  ```c++
  AllocaInst* instToReplace = ...;
  BasicBlock::iterator ii(instToReplace);
  ReplaceInstWithValue(instToReplace->getParent()->getInstList(), ii,                    Constant::getNullValue(PointerType::getUnqual(Type::Int32Ty)));
  ```

- `ReplaceInstWithInst`

  该函数用另一条指令替换特定指令，将新指令插入旧指令所在位置的基本块中，并用新指令替换旧指令的任何用法。 以下示例说明将一个`AllocaInst`替换为另一个。

  ```c++
  AllocaInst* instToReplace = ...;
  BasicBlock::iterator ii(instToReplace);
  
  ReplaceInstWithInst(instToReplace->getParent()->getInstList(), ii,
                      new AllocaInst(Type::Int32Ty, 0, "ptrToReplacedInt"));
  ```

##### Replacing multiple uses of Users and Values

替换用户和值的多种用途您可以使用`Value :: replaceAllUsesWith`和`User :: replaceUsesOfWith`一次更改多个用户。 有关详细信息，请分别参阅值[Value Class](http://llvm.org/doxygen/classllvm_1_1Value.html)和[User Class](http://llvm.org/doxygen/classllvm_1_1User.html)的doxygen文档。

#### Deleting GlobalVariables

从模块中删除全局变量就像删除指令一样简单。 首先，您必须有一个指向要删除的全局变量的指针。 您可以使用此指针将其从其父模块中删除。 例如：

```c++
GlobalVariable *GV = .. ;
GV->eraseFromParent();
```

### How to Create Types

在生成`IR`时，您可能需要一些复杂的类型。 如果您静态地知道这些类型，则可以使用`llvm/Support/TypeBuilder.h`中定义的`TypeBuilder <...> :: get（）`来检索它们。 `TypeBuilder`有两种形式，具体取决于您是构建交叉编译类型还是本机库使用。 `TypeBuilder <T，true>`要求T独立于主机环境，这意味着它是由`llvm :: types`（doxygen）命名空间中的类型以及由这些命名空间构建的指针，函数，数组等构建的。 `TypeBuilder <T，false>`还允许本机C类型，其大小可能取决于主机编译器。 例如，

```c++
FunctionType *ft = TypeBuilder<types::i<8>(types::i<32>*), true>::get();
```

比同等的更容易读写:

```c++
std::vector<const Type*> params;
params.push_back(PointerType::getUnqual(Type::Int32Ty));
FunctionType *ft = FunctionType::get(Type::Int8Ty, params, false);
```

 有关详细信息,参看[class comment](http://llvm.org/doxygen/TypeBuilder_8h_source.html#l00001) 



## Threads and LLVM(null)

## Advanced Topics

## The Core LLVM Class Hierarchy Reference

`#include "llvm/IR/Type.h"`

header source：[Type.h](http://llvm.org/doxygen/Type_8h_source.html)

doxygen info：[Type Clases](http://llvm.org/doxygen/classllvm_1_1Type.html)

核心LLVM类是表示正在核查(inspected)或转换的程序的主要方法。 核心LLVM类在`include/llvm/IR`目录的头文件中定义，并在`lib/IR`目录中实现。 值得注意的是，由于历史原因，这个库被称为`libLLVMCore.so`，而不是`libLLVMIR.so`，正如您所料。

### The Type class and Derived Types

Type是所有类型类的父类。 每个值都有一个类型。 Type不能直接实例化，只能通过其子类实例化。 某些基本类型（VoidType，LabelType，FloatType和DoubleType）具有隐藏的子类。 它们是隐藏的，是因为它们提供的功能超出了Type类提供的功能，为了区别于Type的其他子类。

所有其他类型都是`DerivedType`的子类。 可以命名类型，但这不是必需的。 任何时候都只存在一个给定形状的实例。 这允许使用Type Instance的地址相等性来执行类型相等。 也就是说，给定两个Type *值，如果指针相同，则类型相同。

#### Important Public Methods

- `bool isIntegerTy（）const`：对于任何整数类型，返回true。 
- `bool isFloatingPointTy（）`：如果这是五种浮点类型之一，则返回true。
- `bool isSized（）`：如果类型已知大小(类型占用位数已知)，则返回true。 没有大小的东西是抽象类型，标签和空白。

#### Important Derived Types

`IntegerType`: 

- `DerivedType`的子类，表示任何位宽的整数类型。 可以表示`IntegerType :: MIN_INT_BITS(1)`和`IntegerType :: MAX_INT_BITS`（~800万）之间的任何位宽。
  - `static const IntegerType* ` get（unsigned NumBits）：获取特定位宽的整数类型。
  - `unsigned getBitWidth（）const`：获取整数类型的位宽。

`SequentialType` :

- 这由ArrayType和VectorType子类化。
  - `const Type * getElementType（）const`：返回顺序类型中每个元素的类型。
  - `uint64_t getNumElements（）const`：返回顺序类型中的元素数。

`ArrayType`:

- 这是SequentialType的子类，并定义了数组类型的接口。

`PointerType`:

- 指针类型的类的子类。

`VectorType`:

- 用于矢量类型的SequentialType的子类。 向量类型类似于ArrayType但是区分因为它是第一类类型而ArrayType不是。 向量类型用于向量运算，通常是整数或浮点类型的小向量。

`StructType`:

- 结构类型的DerivedTypes的子类。

`FunctionType`:

- 函数类型的DerivedTypes的子类。
  - `bool isVarArg（）const`：如果是vararg函数，则返回true。
  - `const Type * getReturnType（）const`：返回函数的返回类型。 
  - `const Type * getParamType（unsigned i）`：返回第i个参数的类型。
  - `const unsigned getNumParams（）const`：返回形式参数的数量。

### The Module class

`#include "llvm/IR/Module.h"`

header source: [Module.h](http://llvm.org/doxygen/Module_8h_source.html)

doxygen info: [Module Class](http://llvm.org/doxygen/classllvm_1_1Module.html)

`Module`类表示LLVM程序中存在的顶层结构。 LLVM `Module`实际上是原始程序的转换单元或由链接器合并的若干转换单元的组合。 Module类跟踪[Function](http://llvm.org/docs/ProgrammersManual.html#c-function)列表，[GlobalVariables](http://llvm.org/docs/ProgrammersManual.html#globalvariable)列表和[SymbolTable](http://llvm.org/docs/ProgrammersManual.html#symboltable)。 此外，它还包含一些有用的成员函数，可以使常见操作变得简单。 

#### Important Public Members of the Module class

- `Module::Module(std::string name = "")`

  构建 [Module](http://llvm.org/docs/ProgrammersManual.html#module)很容易。 您可以选择为其提供名称（可能基于转换单元的名称）。

- `Module::iterator`- 函数列表迭代器的Typedef

  `Module::const_iterator` - const_iterator的Typedef。

  `begin()`, `end()`, `size()`, `empty()`

  const_iteraThese的Typedef是转发方法，可以轻松访问Module对象的Function list的内容。

- Module::FunctionListType &getFunctionList()

  返回[Function](http://llvm.org/docs/ProgrammersManual.html#c-function)列表。 当您需要更新列表或执行没有转发方法的复杂操作时，必须使用此选项

***

- `Module::global_iterator` - Typedef用于全局变量列表迭代器

  `Module::const_global_iterator` - const_iterator的Typedef。

  `global_begin()`, `global_end()`, `global_size()`, `global_empty()`

  返回函数列表。 当您需要更新列表或执行不具有的复杂操作时，必须使用这些转发方法，以便于访问Module对象的[GlobalVariable](http://llvm.org/docs/ProgrammersManual.html#globalvariable)列表的内容。转发方法...

- `Module::GlobalListType &getGlobalList()`

  返回GlobalVariables列表。 当您需要更新列表或执行没有转发方法的复杂操作时，必须使用此选项。

***

- `SymbolTable *getSymbolTable()`

  返回对此模块的[SymbolTable](http://llvm.org/docs/ProgrammersManual.html#symboltable)的引用。

***

- `Function *getFunction(StringRef Name) const`

  在Module [SymbolTable](http://llvm.org/docs/ProgrammersManual.html#symboltable)中查找指定的函数。 如果它不存在，则返回null。

- `Function *getOrInsertFunction(const std::string &Name, const FunctionType *T)`

  在Module SymbolTable 中查找指定的函数。 如果它不存在，请为该函数添加外部声明并将其返回。

- `std::string getTypeName(const Type *Ty)`

  如果SymbolTable中至少有一个指定Type的条目，则返回该条目。 否则返回空字符串。

- `bool addTypeName(const std::string &Name, const Type *Ty)`

  在SymbolTable映射名称中插入一个条目到Ty。 如果此名称已有条目，则返回true并且不修改SymbolTable。

### The Value class

`#include "llvm/IR/Value.h"`

header source: [Value.h](http://llvm.org/doxygen/Value_8h_source.html)

doxygen info: [Value Class](http://llvm.org/doxygen/classllvm_1_1Value.html)

Value类是LLVM源代码库中最重要的类。 它表示一个类型值，可以用作（除其他外）作为指令的操作数。 有许多不同类型的值，例如[常量](http://llvm.org/docs/ProgrammersManual.html#constant)，[参数](http://llvm.org/docs/ProgrammersManual.html#argument)。 甚至[指令](http://llvm.org/docs/ProgrammersManual.html#instruction)和[函数](http://llvm.org/docs/ProgrammersManual.html#c-function)都是值。

对于程序，可以在LLVM表示中多次使用特定值。 例如，函数的传入参数（用Argument类的实例表示）被引用该参数的函数中的每个指令“使用”。 为了跟踪这种关系，Value类保留了使用它的所有用户的列表（User类是LLVM图中可以引用值的所有节点的基类）。 此使用列表是LLVM如何表示程序中的def-use信息，可通过use_ *方法访问，如下所示。

因为LLVM是类型化表示，所以每个LLVM值都是类型化的，并且此[类型](http://llvm.org/docs/ProgrammersManual.html#type)可通过getType（）方法获得。 此外，可以命名所有LLVM值。 Value的“名称”是在LLVM代码中打印的符号字符串：

```
%foo = add i32 1, 2
```

该指令的名称是“foo”。 请注意，可能缺少任何值的名称（空字符串），因此名称应仅用于调试（使源代码更易于阅读，调试打印输出），不应使用它们来跟踪值或在 他们。 为此，请使用指向Value本身的std :: map指针。

LLVM的一个重要方面是SSA变量与产生它的操作之间没有区别。 因此，对指令生成的值的任何引用（或者可用作传入参数的值）都表示为指向表示此值的类的实例的直接指针。 虽然这可能需要一些时间来习惯，但它简化了表示并使其更容易操作。

#### Important Public Members of the Value class

- `Value::use_iterator` - 在use-list上使用Typedef作为迭代器

  `Value::const_use_iterator` - 使用列表中的const_iterator的typedef

  `unsigned use_size()` - 返回值的用户数。

  `bool use_empty()` - 如果没有用户，则返回true。

  `use_iterator use_begin()` - 获取use-list开头的迭代器。

  `use_iterator use_end()` - 获取使用列表末尾的迭代器。

  `User *use_back()` - 返回列表中的最后一个元素。

  这些方法是访问LLVM中的def-use信息的接口。 与LLVM中的所有其他迭代器一样，命名约定遵循[STL](http://llvm.org/docs/ProgrammersManual.html#stl)定义的约定。

- `Type *getType() const` 此方法返回值的类型。

- `bool hasName() const`

  `std::string getName() const`

  `void setName(const std::string &Name)`

  此系列方法用于访问并为Value指定名称，请注意[上述预防措施](http://llvm.org/docs/ProgrammersManual.html#namewarning)。

- `void replaceAllUsesWith(Value *V)`

  此方法遍历值的使用列表，将当前值的所有用户更改为引用“V”。 例如，如果您检测到指令始终生成常量值（例如通过常量折叠），则可以使用如下常量替换指令的所有用法：

```c++
Inst->replaceAllUsesWith(ConstVal);
```

### The User class

`#include "llvm/IR/User.h"`

header source: [User.h](http://llvm.org/doxygen/User_8h_source.html)

doxygen info: [User Class](http://llvm.org/doxygen/classllvm_1_1User.html)

Superclass: [Value](http://llvm.org/docs/ProgrammersManual.html#value)

 User类是可以引用Values的所有LLVM节点的公共基类。 它公开了一个“操作数”列表，它是用户所指的所有值。 User类本身是Value的子类。

用户的操作数直接指向它所引用的LLVM值。 因为LLVM使用静态单一分配（SSA）形式，所以只能引用一个定义，允许这种直接连接。 此连接在LLVM中提供use-def信息。

#### Important Public Members of the User class

User类以两种方式公开操作数列表：通过索引访问接口和基于迭代器的接口。

- `Value *getOperand(unsigned i)`

  `unsigned getNumOperands()`

  这两种方法以方便的形式公开用户的操作数以便直接访问。

- `User::op_iterator` - 在操作数列表上输入itdef作为迭代器

  `op_iterator op_begin()` - 获取操作数列表开头的迭代器。

  `op_iterator op_end()` - 获取操作数列表末尾的迭代器。

  这些方法一起构成了基于迭代器的用户操作数界面。

### The Instruction class

`#include "llvm/IR/Instruction.h"`

header source: [Instruction.h](http://llvm.org/doxygen/Instruction_8h_source.html)

doxygen info: [Instruction Class](http://llvm.org/doxygen/classllvm_1_1Instruction.html)

Superclasses: [User](http://llvm.org/docs/ProgrammersManual.html#user), [Value](http://llvm.org/docs/ProgrammersManual.html#value)

`Instruction`类是所有LLVM指令的公共基类。它只提供了一些方法，但却是一个非常常用的类。由指令类本身跟踪的主要数据是操作码（指令类型）和嵌入指令的父BasicBlock。为了表示特定类型的指令，使用了许多指令子类之一。

因为Instruction类是[User](http://llvm.org/docs/ProgrammersManual.html#user)类的子类，所以可以以与其他Users相同的方式访问其操作数（使用`getOperand()/ getNumOperands()和op_begin()/op_end()方法`）。 Instruction类的一个重要文件是`llvm / Instruction.def`文件。此文件包含有关LLVM中各种不同类型指令的一些元数据。它描述了用作操作码的枚举值（例如，`Instruction :: Add和Instruction :: ICmp`），以及实现指令的具体子类（例如[BinaryOperator](http://llvm.org/docs/ProgrammersManual.html#binaryoperator)和[CmpInst](http://llvm.org/docs/ProgrammersManual.html#cmpinst))。不幸的是，在这个文件中使用宏会混淆doxygen，所以这些枚举值在[doxygen](http://llvm.org/doxygen/classllvm_1_1Instruction.html)中没有正确显示。

#### Important Subclasses of the Instruction class

- `BinaryOperator`

  此子类表示所有两个操作数指令，其操作数必须是相同类型，但比较指令除外。

- `CastInst` 此子类是12个转换指令的父级。 它提供了关于演员说明的常见操作。

- `CmpInst` 此子类表示两个比较指令ICmpInst（整数opreands）和FCmpInst（浮点操作数）。

- `TerminatorInst`

  此子类是所有终止符指令（可以终止块的指令）的父节点。

#### Important Public Members of the Instruction class

- `BasicBlock *getParent()`

  返回嵌入此指令的[BasicBlock](http://llvm.org/docs/ProgrammersManual.html#basicblock)。

- `bool mayWriteToMemory()`

  如果指令写入内存，则返回true，即它是一个调用，空闲，调用或存储。

- `unsigned getOpcode()`

  返回指令的操作码。

- `Instruction *clone() const`

  返回指定指令的另一个实例，除了指令没有父节点（即它没有嵌入BasicBlock）之外，在所有方面都与原始指令相同，并且它没有名称。

### The Constant class and subclasses

`Constant`类和`subclassesConstant`表示不同类型常量的基类。 它由`ConstantInt`，`ConstantArray`等子类表示，用于表示各种类型的常量。 [GlobalValue](http://llvm.org/docs/ProgrammersManual.html#globalvalue)也是一个子类，表示全局变量或函数的地址。

#### Important Subclasses of Constant

- `ConstantInt`:Constant的重要子类Constant的这个子类表示任何宽度的整数常量。
  - `const APInt& getValue() const`: 返回此常量的基础值，即APInt值。
  - `int64_t getSExtValue() const`: 通过符号扩展将基础APInt值转换为int64_t。 如果APInt的值（不是位宽）太大而不适合int64_t，则会产生断言。 因此，不鼓励使用此方法。
  - `uint64_t getZExtValue() const`: 通过零扩展将基础APInt值转换为uint64_t。 如果APInt的值（不是位宽）太大而不适合uint64_t，则会产生断言。 因此，不鼓励使用此方法。
  - `static ConstantInt* get(const APInt& Val)`: 返回表示Val提供的值的ConstantInt对象。 该类型隐含为与Val的位宽对应的IntegerType。
  - `static ConstantInt* get(const Type *Ty, uint64_t Val)`: 返回ConstantInt对象，该对象表示Val为整数类型Ty提供的值。
- `ConstantFP` : 此类表示浮点常量。
  - `double getValue() const`: 返回此常量的基础值。
- `ConstantArray` : 这代表一个常数数组。
  - `const std::vector<Use> &getValues() const`: 返回构成此数组的组件常量的向量。
- `ConstantStruct `: 这表示一个常量结构。
  - `const std::vector<Use> &getValues() const`: 返回构成此数组的组件常量的向量。
- `GlobalValue`:这表示全局变量或函数。 在任何一种情况下，该值都是一个固定的固定地址（链接后）。

### The GlobalValue class

`#include "llvm/IR/GlobalValue.h"`

header source: [GlobalValue.h](http://llvm.org/doxygen/GlobalValue_8h_source.html)

doxygen info: [GlobalValue Class](http://llvm.org/doxygen/classllvm_1_1GlobalValue.html)

Superclasses: [Constant](http://llvm.org/docs/ProgrammersManual.html#constant), [User](http://llvm.org/docs/ProgrammersManual.html#user), [Value](http://llvm.org/docs/ProgrammersManual.html#value)

Global values（[GlobalVariables](http://llvm.org/docs/ProgrammersManual.html#globalvariable)或[Function](http://llvm.org/docs/ProgrammersManual.html#c-function)）是在所有函数体(body)中,唯一可见的LLVM值。因为它们在全局范围内可见，所以它们还在不同转换单元中,与定义其他全局变量相关联。为了控制链接过程，GlobalValues知道它们的链接规则。具体来说，GlobalValues知道它们是否具有内部链接或外部链接，如LinkageTypes枚举所定义。

如果GlobalValue具有内部链接（相当于C中的静态链接），则当前转换单元外部的代码不可见，并且不参与链接。如果它具有外部链接，则外部代码可以看到它，并且确实参与链接。除了链接信息，GlobalValues还会跟踪它们当前所属的模块。

因为GlobalValues是内存对象，所以它们总是由它们的地址引用。因此，全局类型始终是指向其内容的指针。在使用GetElementPtrInst指令时记住这一点很重要，因为必须首先取消引用此指针。例如，如果你有一个GlobalVariable（GlobalValue的子类），它是一个24 int的数组，键入[24 x i32]，那么GlobalVariable是指向该数组的指针。虽然此数组的第一个元素的地址和GlobalVariable的值相同，但它们具有不同的类型。 GlobalVariable的类型是[24 x i32]。第一个元素的类型是i32。因此，访问全局值需要先使用GetElementPtrInst取消引用指针，然后才能访问其元素。 “[LLVM语言参考手册](http://llvm.org/docs/LangRef.html#globalvars)”对此进行了解释。

#### Important Public Members of the GlobalValue class

- `bool hasInternalLinkage() const`

  `bool hasExternalLinkage() const`

  `void setInternalLinkage(bool HasInternalLinkage)`

  这些方法操纵GlobalValue的链接特征。

- `Module *getParent()`

  这将返回GlobalValue当前嵌入的[Module](http://llvm.org/docs/ProgrammersManual.html#module)。

### The Function class

`#include "llvm/IR/Function.h"`

header source: [Function.h](http://llvm.org/doxygen/Function_8h_source.html)

doxygen info: [Function Class](http://llvm.org/doxygen/classllvm_1_1Function.html)

Superclasses: [GlobalValue](http://llvm.org/docs/ProgrammersManual.html#globalvalue), [Constant](http://llvm.org/docs/ProgrammersManual.html#constant), [User](http://llvm.org/docs/ProgrammersManual.html#user), [Value](http://llvm.org/docs/ProgrammersManual.html#value)

Function类表示LLVM中的单个执行过程。 它实际上是LLVM层次结构中更复杂的类之一，因为它必须跟踪大量数据。 Function类跟踪[BasicBlocks](http://llvm.org/docs/ProgrammersManual.html#basicblock)列表，正式[Arguments](http://llvm.org/docs/ProgrammersManual.html#argument)列表和[SymbolTable](http://llvm.org/docs/ProgrammersManual.html#symboltable)。

 [BasicBlock](http://llvm.org/docs/ProgrammersManual.html#basicblock)s列表是Function对象中最常用的部分。该列表强制函数中块的隐式排序，其表示代码将如何在后端布局。此外，第一个BasicBlock是函数的隐式入口节点。 LLVM中明确表示，分支作为此初始块是不合法的。没有隐式退出节点，实际上单个Function可能有多个退出节点。如果BasicBlock列表为空，则表示Function实际上是一个函数声明：函数的函数体尚未链接。

除了 [BasicBlock](http://llvm.org/docs/ProgrammersManual.html#basicblock)s列表之外，Function类还跟踪函数接收的正式[Argument](http://llvm.org/docs/ProgrammersManual.html#argument)s 列表。此容器管理Argument节点的生存期，就像BasicBlock列表对BasicBlocks一样。

 [SymbolTable](http://llvm.org/docs/ProgrammersManual.html#symboltable) 是一种非常少使用的LLVM功能，仅在您必须按名称查找值时使用。除此之外，SymbolTable在内部用于确保函数体中的Instructions，BasicBlocks或Arguments的名称之间没有冲突。

请注意，Function可以是 [GlobalValue](http://llvm.org/docs/ProgrammersManual.html#globalvalue)，可以是[Constant](http://llvm.org/docs/ProgrammersManual.html#constant).。函数的值是它的地址（链接后），保证是常量。

#### Important Public Members of the Function

- `Function(const FunctionType *Ty, LinkageTypes Linkage, const std::string &N = "", Module*Parent = 0)`

  当您需要创建新函数以添加程序时使用的构造函数。 构造函数必须指定要创建的函数的类型以及函数应具有的链接类型。 FunctionType参数指定函数的形式参数和返回值。 相同的 [FunctionType](http://llvm.org/docs/ProgrammersManual.html#functiontype)值可用于创建多个函数。 Parent参数指定定义函数的Module。 如果提供了此参数，则该函数将自动插入到该模块的函数列表中。

- `bool isDeclaration()`

  返回函数是否已定义主体。 如果函数是“外部引用(external)”，则它没有函数体，因此必须通过链接到另一个定义此函数的转换单元中来解析。

- `Function::iterator` - Typedef for basic block list iterator

  `Function::const_iterator` - Typedef for const_iterator.

  `begin()`, `end()`, `size()`, `empty()`

  这些转发方法可以轻松访问Function对象的 [BasicBlock](http://llvm.org/docs/ProgrammersManual.html#basicblock)列表的内容。 

- `Function::BasicBlockListType &getBasicBlockList()`

  返回BasicBlocks列表。 当您需要更新列表或执行没有转发方法的复杂操作时，必须使用此选项。

- `Function::arg_iterator` - 参数列表迭代器的Typedef

  `Function::const_arg_iterator` - const_iterator的Typedef。

  `arg_begin()`, `arg_end()`, `arg_size()`, `arg_empty()`

  这些转发方法可以轻松访问Function对象的[Argument](http://llvm.org/docs/ProgrammersManual.html#argument)列表的内容。

- `Function::ArgumentListType &getArgumentList()`

  返回Argument列表。 当您需要更新列表或执行没有转发方法的复杂操作时，必须使用此选项。

- `BasicBlock &getEntryBlock()`

  返回函数的条目BasicBlock。 因为函数的入口块始终是第一个块，所以它返回Function的第一个块。

- `Type *getReturnType()`

  `FunctionType *getFunctionType()`

   这将遍历函数的类型并返回函数的返回类型或实际函数的[FunctionType](http://llvm.org/docs/ProgrammersManual.html#functiontype)。

- `SymbolTable *getSymbolTable()`

  返回指向此函数的[SymbolTable](http://llvm.org/docs/ProgrammersManual.html#symboltable) 的指针。

### The GlobalVariable class

`#include "llvm/IR/GlobalVariable.h"`

header source: [GlobalVariable.h](http://llvm.org/doxygen/GlobalVariable_8h_source.html)

doxygen info: [GlobalVariable Class](http://llvm.org/doxygen/classllvm_1_1GlobalVariable.html)

Superclasses: [GlobalValue](http://llvm.org/docs/ProgrammersManual.html#globalvalue), [Constant](http://llvm.org/docs/ProgrammersManual.html#constant), [User](http://llvm.org/docs/ProgrammersManual.html#user), [Value](http://llvm.org/docs/ProgrammersManual.html#value)

全局变量用（意外惊喜）GlobalVariable类表示。 与函数一样，GlobalVariables也是[GlobalValue](http://llvm.org/docs/ProgrammersManual.html#globalvalue)的子类，因此总是由它们的地址引用（全局值必须存在于内存中，因此它们的“名称”指的是它们的常量地址）。 有关详细信息，请参阅[GlobalValue](http://llvm.org/docs/ProgrammersManual.html#globalvalue)。 全局变量可能具有初始值（必须是 [Constant](http://llvm.org/docs/ProgrammersManual.html#constant)），如果它们具有初始化程序，则它们本身可以标记为“常量”（表示它们的内容在运行时永远不会更改）。

#### Important Public Members of the GlobalVariable class

- `GlobalVariable(const Type *Ty, bool isConstant, LinkageTypes &Linkage, Constant *Initializer =0, const std::string &Name = "", Module* Parent = 0)`

  创建指定类型的新全局变量。 如果isConstant为true，则全局变量将被标记为对程序不变。 Linkage参数指定变量的链接类型（内部，外部，弱，linkonce，追加）。 如果链接是InternalLinkage，WeakAnyLinkage，WeakODRLinkage，LinkOnceAnyLinkage或LinkOnceODRLinkage，那么结果全局变量将具有内部链接。 AppendingLinkage将变量的所有实例（在不同的转换单元中）连接在一起作为单个变量，但仅适用于数组。 有关链接类型的更多详细信息，请[参阅LLVM语言参考]( [LLVM Language Reference](http://llvm.org/docs/LangRef.html#modulestructure))。 可选地，也可以为全局变量指定初始化器，名称和放置变量的模块。

- `bool isConstant() const`

  如果这是一个已知在运行时不被修改的全局变量，则返回true。

- `bool hasInitializer()`

  如果此全局变量具有初始值设定项，则返回true。

- `Constant *getInitializer()`

  返回GlobalVariable的初始值。 如果没有初始化程序，则调用此方法是不合法的。

### The BasicBlock class

`#include "llvm/IR/BasicBlock.h"`

header source: [BasicBlock.h](http://llvm.org/doxygen/BasicBlock_8h_source.html)

doxygen info: [BasicBlock Class](http://llvm.org/doxygen/classllvm_1_1BasicBlock.html)

Superclass: [Value](http://llvm.org/docs/ProgrammersManual.html#value)

 此类表示代码的单个条目单个退出部分，通常称为编译器社区的基本块。 BasicBlock类维护一个 [Instruction](http://llvm.org/docs/ProgrammersManual.html#instruction)列表，它们构成块的主体。 匹配语言定义，这个指令列表的最后一个元素总是一个终结符指令（ [TerminatorInst](http://llvm.org/docs/ProgrammersManual.html#terminatorinst) 类的子类）。

除了跟踪组成块的指令列表之外，BasicBlock类还跟踪它嵌入的 [Function](http://llvm.org/docs/ProgrammersManual.html#c-function)。

请注意，BasicBlocks本身就是 [Value](http://llvm.org/docs/ProgrammersManual.html#value)s，因为它们被分支等指令引用，并且可以进入切换表。 BasicBlocks有类型标签。

#### Important Public Members of the BasicBlock class

- `BasicBlock(const std::string &Name = "", Function *Parent = 0)`

  BasicBlock构造函数用于创建插入函数的新基本块。 构造函数可选地为新块命名，并使用[Function](http://llvm.org/docs/ProgrammersManual.html#c-function)将其插入。 如果指定了Parent参数，则新的BasicBlock会自动插入到指定Function的末尾，如果未指定，则必须手动将BasicBlock插入到Function中。

- `BasicBlock::iterator` - Typedef用于指令列表迭代器

  `BasicBlock::const_iterator` - Typedef for const_iterator.

  `begin()`, `end()`, `front()`, `back()`, `size()`, `empty()` STL-用于访问指令列表的样式函数。

  这些方法和typedef是转发函数，它们具有与相同名称的标准库方法相同的语义。 这些方法以易于操作的方式公开基本块的基础指令列表。 要获得完整的容器操作（包括更新列表的操作），必须使用getInstList（）方法。

- `BasicBlock::InstListType &getInstList()`

  此方法用于访问实际包含指令的基础容器。 当BasicBlock类中没有转发函数用于您要执行的操作时，必须使用此方法。 由于没有用于“更新”操作的转发功能，因此如果要更新BasicBlock的内容，则需要使用此功能。

- `Function *getParent()`

  返回指向嵌入块的函数的指针，如果无家可归，则返回空指针。

- `TerminatorInst *getTerminator()`

  返回指向出现在BasicBlock末尾的终止符指令的指针。 如果没有终结器指令，或者块中的最后一条指令不是终结符，则返回空指针。

### The Argument class

Value的这个子类定义了函数的传入形式参数的接口。 函数维护其正式参数的列表。 参数具有指向父函数的指针。