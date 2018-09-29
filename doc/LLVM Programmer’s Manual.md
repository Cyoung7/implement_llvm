# LLVM Programmer’s Manual

[原文链接](http://llvm.org/docs/ProgrammersManual.html#the-c-standard-template-library)

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

### Error handling(null)



## Picking the Right Data Structure for a Task

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

### Making simple changes(null)



## Threads and LLVM

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

### The GlobalValue class(null)

