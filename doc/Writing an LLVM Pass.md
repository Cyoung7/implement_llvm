# Writing an LLVM Pass

Information on how to write LLVM transformations and analyses.(LLVM的转换和分析)

./gh-md-toc /media/cyoung/000E88CC0009670E/CLionProjects/implement_llvm/doc/Writing\ an\ LLVM\ Pass.md

[原文链接](http://llvm.org/docs/WritingAnLLVMPass.html)

[TOC]

   * [Writing an LLVM Pass](#writing-an-llvm-pass)
      * [Introduction — What is a pass?](#introduction--what-is-a-pass)
      * [Quick Start — Writing hello world](#quick-start--writing-hello-world)
         * [Setting up the build environment](#setting-up-the-build-environment)
         * [Basic code required](#basic-code-required)
         * [Running a pass with <em>opt</em>](#running-a-pass-with-opt)
      * [Pass classes and requirements](#pass-classes-and-requirements)
         * [The ImmutablePass class](#the-immutablepass-class)
         * [The ModulePass class](#the-modulepass-class)
            * [The runOnModule method](#the-runonmodule-method)
         * [The CallGraphSCCPass class](#the-callgraphsccpass-class)
            * [The doInitialization(CallGraph &amp;) method](#the-doinitializationcallgraph--method)
            * [The runOnSCC method](#the-runonscc-method)
            * [The doFinalization(CallGraph &amp;) method](#the-dofinalizationcallgraph--method)
         * [The FunctionPass class](#the-functionpass-class)
            * [The doInitialization(Module &amp;) method](#the-doinitializationmodule--method)
            * [The runOnFunction method](#the-runonfunction-method)
            * [The doFinalization(Module &amp;) method](#the-dofinalizationmodule--method)
         * [The LoopPass class](#the-looppass-class)
            * [The doInitialization(Loop *, LPPassManager &amp;) method](#the-doinitializationloop--lppassmanager--method)
            * [The runOnLoop method](#the-runonloop-method)
            * [The doFinalization() method](#the-dofinalization-method)
         * [The RegionPass class](#the-regionpass-class)
            * [The doInitialization(Region *, RGPassManager &amp;) method](#the-doinitializationregion--rgpassmanager--method)
            * [The runOnRegion method](#the-runonregion-method)
            * [The doFinalization() method](#the-dofinalization-method-1)
         * [The BasicBlockPass class](#the-basicblockpass-class)
            * [The doInitialization(Function &amp;) method](#the-doinitializationfunction--method)
            * [The runOnBasicBlock method](#the-runonbasicblock-method)
            * [The doFinalization(Function &amp;) method](#the-dofinalizationfunction--method)
         * [The MachineFunctionPass class](#the-machinefunctionpass-class)
            * [The runOnMachineFunction(MachineFunction &amp;MF) method](#the-runonmachinefunctionmachinefunction-mf-method)
         * [Pass registration](#pass-registration)
            * [The print method](#the-print-method)
         * [Specifying interactions between passes](#specifying-interactions-between-passes)
            * [The getAnalysisUsage metho](#the-getanalysisusage-metho)
            * [The AnalysisUsage::addRequired&lt;&gt; and AnalysisUsage::addRequiredTransitive&lt;&gt; methods](#the-analysisusageaddrequired-and-analysisusageaddrequiredtransitive-methods)
            * [The AnalysisUsage::addPreserved&lt;&gt; method](#the-analysisusageaddpreserved-method)
            * [Example implementations of getAnalysisUsage](#example-implementations-of-getanalysisusage)
            * [The getAnalysis&lt;&gt; and getAnalysisIfAvailable&lt;&gt; methods](#the-getanalysis-and-getanalysisifavailable-methods)
         * [Implementing Analysis Groups](#implementing-analysis-groups)
            * [Analysis Group Concepts](#analysis-group-concepts)
            * [Using RegisterAnalysisGroup](#using-registeranalysisgroup)
      * [Pass Statistics](#pass-statistics)
         * [What PassManager does](#what-passmanager-does)
            * [The releaseMemory method](#the-releasememory-method)
      * [Registering dynamically loaded passes](#registering-dynamically-loaded-passes)
         * [Using existing registries](#using-existing-registries)
         * [Creating new registries](#creating-new-registries)
         * [Using GDB with dynamically loaded passes](#using-gdb-with-dynamically-loaded-passes)
            * [Setting a breakpoint in your pass](#setting-a-breakpoint-in-your-pass)
            * [Miscellaneous Problems](#miscellaneous-problems)
         * [Future extensions planned](#future-extensions-planned)
            * [Multithreaded LLVM](#multithreaded-llvm)

## Introduction — What is a pass?

LLVM Pass框架是LLVM系统的重要组成部分，因为LLVM传递(transformations)存在着编译器大多数有趣部分。通过执行构成编译器的转换和优化，它们构建这些转换所使用到的分析结果，基于以上，它们是编译器代码的结构化技术。

所有LLVMpasses都是[Pass](http://llvm.org/doxygen/classllvm_1_1Pass.html)类的子类，它通过重写从Pass继承的虚方法来实现功能。根据pass的工作方式，您应该继承[ModulePass](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-modulepass) , [CallGraphSCCPass](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-callgraphsccpass), [FunctionPass](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-functionpass) 或[LoopPass](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-looppass)，或[RegionPass](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-regionpass)或 [BasicBlockPass](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-basicblockpass) 类，这样可以为系统提供有关pass操作的更多信息，以及如何与其他pass结合使用。 LLVM Pass Framework的一个主要特性是对您的pass有所约束（由它们派生的类指示）来让pass以高效的方式运行。

我们首先向您展示如何构建Pass，从设置代码到编译，加载和执行它。基础知识完成后，将讨论更多高级功能。

## Quick Start — Writing hello world

在这里，我们描述如何编写passes的“hello world”。 “Hello”pass旨在简单地打印出正在编译的程序中存在的非外部函数的名称。 它根本不修改程序，它只是检查它。 此pass的源代码和文件位于`lib/Transforms/Hello`目录中的LLVM源代码树中。

### Setting up the build environment

首先，配置和构建LLVM。 接下来，您需要在LLVM源代码库中的某个位置创建一个新目录。 对于这个例子，我们假设您创建了`lib/Transforms/Hello`。 最后，您必须设置一个构建脚本，该脚本将编译新pass的源代码。 为此，请将以下内容复制到CMakeLists.txt中：

```cmake
add_llvm_loadable_module( LLVMHello
  Hello.cpp
  PLUGIN_TOOL
  opt
  )
```

和以下一行 `lib/Transforms/CMakeLists.txt`:

```cmake
add_subdirectory(Hello)
```

（请注意，已经有一个名为Hello的目录，带有示例“Hello”pass;您可以使用它 - 在这种情况下，您不需要修改任何 `CMakeLists.txt`文件 - 或者，如果您想从头开始创建所有内容 ，使用另一个名字。）

此构建脚本指定编译当前目录中的Hello.cpp文件并将其链接到共享对象`$(LEVEL)/lib/LLVMHello.so`，该工具可由opt工具通过其-load选项动态加载。 如果您的操作系统使用.so之外的后缀（例如Windows或Mac OS X），则将使用相应的扩展名。

现在我们已经设置了构建脚本，我们只需要为pass本身编写代码。

### Basic code required

现在我们有了编译新pass的方法，我们只需要编写它。 从以下开始：

```c++
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
```

这是必需的，因为我们正在编写Pass，我们正在使用 [Function](http://llvm.org/doxygen/classllvm_1_1Function.html)s，我们将进行一些打印。

接下来我们有：

```c++
using namespace llvm;
```

...这是必需的，因为包含文件中的函数存在于llvm命名空间中。

接下来我们有：

```c++
namespace {
```

...开始一个匿名命名空间。 匿名命名空间是C++的“静态”关键字是C（在全局范围内）。 它使在匿名命名空间内声明的内容仅对当前文件可见。 如果您不熟悉它们，请参阅一本体面的C++书籍以获取更多信息。

接下来，我们声明我们的传递本身：

```c++
struct Hello : public FunctionPass {
```

这声明了一个“Hello”类，它是[FunctionPass](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-functionpass)的子类。 稍后将详细描述不同的内置pass子类，但是现在，知道FunctionPass一次对一个函数进行操作。

```c++
static char ID;
Hello() : FunctionPass(ID) {}
```

这声明了LLVM用于标识pass的传递标识符。 这允许LLVM避免使用昂贵的C++运行时信息。

```c++
  bool runOnFunction(Function &F) override {
    errs() << "Hello: ";
    errs().write_escaped(F.getName()) << '\n';
    return false;
  }
}; // end of struct Hello
}  // end of anonymous namespace
```

我们声明了一个 [runOnFunction](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-runonfunction)方法，它覆盖了从 [FunctionPass](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-functionpass)继承的抽象虚方法。 这是我们应该做的事情，所以我们只用每个函数的名称打印出我们的消息。

```c++
char Hello::ID = 0;
```

我们在这里初始化通行证ID。 LLVM使用ID的地址来标识传递，因此初始化值并不重要。

```c++
static RegisterPass<Hello> X("hello", "Hello World Pass",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);
```

最后，我们e [register our class](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-registration) Hello，给它一个命令行参数“hello”，并命名为“Hello World Pass”。 最后两个参数描述了它的行为：如果传递遍历CFG而不修改它，那么第三个参数设置为true; 如果pass是分析遍，例如dominator tree pass，则提供true作为第四个参数。

整体而言，.cpp文件如下所示：

```c++
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
struct Hello : public FunctionPass {
  static char ID;
  Hello() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override {
    errs() << "Hello: ";
    errs().write_escaped(F.getName()) << '\n';
    return false;
  }
}; // end of struct Hello
}  // end of anonymous namespace

char Hello::ID = 0;
static RegisterPass<Hello> X("hello", "Hello World Pass",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);
```

现在它们一起使用，从构建目录的顶层使用简单的“gmake”命令编译该文件，您应该获得一个新文件“lib / LLVMHello.so”。 请注意，此文件中的所有内容都包含在匿名命名空间中 - 这反映了这样一个事实：pass是自包含的单元，不需要外部接口（尽管它们可以使用它们）。

### Running a pass with *opt*

现在您有了一个全新的闪亮共享对象文件，我们可以使用opt命令通过您的传递来运行LLVM程序。 因为您使用RegisterPass注册了您的通行证，所以一旦加载，您就可以使用opt工具来访问它。

要对其进行测试，请按照 [Getting Started with the LLVM System](https://llvm.org/docs/GettingStarted.html)末尾的示例将“Hello World”编译为LLVM。 我们现在可以通过这样的转换运行程序的bitcode文件（hello.bc）（或者当然，任何bitcode文件都可以）：

```
$ opt -load lib/LLVMHello.so -hello < hello.bc > /dev/null
Hello: __main
Hello: puts
Hello: main
```

[`-load`](https://llvm.org/docs/CommandGuide/lli.html#cmdoption-load)选项指定opt应该将您的传递加载为共享对象，这使得“-hello”成为有效的命令行参数（这是您需要 [register your pass](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-registration))的一个原因）。 因为Hello pass不以任何有趣的方式修改程序，所以我们只丢弃opt的结果（将它发送到 /dev/null）。

要查看您注册的其他字符串发生了什么，请尝试使用[`-help`](https://llvm.org/docs/CommandGuide/FileCheck.html#cmdoption-help)选项运行opt：

```
$ opt -load lib/LLVMHello.so -help
OVERVIEW: llvm .bc -> .bc modular optimizer and analysis printer

USAGE: opt [subcommand] [options] <input bitcode file>

OPTIONS:
  Optimizations available:
...
    -guard-widening           - Widen guards
    -gvn                      - Global Value Numbering
    -gvn-hoist                - Early GVN Hoisting of Expressions
    -hello                    - Hello World Pass
    -indvars                  - Induction Variable Simplification
    -inferattrs               - Infer set function attributes
...
```

传递名称将作为传递的信息字符串添加，为opt用户提供一些文档。 既然你有一个工作通行证，你就可以继续前进，让它做你想要的酷转换。 一旦你完成所有的工作和测试，找出你的传球速度可能会很有用。  [PassManager](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-passmanager)提供了一个很好的命令行选项（--time-passes），允许您获取有关传递执行时间的信息以及您排队的其他传递。 例如：

```shell
$ opt -load lib/LLVMHello.so -hello -time-passes < hello.bc > /dev/null
Hello: __main
Hello: puts
Hello: main
===-------------------------------------------------------------------------===
                      ... Pass execution timing report ...
===-------------------------------------------------------------------------===
  Total Execution Time: 0.0007 seconds (0.0005 wall clock)

   ---User Time---   --User+System--   ---Wall Time---  --- Name ---
   0.0004 ( 55.3%)   0.0004 ( 55.3%)   0.0004 ( 75.7%)  Bitcode Writer
   0.0003 ( 44.7%)   0.0003 ( 44.7%)   0.0001 ( 13.6%)  Hello World Pass
   0.0000 (  0.0%)   0.0000 (  0.0%)   0.0001 ( 10.7%)  Module Verifier
   0.0007 (100.0%)   0.0007 (100.0%)   0.0005 (100.0%)  Total
```

如您所见，我们上面的实现非常快。 opt工具会自动插入列出的其他passes，以验证passes发出的LLVM是否仍然有效并且格式良好的LLVM（尚未以某种方式被破坏）。

现在你已经看到了传递背后的机制的基础知识，我们可以讨论它们如何工作以及如何使用它们的更多细节。

## Pass classes and requirements

在设计新传递时，您应该做的第一件事就是确定应该为传递子类化的类。  [Hello World](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-basiccode)示例使用 [FunctionPass](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-functionpass)类进行实现，但我们没有讨论为什么或何时发生这种情况。 在这里，我们讨论可用的类，从最一般到最具体。

为Pass选择超类时，您应该选择最具体的类，同时仍然能够满足列出的要求。 这为LLVM Pass Infrastructure提供了优化passes运行所需的信息，因此生成的编译器不会非常慢。

### The ImmutablePass class

最简单和无聊的传递类型是“[ImmutablePass](http://llvm.org/doxygen/classllvm_1_1ImmutablePass.html)”类。 此传递类型用于不必运行的传递，不更改状态，永远不需要更新。 这不是正常类型的转换或分析，但可以提供有关当前编译器配置的信息。

虽然很少使用此传递类，但提供有关正在编译的当前目标机器的信息以及可能影响各种转换的其他静态信息非常重要。

ImmutablePasses永远不会使其他转换无效，永远不会失效，永远不会“运行”。

### The ModulePass class

 [ModulePass](http://llvm.org/doxygen/classllvm_1_1ModulePass.html)类是您可以使用的所有超类中最常用的类。 从ModulePass派生表明您的传递使用整个程序作为一个单元，以无法预测的顺序引用函数体，或添加和删除函数。 因为对ModulePass子类的行为一无所知，所以不能对它们的执行进行优化。

模块传递可以使用函数级别传递（例如，支配者）使用`getAnalysis`接口`getAnalysis <DominatorTree>（llvm :: Function *）`来提供检索分析结果的函数，如果函数传递不需要任何模块或不可变传递。 注意，这只能对分析运行的函数进行，例如， 在统治者的情况下，你应该只要求DominatorTree进行函数定义，而不是声明。

要编写正确的ModulePass子类，请从ModulePass派生并使用以下签名重载runOnModule方法：

#### The runOnModule method

```c++
virtual bool runOnModule(Module &M) = 0;
```

runOnModule方法执行pass的有趣工作。 如果模块被转换修改，它应该返回true，否则返回false。

### The CallGraphSCCPass class

 [CallGraphSCCPass](http://llvm.org/doxygen/classllvm_1_1CallGraphSCCPass.html)用于需要在调用图上自动遍历程序的传递（在调用者之前调用callees）。从CallGraphSCCPass派生提供了一些构建和遍历CallGraph的机制，但也允许系统优化CallGraphSCCPasses的执行。如果您的通行证符合下面列出的要求，并且不符合 [FunctionPass](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-functionpass) 或 [BasicBlockPass](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-basicblockpass),的要求，则应从CallGraphSCCPass派生。

TODO：简要解释一下SCC，Tarjan的算法和B-U的含义。

为了明确，CallGraphSCCPass子类是：

- ...不允许检查或修改除当前SCC以及SCC的直接呼叫者和直接被叫者之外的任何功能。
- ...需要保留当前的CallGraph对象，更新它以反映对程序所做的任何更改。
- ...不允许在当前模块中添加或删除SCC，但它们可能会更改SCC的内容。
- ...允许在当前模块中添加或删除全局变量。
- ...允许在runOnSCC（包括全局数据）的调用中维护状态。

在某些情况下，实现CallGraphSCCPass有点棘手，因为它必须处理具有多个节点的SCC。如果修改程序，下面描述的所有虚拟方法都应返回true，否则返回false。

#### The doInitialization(CallGraph &) method

```
virtual bool doInitialization(CallGraph &CG);
```

`doInitialization`方法允许执行CallGraphSCCPas不允许执行的大部分操作。 它们可以添加和删除函数，获取函数指针等.doInitialization方法旨在执行不依赖于正在处理的SCC的简单初始化类型的东西。 doInitialization方法调用未安排与任何其他传递执行重叠（因此它应该非常快）。

#### The runOnSCC method

```
virtual bool runOnSCC(CallGraphSCC &SCC) = 0;
```

runOnSCC方法执行pass的有趣工作，如果模块被转换修改则返回true，否则返回false。

#### The doFinalization(CallGraph &) method

```
virtual bool doFinalization(CallGraph &CG);
```

doFinalization方法是一种不常用的方法，当传递框架为正在编译的程序中的每个SCC调用runOnSCC时调用该方法。

### The FunctionPass class

与ModulePass子类相比， [FunctionPass](http://llvm.org/doxygen/classllvm_1_1Pass.html) 子类确实具有系统可以预期的可预测的本地行为。 所有FunctionPass都独立于程序中的所有其他功能执行程序中的每个功能。 FunctionPasses不要求它们按特定顺序执行，FunctionPasses不要修改外部函数。

显而易见，不允许FunctionPass子类：

- 检查或修改当前正在处理的功能以外的功能。
- 在当前模块中添加或删除功能。
- 在当前模块中添加或删除全局变量。
- 跨[runOnFunction](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-runonfunction)调用（包括全局数据）维护状态。

实现FunctionPass通常很简单（例如，请参阅Hello World pass）。 FunctionPasses可能会使三个虚拟方法超载以完成其工作。 如果修改程序，所有这些方法都应返回true，否则返回false。

#### The doInitialization(Module &) method

```c++
virtual bool doInitialization(Module &M);
```

doInitialization方法允许执行不允许FunctionPasses执行的大部分操作。 它们可以添加和删除函数，获取函数指针等.doInitialization方法旨在执行不依赖于正在处理的函数的简单初始化类型的东西。 doInitialization方法调用未安排与任何其他传递执行重叠（因此它应该非常快）。

如何使用此方法的一个很好的例子是 [LowerAllocations](http://llvm.org/doxygen/LowerAllocations_8cpp-source.html)传递。 此过程将malloc和free指令转换为依赖于平台的malloc（）和free（）函数调用。 它使用doInitialization方法获取对其所需的malloc和free函数的引用，如有必要，可将原型添加到模块中。

#### The runOnFunction method

```c++
virtual bool runOnFunction(Function &F) = 0;
```

 runOnFunction方法必须由您的子类实现，以执行传递的转换或分析工作。 像往常一样，如果修改了函数，则应返回true值。

#### The doFinalization(Module &) method

```
virtual bool doFinalization(Module &M);
```

doFinalization方法是一种不经常使用的方法，当传递框架为正在编译的程序中的每个函数调用[runOnFunction](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-runonfunction)时调用该方法。

### The LoopPass class

所有LoopPass都在函数中的每个循环上执行，与函数中的所有其他循环无关。 LoopPass以循环嵌套顺序处理循环，以便最后处理最外层循环。

允许LoopPass子类使用LPPassManager接口更新循环嵌套。 实现循环传递通常很简单。 LoopPasses可能会使三个虚拟方法超载以完成其工作。 如果修改程序，所有这些方法都应该返回true，否则应该返回false。

LoopPass子类旨在作为主循环传递管道的一部分运行，需要保留其他循环在其管道所需的所有相同的函数分析。 为了简化这一过程，LoopUtils.h提供了一个getLoopAnalysisUsage函数。 它可以在子类的getAnalysisUsage覆盖中调用，以获得一致和正确的行为。 类似地，INITIALIZE_PASS_DEPENDENCY（LoopPass）将初始化这组功能分析。

#### The doInitialization(Loop *, LPPassManager &) method

```c++
virtual bool doInitialization(Loop *, LPPassManager &LPM);
```

doInitialization方法旨在执行不依赖于正在处理的函数的简单初始化类型的东西。 doInitialization方法调用未安排与任何其他传递执行重叠（因此它应该非常快）。 LPPassManager接口应用于访问功能或模块级别的分析信息。

#### The runOnLoop method

```c++
virtual bool runOnLoop(Loop *, LPPassManager &LPM) = 0;
```

您的子类必须实现runOnLoop方法才能执行传递的转换或分析工作。 像往常一样，如果修改了函数，则应返回true值。 LPPassManager接口应该用于更新循环嵌套。

#### The doFinalization() method

```c++
virtual bool doFinalization();
```

doFinalization方法是一种不经常使用的方法，当传递框架为正在编译的程序中的每个循环调用runOnLoop时调用该方法。

### The RegionPass class

RegionPass类似于 [LoopPass](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-looppass)，但在函数中的每个单个条目单个退出区域上执行。 RegionPass以嵌套顺序处理区域，以便最后处理最外层区域。

允许RegionPass子类使用RGPassManager接口更新区域树。 您可以重载RegionPass的三个虚方法来实现您自己的区域传递。 如果修改程序，所有这些方法都应该返回true，否则应该返回false。

#### The doInitialization(Region *, RGPassManager &) method

```c++
virtual bool doInitialization(Region *, RGPassManager &RGM);
```

doInitialization方法旨在执行不依赖于正在处理的函数的简单初始化类型的东西。 doInitialization方法调用未安排与任何其他传递执行重叠（因此它应该非常快）。 RPPassManager接口应用于访问功能或模块级别分析信息。

#### The runOnRegion method

```c++
virtual bool runOnRegion(Region *, RGPassManager &RGM) = 0;
```

runOnRegion方法必须由您的子类实现，以执行传递的转换或分析工作。 像往常一样，如果修改了区域，则应返回真值。 应使用RGPassManager接口更新区域树。

#### The doFinalization() method

```c++
virtual bool doFinalization();
```

doFinalization方法是一种不常用的方法，当pass框架为正在编译的程序中的每个区域调用runOnRegion时调用该方法。

### The BasicBlockPass class

`BasicBlockPasses` 就像[FunctionPass’s](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-functionpass)一样，除了它们必须一次限制它们的检查和修改范围到一个基本块。 因此，他们不允许执行以下任何操作：

- 修改或检查当前基本块之外的任何基本块。
- 在 [runOnBasicBlock](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-runonbasicblock).的调用中维护状态。
- 修改控制流图（通过更改终止符指令）
- 任何禁止使用 [FunctionPasses](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-functionpass)的东西。

BasicBlockPasses对传统的本地和“窥视孔”优化非常有用。 它们可能会覆盖[FunctionPass’s](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-functionpass) 所具有的相同的[doInitialization(Module &)](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-doinitialization-mod)和 [doFinalization(Module &)](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-dofinalization-mod) 方法，但也可以实现以下虚拟方法：

#### The doInitialization(Function &) method

```c++
virtual bool doInitialization(Function &F);
```

 `doInitialization` 方法允许执行`BasicBlockPass`es不允许执行的大部分操作，但FunctionPasses可以执行。 doInitialization方法旨在执行简单的初始化，该初始化不依赖于正在处理的BasicBlock。 doInitialization方法调用未安排与任何其他传递执行重叠（因此它应该非常快）。

#### The runOnBasicBlock method

```c++
virtual bool runOnBasicBlock(BasicBlock &BB) = 0;
```

重写此函数以执行BasicBlockPass的工作。 此功能不允许检查或修改参数以外的基本块，也不允许修改CFG。 如果修改了基本块，则必须返回true值。

#### The doFinalization(Function &) method

```c++
virtual bool doFinalization(Function &F);
```

doFinalization方法是一种不经常使用的方法，当传递框架为正在编译的程序中的每个BasicBlock调用[runOnBasicBlock](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-runonbasicblock)时调用该方法。 这可用于执行按功能完成。

### The MachineFunctionPass class

`MachineFunctionPass`是LLVM代码生成器的一部分，它在程序中每个LLVM函数的机器相关表示上执行。

代码生成器传递由TargetMachine :: addPassesToEmitFile和类似例程专门注册和初始化，因此通常不能从opt或bugpoint命令运行它们。

MachineFunctionPass也是一个FunctionPass，因此适用于FunctionPass的所有限制也适用于它。 MachineFunctionPasses还有其他限制。 特别是，不允许MachineFunctionPasses执行以下任何操作：

- 修改或创建任何LLVM IR指令，BasicBlocks，Arguments，Functions，GlobalVariables，GlobalAliases或Modules。
- 修改当前正在处理的MachineFunction以外的MachineFunction。
- 跨 [runOnMachineFunction](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-runonmachinefunction) （包括全局数据）的调用维护状态。

#### The runOnMachineFunction(MachineFunction &MF) method

```c++
virtual bool runOnMachineFunction(MachineFunction &MF) = 0;
```

runOnMachineFunction可以被认为是MachineFunctionPass的主要入口点; 也就是说，您应该覆盖此方法以执行MachineFunctionPass的工作。

在Module中的每个MachineFunction上调用runOnMachineFunction方法，以便MachineFunctionPass可以对函数的机器相关表示执行优化。 如果您想获得正在使用的MachineFunction的LLVM函数，请使用MachineFunction的getFunction（）访问器方法 - 但请记住，您不能从MachineFunctionPass修改LLVM函数或其内容。

### Pass registration

在Hello World示例传递中，我们说明了传递注册的工作原理，并讨论了使用它的一些原因以及它的用途。 在这里，我们讨论如何以及为什么通过注册。

如上所述，通过RegisterPass模板注册。 template参数是要在命令行上使用的传递的名称，用于指定应将传递添加到程序中（例如，使用opt或bugpoint）。 第一个参数是传递的名称，它将用于程序的 [`-help`](https://llvm.org/docs/CommandGuide/FileCheck.html#cmdoption-help)输出，以及-debug-pass选项生成的调试输出。

如果您希望传递易于转储，则应实现虚拟打印方法：

#### The print method

```c++
virtual void print(llvm::raw_ostream &O, const Module *M) const;
```

打印方法必须通过“分析”来实现，以便打印分析结果的人类可读版本。 这对于调试分析本身以及其他人来确定分析如何工作非常有用。 使用opt -analyze参数来调用此方法。

llvm :: raw_ostream参数指定要在其上写入结果的流，Module参数提供指向已分析程序的顶级模块的指针。 但请注意，在某些情况下（例如从调试器调用Pass :: dump（）），此指针可能为NULL，因此它只应用于增强调试输出，不应该依赖它。

### Specifying interactions between passes

PassManager的主要职责之一是确保传递正确地相互交互。 因为PassManager试图 [optimize the execution of passes](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-passmanager)，所以它必须知道传递如何相互交互以及各种传递之间存在哪些依赖关系。 为了跟踪这一点，每次传递都可以声明在当前传递之前需要执行的传递集，以及当前传递失效的传递。

通常，此功能用于要求在运行传递之前计算分析结果。 运行任意转换过程可以使计算的分析结果无效，这是失效集指定的。 如果传递未实现 [getAnalysisUsage](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-getanalysisusage)方法，则默认为没有任何先决条件传递，并使所有其他传递无效。

#### The getAnalysisUsage metho

```c++
virtual void getAnalysisUsage(AnalysisUsage &Info) const;
```

通过实现getAnalysisUsage方法，可以为转换指定必需和无效的集合。 实现应该在 [AnalysisUsage](http://llvm.org/doxygen/classllvm_1_1AnalysisUsage.html)对象中填写有关哪些传递是必需的而不是无效的信息。 要执行此操作，传递可以在AnalysisUsage对象上调用以下任何方法：

#### The AnalysisUsage::addRequired<> and AnalysisUsage::addRequiredTransitive<> methods

如果你的传球需要执行前一个传球（例如分析），它可以使用这些方法之一安排在传球之前运行。 LLVM有许多不同类型的分析和传递，可以满足从DominatorSet到BreakCriticalEdges的范围。 例如，要求BreakCriticalEdges保证在运行pass时CFG中没有关键边缘。

一些分析[chain](https://llvm.org/docs/AliasAnalysis.html#aliasanalysis-chaining) 到其他分析来完成它们的工作。 例如，需要`AliasAnalysis <AliasAnalysis>`实现链接到其他别名分析过程。 在分析链的情况下，应使用addRequiredTransitive方法而不是addRequired方法。 这通知PassManager，只要需要通过，传递所需的传递应该是活动的。

#### The AnalysisUsage::addPreserved<> method

PassManager的一项工作是优化分析的运行方式和时间。特别是，它试图避免重新计算数据，除非它需要。出于这个原因，允许传递声明它们保留（即，它们不会使现有分析无效），如果它可用的话。例如，简单的常量折叠传递不会修改CFG，因此它不可能影响支配者分析的结果。默认情况下，假定所有传递都使所有其他传递无效。

AnalysisUsage类提供了几种在与addPreserved相关的特定情况下有用的方法。特别是，可以调用setPreservesAll方法来指示传递根本不修改LLVM程序（对于分析也是如此），并且setPreservesCFG方法可以由更改程序中的指令但不修改CFG或终止符指令（请注意，此属性是为[BasicBlockPass](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-basicblockpass)隐式设置的）。

addPreserved对于像BreakCriticalEdges这样的转换特别有用。这个过程知道如何更新一小组循环和支配者相关的分析（如果它们存在），所以它可以保留它们，尽管它会破坏CFG。

#### Example implementations of getAnalysisUsage

```c++
// This example modifies the program, but does not modify the CFG
void LICM::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesCFG();
  AU.addRequired<LoopInfoWrapperPass>();
}
```

#### The getAnalysis<> and getAnalysisIfAvailable<> methods

Pass :: getAnalysis <>方法由您的类自动继承，使您可以访问使用 [getAnalysisUsage](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-getanalysisusage)方法声明的通道。 它需要一个模板参数来指定所需的传递类，并返回对该传递的引用。 例如：

class you want, and returns a reference to that pass. For example:

```c++
bool LICM::runOnFunction(Function &F) {
  LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  //...
}
```

此方法调用返回对所需传递的引用。 如果您尝试获取未在[getAnalysisUsage](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-getanalysisusage)实现中声明的分析，则可能会导致运行时断言失败。 此方法可以由run *方法实现调用，也可以由run *方法调用的任何其他本地方法调用。

模块级别传递可以使用此接口使用功能级别分析信息。 例如：

```c++
bool ModuleLevelPass::runOnModule(Module &M) {
  //...
  DominatorTree &DT = getAnalysis<DominatorTree>(Func);
  //...
}
```

在上面的示例中，在返回对所需传递的引用之前，传递管理器调用runOnFunction for DominatorTree。

如果您的传递能够更新分析（如上所述），则可以使用getAnalysisIfAvailable方法，该方法返回指向分析的指针（如果它处于活动状态）。 例如：

```
if (DominatorSet *DS = getAnalysisIfAvailable<DominatorSet>()) {
  // A DominatorSet is active.  This code will update it.
}
```

### Implementing Analysis Groups

现在我们已经了解了如何定义传球，如何使用它们以及如何从其他传球中获得传球的基础知识，现在是时候让它变得更有趣了。到目前为止，我们看到的所有传递关系都非常简单：一次传递依赖于另一个特定传递，在它可以运行之前运行。对于许多应用来说，这很好，对于其他应用，需要更大的灵活性。

特别是，定义了一些分析，使得分析结果只有一个简单的界面，但有多种计算方法。例如，考虑别名分析。最琐碎的别名分析为任何别名查询返回“may alias”。最复杂的分析是流量敏感的，上下文敏感的过程间分析，可能需要花费大量的时间来执行（显然，这两个极端之间有很大空间用于其他实现）。为了干净地支持这种情况，LLVM Pass Infrastructure支持分析组的概念。

#### Analysis Group Concepts

Analysis Group是一个简单的界面，可以通过多个不同的传递来实现。分析组可以像传球一样被赋予人类可读的名称，但与传递不同，它们不需要从Pass类派生。分析组可以具有一个或多个实现，其中之一是“默认”实现。

客户端传递使用分析组，就像其他传递一样：AnalysisUsage :: addRequired（）和Pass :: getAnalysis（）方法。为了解决此要求， [PassManager](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-passmanager) 会扫描可用的传递以查看是否有任何分析组的实现可用。如果没有，则为要使用的传递创建默认实现。 [interaction between passes](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-interaction) 的所有标准规则仍然适用。

虽然 [Pass Registration](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-registration)对于正常传递是可选的，但是必须注册所有分析组实现，并且必须使用 [INITIALIZE_AG_PASS](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-registeranalysisgroup)模板来加入实现池。此外，必须使用 [RegisterAnalysisGroup](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-registeranalysisgroup)注册接口的默认实现。

作为分析组的具体实例，请考虑 [AliasAnalysis](http://llvm.org/doxygen/classllvm_1_1AliasAnalysis.html)分析组。别名分析界面（ [basicaa](http://llvm.org/doxygen/structBasicAliasAnalysis.html) pass）的默认实现只做了一些简单的检查，不需要进行大量的计算分析（例如：两个不同的全局变量永远不能互为别名等）。使用 [AliasAnalysis](http://llvm.org/doxygen/classllvm_1_1AliasAnalysis.html)接口的传递（例如 [gvn](http://llvm.org/doxygen/classllvm_1_1GVN.html)）不关心实际提供别名分析的实现，它们只使用指定的接口。

从用户的角度来看，命令就像正常一样工作。发出命令opt -gvn ...将导致basicaa类被实例化并添加到传递序列中。发出命令opt -somefancyaa -gvn ...将导致gvn传递使用somefancyaa别名分析（实际上并不存在，它只是一个假设的例子）。

#### Using RegisterAnalysisGroup

RegisterAnalysisGroup模板用于注册分析组本身，而INITIALIZE_AG_PASS用于将传递实现添加到分析组。 首先，应该注册一个分析组，并为其提供一个人类可读的名称。 与传递注册不同，没有为分析组接口本身指定命令行参数，因为它是“抽象的”：

```C++
static RegisterAnalysisGroup<AliasAnalysis> A("Alias Analysis");
```

注册分析后，pass可以使用以下代码声明它们是接口的有效实现：

```c++
namespace {
  // Declare that we implement the AliasAnalysis interface
  INITIALIZE_AG_PASS(FancyAA, AliasAnalysis , "somefancyaa",
      "A more complex alias analysis implementation",
      false,  // Is CFG Only?
      true,   // Is Analysis?
      false); // Is default Analysis Group implementation?
}
```

这只显示了一个类FancyAA，它使用INITIALIZE_AG_PASS宏来注册和“加入”AliasAnalysis分析组。 分析组的每个实现都应该使用此宏加入。

```c++
namespace {
  // Declare that we implement the AliasAnalysis interface
  INITIALIZE_AG_PASS(BasicAA, AliasAnalysis, "basicaa",
      "Basic Alias Analysis (default AA impl)",
      false, // Is CFG Only?
      true,  // Is Analysis?
      true); // Is default Analysis Group implementation?
}
```

这里我们展示如何指定默认实现（使用INITIALIZE_AG_PASS模板的最后一个参数）。 对于要使用的分析组，必须始终只有一个默认实现可用。 只有默认实现可以从ImmutablePass派生。 这里我们声明BasicAliasAnalysis传递是接口的默认实现。

## Pass Statistics

 [Statistic](http://llvm.org/doxygen/Statistic_8h_source.html)类旨在通过传递方式公开各种成功指标。 在命令行上启用[`-stats`](https://llvm.org/docs/CommandGuide/lli.html#cmdoption-stats)命令行选项时，将在运行结束时打印这些统计信息。 有关详细信息，请参阅“程序员手册”中的“统计”部分。

### What PassManager does

 [PassManager](http://llvm.org/doxygen/PassManager_8h_source.html) 类获取传递列表，确保正确设置其[prerequisites](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-interaction) ，然后调度传递以高效运行。所有运行传递的LLVM工具都使用PassManager来执行这些传递。

PassManager做了两件主要的事情来尝试减少一系列传递的执行时间：

- 分享分析结果。 PassManager尝试尽可能避免重新计算分析结果。这意味着要跟踪哪些分析已经可用，哪些分析失效，以及需要为通过运行哪些分析。工作的一个重要部分是PassManager跟踪所有分析结果的确切生命周期，允许它在不再需要时释放分配给保存分析结果的内存 [free memory](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-releasememory)。

- 管道程序执行传递。 PassManager尝试通过将传递流水线化，从而在一系列传递中获得更好的缓存和内存使用行为。这意味着，给定一系列连续的[FunctionPass](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-functionpass)，它将执行第一个函数上的所有[FunctionPass](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-functionpass)，然后执行第二个函数上的所有[FunctionPass](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-functionpass)等等，直到整个程序运行完遍。

  这改进了编译器的缓存行为，因为它一次只触及单个函数的LLVM程序表示，而不是遍历整个程序。它减少了编译器的内存消耗，因为，例如，一次只需要计算一个 [DominatorSet](http://llvm.org/doxygen/classllvm_1_1DominatorSet.html) 。这也使得将来可以实现一些 [interesting enhancements](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-smp) 功能。

PassManager的有效性直接受到它所调度的传递行为的信息量的影响。例如，面对未实现的[getAnalysisUsage](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-getanalysisusage)方法，“保留”集合是故意保守的。如果不实施它将不会允许任何分析结果贯穿执行传递。

PassManager类公开了一个--debug-pass命令行选项，这些选项对于调试传递执行，查看工作原理以及诊断何时应该保留比当前更多的分析非常有用。 （要获取有关--debug-pass选项的所有变体的信息，只需键入“opt -help-hidden”）。

例如，通过使用-debug-pass = Structure选项，我们可以看到 [Hello World](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-basiccode)传递如何与其他传递进行交互。让我们尝试使用gvn和licm传递：

```
$ opt -load lib/LLVMHello.so -gvn -licm --debug-pass=Structure < hello.bc > /dev/null
ModulePass Manager
  FunctionPass Manager
    Dominator Tree Construction
    Basic Alias Analysis (stateless AA impl)
    Function Alias Analysis Results
    Memory Dependence Analysis
    Global Value Numbering
    Natural Loop Information
    Canonicalize natural loops
    Loop-Closed SSA Form Pass
    Basic Alias Analysis (stateless AA impl)
    Function Alias Analysis Results
    Scalar Evolution Analysis
    Loop Pass Manager
      Loop Invariant Code Motion
    Module Verifier
  Bitcode Writer
```

此输出显示构建通道的时间。 在这里，我们看到GVN使用支配树信息来完成它的工作。 LICM传递使用自然循环信息，它也使用支配树。

在LICM传递之后，模块验证程序运行（由opt工具自动添加），它使用支配树来检查生成的LLVM代码是否格式正确。 注意，支配树计算一次，并由三遍共享。

让我们看看当我们在两次传递之间运行 [Hello World](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-basiccode)传递时这会如何变化：

```
$ opt -load lib/LLVMHello.so -gvn -hello -licm --debug-pass=Structure < hello.bc > /dev/null
ModulePass Manager
  FunctionPass Manager
    Dominator Tree Construction
    Basic Alias Analysis (stateless AA impl)
    Function Alias Analysis Results
    Memory Dependence Analysis
    Global Value Numbering
    Hello World Pass
    Dominator Tree Construction
    Natural Loop Information
    Canonicalize natural loops
    Loop-Closed SSA Form Pass
    Basic Alias Analysis (stateless AA impl)
    Function Alias Analysis Results
    Scalar Evolution Analysis
    Loop Pass Manager
      Loop Invariant Code Motion
    Module Verifier
  Bitcode Writer
Hello: __main
Hello: puts
Hello: main
```

在这里，我们看到 [Hello World](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-basiccode)传递已经杀死了Dominator Tree传递，即使它根本不修改代码！ 要解决此问题，我们需要在pass中添加以下 [getAnalysisUsage](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-getanalysisusage)方法：

```
// We don't modify the program, so we preserve all analyses
void getAnalysisUsage(AnalysisUsage &AU) const override {
  AU.setPreservesAll();
}
```

现在当我们运行pass时，我们得到这个输出：

```
$ opt -load lib/LLVMHello.so -gvn -hello -licm --debug-pass=Structure < hello.bc > /dev/null
Pass Arguments:  -gvn -hello -licm
ModulePass Manager
  FunctionPass Manager
    Dominator Tree Construction
    Basic Alias Analysis (stateless AA impl)
    Function Alias Analysis Results
    Memory Dependence Analysis
    Global Value Numbering
    Hello World Pass
    Natural Loop Information
    Canonicalize natural loops
    Loop-Closed SSA Form Pass
    Basic Alias Analysis (stateless AA impl)
    Function Alias Analysis Results
    Scalar Evolution Analysis
    Loop Pass Manager
      Loop Invariant Code Motion
    Module Verifier
  Bitcode Writer
Hello: __main
Hello: puts
Hello: main
```

这表明我们不会意外地使支配者信息无效，因此不必计算两次。

#### The releaseMemory method

```c++
virtual void releaseMemory();
```

PassManager会自动确定何时计算分析结果，以及保留它们的时间。 因为传递对象本身的生命周期实际上是编译过程的整个持续时间，所以当它们不再有用时，我们需要一些方法来释放分析结果。 releaseMemory虚拟方法是执行此操作的方法。

如果您正在编写分析或任何其他保留大量状态的传递（供另一个“需要”传递并使用[getAnalysis](https://llvm.org/docs/WritingAnLLVMPass.html#writing-an-llvm-pass-getanalysis) 方法的pass），您应该实现releaseMemory，以便释放分配的内存来维护 内部状态。 在传递中下一次调用run *之前，在类的run *方法之后调用此方法。

## Registering dynamically loaded passes

在使用LLVM构建生产质量工具时，大小很重要，既用于分发，也用于在目标系统上运行时调整驻留代码大小。因此，期望选择性地使用一些通道，同时省略其他通道并保持稍后改变配置的灵活性。您希望能够完成所有这些，并向用户提供反馈。这是通行证登记发挥作用的地方。

传递注册的基本机制是MachinePassRegistry类和MachinePassRegistryNode的子类。

MachinePassRegistry的实例用于维护MachinePassRegistryNode对象的列表。此实例维护列表并向命令行界面传达添加和删除。

MachinePassRegistryNode子类的实例用于维护有关特定传递的信息。此信息包括命令行名称，命令帮助字符串以及用于创建传递实例的函数的地址。其中一个实例的全局静态构造函数向对应的MachinePassRegistry注册，静态析构函数取消注册。因此，在工具中静态链接的传递将在启动时注册。动态加载的传递将在加载时注册，并在卸载时注销。

### Using existing registries

有预定义的注册表来跟踪指令调度（RegisterScheduler）和寄存器分配（RegisterRegAlloc）机器通过。 这里我们将描述如何注册寄存器分配器机器通过。

实现你的寄存器分配器机器通行证。 在您的register allocator .cpp文件中添加以下include：

```
#include "llvm/CodeGen/RegAllocRegistry.h"
```

同样在您的register allocator .cpp文件中，以下列形式定义创建者函数：

```
FunctionPass *createMyRegisterAllocator() {
  return new MyRegisterAllocator();
}
```

请注意，此函数的签名应与RegisterRegAlloc :: FunctionPassCtor的类型匹配。 在同一个文件中添加“安装”声明，格式如下：

```
static RegisterRegAlloc myRegAlloc("myregalloc",
                                   "my register allocator help string",
                                   createMyRegisterAllocator);
```

请注意，帮助字符串之前的两个空格在 [`-help`](https://llvm.org/docs/CommandGuide/FileCheck.html#cmdoption-help)查询上产生了整洁的结果。

```
$ llc -help
  ...
  -regalloc                    - Register allocator to use (default=linearscan)
    =linearscan                -   linear scan register allocator
    =local                     -   local register allocator
    =simple                    -   simple register allocator
    =myregalloc                -   my register allocator help string
  ...
```

就是这样。 用户现在可以自由使用-regalloc = myregalloc作为选项。 除了使用RegisterScheduler类之外，注册指令调度程序是类似的。 请注意，RegisterScheduler :: FunctionPassCtor与RegisterRegAlloc :: FunctionPassCtor明显不同。

要强制将寄存器分配器加载/链接到llc / lli工具，请将创建者函数的全局声明添加到Passes.h，并将“伪”调用行添加到llvm / Codegen / LinkAllCodegenComponents.h。

### Creating new registries

最简单的入门方法是克隆一个现有的注册表; 我们推荐llvm / CodeGen / RegAllocRegistry.h。 要修改的关键是类名和FunctionPassCtor类型。

然后你需要声明注册表。 示例：如果您的pass注册表是RegisterMyPasses，则定义：

```c++
MachinePassRegistry RegisterMyPasses::Registry;
```

最后，为你的传递声明命令行选项。 例：

```
cl::opt<RegisterMyPasses::FunctionPassCtor, false,
        RegisterPassParser<RegisterMyPasses> >
MyPassOpt("mypass",
          cl::init(&createDefaultMyPass),
          cl::desc("my pass option help"));
```

这里的命令选项是“mypass”，createDefaultMyPass是默认创建者。

### Using GDB with dynamically loaded passes

不幸的是，使用带有动态加载传递的GDB并不像应该的那样容易。 首先，您不能在尚未加载的共享对象中设置断点，其次在共享对象中存在内联函数的问题。 以下是使用GDB调试传递的一些建议。

为了便于讨论，我将假设您正在调试由opt调用的转换，尽管此处描述的内容不依赖于此。

#### Setting a breakpoint in your pass

你要做的第一件事就是在opt过程中启动gdb：

```
gdb opt
GNU gdb 5.0
Copyright 2000 Free Software Foundation, Inc.
GDB is free software, covered by the GNU General Public License, and you are
welcome to change it and/or distribute copies of it under certain conditions.
Type "show copying" to see the conditions.
There is absolutely no warranty for GDB.  Type "show warranty" for details.
This GDB was configured as "sparc-sun-solaris2.6"...
(gdb)
```

请注意，opt中包含大量调试信息，因此加载需要时间。 耐心点。 由于我们无法在传递中设置断点（共享对象直到运行时才加载），我们必须执行该过程，并在它调用我们的传递之前停止它，但是在它加载了共享对象之后。 最简单的方法是在PassManager :: run中设置断点，然后使用您想要的参数运行该进程：

```
$ (gdb) break llvm::PassManager::run
Breakpoint 1 at 0x2413bc: file Pass.cpp, line 70.
(gdb) run test.bc -load $(LLVMTOP)/llvm/Debug+Asserts/lib/[libname].so -[passoption]
Starting program: opt test.bc -load $(LLVMTOP)/llvm/Debug+Asserts/lib/[libname].so -[passoption]
Breakpoint 1, PassManager::run (this=0xffbef174, M=@0x70b298) at Pass.cpp:70
70      bool PassManager::run(Module &M) { return PM->run(M); }
(gdb)
```

一旦opt在PassManager :: run方法中停止，您现在可以在传递中自由设置断点，以便您可以跟踪执行或执行其他标准调试。

#### Miscellaneous Problems

一旦掌握了基础知识，GDB就会遇到一些问题，一些是解决方案，一些是没有解决方案。

内联函数具有伪造的堆栈信息。通常，GDB在获取堆栈跟踪和单步执行内联函数方面做得非常好。但是，当动态加载传递时，它会以某种方式完全失去此功能。我所知道的唯一解决方案是取消内联函数（将其从类的主体移动到.cpp文件）。
重新启动程序会破坏断点。按照上述信息后，您已成功获得通行证中的一些断点。接下来你知道，你重新启动程序（即你再次键入“run”），然后开始得到关于断点不可设置的错误。我发现“修复”此问题的唯一方法是删除已在传递中设置的断点，运行程序，并在PassManager :: run中执行停止后重新设置断点。
希望这些技巧可以帮助解决常见的案例调试问题。如果您想提供自己的一些提示，请联系 [Chris](mailto:sabre%40nondot.org)。

### Future extensions planned

尽管LLVM Pass基础设施非常强大，并且做了一些漂亮的东西，但我们希望将来添加一些东西。 这是我们要去的地方：

#### Multithreaded LLVM

多CPU机器变得越来越普遍，编译永远不够快：显然我们应该允许多线程编译器。 由于为上面的传递定义了语义（特别是它们不能在其run *方法的调用中维护状态），实现多线程编译器的一种很好的干净方法是让PassManager类创建每个传递对象的多个实例，并允许 单独的实例同时攻击程序的不同部分。

此实现将阻止每个传递必须实现多线程构造，只需要LLVM核心在几个位置（对于全局资源）具有锁定。 虽然这是一个简单的扩展，但我们根本没有时间（或多处理器机器，因此有理由）来实现它。 尽管如此，我们仍然保持LLVM准备好SMP，你也应该这样做。