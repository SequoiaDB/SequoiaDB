/*******************************************************************************
@description 生成 basic_operation 目录下除 Error.js 文件之外的所有包装类（包括 commlib）
            1. 使用 showClassfull 得到所有内置对象和全局方法
            2. 使用 showClassfull("内置对象") 打印出内置对象所有的静态方法和成员方法
            3. 往文件中写声明和封装方法语句（装饰器模式），当内置类被封装，其内所有方法丢失，所以内置类最早封装
            4. 生成 commlib.js 并提供 Sdb 实例
            5. 当前方法内嵌进 all_prepare.js，调用 runtest.sh 会执行一次，也可直接使用 sdb -f generateFiles.js 生成文件
@author  lyy
@time  2020-09-25
*******************************************************************************/
generateFiles();

function generateFiles ()
{
   // 分出内置类和全局方法
   // 内置类：str[0]
   // 全局方法：str[1];
   var str = showClassfull().split( ":\n" ).map( function( classAndFunc ) { return classAndFunc.split( "\n" ).slice( 0, -1 ).map( function( s ) { return s.trim() } ); } ).slice( 1 );
   var commFileContent = 'import( "./Error.js" );' + "\n";

   // 封装内置类
   var classes = str[0];
   for( var i = 0; i < classes.length; i++ )
   {
      // 获取静态方法和成员方法
      var staicFunc = showClassfull( classes[i] ).split( "static functions:\n" )[1].split( "'s member functions" )[0].split( "\n" ).slice( 0, -1 ).map( function( s ) { return s.trim().slice( 0, -2 ) } );
      var memFunc = showClassfull( classes[i] ).split( "member functions:\n" )[1].split( "\n" ).slice( 0, -1 ).map( function( s ) { return s.trim().slice( 0, -2 ) } );

      var fileContent = "";

      /* e.g:
         if ( tmpSdb == undefined )
         {
            var tmpSdb = {
             dropCS: Sdb.prototype.dropCS,
             createCS: Sdb.prototype.createCS
          };
         }
      */
      var varStr = "if ( tmp" + classes[i] + " == undefined )\n";
      varStr += "{\n";
      varStr += "   var tmp" + classes[i] + " = {";
      for( var j = 0; j < memFunc.length; j++ )
      {
         varStr += "\n      " + memFunc[j] + ": " + classes[i] + ".prototype." + memFunc[j];
         if( j != memFunc.length - 1 ) { varStr += ","; } else { varStr += "\n   };"; }
      }
      varStr += "\n}"
      fileContent += varStr + "\n";

      /* e.g:
         var funcSdb = (funcSdb == undefined) ? Sdb : funcSdb;
      */
      var varStr = "var func" + classes[i] + " = ( func" + classes[i] + " == undefined ) ? " + classes[i] + " : func" + classes[i] + ";";
      fileContent += varStr + "\n";

      /* e.g:
            var funcSdbhelp = funcSdb.help;
      */
      for( var j = 0; j < staicFunc.length; j++ )
      {
         var varStr = "var func" + classes[i] + staicFunc[j] + " = func" + classes[i] + "." + staicFunc[j] + ";";
         fileContent += varStr + "\n";
      }

      /* e.g:
          Sdb=function(){try{return funcSdb.apply( this, arguments ); } catch( e ) {  throw new Error(e) } };
      */
      var evalStr = classes[i] + "=function(){try{return func" + classes[i] + ".apply( this, arguments ); } catch( e ) { throw new Error(e) } };";
      fileContent += evalStr + "\n";

      /* e.g:
         Sdb.help = function(){try{ return funcSdbhelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
      */
      for( var j = 0; j < staicFunc.length; j++ )
      {
         var evalStr = classes[i] + "." + staicFunc[j] + " = function(){" +
            "try{ return func" + classes[i] + staicFunc[j] + ".apply( this, arguments ); } catch( e ) { throw new Error(e) } };";
         fileContent += evalStr + "\n";
      }

      /* e.g:
        Sdb.prototype.close=function(){try{return tmpSdb.close.apply(this,arguments);}catch(e){ throw new Error(e);}};
      */
      for( var j = 0; j < memFunc.length; j++ )
      {
         var evalStr = classes[i] + ".prototype." + memFunc[j] + "=function(){try{return tmp" + classes[i] + "." + memFunc[j]
            + ".apply(this,arguments);}catch(e){throw new Error(e);}};";
         fileContent += evalStr + "\n";
      }

      var fileName = classes[i] + ".js";
      compareAndSave( fileName, fileContent );

      commFileContent += 'import( "./' + fileName + '" );' + "\n";
   }

   // 封装全局方法
   var funcs = str[1].filter( function( s ) { if( s.indexOf( "import" ) == -1 ) { return true; } } ).map( function( s ) { return s.trim().slice( 0, -2 ) } );

   var fileContent = "";

   /* e.g:
       var tmpGlobal = {
          catPath: catPath,
          displayManual: displayManual,
       }
   */
   var varStr = "var tmpGlobal = {";
   for( var i = 0; i < funcs.length; i++ )
   {
      varStr += "\n   " + funcs[i] + ": " + funcs[i];
      if( i != funcs.length - 1 ) { varStr += ","; } else { varStr += "\n};"; }
   }
   fileContent += varStr + "\n";

   /* e.g:
     sleep=function(){try{return tmpGlobal.sleep.apply(this,arguments);}catch(e){ throw new Error(e)}};
   */
   for( var i = 0; i < funcs.length; i++ )
   {
      var evalStr = funcs[i] + "=function(){try{return tmpGlobal." + funcs[i] + ".apply(this,arguments);}"
         + "catch(e){ throw new Error(e)}};";
      fileContent += evalStr + "\n";
   }

   commFileContent += 'import( "./Global.js" );' + "\n";
   commFileContent += 'var db = new Sdb(db);' + "\n";

   compareAndSave( "commlib.js", commFileContent );

   compareAndSave( "Global.js", fileContent );
}

function compareAndSave ( fileName, fileContent )
{
   var filePath = getSelfPath() + "/basic_operation/" + fileName;
   var file = new File( filePath );
   println( "生成文件" + filePath );
   File.chmod( filePath, 0644, false );
   file.truncate();
   file.write( fileContent );
   file.close();
}
