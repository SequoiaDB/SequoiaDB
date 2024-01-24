工程概述
=================
testcase_new 是 sequoiadb 的测试工程，包含 sequoiadb engine 基本功能测试、
性能测试、可靠性测试，sequoiadb driver 测试

目录说明
=================
```sh
├── story               story 功能对应的测试用例 
│
├── driver              驱动相关测试用例，包括 C、C++、java、Python、php 用例      
│ 
├── sdv                 sdv 是迭代测试，每个迭代划分为多个 story，当 story 完成后会进行 sdv 测试                      
│  
├── performance         性能相关测试用例
│  
├── relibbility         可靠性相关测试用例
│  
├── om                  SAC 相关测试用例                                                                            
│                                                                                                                                                                                                                                                                                                                                                                                                                                  
├── tdd                 开发人员的测试用例
│ 
├── tools               测试项目所需的工具和依赖             
│ 
└── valgrind autotest   内存泄漏相关测试用例
```

用例的详细说明
=================

story 用例
=================
story 目录下的用例用于测试 Sequoiadb 的基本功能，如增删改查、事务、lob 等。根据用例的实现方式，该目录又分为如下两种用例：

js，简单、轻量的测试用例  
java，复杂、多并发的测试用例

story/js 用例的运行方式
-----------------
通过 runtest.sh 执行 js 用例，runtest.sh 位于 sequoiadb 工程的根目录下。例：
```lang-bash
$ ./runtest.sh -p transaction  -n 11810 -h localhost -c 11800 -print -t story
```

story/java 用例的运行方式
-----------------
java 用例通过 mvn 命令执行，执行时可以指定不同的用例配置文件，具体如下：

　1.　testng.xml    普通测试用例的配置文件  
　2.　sync.xml      同步测试用例的配置文件  
　3.　configure.xml 配置类测试用例的配置文件  
　4.　largedata.xml 大数据测试用例的配置文件  
　5.　smoke.xml     冒烟测试的配置文件，暂未使用

java 用例的具体执行方式：

　1.　自行下载 java 驱动的 jar 包  
　2.　修改用例配置文件，以 testng.xml 为例
```lang-bash
$ cd testcase_new/story/java/
$ vim testng.xml
```
　3.　使用 mvn 执行用例
```lang-bash
$ mvn surefire-report:report -DxmlFileName=< TESTCASE_XMLFILE > -Dsdbdriver=< JAVA_DRIVER_VERSION > -DsdbdriverDir=< JAVA_DRIVER_PATH > -DthreadexecutorDir=testcase_new/tools/ -DreportDir=testcase_new/story/java/report -DHOSTNAME=localhost -DSVCNAME=11810
```
　mvn 命令参数说明
```lang-bash
参数说明：  
-DxmlFileName：          指定要运行的用例配置文件  
-Dsdbdriver：            指定 java 驱动的版本，如 3.4.4-SNAPSHOT  
-DsdbdriverDir：         指定驱动所在目录  
-DthreadexecutorDir：    指定所需线程池依赖路径  
-DreportDir：            指定执行生成的日志的存放路                          
-DHOSTNAME：             指定要连接的SDB地址  
-DSVCNAME：              指定对应SDB的协调端口
```

sdv 用例
=================
sdv 目录下的用例用于测试 Sequoiadb 的各种特性，如数据同步、事务、数据分区等，该目录又分为如下两种用例：

1.　js，简单、轻量的测试用例  
2.　java，复杂、多并发的测试用例

用例的执行方式可以参考 story 目录下的用例

driver  用例
=================
driver 用例用于测试各种驱动的功能，包括CSharp、C、C++、java、php、python 驱动

driver/java 目录下的用例
-----------------
该目录下的用例用于测试 java 驱动的功能，可以通过指定不同的用例配置文件，来运行该目录下不同特性的用例，具体的配置文件如下：

1.　testng.xml    普通测试用例的配置文件  
2.　sync.xml      同步测试用例的配置文件  
3.　configure.xml 配置类测试用例的配置文件  
4.　largedata.xml 大数据测试用例的配置文件  
5.　smoke.xml     冒烟测试的配置文件，暂未使用　　

用例的运行方式可参考 story/java 的用例运行方式

driver/C# 目录下的用例
-----------------
该用例目录下的用例用于测试 C# 驱动的功能，用例的执行方式如下：

　1.　自行下载 C# 驱动库（C# 只能在 windows 下运行）  
　2.　使用 VS2010 打开 C# 测试用例工程  
　3.　C# 用例工程导入 C# 驱动库  
　4.　修改 Config.xml 中的用例配置  
　5.　运行用例  

driver/c 目录下的用例
-----------------
该目录下的用例用于测试 C 驱动的功能，用例执行方式如下：  
　1.　下载 C 驱动  
　2.　编译 C 驱动用例
```lang-bash
$ export LD_LIBRARY_PATH=< C_DRIVER_PATH >/sdbdriver/lib
$ cd testcase/driver/c
$ chmod +X runC.sh  
$ ./runC.sh -noup
```
如果不想设置 LD_LIBRARY_PATH，可以通过 scons 编译 C 驱动用例。--dd 参数表示编译 debug 版
```lang-bash
$ scons --include_path=< C_DRIVER_INCLUDE_PATH > --lib_path=< C_DRIVER_LIB_PATH > --dd  
```
　3.　执行 C 驱动用例，可自行设置执行参数
```lang-bash
$ ./runC.sh -noup -nocompile -test -hostname localhost -svcname 11810
```

driver/cpp 目录下的用例
-----------------
该目录下的用例用于测试 CPP 驱动的功能，用例执行方式如下：  

　1.　下载 CPP 驱动  
　2.　编译 CPP 驱动用例，编译方式可参看 driver/c 用例的编译方式  
　3.　执行 CPP 驱动用例，可自行设置执行参数  
```lang-bash
$ ./runCPP.sh-noup -nocompile -test -hostname localhost -svcname 11810
```

driver/php 目录下的用例
-----------------
　1.　下载 php 驱动  
　2.　进入 php 文件夹  
```lang-bash
$ cd driver/php
```
　3.　根据注释修改 phpunit.xml
```lang-bash
$ vim phpunit.xml
```
　4.　runPhp.sh 执行一个文件夹内所有用例:
```lang-bash
$ ./runPhp.sh sdb
```
driver/python 目录下的用例
-----------------
该目录下的用例用于测试 python 驱动的功能，用例执行方式如下：  

　1.　环境准备，安装 XMLRunner 对应的插件及依赖包  
　2.　下载、安装 python 驱动，如果需要使用 python3 执行用例，则需要额外安装 python3 对应的驱动  
　3.　进入 python 驱动用例目录  
```lang-bash
$ cd driver/python
```
　4.　修改用例配置文件 config.json
```lang-bash
$ vim config.json
```
　5.　执行所有测试用例
```lang-bash
$ python run_all_case.py
```
　6.　执行指定目录下的所有用例，以 meteData 目录为例
```lang-bash
$ python -m unittest discover -s "meteData"
```
　7.　执行指定文件下的用例，以 test_node_12499_12500.py 为例
```lang-bash
$ python -m unittest discover clustermanager "test_node_12499_12500.py"
```
　8.　python3 执行用例的方式与 python 相同，如
```lang-bash
$ python3 run_all_case.py
```
om 测试用例
=================
om 测试用例为sac的临时测试用例，现手动执行，已不再维护

performance 用例
=================
参考 performance 下的步骤说明文档

reliability 用例
=================
可靠性测试用例，参考 story/java 用例的运行方式 一节运行用例