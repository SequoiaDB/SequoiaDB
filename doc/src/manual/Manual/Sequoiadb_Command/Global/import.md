
##名称##

import - 导入执行指定的 js 文件。

##语法##

**import(\<filename\>)**

##类别##

Global

##描述##

在编写新的js脚本时存在重用现有脚本的可能性。可通过该命令将 js 文件导入并执行。


**Note:**  

1. 如果import()嵌套导入同个文件多次，会跳过后续的文件导入。  
2. 在一段js脚本中，如果导入的文件和该脚本对同个函数有多个定义，那么
   在先导入文件再在脚本中定义相同函数的场景下，实际生效的函数定义将
   会是导入文件中的函数定义。导致这个现象的原因是js运行前会先读取该
   段js脚本的函数定义，而导入的文件中的函数定义是在运行import方法时
   才读取的，这导致了最终生效的是导入文件中的函数定义。通过将脚本中
   的函数放置到别的文件中再导入，可以规避这个问题。

##参数##

* `filename` ( *String*， *必填* )

	js文件的相对路径名或全路径名。


##返回值##

成功：导入文件的返回值。

失败：抛出异常。

##错误##

`import()`函数常见异常如下：

| 错误码 | 错误类型 | 可能的原因 | 解决方法 |
| ------ | --- | ------ | ------ |
| -152 | SDB_SPT_EVAL_FAIL | 导入的文件执行失败| 根据错误信息提供的行号解决问题	|

当异常抛出时，可以通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取[错误码](manual/Manual/Sequoiadb_error_code.md)，
或通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息。
可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)了解更多内容。

##版本##

v2.9及以上版本。

##示例##

1. 导入执行 helloWorld.js 文件

    1) helloWorld.js 内容如下：

    ```lang-javascript
    function sayHello()
    {
        println( "hello world" ) ;
    }
    println( "import helloWorld.js" ) ;
    ```

	2) 导入执行 helloWorld.js 并调用定义的方法

    ```lang-javascript
	> import( 'helloWorld.js' )
    import helloWorld.js
    Takes 0.000901s.
    > sayHello()
    hello world
    Takes 0.000475s.
 	```

2. 函数重复定义问题及规避方法
   
    * 问题描述举例

        1) funcDef.js 内容如下：

        ```lang-javascript
        function test()
        {
            println( "defined in funcDef.js" ) ;
        }  
        ```

        2) test.js 内容如下：

        ```lang-javascript
        import( './funcDef.js' ) ;
        function test()
        {
            println( 'defined in test.js' ) ;
        }
        test() ;
        ```

        3) 使用 sdb 执行 test.js 文件

        ```lang-bash
        $ ./sdb -f test.js 
        defined in funcDef.js
        ```

        可以发现，实际生效的是 funcDef.js 中的函数定义。

    * 规避方法

        可以通过将脚本中的函数定义放置到单独的文件再导入来规避这个问题

        1) 增加文件 userDef.js,内容如下：

        ```lang-javascript
        function test()
        {
            println( 'defined in userDef.js' ) ;
        }
        ```

        2) test.js 内容改为：

        ```lang-javascript
        import( './funcDef.js' ) ;
        import( './userDef.js' ) ;
        test() ;
        ```

        3) 使用 sdb 执行 test.js 文件

        ```lang-bash
        $ ./sdb -f test.js 
        defined in userDef.js
        ```

        可以发现，实际生效的是 userDef.js 中的函数定义。  