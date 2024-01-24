/***********************************************************************
*@Description : test import importOnce js file with postfix not .js
*               seqDB-12215:使用import importOnce导入后缀名不为.js的js文件
*@author      : Liang XueWang 
***********************************************************************/
// js file to import importOnce with postfix not .js
var importPostfixFile = WORKDIR + "/importPostfix_12215.txt";
var importOncePostfixFile = WORKDIR + "/importOncePostfix_12215.csv";

var a = 0;
var b = 0;
main( test );

function test ()
{
   // create file
   createPostfixFile();

   // import importOnce file

   import( importPostfixFile );
   importOnce( importOncePostfixFile );

   if( a !== 1 || b !== 1 )
   {
      throw new Error( "check variable after import importOnce 1 1" + a + " " + b );
   }

   // remove file
   removeFile( importPostfixFile );
   removeFile( importOncePostfixFile );
}
function createPostfixFile ()
{
   var file = new File( importPostfixFile );
   file.write( "a++" );
   file.close();
   file = new File( importOncePostfixFile );
   file.write( "b++" );
   file.close();
}