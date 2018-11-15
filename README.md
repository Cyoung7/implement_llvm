# implement_llvm

环境:ubuntu16.04 , Jetbrain Clion IDE

网上的入门教程很多,[@snsn1984](https://blog.csdn.net/snsn1984/article/details/81283070),但是没有一个手把手上手实践的教程,最近几天倒腾llvm,创建了第一个实践项目,下面就将把这个过程梳理下来,供小白参考

## 1.下载源码,编译

[llvm官方文档](http://llvm.org/docs/GettingStarted.html) [clang官方文档](http://clang.llvm.org/get_started.html),比较详细,但是太冗长,下边给出简单方法:

### git clone源码:

```shell
cd youLLVM_Path/
#llvm源码
git clone https://git.llvm.org/git/llvm.git/
cd llvm/tools
#clang源码,放在llvm/tools下
git clone https://git.llvm.org/git/compiler-rt.git/
cd ../projects
#compiler-rt源码,放在llvm/projects下
git clone https://git.llvm.org/git/openmp.git/
```

### 编译源码

```shell
cd ..
#来到llvm root目录下
mkdir build
cd build
cmake -G "Unix Makefiles" ..
make -j4
```

现在源码llvm已经来到8.0.0版本,以上编译默认debug模式,编译完成需要50G存储空间,所以编译之前需要保证磁盘空间充足,加上后面安装需要的35G,所以需要为llvm保留100G左右的磁盘空间

## 2.安装

由于之前安装tvm时的需要,系统本身已经安装了llvm6.0,[安装方式](https://apt.llvm.org/):

```shell
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key|sudo apt-key add -
# Fingerprint: 6084 F3CF 814B 57C1 CF12 EFD5 15CF 4D18 AF4F 7421 
sudo apt-get install clang-6.0 lldb-6.0 lld-6.0
```

如果想将以上编译结果直接安装到系统中,很简单,但是不建议,因为消耗的系统空间太大

```shell
#不建议!!!
# build 目录下:
sudo make install
```

于是乎,想将源码安装在指定目录,为这一步,一个小白倒腾了整整一天,最后还是在[官网](http://llvm.org/docs/CMake.html)找到了答案:

现在假设安装在llvm root目录下的build_install下:

```shell
cd llvmRootPath/
#创建安装目录
mkdir build_install
cd build
#安装到指定目录
cmake -DCMAKE_INSTALL_PREFIX=/llvmRootPath/build_install -P cmake_install.cmake
```

此时已经将llvm安装到build_install下

## 3.创建第一个llvm项目

打开clion,新建项目,项目名: implement_llvm

如何在CMakeList.txt中引用项目,答案还是在[官网](http://llvm.org/docs/CMake.html#embedding-llvm-in-your-project)

```cmake
cmake_minimum_required(VERSION 3.4.3)
project(SimpleProject)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set your project compile flags.
# E.g. if using the C++ header files
# you will need to enable C++11 support
# for your compiler.

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Now build our tools
add_executable(simple-tool tool.cpp)

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs support core irreader)

# Link against LLVM libraries
target_link_libraries(simple-tool ${llvm_libs})

```

但是,重点来了,由于之前系统安装了llvm6.0,cmake会优先在系统中找到llvm6.0,如何调用以上源码安装库,一个小白又倒腾了半天

官网一句话:The `find_package(...)` directive when used in CONFIG mode (as in the above example) will look for the `LLVMConfig.cmake` file in various locations (see cmake manual for details),It creates a `LLVM_DIR` cache entry to save the directory where `LLVMConfig.cmake` is found 

大意是说:find_package采用config模式,会根据LLVM_DIR缓存条目(cache entry)来寻找`LLVMConfig.cmake` ,这个缓存条目是个什么玩意儿???懵逼

将环境变量中加入`export LLVM_DIR=/llvmRootPath/build_install`,等等一系列措施,最后找到了方法

在CMakeList.txt里加上一句话

```cmake
cmake_minimum_required(VERSION 3.4.3)
project(SimpleProject)

#加上一句,这是config模式来寻找LLVMConfig.cmake的路径,添加即可
set(LLVM_DIR /llvmRootPath/build_install/lib/cmake/llvm)

find_package(LLVM REQUIRED CONFIG)
#-----后面省略-----
```

于是乎,cmake就可以快乐的找到llvm8.0.0了,第一个项目创建完成

踩坑到此为止,后面利用此项目来学习llvm.欢迎交流.

## 4.[llvm tutotial](https://llvm.org/docs/tutorial/index.html#kaleidoscope-implementing-a-language-with-llvm)

利用google翻译的[中文文档](https://github.com/Cyoung7/implement_llvm/blob/master/doc/Kaleidoscope%20Tutorial.md)(待校对)

## 5.[LLVM Programmer’s Manual](https://llvm.org/docs/ProgrammersManual.html)

llvm编程手册，利用google翻译的[中文文档](https://github.com/Cyoung7/implement_llvm/blob/master/doc/LLVM%20Programmer%E2%80%99s%20Manual.md)(待校对)

## 6.[LLVM Language Reference Manual](https://llvm.org/docs/LangRef.html)

llvm IR手册，利用google翻译的[中文文档](https://github.com/Cyoung7/implement_llvm/blob/master/doc/LLVM%20Language%20Reference%20Manual.md)(待校对)

