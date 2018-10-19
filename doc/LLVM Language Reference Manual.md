# LLVM Language Reference Manual

[原文链接](http://llvm.org/docs/LangRef.html)

目录生成指令:

./gh-md-toc /home/cyoung/CLionProjects/implement_llvm/doc/LLVM\ Language\ Reference\ Manual.md 

[TOC]

## Abstract

本文档是LLVM汇编语言的参考手册。 LLVM是一种基于静态单一分配（SSA）的表示，它提供类型安全，低层操作，灵活性以及能够干净地表示“所有”高级语言的能力。 它是LLVM编译策略的所有阶段中使用的公共代码表示。

## Introduction

LLVM代码表示被设计为以三种不同的形式使用：内存中编译IR,磁盘上bitcode表示（适合于由即时编译(JIT)快速加载），以及作为人类可读组件语言表征。这允许LLVM为高效的编译转换和分析提供强大的中间表示，同时自然提供调试和可视化转换。三种不同形式的LLVM都是等价的。本文档描述了人类可读的表示和符号。

LLVM表示旨在轻量级和底层的，同时具有表答能力，类型和可扩展性。它的目标是成为一种“通用IR”，通过处于足够底层，高层的ideas可以干净地映射到它（类似于微处理器是“通用IR”的方式，允许许多源语言映射到它们）。通过提供类型信息，LLVM可以用作目标优化：例如，通过指针分析，可以证明永远不会在当前函数之外访问C自动变量，从而可以将其升级为简单的SSA值而不是内存本地变量。

## Well-Formedness

值得注意的是，本文档描述了“格式优良”的LLVM汇编语言。 解析器接受的内容与“格式优良”的内容之间存在一定差异。 例如，以下指令在语法上是可以的，但没有很好地格式：

```
%x = add i32 1, %x
```

因为`％x`的定义并未占据其所有用途。 LLVM基础结构提供了一个验证pass，可用于验证LLVM模块是否格式正确。 在解析完输入程序之后，并在输出bitcode之前,解析器会自动运行此pass来优化程序。 验证程序Pass会在转换passes中指出的违规错误或将错误输入到解析器。

## Identifiers

LLVM标识符有两种基本类型：全局和本地。 全局标识符（函数，全局变量）以`@`字符开头。 本地标识符（注册名称，类型）以`％`字符开头。 此外，标识符有三种不同的格式，用于不同的目的：

- 命名值表示为带有前缀(前缀为%或@)的字符串。 例如，`％foo`，`@ DivivyByZero`，`％a.really.long.identifier`。 使用的实际正则表达式是`[%@][-a-zA-Z$._][-a-zA-Z$._0-9]`。 在名称中需要其他字符的标识符可以用引号括起来。 可以使用`\ xx`转义特殊字符，其中`xx`是十六进制字符的ASCII代码。 这样，任何字符都可以在名称值中使用，甚至可以引用它们自己。 `\ 01`前缀可用于全局值以抑制重整。
- 未命名的值表示为带有前缀的无符号数值。 例如，`％12`，`@ 2`，`％44`。
- 常量，在下面的常量部分中描述。

LLVM要求值以前缀开头有两个原因：编译器不需要担心与保留字的名称冲突，并且可以在将来扩展保留字集而不会受到干扰。 此外，未命名的标识符允许编译器快速给出临时变量，而无需担心避免符号表冲突的问题。

LLVM中的保留字与其他语言中的保留字非常相似。 对于不同的操作码（'add'，'bitcast'，'ret'等等），有关键字用于原始类型名称（'void'，'i32'等等）等等。 这些保留字不会与变量名冲突，因为它们都不以前缀字符（'％'或'@'）开头。

以下是将整数变量`％X`乘以8的LLVM代码示例：

简单的方式：

```
%result = mul i32 %X, 8
```

简化(%x的3次方)后：

```
%result = shl i32 %X, 3
```

而笨办法(8个%x相加)：

```
%0 = add i32 %X, %X           ; yields i32:%0
%1 = add i32 %0, %0           ; yields i32:%1
%result = add i32 %1, %1
```

`%0`...为未命名值

将`％X`乘以8的最后一种方法说明了LLVM的几个重要词法特征：

- 注释用';'分隔，直到行尾。
- 如果计算结果未分配给命名值，则会创建未命名的临时值。
- 未命名的临时值按顺序编号（使用每个函数递增计数器，从0开始）。 请注意，此编号中包含basic blocks和未命名的函数参数。 例如，如果没有为basic blocks指定label名称并且所有函数参数都已命名，则它将获得数字0。

它还显示了我们在本文档中遵循的约定。 在演示说明时，我们将遵循一条带有注释的指令，该注释定义了所生成的值的类型和名称。

## High Level Structure

### Module Structure

LLVM程序由Module组成，每个Module都是输入程序的转换单元。 一个模块由函数，全局变量和符号表组成。 模块可以与LLVM链接器组合在一起。可以用来合并函数（和全局变量）定义，解决前向声明，合并符号表。 以下是“hello world”模块的示例：

```
; Declare the string constant as a global constant.
@.str = private unnamed_addr constant [13 x i8] c"hello world\0A\00"

; External declaration of the puts function
declare i32 @puts(i8* nocapture) nounwind

; Definition of main function
define i32 @main() {   ; i32()*
  ; Convert [13 x i8]* to i8*...
  %cast210 = getelementptr [13 x i8], [13 x i8]* @.str, i64 0, i64 0

  ; Call puts function to write out the string to stdout.
  call i32 @puts(i8* %cast210)
  ret i32 0
}

; Named metadata
!0 = !{i32 42, null, !"string"}
!foo = !{!0}
```

此示例由名为`.str`的全局变量，“puts”函数的外部声明，“main”的[函数定义](http://llvm.org/docs/LangRef.html#functionstructure)和[元数据命名](http://llvm.org/docs/LangRef.html#namedmetadatastructure)“foo”组成。

通常，模块由全局值列表组成（其中函数和全局变量都是全局值）。 全局值由指向内存位置的指针（在本例中,指向char数组的指针与指向函数的指针）表示，并具有下面链接类型(linkage types)中的一个。

### Linkage Types

所有全局变量和函数都具有以下类型的链接之一：

`private`:

- 具有“`private`”链接的全局值只能由当前模块中的对象直接访问。 特别的，将代码链接到具有私有全局值的模块可能会导致私有值被重命名以避免冲突。 由于符号是模块(symbol)私有的，因此可以更新所有引用。 这不会显示在目标文件的任何符号表中。

`internal`:

- 与private类似，但该值在对象文件中显示为本地符号（在ELF的情况下为STB_LOCAL）。 这对应于C中'static'关键字的概念。

`available_externally`:

- 具有“available_externally”链接的全局变量永远不会发送(emitted)到与LLVM模块对应的目标文件中。 从链接器的角度来看，available_externally global等同于外部声明。 它们的存在允许在知道全局定义的情况下进行内联和其他优化，已知该定义位于模块之外的某个位置。 允许随意丢弃具有available_externally链接的全局变量，并允许内联和其他优化。 此链接类型仅允许用在定义上，而不是声明。

`linkonce`:

- 当链接发生时，具有“linkonce”链接的全局变量与其他同名的全局变量合并。 这可以用于实现某些形式的内联函数，模板或其他代码，这些代码必须在使用它的每个转换单元中生成，但是稍后可以用更明确的定义覆盖body。 允许丢弃未引用的linkonce全局变量。 请注意，linkonce链接实际上不允许优化器将此函数的b函数体内联到调用者中，因为它不知道函数的这个定义是否是程序中的确切定义，或者它是否会被更强的定义覆盖。 要启用内联和其他优化，请使用“linkonce_odr”链接。

`weak`:

- “`weak`”链接具有与linkonce链接相同的合并语义，除了可以不丢弃具有弱链接的未引用的全局变量。 这用于在C源代码中声明为“弱”的全局变量。

`common`:

- “common”链接最类似于“弱”链接，但它们用于C中的暂定定义，例如全局范围内的“int X;”。 具有“公共”链接的符号以与弱符号相同的方式合并，如果未引用，则不能删除它们。 公共符号可能没有明确的部分，必须具有零初始化程序，并且可能不会标记为“常量”。 函数和别名可能没有共同的链接。

`appending`:

- “追加”链接只能应用于指向数组类型的指针的全局变量。 当两个具有附加链接的全局变量链接在一起时，两个全局数组将附加在一起。 这是LLVM，类型安全，相当于当.o文件链接时，系统链接器将“部分”附加到具有相同名称的“部分”。

  不幸的是，这与.o文件中的任何功能都不对应，因此它只能用于像llvm.global_ctors这样的变量，这些变量是llvm专门解释的。

`extern_weak`:

- 此链接的语义遵循ELF目标文件模型：符号在链接之前是弱的，如果没有链接，则符号变为null而不是未定义的引用。

`linkonce_odr, weak_odr`:

- linkonce_odr，weak_odr有些语言允许合并不同的全局变量，例如具有不同语义的两个函数。 其他语言（如C ++）确保只合并等效的全局变量（“一个定义规则” - “ODR”）。 这些语言可以使用linkonce_odr和weak_odr链接类型来指示全局只与等效的全局变量合并。 这些链接类型与其非odr版本相同。

`external`:

- externalf没有使用上述标识符，全局是外部可见的，这意味着它参与链接并可用于解析外部符号引用。

函数声明具有除external或extern_weak之外的任何链接类型是非法的。

### Calling Conventions(调用公约)

LLVM [functions](http://llvm.org/docs/LangRef.html#functionstructure)， [calls](http://llvm.org/docs/LangRef.html#i-call)和 [invokes](http://llvm.org/docs/LangRef.html#i-invoke)都可以可以为调用指定可选的调用约定(calling conventions)。 任何一对动态调用者(caller)/被调用者(callee)的调用约定必须匹配，否则程序的行为是未定义的。 LLVM支持以下调用约定，将来可能会添加更多内容：

`“ccc”` - C的调用约定
此调用约定（如果未指定其他调用约定，则为缺省值）与目标C调用约定匹配。 此调用约定支持varargs函数调用，并容忍声明的原型中的一些不匹配并实现函数的声明（正常C）。

`“fastcc”`- 快速召唤的惯例
该调用约定试图尽可能快地进行调用（例如，通过寄存器中的事物）。 此调用约定允许目标使用它想要为目标生成快速代码的任何技巧，而不必遵循外部指定的ABI（应用程序二进制接口）。 [只有在使用GHC或HiPE约定时才能优化尾调用](http://llvm.org/docs/CodeGenerator.html#id80)。 此调用约定不支持varargs，并要求所有callees的原型与函数定义的原型完全匹配。

`“coldcc”` - 冷酷的召唤惯例
该调用约定试图在调用未被普遍执行的假设下使调用者中的代码尽可能高效。因此，这些调用通常会保留所有寄存器，以便调用不会破坏调用方的任何有效范围。此调用约定不支持varargs，并要求所有callees的原型与函数定义的原型完全匹配。此外，内联器不考虑内联的此类函数调用。

`“cc 10” `- GHC会议
此调用约定已专门用于格拉斯哥Haskell编译器( [Glasgow Haskell Compiler (GHC)](http://www.haskell.org/ghc))。它通过寄存器传递所有内容，通过禁用被调用者保存寄存器来达到极限。不应轻易使用此调用约定，但仅适用于特定情况，例如在实现函数式编程语言时常用的寄存器固定性能技术的替代方法。目前只有X86支持此约定，它具有以下限制：

- 在X86-32上，最多只支持4位类型参数。不支持任何浮点类型。
- 在X86-64上，最多只支持10位类型参数和6个浮点参数。

此调用约定支持尾调用优化([tail call optimization](http://llvm.org/docs/CodeGenerator.html#id80))，但要求调用者和被调用者都使用它。

`“cc 11” `- HiPE呼叫惯例
此调用约定专门用于高性能Erlang [High-Performance Erlang (HiPE)](http://www.it.uu.se/research/group/hipe/)编译器，即爱立信开源Erlang / OTP系统( [Ericsson’s Open Source Erlang/OTP system](http://www.erlang.org/download.shtml))的本机代码编译器。它使用了比普通C调用约定更多的寄存器来传递参数，并且没有定义被调用者保存的寄存器。调用约定正确地支持 [tail call optimization](http://llvm.org/docs/CodeGenerator.html#id80)，但要求调用者和被调用者都使用它。它使用寄存器固定机制，类似于GHC的惯例，用于将经常访问的运行时组件固定到特定的硬件寄存器。目前只有X86支持这种约定（32位和64位）。

`“webkit_jscc”` - WebKit的JavaScript调用约定
此调用约定已针对[WebKit FTL JIT](https://trac.webkit.org/wiki/FTLJIT)实现。它从右到左传递堆栈中的参数（如cdecl所做），并在平台的惯用返回寄存器中返回一个值。

`“anyregcc”`- 代码修补的动态调用约定
这是一种特殊约定，支持修补任意代码序列以代替调用站点。此约定强制将调用参数放入寄存器，但允许它们动态分配。这当前只能用于对llvm.experimental.patchpoint的调用，因为只有这个内在函数在边表中记录其参数的位置。请参阅[Stack maps and patch points in LLVM](http://llvm.org/docs/StackMaps.html)。

`“preserve_mostcc”` - PreserveMost调用约定
此调用约定尝试使调用者中的代码尽可能不引人注目。此约定的行为与关于如何传递参数和返回值的C调用约定相同，但它使用一组不同的调用者/被调用者保存的寄存器。这减轻了在呼叫者呼叫之前和之后保存和恢复大寄存器组的负担。如果参数在被调用者保存的寄存器中传递，则调用者将在整个调用中保留它们。这不适用于被调用者保存的寄存器中返回的值。

- 在X86-64上，被调用者保留所有通用寄存器，R11除外。 R11可用作临时寄存器。浮点寄存器(XMM / YMM)不会被保留，需要由调用者保存。

此约定背后的想法是支持对具有热路径和冷路径的运行时函数的调用。热路径通常是一小段代码，不使用许多寄存器。冷路径可能需要调用另一个函数，因此只需要保留调用者保存的寄存器，这些寄存器尚未被调用者保存。在调用者/被调用者保存的寄存器方面，PreserveMost调用约定与冷调用约定非常相似，但它们用于不同类型的函数调用。 coldcc用于很少执行的函数调用，而preserve_mostcc函数调用则用于热路径并且肯定执行很多。此外，preserve_mostcc不会阻止内联器内联函数调用。

此调用约定将由ObjectiveC运行时的未来版本使用，因此此时仍应被视为实验。尽管创建此约定是为了优化对ObjectiveC运行时的某些运行时调用，但它不仅限于此运行时，并且将来也可能被其他运行时使用。当前的实现仅支持X86-64，但目的是在未来支持更多的体系结构。

`“preserve_allcc”` - PreserveAll调用约定
	此调用约定试图使调用者中的代码比PreserveMost调用约定更少侵入。此调用约定的行为与关于如何传递参数和返回值的C调用约定相同，但它使用一组不同的调用者/被调用者保存的寄存器。这消除了在调用者中调用之前和之后保存和恢复大型寄存器集的负担。如果参数在被调用者保存的寄存器中传递，则调用者将在整个调用中保留它们。这不适用于被调用者保存的寄存器中返回的值。

- 在X86-64上，被调用者保留所有通用寄存器，R11除外。 R11可用作临时寄存器。此外，它还保留所有浮点寄存器（XMM / YMM）。

此约定背后的想法是支持对不需要调用任何其他函数的运行时函数的调用。

此调用约定与PreserveMost调用约定一样，将由ObjectiveC运行时的未来版本使用，此时应被视为实验。

`“cxx_fast_tlscc”` - 访问函数的CXX_FAST_TLS调用约定
	Clang生成一个访问函数来访问C ++风格的TLS。访问功能通常具有入口块，出口块和第一次运行的初始化块。进入和退出块可以访问一些TLS IR变量，每个访问将降低到特定于平台的序列。

此调用约定旨在通过保留尽可能多的寄存器来最小化调用者的开销（所有在快速路径上保持的寄存器，由入口和出口块组成）。

此调用约定的行为与关于如何传递参数和返回值的C调用约定相同，但它使用一组不同的调用者/被调用者保存的寄存器。

鉴于每个平台都有自己的降序，因此它自己的保留寄存器集，我们不能使用现有的PreserveMost。

- 在X86-64上，被调用者保留所有通用寄存器，RDI和RAX除外。

`“swiftcc”` - 这个调用约定用于Swift语言。
在X86-64上，RCX和R8可用于其他整数返回，XMM2和XMM3可用于额外的FP /向量返回。
在iOS平台上，我们使用AAPCS-VFP调用约定。

`“cc <n>”` - 编号惯例
	任何调用约定都可以通过数字指定，允许使用特定于目标的调用约定。 目标特定的调用约定从64开始。

可以根据需要添加/定义更多调用约定，以支持Pascal约定或任何其他众所周知的与目标无关的约定。

### Visibility Styles(可见样式)

所有全局变量和函数都具有以下可见性样式之一：

`“default”` - 默认样式
	在使用ELF目标文件格式的目标上，默认可见性意味着该声明对其他模块可见，并且在共享库中，意味着可以覆盖声明的实体。在Darwin上，默认可见性意味着声明对其他模块可见。默认可见性对应于语言中的“外部链接”。
`“hidden” `- 隐藏的风格
	具有隐藏可见性的对象的两个声明如果它们位于同一共享对象中，则引用相同的对象。通常，隐藏的可见性表示该符号不会放入动态符号表中，因此其他模块（可执行文件或共享库）不能直接引用它。
`“protected”`- 受保护的风格
	在ELF上，受保护的可见性表示符号将放置在动态符号表中，但定义模块中的引用将绑定到本地符号。也就是说，该符号不能被另一个模块覆盖。

具有内部或专用链接的符号必须具有默认可见性。

### DLL Storage Classes(dll存储类)

所有全局变量，函数和别名都可以具有以下DLL存储类之一：

`dllimport`
`dllimport`使编译器通过指向由导出符号的DLL设置的指针的全局指针来引用函数或变量。 在Microsoft Windows目标上，指针名称是通过组合__imp_和函数或变量名称形成的。

`dllexport`
`dllexport`使编译器提供指向DLL中指针的全局指针，以便可以使用dllimport属性引用它。 在Microsoft Windows目标上，指针名称是通过组合__imp_和函数或变量名称形成的。 由于此存储类用于定义dll接口，因此编译器，汇编器和链接器知道它是外部引用的，并且必须避免删除该符号。

### Thread Local Storage Models

变量可以定义为`thread_local`，这意味着它不会被线程共享（每个线程都有一个独立的变量副本）。并非所有目标都支持线程局部变量。可选地，可以指定TLS模型：

`localdynamic`
	对于仅在当前共享库中使用的变量。
`initialexec`
	对于不会动态加载的模块中的变量。
`localexec`
	对于可执行文件中定义的变量，仅在其中使用。

如果没有给出明确的模型，则使用“一般动态”模型。

模型对应于[ELF Handling For Thread-Local Storage](http://people.redhat.com/drepper/tls.pdf);有关在何种情况下可以使用不同模型的详细信息，请参阅线程局部存储的ELF处理。如果不支持指定的模型，或者可以更好地选择模型，目标可以选择不同的TLS模型。

也可以在别名中指定模型，但它仅控制别名的访问方式。它对别名没有任何影响。

对于没有链接器支持ELF TLS模型的平台，-femulated-tls标志可用于生成GCC兼容的模拟TLS代码。

### Runtime Preemption Specifiers

全局变量，函数和别名可以具有可选的运行时抢占说明符。 如果未明确给出抢占说明符，则假定符号为`dso_preemptable`。

`dso_preemptable`
指示在运行时可以用链接单元外部的符号替换函数或变量。
`dso_local`
编译器可以假设标记为dso_local的函数或变量将解析为同一链接单元内的符号。 即使定义不在此编译单元内，也将生成直接访问。

### Structure Types

LLVM IR允许您指定“identified”和“文字” [structure types](http://llvm.org/docs/LangRef.html#t-struct)。 文字类型在结构上是唯一的，但识别的类型从未是唯一的。 不透明结构类型( [opaque structural type](http://llvm.org/docs/LangRef.html#t-opaque))也可用于转发声明尚未可用的类型。

已识别的结构规范的示例是：

```
%mytype = type { %mytype*, i32 }
```

在LLVM 3.0发布之前，已识别的类型在结构上是独一无二的。 在最近的LLVM版本中，只有文字类型是唯一的。

### Non-Integral Pointer Type

注意：非整数指针类型是一项正在进行的工作，此时它们应被视为实验性的。

LLVM IR可选地允许前端通过 [datalayout string](http://llvm.org/docs/LangRef.html#langref-datalayout)将某些地址空间中的指针表示为“非整数”。 非整数指针类型表示具有未指定的按位表示的指针; 也就是说，积分表示可以是目标依赖的或不稳定的（不受固定整数的支持）。

将整数转换为非整数指针类型的inttoptr指令是错误类型的，并且将非整数指针类型的值转换为整数的ptrtoint指令也是如此。 所述指令的矢量版本也是错误的。

### Global Variables

全局变量定义在编译时分配的内存区域而不是运行时。

必须初始化全局变量定义。

也可以声明其他翻译单元中的全局变量，在这种情况下，它们没有初始化程序。

全局变量定义或声明可以具有要放置的显式部分，并且可以指定可选的显式对齐。如果变量声明的显式或推断的部分信息与其定义之间存在不匹配，则结果行为未定义。

变量可以定义为全局常量，表示永远不会修改变量的内容（启用更好的优化，允许将全局数据放在可执行文件的只读部分中等）。请注意，需要运行时初始化的变量不能标记为常量，因为存储了变量。

LLVM明确允许将全局变量的声明标记为常量，即使全局变量的最终定义不是。此功能可用于稍微更好地优化程序，但需要语言定义以保证基于“常量”的优化对于不包含定义的转换单元有效。

作为SSA值，全局变量定义范围内的指针值（即它们支配）程序中的所有基本块。全局变量总是定义指向其“内容”类型的指针，因为它们描述了一个内存区域，LLVM中的所有内存对象都是通过指针访问的。

全局变量可以用unnamed_addr标记，表示地址不重要，只有内容。如果它们具有相同的初始值，则可以将标记为这样的常量与其他常量合并。请注意，具有重要地址的常量可以与unnamed_addr常量合并，结果是其地址重要的常量。

如果给出local_unnamed_addr属性，则知道该模块中的地址不重要。

可以声明全局变量驻留在特定于目标的编号地址空间中。对于支持它们的目标，地址空间可能会影响优化的执行方式和/或用于访问变量的目标指令。默认地址空间为零。地址空间限定符必须位于任何其他属性之前。

LLVM允许为全局变量指定显式部分。如果目标支持它，它将向指定的部分发出全局变量。此外，如果目标具有必要的支持，则全局可以放置在comdat中。

外部声明可以指定明确的部分。对于使用此信息的目标，部分信息保留在LLVM IR中。将节信息附加到外部声明是断言其定义位于指定的节中。如果定义位于不同的部分，则行为未定义。

默认情况下，通过假设模块中定义的全局变量未在全局初始化程序启动之前从其初始值进行修改来优化全局初始值设定项。即使对于可能从模块外部访问的变量也是如此，包括具有外部链接或出现在@ llvm.used或dllexported变量中的变量。可以通过使用external_initialized标记变量来抑制此假设。

可以为全局指定显式对齐，其必须是2的幂。如果不存在，或者如果对齐设置为零，则全局的对齐由目标设置为任何方便的方式。如果指定了显式对齐，则强制全局具有该对齐。如果全局具有指定的部分，则不允许目标和优化器过度对齐全局。在这种情况下，额外的对齐可以是可观察的：例如，代码可以假设全局变量在其部分中密集地打包并尝试作为数组迭代它们，对齐填充将破坏此迭代。最大对齐为1 << 29。

Globals还可以有一个[DLL storage class](http://llvm.org/docs/LangRef.html#dllstorageclass)，一个可选的 [runtime preemption specifier](http://llvm.org/docs/LangRef.html#runtime-preemption-model)，一个可选的 [global attributes](http://llvm.org/docs/LangRef.html#glattrs) 和一个可选的附加[metadata](http://llvm.org/docs/LangRef.html#metadata)列表。

变量和别名可以具有 [Thread Local Storage Model](http://llvm.org/docs/LangRef.html#tls-model)。

句法：

```
@<GlobalVarName> = [Linkage] [PreemptionSpecifier] [Visibility]
                   [DLLStorageClass] [ThreadLocal]
                   [(unnamed_addr|local_unnamed_addr)] [AddrSpace]
                   [ExternallyInitialized]
                   <global | constant> <Type> [<InitializerConstant>]
                   [, section "name"] [, comdat [($name)]]
                   [, align <Alignment>] (, !name !N)*
```

例如，以下定义了带有初始值设定项，节和对齐的编号地址空间中的全局：

```
@G = addrspace(5) constant float 1.0, section "foo", align 4
```

以下示例仅声明一个全局变量:

```
@G = external global i32
```

以下示例使用initialexec TLS模型定义线程局部全局：

```
@G = thread_local(initialexec) global i32 0, align 4
```

### Functions

LLVM函数定义包括“define”关键字，可选的链接类型([linkage type](http://llvm.org/docs/LangRef.html#linkage))，可选的运行时抢占说明符([runtime preemption specifier](http://llvm.org/docs/LangRef.html#runtime-preemption-model))，可选的可见性样式( [visibility style](http://llvm.org/docs/BitCodeFormat.html#visibility))，可选的 [DLL storage class](http://llvm.org/docs/LangRef.html#dllstorageclass)，可选的 [calling convention](http://llvm.org/docs/LangRef.html#callingconv)，可选的unnamed_addr属性，返回类型，可选返回类型的 [parameter attribute](http://llvm.org/docs/LangRef.html#paramattrs) ，函数名称，（可能为空）参数列表（每个都带有可选的 [parameter attribute](http://llvm.org/docs/LangRef.html#paramattrs)），可选的 [function attributes](http://llvm.org/docs/LangRef.html#fnattrs)，可选的address space，可选的部分，可选的对齐，可选的 [comdat](http://llvm.org/docs/LangRef.html#langref-comdats)，可选的 [garbage collector name](http://llvm.org/docs/LangRef.html#gc)，可选 [prefix](http://llvm.org/docs/LangRef.html#prefixdata)，可选 [prologue](http://llvm.org/docs/LangRef.html#prologuedata)，可选[personality](http://llvm.org/docs/LangRef.html#personalityfn)，附加 [metadata](http://llvm.org/docs/LangRef.html#metadata)的可选列表，左大括号，基本块列表和结束大括号。

LLVM函数声明包含“declare”关键字，可选的 [linkage type](http://llvm.org/docs/LangRef.html#linkage)，可选的[visibility style](http://llvm.org/docs/BitCodeFormat.html#visibility)，可选的[DLL storage class](http://llvm.org/docs/LangRef.html#dllstorageclass)，可选的[calling convention](http://llvm.org/docs/LangRef.html#callingconv)，可选的unnamed_addr或local_unnamed_addr属性，可选的地址空间，返回类型，可选返回类型的 [parameter attribute](http://llvm.org/docs/LangRef.html#paramattrs)，函数名称，可能为空的参数列表，可选对齐，可选的 [garbage collector name](http://llvm.org/docs/LangRef.html#gc)，可选[prefix](http://llvm.org/docs/LangRef.html#prefixdata)和可选的 [prologue](http://llvm.org/docs/LangRef.html#prologuedata)。

函数定义包含基本块列表，形成函数的CFG（控制流图）。每个基本块可以可选地以标签开始（给基本块一个符号表条目），包含指令列表，并以终结符指令（例如分支或函数返回）结束。如果未提供显式标签，则使用与未命名的临时值相同的计数器中的下一个值为块分配隐式编号标签（[参见上文](http://llvm.org/docs/LangRef.html#identifiers)）。例如，如果函数入口块没有显式标签，则会为其分配标签“％0”，然后该块中的第一个未命名临时块将为“％1”，等等。

函数中的第一个基本块在两个方面是特殊的：它在函数入口处立即执行，并且不允许具有先前的基本块（即，不能有任何分支到函数的入口块）。因为块可以没有前驱，所以它也不能有任何[PHI节点](http://llvm.org/docs/LangRef.html#i-phi)。

LLVM允许为函数指定显式部分。如果目标支持它，它将向指定的部分发出函数。此外，该功能可以放在COMDAT中。

可以为函数指定显式对齐。如果不存在，或者对齐设置为零，则功能的对齐由目标设置为任何方便的感觉。如果指定了显式对齐，则强制该函数至少具有那么多对齐。所有对齐必须是2的幂。

如果给出了unnamed_addr属性，则知道该地址不重要，并且可以合并两个相同的函数。

如果给出local_unnamed_addr属性，则知道该模块中的地址不重要。

如果未给出显式地址空间，则默认为[datalayout string](http://llvm.org/docs/LangRef.html#langref-datalayout)中的程序地址空间。

句法：

```
define [linkage] [PreemptionSpecifier] [visibility] [DLLStorageClass]
       [cconv] [ret attrs]
       <ResultType> @<FunctionName> ([argument list])
       [(unnamed_addr|local_unnamed_addr)] [AddrSpace] [fn Attrs]
       [section "name"] [comdat [($name)]] [align N] [gc] [prefix Constant]
       [prologue Constant] [personality Constant] (!name !N)* { ... }
```

参数列表是以逗号分隔的参数序列，其中每个参数的格式如下：

句法：

```
<type> [parameter Attrs] [name]
```

### Aliases(别名)

与函数或变量不同，别名不会创建任何新数据。 它们只是现有职位的新符号和元数据。

别名具有名称和别名，可以是全局值或常量表达式。

别名可以具有可选的 [linkage type](http://llvm.org/docs/LangRef.html#linkage)，可选的[runtime preemption specifier](http://llvm.org/docs/LangRef.html#runtime-preemption-model)，可选的[visibility style](http://llvm.org/docs/BitCodeFormat.html#visibility)，可选的 [DLL storage class](http://llvm.org/docs/LangRef.html#dllstorageclass) 和可选的 [tls model](http://llvm.org/docs/LangRef.html#tls-model)。

句法：

```
@<Name> = [Linkage] [PreemptionSpecifier] [Visibility] [DLLStorageClass] [ThreadLocal] [(unnamed_addr|local_unnamed_addr)] alias <AliaseeTy>, <AliaseeTy>* @<Aliasee>
```

链接必须是`private`，`internal`，`linkonce`，`weak`，l`inkonce_odr`，`weak_odr`，`external`之一。 请注意，某些系统链接器可能无法正确处理丢弃别名的弱符号。

不是unnamed_addr的别名保证与别名表达式具有相同的地址。 unnamed_addr仅保证指向相同的内容。

如果给出local_unnamed_addr属性，则知道该模块中的地址不重要。

由于别名只是第二个名称，因此应用了一些限制，其中一些限制只能在生成目标文件时进行检查：

- 定义别名的表达式必须在汇编时可计算。 由于它只是一个名称，因此不能使用重定位。
- 表达式中的别名可能较弱，因为中间别名被覆盖的可能性无法在目标文件中表示。
- 表达式中没有全局值可以是声明，因为这将需要重定位，这是不可能的。

### IFuncs

IFuncs与别名一样，不会创建任何新数据或函数。 它们只是动态链接器在运行时通过调用解析器函数解析的新符号。

IFuncs有一个名称和一个解析器，它是动态链接器调用的函数，它返回与该名称关联的另一个函数的地址。

IFunc可以具有可选的 [linkage type](http://llvm.org/docs/LangRef.html#linkage)和可选的 [visibility style](http://llvm.org/docs/BitCodeFormat.html#visibility)。

句法：

```
@<Name> = [Linkage] [Visibility] ifunc <IFuncTy>, <ResolverTy>* @<Resolver>
```

### Comdats

Comdat IR提供对COFF和ELF目标文件COMDAT功能的访问。

Comdats有一个代表COMDAT密钥的名称。 如果链接器通过某个其他键选择该键，则指定此键的所有全局对象将仅在最终目标文件中结束。 别名被置于其别名计算到的相同COMDAT中（如果有的话）。

Comdats有一种选择类型可以提供关于链接器应如何在两个不同目标文件中的键之间进行选择的输入。

句法：

```
$<Name> = comdat SelectionKind
```

选择类型必须是以下之一：

`any`
	链接器可以选择任何COMDAT键，选择是任意的。
`exactmatch`
	链接器可以选择任何COMDAT密钥，但这些部分必须包含相同的数据。
`largest`
	链接器将选择包含最大COMDAT键的节。
`noduplicates`
	链接器要求只存在具有此COMDAT键的节。
`samesize`
	链接器可以选择任何COMDAT密钥，但这些部分必须包含相同数量的数据。
请注意，Mach-O平台不支持COMDAT，ELF和WebAssembly仅支持任何选择类型。

以下是COMDAT组的示例，其中仅在COMDAT键的部分最大时才选择函数：

```
$foo = comdat largest
@foo = global i32 2, comdat($foo)

define void @bar() comdat($foo) {
  ret void
}
```

作为语法糖，如果名称与全局名称相同，则可以省略$ name：

```
$foo = comdat any
@foo = global i32 2, comdat
```

在COFF目标文件中，这将创建一个COMDAT部分，其中选择类型IMAGE_COMDAT_SELECT_LARGEST包含@foo符号的内容，另一个COMDAT部分具有选择类型IMAGE_COMDAT_SELECT_ASSOCIATIVE，该部分与第一个COMDAT部分相关并包含@bar符号的内容。

全局对象的属性有一些限制。 它或它的别名在定位COFF时必须与COMDAT组具有相同的名称。 可以在链接时使用此对象的内容和大小，以确定根据选择类型选择哪些COMDAT组。 因为对象的名称必须与COMDAT组的名称匹配，所以全局对象的链接不能是本地的; 如果符号表中发生冲突，则可以重命名本地符号。

COMDATS和部分属性的组合使用可能会产生令人惊讶的结果。 例如：

```
$foo = comdat any
$bar = comdat any
@g1 = global i32 42, section "sec", comdat($foo)
@g2 = global i32 42, section "sec", comdat($bar)
```

从目标文件的角度来看，这需要创建两个具有相同名称的部分。 这是必要的，因为两个全局变量属于不同的COMDAT组，并且在目标文件级别的COMDAT由部分表示。

请注意，除了使用COMDAT IR指定的任何IR构造之外，某些IR构造（如全局变量和函数）可以在目标文件中创建COMDAT。 当代码生成器配置为在各个部分中发出全局变量时（例如，当-data-sections或-function-sections被提供给llc时），就会出现这种情况。 

### Named Metadata

*元数据命名*

命名元数据是元数据的集合。  [Metadata nodes](http://llvm.org/docs/LangRef.html#metadata)节点（但不是元数据字符串）是命名元数据的唯一有效操作数。

- 命名元数据表示为带有元数据前缀的字符串。 元数据名称的规则与标识符的规则相同，但不允许使用引号。 “\ xx”类型转义仍然有效，它允许任何字符成为名称的一部分。

句法：

```
; Some unnamed metadata nodes, which are referenced by the named metadata.
!0 = !{!"zero"}
!1 = !{!"one"}
!2 = !{!"two"}
; A named metadata.
!name = !{!0, !1, !2}
```

### Parameter Attributes

返回类型和函数类型的每个参数可以具有与它们相关联的一组参数属性。 参数属性用于传递有关函数结果或参数的其他信息。 参数属性被认为是函数的一部分，而不是函数类型，因此具有不同参数属性的函数可以具有相同的函数类型。

参数属性是遵循指定类型的简单关键字。 如果需要多个参数属性，则它们是空格分隔的。 例如：

```
declare i32 @printf(i8* noalias nocapture, ...)
declare i32 @atoi(i8 zeroext)
declare signext i8 @returns_signed_char()
```

请注意，函数结果（nounwind，readonly）的任何属性都紧跟在参数列表之后。

目前，只定义了以下参数属性：

`zeroext`:
	这向代码生成器指示参数或返回值应该被调用者（对于参数）或被调用者（对于返回值）零扩展到目标的ABI所需的范围。
`signext`:
	这向代码生成器指示参数或返回值应该被调用者（对于参数）或被调用者（对于返回值）符号扩展到目标的ABI（通常是32位）所需的范围。 。
`inreg`:
	这表明该参数或返回值应以特定的目标依赖方式处理，同时为函数调用或返回发出代码（通常，通过将其放入寄存器而不是内存，尽管某些目标使用它来区分两个不同种类的寄存器）。使用此属性是特定于目标的。
`byval`:
	这表明指针参数确实应该通过值传递给函数。该属性意味着在调用者和被调用者之间建立了一个隐藏的指针对象副本，因此被调用者无法修改调用者中的值。此属性仅对LLVM指针参数有效。它通常用于按值传递结构和数组，但对指向标量的指针也有效。该副本被认为属于调用者而不是被调用者（例如，readonly函数不应该写入byval参数）。这不是返回值的有效属性。

​	byval属性还支持使用align属性指定对齐方式。它指示要形成的堆栈槽的对齐以及指定给调用站点的指针的已知对齐。如果未指定对齐，则代码生成器会进行特定于目标的假设。

`inalloca`:
	inalloca参数属性允许调用者获取传出堆栈参数的地址。 inalloca参数必须是指向alloca指令生成的堆栈内存的指针。 alloca或参数分配也必须使用inalloca关键字进行标记。只有最后一个参数可能具有inalloca属性，并且该参数保证在内存中传递。

​	最多一次调用可以使用一个参数分配，因为该调用可以解除分配。 inalloca属性不能与影响参数存储的其他属性一起使用，例如inreg，nest，sret或byval。 inalloca属性还会禁用LLVM隐式降低大型聚合返回值，这意味着前端作者必须使用sret指针降低它们。

​	到达调用站点时，参数分配必须是仍处于活动状态的最新堆栈分配，或者行为未定义。在参数分配之后和调用站点之前可以分配额外的堆栈空间，但必须使用 [llvm.stackrestore](http://llvm.org/docs/LangRef.html#int-stackrestore).清除它。

有关如何使用此属性的更多信息，请参阅 [Design and Usage of the InAlloca Attribute](http://llvm.org/docs/InAlloca.html)。

`sret`
	这表示指针参数指定结构的地址，该结构是源程序中函数的返回值。调用者必须保证该指针有效：被调用者可以假定对结构的加载和存储不要陷阱并且要正确对齐。这不是返回值的有效属性。
`align <n>`
	这表示优化器可以假定指针值具有指定的对齐。

​	请注意，与byval属性结合使用时，此属性具有其他语义。

`noalias`
	这表示在执行函数期间，通过不 [based](http://llvm.org/docs/LangRef.html#pointeraliasing) 参数或返回值的指针值，也不会访问基于参数或返回值通过指针值访问的对象。返回值的属性还具有下面描述的其他语义。呼叫者与被呼叫者共同承担责任，以确保满足这些要求。有关详细信息，请参阅 [alias analysis](http://llvm.org/docs/AliasAnalysis.html#must-may-or-no)中NoAlias响应的讨论。

​	请注意，noalias的这个定义有意类似于C99中对于函数参数的限制。

​	对于函数返回值，C99的限制没有意义，而LLVM的noalias是。此外，当在函数参数上使用时，返回值的noalias属性的语义强于属性的语义。在函数返回值上，noalias属性指示该函数的作用类似于系统内存分配函数，从存储中返回指向已分配存储的指针，以便调用者可访问的任何其他对象。

`nocapture`
	这表明被调用者没有生成比被调用者本身更长的指针的任何副本。这不是返回值的有效属性。认为在易失性操作中使用的地址被捕获。
`nest`
这表明可以使用trampoline intrinsics删除指针参数。这不是返回值的有效属性，只能应用于一个参数。
`returned`
`这表示该函数始终将参数作为其返回值返回。这是对生成调用者时使用的优化器和代码生成器的提示，允许值传播，尾调用优化以及在某些情况下省略寄存器保存和恢复;生成被调用者时不会检查或强制执行。参数和函数返回类型必须是[bitcast instruction](http://llvm.org/docs/LangRef.html#i-bitcast)的有效操作数。这不是返回值的有效属性，只能应用于一个参数。
`nonnull`
	这表示参数或返回指针不为空。此属性仅可应用于指针类型的参数。 LLVM不会检查或强制执行此操作;如果参数或返回指针为null，则行为未定义。
`dereferenceable(<n>)`
	这表示参数或返回指针是可解除引用的。此属性仅可应用于指针类型的参数。可以从推测性地加载可解除引用的指针而没有陷阱的风险。必须在括号中提供已知可解除引用的字节数。字节数小于指针类型的大小是合法的。 nonnull属性并不意味着解除引用性（考虑指向超出数组末尾的一个元素的指针），但是可解除引用（`<n>`）确实意味着在addrspace（0）（这是默认地址空间）中是非空的。

`dereferenceable_or_null（<N>）`
	这表示参数或返回值不是同时非空和不可解除引用（最多<n>个字节）。所有使用dereferenceable_or_null（<n>）标记的非空指针都是可解除引用的（<n>）。对于地址空间0，dereferenceable_or_null（<n>）意味着指针恰好是可解除引用（<n>）或null之一，而在其他地址空间中，dereferenceable_or_null（<n>）意味着指针至少是可解引用的指针之一（< n>）或null（即它可以是null和dereferenceable（<n>））。此属性仅可应用于指针类型的参数。
`swiftself`
	这表示该参数是self / context参数。这不是返回值的有效属性，只能应用于一个参数。
`swifterror`
	该属性有助于建模和优化Swift错误处理。它可以应用于具有指向指针类型的指针或指针大小的alloca的参数。在调用站点，与swifterror参数对应的实际参数必须来自swifterror alloca或调用者的swifterror参数。 swifterror值（参数或alloca）只能从swifterror参数加载和存储，或用作swifterror参数。这不是返回值的有效属性，只能应用于一个参数。

​	这些约束允许调用约定通过将它们与调用边界处的特定寄存器相关联而不是将它们放在内存中来优化对swifterror变量的访问。由于这确实改变了调用约定，因此对参数使用swifterror属性的函数与不与ABI兼容的函数不兼容。

​	这些约束还允许LLVM假定swifterror参数不会为函数中可见的任何其他内存添加别名，并且作为参数传递的swifterror alloca不会转义。

### Garbage Collector Strategy Names

*垃圾回收器策略名称*

每个函数都可以指定一个垃圾收集器策略名称，它只是一个字符串：

```
define void @f() gc "name" { ... }
```

支持的name值包括 [built in to LLVM](http://llvm.org/docs/GarbageCollection.html#builtin-gc-strategies)的值和加载的插件提供的任何值。 指定GC策略将导致编译器更改其输出以支持命名的垃圾收集算法。 请注意，LLVM本身不包含垃圾收集器，此功能仅限于生成可与外部提供的收集器进行互操作的机器代码。

### Prefix Data

*前缀数据*

前缀数据是与函数关联的数据，代码生成器将在函数入口点之前立即发出该函数。 此功能的目的是允许前端将特定于语言的运行时元数据与特定函数相关联，并通过函数指针使其可用，同时仍允许调用函数指针。

为了访问给定函数的数据，程序可以将函数指针指向指向常量类型和解引用索引-1的指针。 这意味着IR符号指向刚好超过前缀数据的末尾。 例如，以单个i32注释的函数为例，

```
define void @f() prefix i32 123 { ... }
```

前缀数据可以引用为:

```
%0 = bitcast void* () @f to i32*
%a = getelementptr inbounds i32, i32* %0, i32 -1
%b = load i32, i32* %a
```

前缀数据的布局就好像它是前缀数据类型的全局变量的初始化器一样。 将放置该函数，使前缀数据的开头对齐。 这意味着如果前缀数据的大小不是对齐大小的倍数，则函数的入口点将不会对齐。 如果需要对齐函数的入口点，则必须将填充添加到前缀数据中。

一个函数可能有前缀数据但没有正文。 这与available_externally链接具有类似的语义，因为优化器可以使用数据，但不会在目标文件中发出数据。

### Prologue Data

*序言数据*

prologue属性允许在函数体之前插入任意代码（编码为字节）。 这可用于启用功能热修补和仪表。

为了维护普通函数调用的语义，序言数据必须具有特定的格式。 具体来说，它必须以一系列字节开始，这些字节序列解码为机器指令序列，对模块的目标有效，将控制转移到紧接在序言数据之后的点，而不执行任何其他可见操作。 这允许内联器和其他传递来推理函数定义的语义，而不需要推理序言数据。 显然，这使得序言数据的格式高度依赖于目标。

x86架构的有效序言数据的一个简单例子是i8 144，它编码nop指令：

```
define void @f() prologue i8 144 { ... }
```

通常，序列数据可以通过编码跳过元数据的相对分支指令来形成，如x86_64体系结构的有效序言数据的示例，其中前两个字节编码jmp。+ 10：

```
%0 = type <{ i8, i8, i8* }>
define void @f() prologue %0 <{ i8 235, i8 8, i8* @md}> { ... }
```

一个函数可能有序言数据，但没有正文。 这与available_externally链接具有类似的语义，因为优化器可以使用数据，但不会在目标文件中发出数据。

### Personality Function

personality属性允许函数指定用于异常处理的函数。

### Attribute Groups

属性组是IR中对象引用的属性组。 它们对于保持.ll文件可读是很重要的，因为很多函数将使用相同的属性集。 在.ll文件的退化情况下，对应于单个.c文件，单个属性组将捕获用于构建该文件的重要命令行标志。

属性组是模块级对象。 要使用属性组，对象引用属性组的ID（例如＃37）。 对象可以指代多个属性组。 在这种情况下，合并来自不同组的属性。

下面是一个函数的属性组示例，该函数应始终内联，堆栈对齐为4，并且不应使用SSE指令：

```
; Target-independent attributes:
attributes #0 = { alwaysinline alignstack=4 }

; Target-dependent attributes:
attributes #1 = { "no-sse" }

; Function @f has attributes: alwaysinline, alignstack=4, and "no-sse".
define void @f() #0 #1 { ... }
```

### Function Attributes

设置函数属性以传递有关函数的其他信息。 函数属性被认为是函数的一部分，而不是函数类型，因此具有不同函数属性的函数可以具有相同的函数类型。

函数属性是遵循指定类型的简单关键字。 如果需要多个属性，则它们是空格分隔的。 例如：

```
define void @f() noinline { ... }
define void @f() alwaysinline { ... }
define void @f() alwaysinline optsize { ... }
define void @f() optsize { ... }
```

`alignstack(<n>)`
	该属性表示，当发出序言和结尾时，后端应该强制对齐堆栈指针。在括号中指定所需的对齐，该对齐必须是2的幂。
`allocsize(<EltSizeParam>[, <NumEltsParam>])`
	此属性指示带注释的函数将始终返回至少给定数量的字节（或null）。它的参数是零索引参数号;如果提供了一个参数，那么假设至少CallSite.Args [EltSizeParam]字节在返回的指针处可用。如果提供了两个，则假定CallSite.Args [EltSizeParam] * CallSite.Args [NumEltsParam]字节可用。引用的参数必须是整数类型。没有假设返回的内存块的内容。
`alwaysinline`
	此属性指示内联器应尽可能尝试将此函数内联到调用方，忽略此调用方的任何活动内联大小阈值。
`builtin`
	这表明调用站点的被调用函数应该被识别为内置函数，即使函数的声明使用了nobuiltin属性。这仅在调用站点有效，用于直接调用使用nobuiltin属性声明的函数。
`cold`
	此属性表示很少调用此函数。在计算边缘权重时，由冷函数调用支配的基本块也被认为是冷的;因此，重量轻。
`convergent`
	在一些并行执行模型中，存在不能使控制依赖于任何附加值的操作。我们将此类操作称为收敛，并使用此属性标记它们。

​	收敛属性可以出现在函数或调用/调用指令上。当它出现在函数上时，表示不应该对此函数的调用取决于其他值。例如，内部llvm.nvvm.barrier0是收敛的，因此不能对此内在函数的调用依赖于其他值。

​	当它出现在调用/调用时，收敛属性表示我们应该将调用视为调用收敛函数。这对间接呼叫特别有用;如果没有这个，我们可以将这些调用视为目标是非收敛的。

​	当函数可以证明函数不执行任何收敛操作时，优化器可以删除函数上的收敛属性。类似地，当优化器可以证明调用/调用不能调用收敛函数时，优化器可以去除收敛调用/调用。

`inaccessiblememonly`
	此属性指示该函数只能访问正在编译的模块无法访问的内存。这是一种较弱的readnone形式。如果函数读取或写入其他内存，则行为未定义。
`inaccessiblemem_or_argmemonly`
	此属性指示该函数只能访问正在编译的模块无法访问的内存，或者由其指针参数指向的内存。这是一种较弱的讽刺形式。如果函数读取或写入其他内存，则行为未定义。
`inlinehint`
	此属性表示源代码包含一个暗示，希望内联此函数（例如C / C ++中的“inline”关键字）。这只是一个暗示;它对内衬没有任何要求。
`jumptable`
	该属性表示该函数应该在代码生成时添加到跳转指令表中，并且应该用对相应的跳转指令表函数指针的引用替换对该函数的所有地址引用。请注意，这会为原始函数创建一个新指针，这意味着依赖于函数指针标识的代码可能会中断。因此，任何使用jumptable注释的函数也必须是unnamed_addr。
`minsize`
	此属性表明优化传递和代码生成器传递做出选择，使得此函数的代码大小尽可能小，并执行可能牺牲运行时性能的优化，以便最小化生成的代码的大小。
`naked`
	此属性禁用该功能的序言/结尾发射。这可能会产生特定于系统的后果。
`no-jump-tables`
	当此属性设置为true时，将禁用可通过降低开关大小写生成的跳转表和查找表。
`nobuiltin`
	这表示呼叫站点的被叫方功能未被识别为内置功能。 LLVM将保留原始调用，而不是基于内置函数的语义将其替换为等效代码，除非调用站点使用builtin属性。这在调用站点以及函数声明和定义中有效。
`noduplicate`
	此属性表示无法复制对函数的调用。对noduplicate函数的调用可以在其父函数内移动，但可以不在其父函数内复制。

​	包含noduplicate调用的函数可能仍然是内联候选者，前提是该调用不是通过内联复制的。这意味着该函数具有内部链接，并且只有一个调用站点，因此在内联后原始调用已经死亡。

`noimplicitfloat`
	此属性禁用隐式浮点指令。
`noinline`
	此属性表示内联器在任何情况下都不应该内联此函数。此属性不能与alwaysinline属性一起使用。
`noredzone`
	此属此属性表示代码生成器不应使用红色区域，即使特定于目标的ABI通常允许它。

`indirect-tls-seg-refs`
	此属性指示代码生成器不应使用通过段寄存器的直接TLS访问，即使特定于目标的ABI通常允许它也是如此。
`noreturn`
	此函数属性表示函数永远不会正常返回。如果函数动态返回，则会在运行时生成未定义的行为。
`norecurse`
	此函数属性指示函数不直接或间接调用自身任何可能的调用路径。如果函数递归，这会在运行时产生未定义的行为。
`nounwind`
	此函数属性表示该函数从不引发异常。如果函数确实引发异常，则其运行时行为未定义。但是，标记为nounwind的函数仍可能陷阱或生成异步异常。 LLVM识别的用于处理异步异常（例如SEH）的异常处理方案仍将提供其实现定义的语义。
`"null-pointer-is-valid"`
	如果“null-pointer-is-valid”设置为“true”，则地址空间0中的空地址被认为是存储器加载和存储的有效地址。任何分析或优化都不应将取消引用null的指针视为此函数中的未定义行为。注意：由于在常量表达式中查询此属性的限制，将全局变量的地址与null进行比较仍可能会计算为false。
`optforfuzzing`
	此属性表示应针对最大模糊信号优化此功能。
`optnone`
	此函数属性指示大多数优化过程将跳过此函数，但过程间优化过程除外。代码生成默认为“快速”指令选择器。此属性不能与alwaysinline属性一起使用;此属性也与minsize属性和optsize属性不兼容。

​	此属性也需要在函数上指定noinline属性，因此该函数永远不会内联到任何调用者。只有具有alwaysinline属性的函数才是内联到此函数体内的有效候选者。

`optsize`
	此属性表明优化传递和代码生成器传递会做出选择，使此函数的代码大小保持较低，否则只要它们不会显着影响运行时性能，就会专门进行优化以减少代码大小。
``"patchable-function"` `
	此属性告诉代码生成器，为此函数生成的代码需要遵循某些约定，这些约定使运行时函数可以在以后修补它。此属性的确切效果取决于其字符串值，目前有一种合法的可能性：

- “prologue-short-redirect” - 这种patchable函数用于支持修补函数序言，以线程安全的方式将控制重定向到函数之外。它保证函数的第一条指令足够大以容纳短跳转指令，并且将充分对齐以允许通过原子比较和交换指令完全改变。虽然可以通过插入足够大的NOP来满足第一个要求，但LLVM可以并且将尝试重新使用现有指令（即，无论如何必须发出的指令），因为可修补指令大于短跳转。

  “prologue-short-redirect”目前仅在x86-64上受支持。

  此属性本身并不意味着对过程间优化的限制。修补的所有语义效果可能必须通过链接类型单独传达。

`"probe-stack"`
       该属性表示该函数将触发堆栈末尾的保护区域。它确保对堆栈的访问必须不比保护区域的大小更远，以便先前访问堆栈。它需要一个必需的字符串值，即将被调用的堆栈探测函数的名称。

​	如果具有“probe-stack”属性的函数被内联到具有另一个“probe-stack”属性的函数中，则生成的函数具有调用者的“probe-stack”属性。如果具有“probe-stack”属性的函数被内联到一个根本没有“probe-stack”属性的函数中，则生成的函数具有被调用者的“probe-stack”属性。

`readnone`
	在函数上，此属性指示函数严格基于其参数计算其结果（或决定解除异常），而不取消引用任何指针参数或以其他方式访问调用者可见的任何可变状态（例如，内存，控制寄存器等）功能。它不会通过任何指针参数（包括byval参数）进行写入，也不会更改调用者可见的任何状态。这意味着虽然它不能通过调用C ++异常抛出方法（因为它们写入内存）来展开异常，但可能存在非C ++机制，它们抛出异常而不写入LLVM可见内存。

​	在一个参数上，该属性指示该函数不取消引用该指针参数，即使它可以读取或写入指针指向的内存（如果通过其他指针访问）。

​	如果readnone函数读取或写入程序可见的内存，或者有其他副作用，则行为未定义。如果函数读取或写入readnone指针参数，则行为未定义。

`readonly`
	在函数上，此属性指示函数不通过任何指针参数（包括byval参数）写入或以其他方式修改调用函数可见的任何状态（例如，内存，控制寄存器等）。它可以取消引用可以在调用者中设置的指针参数和读取状态。当使用相同的参数集和全局状态调用时，只读函数始终返回相同的值（或相同地展开异常）。这意味着虽然它不能通过调用C ++异常抛出方法（因为它们写入内存）来展开异常，但可能存在非C ++机制，它们抛出异常而不写入LLVM可见内存。

​	在参数上，此属性指示函数不会通过此指针参数进行写入，即使它可能会写入指针指向的内存。

​	如果只读函数将内存写入程序可见，或者有其他副作用，则行为未定义。如果函数写入只读指针参数，则行为未定义。

`"stack-probe-size"`
	此属性控制堆栈探测器的行为：“probe-stack”属性或ABI所需的堆栈探测器（如果有）。它定义了保护区域的大小。它确保如果函数可能使用的堆栈空间多于保护区域的大小，则将发出堆栈探测序列。它需要一个必需的整数值，默认为4096。

​	如果具有“stack-probe-size”属性的函数内联到具有另一个“stack-probe-size”属性的函数中，则生成的函数具有“stack-probe-size”属性，该属性具有较低的数值。如果具有“stack-probe-size”属性的函数被内联到一个根本没有“stack-probe-size”属性的函数中，则生成的函数具有被调用者的“stack-probe-size”属性。

`"no-stack-arg-probe"`
	此属性禁用ABI所需的堆栈探测（如果有）。
`writeonly`
	在函数上，此属性指示函数可以写入但不从内存中读取。

​	在一个参数上，该属性指示该函数可以写入但不读取该指针参数（即使它可以从指针指向的内存中读取）。

​	如果writeonly函数读取程序可见的内存，或者有其他副作用，则行为未定义。如果函数从writeonly指针参数读取，则行为未定义。

`argmemonly`
	此属性指示函数内部唯一的内存访问是从其指针类型参数指向的对象加载和存储，具有任意偏移。或者换句话说，函数中的所有内存操作只能使用基于其函数参数的指针来引用内存。

​	请注意，argmemonly可以与readonly属性一起使用，以指定该函数只从其参数中读取。

​	如果argmemonly函数读取或写入指针参数以外的内存，或者有其他副作用，则行为未定义。

`returns_twice`
	此属性表示此函数可以返回两次。 C setjmp就是这种功能的一个例子。编译器禁用这些函数调用者中的一些优化（如尾调用）。
`safestack`
	此属性表示已为此功能启用SafeStack保护。

​	如果将具有safestack属性的函数内联到没有safestack属性或具有ssp，sspstrong或sspreq属性的函数中，则生成的函数将具有safestack属性。

`sanitize_address`
	此属性指示为此功能启用了AddressSanitizer检查（动态地址安全性分析）。
`sanitize_memory`
	此属性指示已为此功能启用MemorySanitizer检查（对未初始化的内存的访问的动态检测）。
`sanitize_thread`
	此属性指示为此函数启用了ThreadSanitizer检查（动态线程安全分析）。
`sanitize_hwaddress`
	此属性指示为此函数启用HWAddressSanitizer检查（基于标记指针的动态地址安全性分析）。
`speculative_load_hardening`
	此属性表示应为函数体启用“推测加载强化”。这是一种尽力而为的尝试，旨在缓解基于现代处理器推测执行的基本原则的所有已知的推测执行信息泄漏漏洞。这些漏洞通常被归类为“幽灵变种＃1”漏洞。值得注意的是，这并未试图减轻特定处理器的推测性执行和/或预测设备可能被完全破坏的任何漏洞（例如“分支目标注入”，也就是“分类变体＃2”）。相反，这是一个独立于目标的请求，以加强投机执行错误地加载秘密数据所带来的完全通用风险，使其可用于某些微架构侧通道以进行信息泄露。对于没有任何推测执行或预测器的处理器，预计这将是一个无操作。

​	内联时，属性是粘性的。内联携带此属性的函数将使调用者获得该属性。这是为了提供一个最大保守模型，其中使用此属性注释的函数中的代码将始终（即使在内联后）最终硬化。

`speculatable`
	此函数属性表示该函数除了计算其结果之外没有任何影响，并且没有未定义的行为。请注意，speculatable不足以得出结论，沿着任何特定的执行路径，对此函数的调用次数将不会在外部可观察到。此属性仅对函数和声明有效，而不对单个调用站点有效。如果函数被错误地标记为可推测并且确实表现出未定义的行为，则即使调用站点是死代码，也可能会观察到未定义的行为。
`ssp`
	此属性表示该函数应发出堆栈粉碎保护器。它是以“canary”的形式 - 在从函数返回时检查的局部变量之前放置在堆栈上的随机值，以查看它是否已被覆盖。启发式用于确定函数是否需要堆栈保护程序。使用的启发式方法将为以下功能启用保护器：

- 字符数组大于ssp-buffer-size（默认为8）。

- 包含大于ssp-buffer-size的字符数组的聚合。

- 使用大于ssp-buffer-size的可变大小或常量大小调用alloca（）。

  被识别为需要保护器的变量将被布置在堆叠上，使得它们与堆叠保护器防护装置相邻。

  如果将具有ssp属性的函数内联到没有ssp属性的函数中，则生成的函数将具有ssp属性。

`sspreq`
       此属性表示该函数应始终发出堆栈粉碎保护程序。这会覆盖ssp函数属性。

​	被识别为需要保护器的变量将被布置在堆叠上，使得它们与堆叠保护器防护装置相邻。具体的布局规则是：

- 包含大型数组（> = ssp-buffer-size）的大型数组和结构最接近堆栈保护程序。

- 包含小数组（<ssp-buffer-size）的小数组和结构距离保护器最近。

- 获得地址的变量距离保护者最近。

  如果将具有sspreq属性的函数内联到没有sspreq属性或具有ssp或sspstrong属性的函数中，则生成的函数将具有sspreq属性。

`sspstrong`
       此属性表示该函数应发出堆栈粉碎保护器。在确定函数是否需要堆栈保护程序时，此属性会导致使用强启发式算法。强大的启发式功能可以为以下功能提供保护：

- 任何大小和类型的数组
- 包含任何大小和类型的数组的聚合。
- 调用alloca（）。
- 已经获取其地址的局部变量。

被识别为需要保护器的变量将被布置在堆叠上，使得它们与堆叠保护器防护装置相邻。具体的布局规则是：

- 包含大型数组（> = ssp-buffer-size）的大型数组和结构最接近堆栈保护程序。
- 包含小数组（<ssp-buffer-size）的小数组和结构距离保护器最近。
- 获得地址的变量距离保护者最近。

这会覆盖ssp函数属性。

如果将具有sspstrong属性的函数内联到没有sspstrong属性的函数中，则生成的函数将具有sspstrong属性。

`strictfp`
此属性指示从需要严格浮点语义的作用域调用该函数。 LLVM不会尝试任何需要对浮点舍入模式进行假设的优化，也不会尝试更改浮点状态标志的状态，否则可能会通过调用此函数来设置或清除浮点状态标志。
`"thunk"`
此属性指示函数将通过尾调用委托给其他某个函数。 thunk的原型不应该用于优化目的。调用者应该投射thunk原型以匹配thunk目标原型。
`uwtable`
此属性指示要定向的ABI要求为此函数生成展开表条目，即使我们可以显示没有异常通过它。这通常是ELF x86-64 abi的情况，但是对于某些编译单元可以禁用它。
`nocf_check`
此属性表示不对归属实体执行控制流检查。它禁用特定实体的-fcf-protection = <>以细化HW控制流保护机制。该标志是独立于目标的，并且当前属于函数或函数指针。
`shadowcallstack`
此属性指示已为该功能启用ShadowCallStack检查。检测检查函数prolog和eiplog之间的函数返回地址是否没有变化。它目前是特定于x86_64的。

### Global Attributes

可以设置属性以传达关于全局变量的附加信息。 与 [function attributes](http://llvm.org/docs/LangRef.html#fnattrs)不同，全局变量上的属性被分组到单个[attribute group](http://llvm.org/docs/LangRef.html#attrgrp)中。

### Operand Bundles

操作数包是标记的SSA值集，可以与某些LLVM指令相关联（当前只调用s和调用s）。 在某种程度上，它们就像元数据，但丢弃它们是不正确的，并将改变程序语义。

句法：

```
operand bundle set ::= '[' operand bundle (, operand bundle )* ']'
operand bundle ::= tag '(' [ bundle operand ] (, bundle operand )* ')'
bundle operand ::= SSA value
tag ::= string constant
```

操作数捆绑(Operand bundles)不是函数签名的一部分，并且可以从具有不同类型的操作数包的多个位置调用给定的函数。这反映了操作数bundle在概念上是调用（或调用）的一部分，而不是被调度的被调用者。

操作数捆绑包是一种通用机制，旨在支持托管语言的类似运行时内省功能。虽然操作数束的确切语义取决于bundle标记，但操作数束的存在可以影响程序语义的程度有一些限制。这些限制被描述为“未知”操作数包的语义。只要操作数包的行为在这些限制内是可描述的，LLVM就不需要具有操作数包的特殊知识就不会错误编译包含它的程序。

- 在将控制权转移到被调用者或调用者之前，未知操作数包的包操作数以未知方式转义。
- 使用操作数束调用和调用在进入和退出时在堆上具有未知的读/写效果（即使调用目标是readnone或readonly），除非它们被callsite特定属性覆盖。
- 调用站点上的操作数包不能更改被调用函数的实现。程序间优化照常工作，只要它们考虑前两个属性即可。

下面描述更具体类型的操作数束。

#### Deoptimization Operand Bundles

去优化操作数包的特征在于“deopt”操作数包标记。这些操作数包表示它们所附加的调用站点的备用“安全”延续，并且可以由合适的运行时用于在指定的调用站点处对已编译的帧进行去优化。最多可以有一个“deopt”操作数捆绑附加到呼叫站点。去优化的确切细节超出了语言参考的范围，但它通常涉及将编译的帧重写为一组解释的帧。

从编译器的角度来看，去优化操作数捆绑包使得它们至少只读取的调用站点。他们读取所有指针类型的操作数（即使它们没有被转义）和整个可见堆。除非在去优化期间，去优化操作数包不捕获它们的操作数，在这种情况下，控制将不会返回到编译的帧。

内联器知道如何内联具有去优化操作数束的调用。就像通过正常调用站点内联涉及组成正常和异常延续一样，通过具有去优化操作数束的调用站点内联需要适当地组成“安全”去优化延续。内联器通过将父母的去优化继续前置于内联体中的每个去优化延续来实现这一点。例如。在以下示例中将@f内联到@g中

```
define void @f() {
  call void @x()  ;; no deopt state
  call void @y() [ "deopt"(i32 10) ]
  call void @y() [ "deopt"(i32 10), "unknown"(i8* null) ]
  ret void
}

define void @g() {
  call void @f() [ "deopt"(i32 20) ]
  ret void
}
```

会导致

```
define void @g() {
  call void @x()  ;; still no deopt state
  call void @y() [ "deopt"(i32 20, i32 10) ]
  call void @y() [ "deopt"(i32 20, i32 10), "unknown"(i8* null) ]
  ret void
}
```

前端的责任是以一种方式构造或编码去优化状态，在语法上将调用者的去优化状态前置于被调用者的去优化状态在语义上等同于在被调用者的去优化继续之后组成调用者的去优化继续。

#### Funclet Operand Bundles

Funclet操作数包以“funclet”操作数包标签为特征。 这些操作数捆绑表示呼叫站点位于特定的funclet内。 最多只有一个“funclet”操作数包附加到一个调用站点，它必须只有一个bundle操作数。

如果任何funclet EH pad已被“输入”但未“退出”（根据 [description in the EH doc](http://llvm.org/docs/ExceptionHandling.html#wineh-constraints)），则执行调用或调用哪个是未定义的行为：

- 没有“funclet”包，也不是对nounwind内在的调用，或者
- 有一个“funclet”包，其操作数不是最近进入的尚未退出的funclet EH pad。

类似地，如果没有输入funclet EH pad但尚未退出，则执行调用或使用“funclet”包调用是未定义的行为。

#### GC Transition Operand Bundles

GC转换操作数捆绑包的特征在于“gc-transition”操作数捆绑标记。 这些操作数包将一个调用标记为具有一个GC策略的函数与具有不同GC策略的函数之间的转换。 如果协调GC策略之间的转换需要在调用站点生成额外的代码，则这些包可能包含生成的代码所需的任何值。 有关更多详细信息，请参阅[GC Transitions](http://llvm.org/docs/Statepoints.html#gc-transition-args)。

### Module-Level Inline Assembly

模块可能包含“模块级内联asm”块，它对应于GCC“文件范围内联asm”块。 这些块由LLVM内部连接并作为单个单元处理，但如果需要，可以在.ll文件中分隔。 语法很简单：

```
module asm "inline asm code goes here"
module asm "more can go here"
```

字符串可以通过转义不可打印的字符来包含任何字符。 使用的转义序列只是“\ xx”，其中“xx”是数字的两位十六进制代码。

请注意，组装字符串必须由LLVM的集成汇编程序解析（除非它被禁用），即使在发出.s文件时也是如此。

### Data Layout

*数据排布*

模块可以指定目标特定数据布局字符串，其指定如何在存储器中布置数据。 数据布局的语法很简单：

```
target datalayout = "layout specification"
```

布局规范包含由减号字符（' - '）分隔的规范列表。 每个规范都以字母开头，并且可以在字母后面包含其他信息以定义数据布局的某些方面。 接受的规范如下：

`E`:指定目标以big-endian形式布局数据。也就是说，具有最重要性的位具有最低的地址位置。
`e`:指定目标以little-endian形式布局数据。也就是说，具有最小重要性的位具有最低的地址位置。
`S<size>`:以位为单位指定堆栈的自然对齐方式。堆栈变量的对齐促销仅限于自然堆栈对齐，以避免动态堆栈重新排列。堆栈对齐必须是8位的倍数。如果省略，自然堆栈对齐默认为“未指定”，这不会阻止任何对齐促销。
`P<address space>`:指定与程序存储器对应的地址空间。哈佛架构可以使用它来指定LLVM应该将诸如函数之类的东西放入的空间。如果省略，程序存储空间默认为默认地址空间0，这对应于在同一空间中具有代码和数据的Von Neumann体系结构。
`A<address space>`:指定'alloca'创建的对象的地址空间。默认为默认地址空间0。
`p[n]:<size>:<abi>:<pref>:<idx>`:这指定了指针的大小及其地址空间n的<abi>和<pref>错误对齐。第四个参数<idx>是用于地址计算的索引大小。如果未指定，则默认索引大小等于指针大小。所有尺寸均为位。地址空间n是可选的，如果未指定，则表示默认地址空间0.n的值必须在[1,2 ^ 23]范围内。
`i<size>:<abi>:<pref>`:这指定了给定位<size>的整数类型的对齐方式。 <size>的值必须在[1,2 ^ 23]范围内。
`v<size>:<abi>:<pref>`:这指定了给定位<size>的向量类型的对齐方式。
`f<size>:<abi>:<pref>`:这指定了给定位<size>的浮点类型的对齐方式。只有目标支持的<size>值才有效。所有目标都支持32（float）和64（double）;某些目标也支持80或128（不同口味的长双）。
`a:<abi>:<pref>`:这指定了聚合类型对象的对齐方式。
`m:<mangling>`:如果存在，则指定在输出中损坏llvm名称。带有修改转义字符\ 01的符号将直接传递给汇编程序，而不包含转义字符。修剪样式选项是

- e：ELF mangling：私有符号获得 `.L` 前缀。
- m：Mips mangling：私有符号获得 `$` 前缀。
- o：Mach-O mangling：私有符号获得 `L` 前缀。其他符号获得 `_` 前缀。
- x：Windows x86 COFF mangling：私有符号获得通常的前缀。常规C符号获得`_`前缀。带有`__stdcall`，`__ fastcall`和`__vectorcall`的函数具有附加`@N`的自定义修改，其中N是用于传递参数的字节数。 C++符号以`？`开头,没有任何损害。
- w：Windows COFF mangling：类似于x，但普通C符号不接收_前缀。

`n<size1>:<size2>:<size3>...`:它以位为单位指定目标CPU的一组本机整数宽度。 例如，它可能包含32位PowerPC的n32，PowerPC 64的n32：64或X86-64的n8：16：32：64。 该集合的元素被认为有效地支持大多数通用算术运算。
`ni:<address space0>:<address space1>:<address space2>...`:这将指定地址空间的指针类型指定为 [Non-Integral Pointer Type](http://llvm.org/docs/LangRef.html#nointptrtype)。 0地址空间不能指定为非整数。
在每个采用`<abi>：<pref>`的规范中，指定`<pref>`对齐是可选的。 如果省略，前面的：也应该省略，`<pref>`将等于`<abi>`。

在为给定目标构造数据布局时，LLVM以一组默认规范开始，然后（可能）被datalayout关键字中的规范覆盖。 此列表中给出了默认规范：

- `E` - big endian
- `p:64:64:64` - 64-bit pointers with 64-bit alignment.
- `p[n]:64:64:64` - Other address spaces are assumed to be the same as the default address space.
- `S0` - natural stack alignment is unspecified
- `i1:8:8` - i1 is 8-bit (byte) aligned
- `i8:8:8` - i8 is 8-bit (byte) aligned
- `i16:16:16` - i16 is 16-bit aligned
- `i32:32:32` - i32 is 32-bit aligned
- `i64:32:64` - i64 has ABI alignment of 32-bits but preferred alignment of 64-bits
- `f16:16:16` - half is 16-bit aligned
- `f32:32:32` - float is 32-bit aligned
- `f64:64:64` - double is 64-bit aligned
- `f128:128:128` - quad is 128-bit aligned
- `v64:64:64` - 64-bit vector is 64-bit aligned
- `v128:128:128` - 128-bit vector is 128-bit aligned
- `a:0:64` - aggregates are 64-bit aligned

当LLVM确定给定类型的对齐时，它使用以下规则：

​	1. 如果所寻求的类型与其中一个规范完全匹配，则使用该规范。
	2. 如果未找到匹配，并且所寻求的类型是整数类型，则使用大于所寻求类型的位宽的最小整数类型。 如果		没有规范大于位宽，则使用最大整数类型。 例如，根据上面的默认规范，i7类型将使用i8（下一个最大）的对齐，而i65和i256将使用i64的对齐（指定的最大值）。
	3. 如果没有找到匹配，并且所寻找的类型是矢量类型，那么小于所寻找的矢量类型的最大矢量类型将被用作后退。 这是因为`<128 x double>`可以用例如64` <2 x double>`来实现。

数据布局字符串的功能可能不是您所期望的。 值得注意的是，这不是来自代码生成器应该使用什么对齐的前端的规范。

相反，如果指定，则需要目标数据布局以匹配最终代码生成器期望的内容。 中级优化器使用此字符串来改进代码，这仅在与最终代码生成器使用的内容匹配时才有效。 没有办法生成没有将此特定于目标的细节嵌入IR的IR。 如果您未指定字符串，则默认规范将用于生成数据布局，优化阶段将相应地运行，并根据这些默认规范将目标特异性引入IR。

### Target Triple

模块可以指定描述目标主机的目标三元组串。 目标三元组的语法很简单：

```
target triple = "x86_64-apple-macosx10.7.0"
```

目标三元组字符串由一系列由减号字符（' - '）分隔的标识符组成。 规范形式是：

```
ARCHITECTURE-VENDOR-OPERATING_SYSTEM
ARCHITECTURE-VENDOR-OPERATING_SYSTEM-ENVIRONMENT
```

此信息将传递到后端，以便为正确的体系结构生成代码。 可以使用-mtriple命令行选项在命令行上覆盖它。

### Pointer Aliasing Rules

任何内存访问都必须通过与内存访问的地址范围相关联的指针值来完成，否则行为是未定义的。指针值根据以下规则与地址范围相关联：

- 指针值与与其所基于的任何值相关联的地址相关联。
- 全局变量的地址与变量存储的地址范围相关联。
- 分配指令的结果值与分配的存储的地址范围相关联。
- 默认地址空间中的空指针与无地址相关联。
- 除了零之外的整数常量或从未在LLVM内定义的函数返回的指针值可以与通过LLVM提供的机制之外的机制分配的地址范围相关联。此类范围不得与LLVM提供的机制分配的任何地址范围重叠。

根据以下规则，指针值基于另一个指针值：

- 由标量getelementptr操作形成的指针值基于getelementptr的指针类型操作数。
- 向量getelementptr操作的结果的通道1中的指针基于getelementptr的指向矢量类型的操作数的通道1中的指针。
- bitcast的结果值基于bitcast的操作数。
- 由inttoptr形成的指针值基于所有指针值（直接或间接地）贡献指针值的计算。
- “基于”关系是可传递的。

请注意，“基于”的这个定义有意类似于C99中“基于”的定义，尽管它略微弱一些。

LLVM IR不会将类型与内存关联。加载的结果类型仅指示要加载的内存的大小和对齐方式，以及值的解释。商店的第一个操作数类型同样仅表示商店的大小和对齐方式。

因此，基于类型的别名分析（又名TBAA，aka -fstrict-aliasing）不适用于一般的非修饰LLVM IR。元数据([Metadata](http://llvm.org/docs/LangRef.html#metadata))可以用于编码附加信息，专用优化通道可以使用该附加信息来实现基于类型的别名分析。

### Volatile Memory Accesses

某些内存访问（例如 [load](http://llvm.org/docs/LangRef.html#i-load)’s, [store](http://llvm.org/docs/LangRef.html#i-store)’s, and [llvm.memcpy](http://llvm.org/docs/LangRef.html#int-memcpy)’s）可能标记为`volatile`。 优化器不得更改易失性操作的数量或相对于其他易失性操作更改其执行顺序。 优化器可以相对于非易失性操作改变易失性操作的顺序。 这不是Java的“volatile”，也没有跨线程同步行为。

IR层的易失性加载和存储无法安全地优化为llvm.memcpy或llvm.memmove内在函数，即使这些内在函数被标记为volatile。 同样，后端不应该拆分或合并目标合法的易失性加载/存储指令。

> 解释:平台可以依赖于易失性加载和本机支持的数据宽度的存储作为单个指令执行。 例如，在C中，这适用于具有本机硬件支持的易失性原始类型的l值，但不一定适用于聚合类型。 前端支持这些期望，这些期望在IR中故意未指定。 上述规则确保IR转换不会违反前端与语言的契约。

### Memory Model for Concurrent Operations

LLVM IR没有定义任何启动并行执行线程或注册信号处理程序的方法。尽管如此，还是有特定于平台的方法来创建它们，我们定义了LLVM IR在其存在时的行为。此模型的灵感来自C++ 0x内存模型。

有关此模型的更为非正式的介绍，请参阅LLVM原子指令和并发指南( [LLVM Atomic Instructions and Concurrency Guide](http://llvm.org/docs/Atomics.html))。

我们将发生前(*happens-before*)的部分顺序定义为最不重要的部分顺序

- 是单线程程序的超集，并且
- 当与b同步时，包括从a到b的边。同步与对是由特定于平台的技术引入的，如pthread锁，线程创建，线程连接等，以及原子指令。 （另请参阅 [Atomic Memory Ordering Constraints](http://llvm.org/docs/LangRef.html#ordering)）。

请注意，程序顺序不会在线程和在该线程内执行的信号之间引入先前发生的边缘。

每个（定义的）读操作（加载指令，memcpy，原子加载/读 - 修改 - 写等）R读取由（定义的）写操作写的一系列字节（存储指令，原子存储/读 - 修改 - 写， memcpy等）。出于本节的目的，初始化的全局变量被认为具有初始化程序的写入，该初始化程序是原子的，并且在所讨论的存储器的任何其他读取或写入之前发生。对于读R的每个字节，Rbyte可能会看到对同一字节的任何写操作，除了：

- 如果write1在write2之前发生，而write2在Rbyte之前发生，则Rbyte不会看到write1。
- 如果Rbyte在write3之前发生，那么Rbyte不会看到write3。

鉴于该定义，Rbyte定义如下：

- 如果R是易失性的，则结果与目标有关。 （Volatile应该提供可以在C/C++中支持sig_atomic_t的保证，并且可以用于访问不像普通内存那样的地址。它通常不提供跨线程同步。）
- 否则，如果没有写入在Rbyte之前发生的相同字节，则Rbyte将返回该字节的undef。
- 否则，如果Rbyte可能只看到一次写入，则Rbyte返回该写入所写的值。
- 否则，如果R是原子的，并且Rbyte可能看到的所有写入都是原子的，则它会选择其中一个写入的值。有关如何进行选择的其他限制，请参阅“原子存储器排序约束( [Atomic Memory Ordering Constraints](http://llvm.org/docs/LangRef.html#ordering))”部分。
- 否则Rbyte会返回undef。

R返回由它读取的一系列字节组成的值。这意味着值中的某些字节可能是undef，而整个值都不是undef。注意，这只定义了操作的语义;这并不意味着目标将发出多条指令来读取一系列字节。

请注意，在没有使用任何原子内在函数的情况下，此模型在单线程执行所需的基础上仅对IR转换设置了一个限制：将存储引入到可能无法存储的字节中是不允许的一般。 （具体来说，在另一个线程可能写入和读取地址的情况下，引入存储可以更改一个负载，该负载可能只看到一个写入可能看到多次写入的负载。）

### Atomic Memory Ordering Constraints

原子指令（[cmpxchg](http://llvm.org/docs/LangRef.html#i-cmpxchg), [atomicrmw](http://llvm.org/docs/LangRef.html#i-atomicrmw), [fence](http://llvm.org/docs/LangRef.html#i-fence), [atomic load](http://llvm.org/docs/LangRef.html#i-load), 和 [atomic store](http://llvm.org/docs/LangRef.html#i-store)）采用排序参数来确定与它们同步的同一地址上的哪些其他原子指令。 这些语义是从Java和C ++ 0x借用的，但更加口语化。 如果这些描述不够精确，请检查这些规格（请参阅原子指南中的规范参考）。 围栏指令对这些排序的处理方式略有不同，因为它们没有地址。 有关详细信息，请参阅该说明的文档。

有关排序约束的简单介绍，请参阅 [LLVM Atomic Instructions and Concurrency Guide](http://llvm.org/docs/Atomics.html)。

`unordered`:可以读取的值集由发生在前的部分顺序控制。除非某些操作写入，否则无法读取值。这旨在提供足够强大的保证来模拟Java的非易失性共享变量。无法为读 - 修改 - 写操作指定此排序;它不够强大，不能以任何有趣的方式使它们成为原子。
`monotonic`:除了无序保证之外，每个地址的单调操作还有一个单一的总修改顺序。所有修改订单必须与之前发生的订单兼容。无法保证修改订单可以合并为整个程序的全局总订单（这通常是不可能的）。读取原子读取 - 修改 - 写入操作（[cmpxchg](http://llvm.org/docs/LangRef.html#i-cmpxchg) 和 [atomicrmw](http://llvm.org/docs/LangRef.html#i-atomicrmw)）会在写入值之前立即读取修改顺序中的值。如果在同一地址的另一个原子读取之前发生一次原子读取，则后一次读取必须在地址的修改顺序中看到相同的值或更晚的值。这不允许对同一地址上的单调（或更强）操作进行重新排序。如果地址是由一个线程单调编写的，而其他线程单调地反复读取该地址，则其他线程最终必须看到写入。这对应于C++0x/C1x memory_order_relaxed。
`acquire`:除了单调的保证之外，可以通过释放操作形成与边缘同步的边缘。这是为了模拟C++的memory_order_acquire。
`release`:除了单调的保证之外，如果此操作写入随后由获取操作读取的值，则它与该操作同步。 （这不是完整的描述;请参阅发布序列的C++ 0x定义。）这对应于C++0x/C1x memory_order_release。
`acq_rel`:（获得+发布）在其地址上充当获取和释放操作。这对应于C ++ 0x / C1x memory_order_acq_rel。
`seq_cst`:（顺序一致）:除了acq_rel的保证（获取只读取的操作，仅针对仅写入的操作的释放），所有地址上的所有顺序一致操作都有一个全局总命令，这与之前发生的部分一致订单以及所有受影响地址的修改订单。每个顺序一致的读取都会看到此全局顺序中最后一次写入同一地址。这对应于C++0x/C1x memory_order_seq_cst和Java volatile。

如果原子操作标记为syncscope（“singlethread”），则它仅与同一线程中运行的其他操作（例如，信号处理程序）中的seq_cst总排序同步并且仅参与其中。

如果原子操作被标记为syncscope（`“<target-scope>”`），其中`<target-scope>`是目标特定的同步范围，那么它与目标相关，如果它与其他操作的seq_cst总排序同步并参与其中。

否则，未标记为syncscope（“singlethread”）或syncscope（`“<target-scope>”`）的原子操作与未标记为syncscope（“singlethread”）或syncscope的其他操作的seq_cst总排序同步并参与其中（ “<目标范围>”）。

### Floating-Point Environment

默认的LLVM浮点环境假定浮点指令没有副作用。 结果假设为舍入到最接近的舍入模式。 在此环境中不保留浮点异常状态。 因此，不会尝试创建或保留无效操作（SNaN）或除零异常。

这种无异常假设的好处是可以自由地推测浮点运算，而不需要对浮点模型进行任何其他快速数学松弛。

需要与此不同的行为的代码应使用[Constrained Floating-Point Intrinsics](http://llvm.org/docs/LangRef.html#constrainedfp).。

### Fast-Math Flags

LLVM IR浮点运算（fadd，fsub，fmul，fdiv，frem，fcmp）和调用可以使用以下标志来启用其他不安全的浮点转换。

`nnan`:无NaN - 允许优化假设参数和结果不是NaN。如果参数是nan，或者结果是nan，则会产生毒性值。
`ninf`:No Infs - 允许优化假设参数和结果不是+/- Inf。如果参数为+/- Inf，或者结果为+/- Inf，则会产生毒性值。
`nsz`:No Signed Zeros - 允许优化将零参数或结果的符号视为无关紧要。
`arcp`:允许Reciprocal - 允许优化使用参数的倒数而不是执行除法。
`contract`:允许浮点收缩（例如融合乘法，然后加入融合乘法和加法）。
`afn`:近似函数 - 允许替换函数的近似计算（sin，log，sqrt等）。请参阅适用于LLVM的内部数学函数的位置的浮点内部定义。
`reassoc`:允许浮点指令的重新关联转换。这可能会显着改变浮点数的结果。
`fast`:这个标志暗示了所有其他标志。

### Use-list Order Directives

Use-list指令对每个use-list的内存顺序进行编码，允许重新创建订单。 <order-indexes>是以逗号分隔的索引列表，这些索引分配给引用值的用途。 引用值的use-list会立即按这些索引排序。

Use-list指令可能出现在函数作用域或全局作用域中。 它们不是指令，对IR的语义没有影响。 当它们处于功能范围时，它们必须出现在最终基本块的终结符之后。

如果基本块的地址是通过blockaddress（）表达式获取的，则uselistorder_bb可用于从其函数范围之外对其使用列表重新排序。

句法：

```
uselistorder <ty> <value>, { <order-indexes> }
uselistorder_bb @function, %block { <order-indexes> }
```

举例:

```
define void @foo(i32 %arg1, i32 %arg2) {
entry:
  ; ... instructions ...
bb:
  ; ... instructions ...

  ; At function scope.
  uselistorder i32 %arg1, { 1, 0, 2 }
  uselistorder label %bb, { 1, 0 }
}

; At global scope.
uselistorder i32* @global, { 1, 2, 0 }
uselistorder i32 7, { 1, 0 }
uselistorder i32 (i32) @bar, { 1, 0 }
uselistorder_bb @foo, %bb, { 5, 1, 3, 2, 0, 4 }
```

### Source Filename

源文件名字符串设置为原始模块标识符，例如，当从源代码通过clang前端编译时，它将是已编译源文件的名称。 然后通过IR和bitcode保存。

这对于为概要文件数据中使用的本地函数生成一致的唯一全局标识符是必要的，该标识符将源文件名添加到本地函数名。

源文件名的语法很简单：

```
source_filename = "/path/to/source.c"
```

## Type System

LLVM类型系统是中间表示的最重要特征之一。 通过键入可以直接对中间表示执行多个优化，而无需在转换之前对侧进行额外的分析。 强类型系统使得更容易读取生成的代码并实现在正常的三个地址代码表示上不可行的新分析和转换。

### Void Type

概述：void类型不代表任何值，也没有大小。

句法：

```
void
```

### Function Type

概述：

他的函数类型可以被认为是函数签名。 它由返回类型和形式参数类型列表组成。 函数类型的返回类型是void类型或第一类类型 -  [label](http://llvm.org/docs/LangRef.html#t-label) 和 [metadata](http://llvm.org/docs/LangRef.html#t-metadata)类型除外。

句法：

```
<returntype> (<parameter list>)
```

...其中`'<parameter list>'`是以逗号分隔的类型说明符列表。 可选地，参数列表可以包括类型...，其指示该函数采用可变数量的自变量。 可变参数函数可以使用处理内部函数的可变参数( [variable argument handling intrinsic](http://llvm.org/docs/LangRef.html#int-varargs) )来访问它们的参数。 `'<returntype>'`是除 [label](http://llvm.org/docs/LangRef.html#t-label) 和 [metadata](http://llvm.org/docs/LangRef.html#t-metadata)之外的任何类型。

| Examples:             |                                                              |
| --------------------- | ------------------------------------------------------------ |
| `i32 (i32)`           | 函数输入 `i32`, 返回 `i32`                                   |
| `float (i16, i32*) *` | 输入为 `i16` 和  `i32`的指针[pointer](http://llvm.org/docs/LangRef.html#t-pointer), 返回 `float`的函数指针[Pointer](http://llvm.org/docs/LangRef.html#t-pointer). |
| `i32 (i8*, ...)`      | 可变参数函数, 至少有一个输入为`i8`的指针 [pointer](http://llvm.org/docs/LangRef.html#t-pointer)  (char in C), 返回一个整数, 这在LLVM里是一个显式的 `printf` . |
| `{i32, i32}(i32)`     | 函数输入 `i32`, 返回一个结构 [structure](http://llvm.org/docs/LangRef.html#t-struct) 包含两个 `i32` values |

### First Class Types

[first class](http://llvm.org/docs/LangRef.html#t-firstclass) 类型可能是最重要的类型。 这些类型的值是唯一可以通过指令生成的值。

#### Single Value Types

从CodeGen的角度来看，这些是在寄存器中有效的类型。

##### Integer Type

概述：
整数类型是一种非常简单的类型，它只是为所需的整数类型指定任意位宽。 可以指定从1位到2的23次方-1（大约8百万）的任何位宽。

句法：

```
iN
```

整数占用的位数由N值指定。

Examples:

| `i1`       | a single-bit integer.                        |
| ---------- | -------------------------------------------- |
| `i32`      | a 32-bit integer.                            |
| `i1942652` | a really big integer of over 1 million bits. |

##### Floating-Point Types

| Type        | Description                                     |
| ----------- | ----------------------------------------------- |
| `half`      | 16-bit floating-point value                     |
| `float`     | 32-bit floating-point value                     |
| `double`    | 64-bit floating-point value                     |
| `fp128`     | 128-bit floating-point value (112-bit mantissa) |
| `x86_fp80`  | 80-bit floating-point value (X87)               |
| `ppc_fp128` | 128-bit floating-point value (two 64-bits)      |

##### X86_mmx Type

概述：
x86_mmx类型表示x86计算机上MMX寄存器中保存的值。 允许的操作非常有限：参数和返回值，加载和存储以及bitcast。 用户指定的MMX指令表示为带有参数和/或此类结果的内部或asm调用。 没有这种类型的数组，向量或常量。

句法：

```
x86_mmx
```

##### Pointer Type

概述：
指针类型用于指定内存位置。 指针通常用于引用内存中的对象。

指针类型可以具有可选的地址空间属性，该属性定义指向对象所在的编号地址空间。 默认地址空间为数字零。 非零地址空间的语义是特定于目标的。

请注意，LLVM不允许指向void（void *）的指针，也不允许指向标签的指针（label *）。 请改用i8 *。

句法：

```
<type> *
```

| Examples: |      |
| --------- | ---- |
| `[4 x i32]*`        | A [pointer](http://llvm.org/docs/LangRef.html#t-pointer) to [array](http://llvm.org/docs/LangRef.html#t-array) of four `i32` values. |
| `i32 (i32*) *`      | A [pointer](http://llvm.org/docs/LangRef.html#t-pointer) to a [function](http://llvm.org/docs/LangRef.html#t-function) that takes an `i32*`, returning an `i32`. |
| `i32 addrspace(5)*` | A [pointer](http://llvm.org/docs/LangRef.html#t-pointer) to an `i32` value that resides in address space #5. |

##### Vector Type

概述：
向量类型是表示元素向量的简单派生类型。 当使用单个指令（SIMD）并行操作多个基元数据时，使用矢量类型。 向量类型需要大小（元素数量）和基础原始数据类型。 矢量类型被认为是[first class](http://llvm.org/docs/LangRef.html#t-firstclass)。

句法：

```
< <# elements> x <elementtype> >
```

元素数是一个大于0的常数整数值; elementtype可以是任何整数，浮点或指针类型。 不允许使用大小为零的向量。

| Examples: |      |
| --------- | ---- |
| `<4 x i32>`   | Vector of 4 32-bit integer values.             |
| `<8 x float>` | Vector of 8 32-bit floating-point values.      |
| `<2 x i64>`   | Vector of 2 64-bit integer values.             |
| `<4 x i64*>`  | Vector of 4 pointers to 64-bit integer values. |

#### Label Type

概述：
标签类型代表代码标签。

句法：

```
label
```

#### Token Type

概述：
当值与指令相关联时使用令牌类型，但是值的所有使用都不得试图内省或模糊它。 因此，具有 [phi](http://llvm.org/docs/LangRef.html#i-phi) 或 [select](http://llvm.org/docs/LangRef.html#i-select)类型令牌是不合适的。

句法：

```
token
```

#### Metadata Type

概述：
元数据类型表示嵌入的元数据。 除了函数参数之外，不能从元数据创建派生类型。

句法：

```
metadata
```

#### Aggregate Types

聚合类型是可包含多个成员类型的派生类型的子集。  [Arrays](http://llvm.org/docs/LangRef.html#t-array) and [structs](http://llvm.org/docs/LangRef.html#t-struct)是聚合类型。 [Vectors](http://llvm.org/docs/LangRef.html#t-vector)不被视为聚合类型。

##### Array Type

概述：
数组类型是一种非常简单的派生类型，它在内存中按顺序排列元素。 数组类型需要大小（元素数）和基础数据类型。

句法：

```
[<# elements> x <elementtype>]
```

元素的数量是一个常数整数值; elementtype可以是任何具有大小的类型。

| Examples: |      |
| --------- | ---- |
| `[40 x i32]` | Array of 40 32-bit integer values. |
| `[41 x i32]` | Array of 41 32-bit integer values. |
| `[4 x i8]`   | Array of 4 8-bit integer values.   |

以下是多维数组的一些示例： 

| `[3 x [4 x i32]]`       | 3x4 array of 32-bit integer values.                    |
| ----------------------- | ------------------------------------------------------ |
| `[12 x [10 x float]]`   | 12x10 array of single precision floating-point values. |
| `[2 x [3 x [4 x i16]]]` | 2x3x4 array of 16-bit integer values.                  |

对静态类型隐含的数组末尾之外的索引没有限制（尽管在某些情况下索引超出了已分配对象的范围）。 这意味着可以在具有零长度数组类型的LLVM中实现单维“可变大小的数组”寻址。 例如，LLVM中“pascal样式数组”的实现可以使用类型“{i32，[0 x float]}”。

##### Structure Type

概述：
结构类型用于在内存中一起表示数据成员的集合。结构的元素可以是具有大小的任何类型。

通过使用'getelementptr'指令获取指向字段的指针，使用'load'和'store'访问内存中的结构。使用'extractvalue'和'insertvalue'指令访问寄存器中的结构。

结构可以可选地是“打包”结构，其指示结构的对齐是一个字节，并且元素之间没有填充。在非打包的结构中，字段类型之间的填充是按模块中的DataLayout字符串的定义插入的，这需要与底层代码生成器期望的内容相匹配。

结构可以是“文字”或“识别”。文本结构与其他类型（例如{i32，i32} *）内联定义，而识别的类型总是在顶层定义名称。文字类型的内容是唯一的，因为无法编写文字类型，所以它们永远不会是递归的或不透明的。已识别的类型可以是递归的，可以是不透明的，并且从不是唯一的。

句法：

```
%T1 = type { <type list> }     ; Identified normal struct type
%T2 = type <{ <type list> }>   ; Identified packed struct type
```

| Examples: |      |
| --------- | ---- |
| `{ i32, i32,i32 }`      | A triple of three `i32` values                               |
| `{ float, i32(i32) * }` | A pair, where the first element is a `float` and the second element is a [pointer](http://llvm.org/docs/LangRef.html#t-pointer) to a [function](http://llvm.org/docs/LangRef.html#t-function) that takes an `i32`, returning an `i32`. |
| `<{ i8, i32}>`          | A packed struct known to be 5 bytes in size.                 |

##### Opaque Structure Types

概述：
不透明结构类型用于表示未指定主体的命名结构类型。 这对应于（例如）前向声明结构的C概念。

句法：

```
%X = type opaque
%52 = type opaque
```

| Examples: |      |
| --------- | ---- |
| `opaque` | An opaque type. |

## Constants

LLVM有几种不同的基本类型的常量。 本节将介绍它们及其语法。

### Simple Constants

**Boolean constants** :两个字符串'true'和'false'都是i1类型的有效常量。
**Integer constants**: 标准整数（例如'4'）是整数( [integer](http://llvm.org/docs/LangRef.html#t-integer))类型的常量。 负数可以与整数类型一起使用。
**Floating-point constants** : 浮点常数使用标准十进制表示法（例如123.421），指数表示法（例如1.23421e + 2）或更精确的十六进制表示法（见下文）。 汇编程序需要浮点常量的精确十进制值。 例如，汇编程序接受1.25但拒绝1.3，因为1.3是二进制的重复小数。 浮点常量必须具有浮点([floating-point](http://llvm.org/docs/LangRef.html#t-floating))类型。
**Null pointer constants** : 标识符“null”被识别为空指针常量，并且必须是指针([pointer type](http://llvm.org/docs/LangRef.html#t-pointer))类型。
**Token constants** : 标识符“none”被识别为空令牌常量，并且必须是令牌([token type](http://llvm.org/docs/LangRef.html#t-token))类型。

常量的一个非直观表示法是浮点常量的十六进制形式。例如，形式'double 0x432ff973cafa8000'相当于（但更难以阅读）'double 4.5e + 15'。唯一需要十六进制浮点常数的时间（以及它们由反汇编程序生成的唯一时间）是必须发出浮点常量但是它不能表示为合理数量的十进制浮点数数字。例如，NaN，无穷大和其他特殊值以IEEE十六进制格式表示，因此汇编和反汇编不会导致常量中的任何位发生变化。

当使用十六进制形式时，half，float和double类型的常量使用上面显示的16位数字表示（匹配IEEE754表示为double）;但是，half和float值必须分别精确地表示为IEEE 754半精度和单精度。十六进制格式总是用于long double，并且有三种形式的long double。 x86使用的80位格式表示为0xK，后跟20个十六进制数字。 PowerPC使用的128位格式（两个相邻的双精度数）由0xM后跟32个十六进制数字表示。 IEEE 128位格式由0xL后跟32个十六进制数字表示。长双打仅在与目标上的长双精度格式匹配时才有效。 IEEE 16位格式（半精度）由0xH表示，后跟4个十六进制数字。所有十六进制格式都是big-endian（左边的符号位）。

没有类型x86_mmx的常量。

### Complex Constants

复数常量是简单常量和较小复数常量的（可能是递归的）组合。

**Structure constants** : 结构常量用类似于结构类型定义的表示法表示（以逗号分隔的元素列表，用大括号（{}）包围）。 例如：“{i32 4，float 17.0，i32 * @G}”，其中“@G”被声明为“@G =外部全局i32”。 结构常量必须具有结构类型( [structure type](http://llvm.org/docs/LangRef.html#t-struct))，并且元素的数量和类型必须与该类型指定的元素匹配。

**Array constants** : 数组常量用符号表示，类似于数组类型定义（以逗号分隔的元素列表，用方括号（[]）括起来）。 例如：“[i32 42，i32 11，i32 74]”。 数组常量必须具有数组类型([array type](http://llvm.org/docs/LangRef.html#t-array))，并且元素的数量和类型必须与该类型指定的元素相匹配。 作为一种特殊情况，字符数组常量也可以使用c前缀表示为双引号字符串。 例如：“c”Hello World\0A\00“”。

**Vector constants** : 向量常量用符号表示，类似于向量类型定义（以逗号分隔的元素列表，由小于/大于（<>）包围）。 例如：“<i32 42，i32 11，i32 74，i32 100>”。 向量常量必须具有向量类型( [vector type](http://llvm.org/docs/LangRef.html#t-vector))，并且元素的数量和类型必须与该类型指定的元素匹配。

**Zero initialization** : 字符串'zeroinitializer'可用于将任何类型的值初始化为零，包括标量和聚合类型。 这通常用于避免必须打印大的零初始化器（例如，对于大型阵列），并且总是完全等同于使用显式零初始化器。

**Metadata node** : 元数据节点是没有类型的常量元组。 例如：“！{！0，！{！2，！0}，！”test“}”。 元数据可以引用常量值，例如：“！{！0，i32 0，i8 * @global，i64（i64）* @function，！”str“}”。 与其他类型的常量不同，这些常量被解释为指令流的一部分，元数据是附加其他信息（如调试信息）的地方。

### Global Variable and Function Addresses

 [global variables](http://llvm.org/docs/LangRef.html#globalvars) and [functions](http://llvm.org/docs/LangRef.html#functionstructure)的地址始终是隐式有效的（链接时）常量。 当使用全局标识符( [identifier for the global](http://llvm.org/docs/LangRef.html#identifiers))并且始终具有指针( [pointer](http://llvm.org/docs/LangRef.html#t-pointer))类型时，将显式引用这些常量。 例如，以下是合法的LLVM文件：

```
@X = global i32 17
@Y = global i32 42
@Z = global [2 x i32*] [ i32* @X, i32* @Y ]
```

### Undefined Values

字符串'undef'可以在期望常量的任何地方使用，并指示值的用户可能会收到未指定的位模式。 未定义的值可以是任何类型（除'label'或'void'之外），并且可以在允许常量的任何地方使用。

未定义的值很有用，因为它们向编译器指示无论使用什么值，程序都已定义良好。 这为编译器提供了更多的优化自由。 以下是一些有效的（可能令人惊讶的）转换示例（在伪IR中）：

```
  %A = add %X, undef
  %B = sub %X, undef
  %C = xor %X, undef
Safe:
  %A = undef
  %B = undef
  %C = undef
```

这是安全的，因为所有输出位都受undef位的影响。 根据输入位，任何输出位都可以为零或一。

```
  %A = or %X, undef
  %B = and %X, undef
Safe:
  %A = -1
  %B = 0
Safe:
  %A = %X  ;; By choosing undef as 0
  %B = %X  ;; By choosing undef as -1
Unsafe:
  %A = undef
  %B = undef
```

这些逻辑运算具有不总是受输入影响的位。 例如，如果％X具有零位，则无论“undef”的相应位是什么，“和”操作的输出对于该位始终为零。 因此，优化或假设'和'的结果是'undef'是不安全的。 但是，可以安全地假设'undef'的所有位都可以为0，并将'和'优化为0.同样，可以安全地假设'undef'操作数的所有位都为'或' 可以设置，允许'或'折叠为-1。

```
  %A = select undef, %X, %Y
  %B = select undef, 42, %Y
  %C = select %X, %Y, undef
Safe:
  %A = %X     (or %Y)
  %B = 42     (or %Y)
  %C = %Y
Unsafe:
  %A = undef
  %B = undef
  %C = undef
```

这组示例显示未定义的'select'（和条件分支）条件可以采用任何一种方式，但它们必须来自两个操作数之一。 在％A示例中，如果已知％X和％Y都具有清零的低位，则％A必须具有清零的低位。 但是，在％C示例中，允许优化器假设'undef'操作数可以与％Y相同，从而允许消除整个'select'。

```
  %A = xor undef, undef

  %B = undef
  %C = xor %B, %B

  %D = undef
  %E = icmp slt %D, 4
  %F = icmp gte %D, 4

Safe:
  %A = undef
  %B = undef
  %C = undef
  %D = undef
  %E = undef
  %F = undef
```

这个例子指出两个'undef'操作数不一定相同。 这对于人们来说是令人惊讶的（并且也与C语义相匹配），他们认为“X ^ X”总是为零，即使X未定义。 出于多种原因，情况并非如此，但简短的回答是'undef'“变量”可以在其“有效范围”内任意改变其值。 这是真的，因为变量实际上没有实时范围。 相反，该值在逻辑上从任意寄存器中读取，这些寄存器恰好在需要时发生，因此该值不一定随时间变化。 事实上，％A和％C需要具有相同的语义，或者核心LLVM“替换所有用途”概念不会成立。

```
  %A = sdiv undef, %X
  %B = sdiv %X, undef
Safe:
  %A = 0
b: unreachable
```

这些示例显示了未定义值和未定义行为之间的关键差异。 未定义的值（如'undef'）允许具有任意位模式。 这意味着％A操作可以恒定折叠为'0'，因为'undef'可以为零，零除以任何值为零。 但是，在第二个例子中，我们可以做出更积极的假设：因为允许undef是一个任意值，我们可以假设它可以为零。 由于除以零具有未定义的行为，因此我们可以假设操作根本不执行。 这允许我们删除除以及之后的所有代码。 因为未定义的操作“不可能发生”，优化器可以假设它发生在死代码中。

```
a:  store undef -> %X
b:  store %X -> undef
Safe:
a: <deleted>
b: unreachable
```

可以假定未定义值的存储没有任何影响; 我们可以假设该值被恰好与已经存在的位匹配的位覆盖。 但是，存储到未定义的位置可能会破坏任意内存，因此，它具有未定义的行为。

### Poison Values

毒性值与undef值( [undef values](http://llvm.org/docs/LangRef.html#undefvalues))类似，但它们也表示不能引起副作用的指令或常量表达式仍然检测到导致未定义行为的条件。

目前无法在IR中表示毒性值;它们仅在通过 [add](http://llvm.org/docs/LangRef.html#i-add) nsw标志等操作生成时才存在。

毒性价值行为是根据价值依赖来定义的：

-  [phi](http://llvm.org/docs/LangRef.html#i-phi) 节点以外的值取决于它们的操作数。
-  [phi](http://llvm.org/docs/LangRef.html#i-phi)节点依赖于与其动态前驱基本块相对应的操作数。
- 函数参数取决于其函数的动态调用者中的相应实际参数值。
- [Call](http://llvm.org/docs/LangRef.html#i-call)指令取决于动态将控制权传回给它们的ret指令。
- 调用指令取决于动态将控制权返回给它们的[ret](http://llvm.org/docs/LangRef.html#i-ret) ，[resume](http://llvm.org/docs/LangRef.html#i-resume)或异常抛出调用指令。
- 非易失性加载和存储依赖于所有引用的内存地址的最新存储，遵循IR中的顺序（包括内在函数隐含的加载和存储，例如[@llvm.memcpy](http://llvm.org/docs/LangRef.html#int-memcpy)。）
- 具有外部可见副作用的指令取决于具有外部可见副作用的最近的前一指令，遵循IR中的顺序。 （这包括 [volatile operations](http://llvm.org/docs/LangRef.html#volatile)）
- 如果终结符指令具有多个后继指令，则指令控制依赖于 [terminator instruction](http://llvm.org/docs/LangRef.html#terminators)，并且当控制转移到其中一个后继指令时总是执行指令，并且当控制转移到另一个后续指令时可能不执行指令。
- 另外，如果终止符已将控制转移到不同的后继者，则指令还控制 - 取决于终结符指令，否则它依赖的指令集将是不同的。
- 依赖是可传递的。

毒性值与 [undef values](http://llvm.org/docs/LangRef.html#undefvalues)具有相同的行为，附加效果是任何依赖于毒性值的指令都具有未定义的行为。

这里有些例子：

```
entry:
  %poison = sub nuw i32 0, 1           ; Results in a poison value.
  %still_poison = and i32 %poison, 0   ; 0, but also poison.
  %poison_yet_again = getelementptr i32, i32* @h, i32 %still_poison
  store i32 0, i32* %poison_yet_again  ; memory at @h[0] is poisoned

  store i32 %poison, i32* @g           ; Poison value stored to memory.
  %poison2 = load i32, i32* @g         ; Poison value loaded back from memory.

  store volatile i32 %poison, i32* @g  ; External observation; undefined behavior.

  %narrowaddr = bitcast i32* @g to i16*
  %wideaddr = bitcast i32* @g to i64*
  %poison3 = load i16, i16* %narrowaddr ; Returns a poison value.
  %poison4 = load i64, i64* %wideaddr  ; Returns a poison value.

  %cmp = icmp slt i32 %poison, 0       ; Returns a poison value.
  br i1 %cmp, label %true, label %end  ; Branch to either destination.

true:
  store volatile i32 0, i32* @g        ; This is control-dependent on %cmp, so
                                       ; it has undefined behavior.
  br label %end

end:
  %p = phi i32 [ 0, %entry ], [ 1, %true ]
                                       ; Both edges into this PHI are
                                       ; control-dependent on %cmp, so this
                                       ; always results in a poison value.

  store volatile i32 0, i32* @g        ; This would depend on the store in %true
                                       ; if %cmp is true, or the store in %entry
                                       ; otherwise, so this is undefined behavior.

  br i1 %cmp, label %second_true, label %second_end
                                       ; The same branch again, but this time the
                                       ; true block doesn't have side effects.

second_true:
  ; No side effects!
  ret void

second_end:
  store volatile i32 0, i32* @g        ; This time, the instruction always depends
                                       ; on the store in %end. Also, it is
                                       ; control-equivalent to %end, so this is
                                       ; well-defined (ignoring earlier undefined
                                       ; behavior in this example).
```

### Addresses of Basic Blocks

blockaddress（@function，％block）

'blockaddress'常量计算指定函数中指定基本块的地址，并始终具有i8 *类型。 取入块的地址是非法的。

当用作 ‘[indirectbr](http://llvm.org/docs/LangRef.html#i-indirectbr)’指令的操作数或用于与null进行比较时，此值仅具有已定义的行为。 标签之间的指针相等性测试会导致未定义的行为 - 但是，再次，与null的比较是可以的，并且没有标签等于空指针。 只要不检查位，这可以作为不透明指针大小的值传递。 这允许对这些值执行ptrtoint和算术，只要在indirectbr指令之前重构原始值即可。

最后，当使用值作为内联汇编的操作数时，某些目标可能会提供定义的语义，但这是特定于目标的。

### Constant Expressions

常量表达式用于允许将涉及其他常量的表达式用作常量。 常量表达式可以是任何 [first class](http://llvm.org/docs/LangRef.html#t-firstclass) 类型，并且可以涉及没有副作用的任何LLVM操作（例如，不支持加载和调用）。 以下是常量表达式的语法：

`trunc (CST to TYPE)` : 对常量执行 [trunc operation](http://llvm.org/docs/LangRef.html#i-trunc)。

`zext (CST to TYPE)` : 对常量执行[zext operation](http://llvm.org/docs/LangRef.html#i-zext)。

`sext (CST to TYPE)` : 对常量执行[sext operation](http://llvm.org/docs/LangRef.html#i-sext)

`fptrunc (CST to TYPE) `: 将浮点常量截断为另一个浮点类型。 CST的大小必须大于TYPE的大小。 两种类型都必须是浮点数。

`fpext (CST to TYPE)` : 浮点将常量扩展为另一种类型。 CST的大小必须小于或等于TYPE的大小。 两种类型都必须是浮点数。

`fptoui (CST to TYPE)` : 将浮点常量转换为相应的无符号整数常量。 TYPE必须是标量或矢量整数类型。 CST必须是标量或矢量浮点类型。 CST和TYPE都必须是标量或具有相同数量元素的向量。 如果该值不适合整数类型，则结果为 [poison value](http://llvm.org/docs/LangRef.html#poisonvalues)。

`fptosi (CST to TYPE)` : 将浮点常量转换为相应的有符号整数常量。 TYPE必须是标量或矢量整数类型。 CST必须是标量或矢量浮点类型。 CST和TYPE都必须是标量或具有相同数量元素的向量。 如果该值不适合整数类型，则结果为 [poison value](http://llvm.org/docs/LangRef.html#poisonvalues)。

`uitofp (CST to TYPE)` : 将无符号整数常量转换为相应的浮点常量。 TYPE必须是标量或向量浮点类型。 CST必须是标量或矢量整数类型。 CST和TYPE都必须是标量或具有相同数量元素的向量。

`sitofp (CST to TYPE)` : 将有符号整数常量转换为相应的浮点常量。 TYPE必须是标量或向量浮点类型。 CST必须是标量或矢量整数类型。 CST和TYPE都必须是标量或具有相同数量元素的向量。

`ptrtoint (CST to TYPE)` : 对常量执行 [ptrtoint operation](http://llvm.org/docs/LangRef.html#i-ptrtoint)。

`inttoptr (CST to TYPE)` : 对常量执行[inttoptr operation](http://llvm.org/docs/LangRef.html#i-inttoptr)。 这个真的很危险！

`bitcast (CST to TYPE)` : 将常量CST转换为另一个TYPE。 操作数的约束与[bitcast instruction](http://llvm.org/docs/LangRef.html#i-bitcast)的约束相同。

`addrspacecast (CST to TYPE)` : 将指针的常量指针或常量向量CST转换为不同地址空间中的另一个TYPE。 操作数的约束与 [addrspacecast instruction](http://llvm.org/docs/LangRef.html#i-addrspacecast)的约束相同。

`getelementptr (TY, CSTPTR, IDX0, IDX1, ...)`, `getelementptr inbounds (TY, CSTPTR, IDX0, IDX1, ...)` : 对常量执行 [getelementptr operation](http://llvm.org/docs/LangRef.html#i-getelementptr)。 与 [getelementptr](http://llvm.org/docs/LangRef.html#i-getelementptr)指令一样，索引列表可以具有一个或多个索引，这些索引需要对“指向TY的指针”的类型有意义。

`select (COND, VAL1, VAL2)` : 对常量执行 [select operation](http://llvm.org/docs/LangRef.html#i-select)。

`icmp COND (VAL1, VAL2)` : 对常量执行 [icmp operation](http://llvm.org/docs/LangRef.html#i-icmp) 。

`fcmp COND (VAL1, VAL2)` : 对常量执行[fcmp operation](http://llvm.org/docs/LangRef.html#i-fcmp)。

`extractelement (VAL, IDX)` : 对常量执行 [extractelement operation](http://llvm.org/docs/LangRef.html#i-extractelement) 。

`insertelement (VAL, ELT, IDX)` : 对常量执行 [insertelement operation](http://llvm.org/docs/LangRef.html#i-insertelement)。

`shufflevector (VEC1, VEC2, IDXMASK)` : 对常量执行 [shufflevector operation](http://llvm.org/docs/LangRef.html#i-shufflevector) 。

`extractvalue (VAL, IDX0, IDX1, ...)` : 对常量执行 [extractvalue operation](http://llvm.org/docs/LangRef.html#i-extractvalue) 。 索引列表的解释方式与‘[getelementptr](http://llvm.org/docs/LangRef.html#i-getelementptr)’ 操作中的索引类似。 必须至少指定一个索引值。

`insertvalue (VAL, ELT, IDX0, IDX1, ...)` : 对常量执行[insertvalue operation](http://llvm.org/docs/LangRef.html#i-insertvalue)。 索引列表的解释方式与‘[getelementptr](http://llvm.org/docs/LangRef.html#i-getelementptr)’操作中的索引类似。 必须至少指定一个索引值。

`OPCODE (LHS, RHS)` : 执行LHS和RHS常量的指定操作。 OPCODE可以是[binary](http://llvm.org/docs/LangRef.html#binaryops) 或 [bitwise binary](http://llvm.org/docs/LangRef.html#bitwiseops)操作中的任何一种。 对操作数的约束与对应指令的约束相同（例如，不允许对浮点值进行逐位运算）。

## Other Values(NULL)

## Metadata

## Module Flags Metadata

## Automatic Linker Flags Named Metadata

## ThinLTO Summary

## Intrinsic Global Variables(指令全局变量)

## Instruction Reference(指令参考)

## Intrinsic Functions(内建函数)







