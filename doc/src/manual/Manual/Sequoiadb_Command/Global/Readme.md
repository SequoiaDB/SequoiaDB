Global 类作用于全局范围，包含的函数如下：

| 名称 | 描述 |
|------|------|
| [help()][help] |显示帮助信息 |
| [print()][print] |将输入内容输出到标准输出 |
| [sleep()][sleep] | 睡眠若干毫秒 |
| [forceGC()][forceGC] | 强制 javascript 引擎回收已经释放的对象资源 |
| [showClass()][showClass] | 列举 sdb shell 内置的自定义类及内置的自定义类包含的所有方法 |
| [getErr()][getErr] | 获取错误码的描述信息 |
| [getLastErrMsg()][getLastErrMsg] | 获取前一次操作的详细错误信息 |
| [getLastErrObj()][getLastErrObj] | 以 bson 对象的方式，返回前一次操作的详细错误信息 |
| [getLastError()][getLastError] | 获取前一次操作返回的错误码 |
| [setLastErrMsg()][setLastErrMsg] | 设置前一次操作的详细错误信息 |
| [setLastErrObj()][setLastErrObj] | 以 bson 对象的方式，设置前一次操作的详细错误信息 |
| [setLastError()][setLastError] | 设置前一次操作返回的错误码 |
| [getExePath()][getExePath] | 获取执行当前 js 脚本的程序的位置目录 |
| [getRootPath()][getRootPath] | 获取执行当前 js 脚本的程序的工作目录 |
| [getSelfPath()][getSelfPath] | 获取当前执行的 js 脚本的位置目录 |
| [import()][import] | 导入执行指定的 js 文件 |
| [importOnce()][importOnce] | 全局只导入执行一次指定的 js 文件 |
| [jsonFormat()][jsonFormat] | 设置格式化打印 BSON |
| [traceFmt()][traceFmt] | 将 trace 文件格式化为用户可读的内容并输出到指定文件 |

[^_^]:
     本文使用的所有引用及链接
[help]:manual/Manual/Sequoiadb_Command/Global/help.md
[print]:manual/Manual/Sequoiadb_Command/Global/print.md
[sleep]:manual/Manual/Sequoiadb_Command/Global/sleep.md
[forceGC]:manual/Manual/Sequoiadb_Command/Global/forceGC.md
[showClass]:manual/Manual/Sequoiadb_Command/Global/showClass.md
[getErr]:manual/Manual/Sequoiadb_Command/Global/getErr.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastErrObj]:manual/Manual/Sequoiadb_Command/Global/getLastErrObj.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[setLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/setLastErrMsg.md
[setLastErrObj]:manual/Manual/Sequoiadb_Command/Global/setLastErrObj.md
[setLastError]:manual/Manual/Sequoiadb_Command/Global/setLastError.md
[getExePath]:manual/Manual/Sequoiadb_Command/Global/getExePath.md
[getRootPath]:manual/Manual/Sequoiadb_Command/Global/getRootPath.md
[getSelfPath]:manual/Manual/Sequoiadb_Command/Global/getSelfPath.md
[import]:manual/Manual/Sequoiadb_Command/Global/import.md
[importOnce]:manual/Manual/Sequoiadb_Command/Global/importOnce.md
[jsonFormat]:manual/Manual/Sequoiadb_Command/Global/jsonFormat.md
[traceFmt]:manual/Manual/Sequoiadb_Command/Global/traceFmt.md