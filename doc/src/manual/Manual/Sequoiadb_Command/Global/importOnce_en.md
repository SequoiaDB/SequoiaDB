
##NAME##

importOnce - Global only import and eval js file once.

##SYNOPSIS##

**importOnce(\<filename\>)**

##CATEGORY##

Global

##DESCRIPTION##

Meets the demand to import existing js files.

**Note:**

1. In a js script, if the imported file and the script 
   have multiple definitions for the same function,
   then if you first import the file and then define 
   the same function in the script,the actual function 
   definition will be the function defined in the import 
   file.The reason for this phenomenon is that js engine 
   will read the function definition of the js script 
   before running,and the function definition in the 
   imported file is read when the import method is run,
   this lead to the effective function definition is the 
   function definition in the imported file.By placing 
   the function definition in the script into another 
   file and then importing it,you can circumvent this problem.

##PARAMETERS##

* `filename` ( *String*， *Required* )

   The relative path or the full path of js file。

##RETURN VALUE##

On success, importOnce() returns the value of the imported file.

On error, exception will be thrown.

##ERRORS##

| Error code | Error type | Description | solution |
| ------ | --- | ------ | ------ |
| -152 | SDB_SPT_EVAL_FAIL | Evalution failed with error| Debug by error line number	|

When exception happen, use getLastError() to get the error code and use getLastErrMsg() to get error message.  For more detial, please reference to Troubleshooting.

##HISTORY##

Since v2.9

##EXAMPLES##

1. Import and eval helloWorld.js repeatly,only import and eval for the first time.

    1) The content of helloWorld.js as below:

    ```lang-javascript
    function sayHello()
    {
        println( "hello world" ) ;
    }
    println( "import helloWorld.js" ) ;
    ```

    2) Import and eval helloWorld.js repeatly,only import and eval for the first time.

	```lang-javascript
	> importOnce( 'helloWorld.js' )
    import helloWorld.js
    Takes 0.000849s.
    > importOnce( 'helloWorld.js' )
    Takes 0.000354s.
    > sayHello()
    hello world
    Takes 0.000436s.
 	```

2. The problem of repeated function definition and the way to avoid problem
   
    * Problem description and example

        1) The content of funcDef.js as below:

        ```lang-javascript
        function test()
        {
            println( "defined in funcDef.js" ) ;
        }  
        ```

        2) The content of test.js as below：

        ```lang-javascript
        importOnce( './funcDef.js' ) ;
        function test()
        {
            println( 'defined in test.js' ) ;
        }
        test() ;
        ```

        3) Use sdb to run test.js

        ```lang-javascript
        $ ./sdb -f test.js 
        defined in funcDef.js
        ```

        The result means the effective function definition is the function definition in the funcDef.js.

    * The way to avoid problem

        By placing the function definition in the script into another file and then importing it,you can circumvent this problem.

        1) Create a file named userDef.js,the content as below：

        ```lang-javascript
        function test()
        {
            println( 'defined in userDef.js' ) ;
        }
        ```

        2) The content of test.js is changed as below：

        ```lang-javascript
        importOnce( './funcDef.js' ) ;
        importOnce( './userDef.js' ) ;
        test() ;
        ```

        3) Use sdb to run test.js

        ```lang-javascript
        $ ./sdb -f test.js
        defined in userDef.js
        ```

        The result means the effective function definition is the function definition in the userDef.js.