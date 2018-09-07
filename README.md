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





