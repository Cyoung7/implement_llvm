# LLVM Language Reference Manual

[原文链接](http://llvm.org/docs/LangRef.html)

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

### Calling Conventions(调用公约)(null)

## Type System

## Constants

## Other Values

## Metadata

## Module Flags Metadata

## Automatic Linker Flags Named Metadata

## ThinLTO Summary

## Intrinsic Global Variables(指令全局变量)

## Instruction Reference(指令参考)

## Intrinsic Functions(内建函数)







