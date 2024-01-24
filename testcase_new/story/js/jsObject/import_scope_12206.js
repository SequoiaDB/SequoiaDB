/***********************************************************************
*@Description : test import importOnce js file, check scope
*               seqDB-12206:在函数中使用import importOnce，检查作用域
*@author      : Liang XueWang 
***********************************************************************/
// js file to test import scope
var importScopeFile = WORKDIR + "/importScope_12206.js";
// js file to test importOnce scope
var importOnceScopeFile = WORKDIR + "/importOnceScope_12206.js";


main( test );
function test ()
{




   // create file
   createScopeTestFile();

   // test import scope inner function
   testImportScope();

   // test importOnce scope inner function
   testImportOnceScope();

   // test import importOnce outer function
   if( typeof ( mod ) !== "function" || typeof ( v ) !== "number" ||
      typeof ( pow ) !== "function" || typeof ( w ) !== "number" )
   {
      throw new Error( "test import importOnce scope outer function function number function number" + typeof ( mod ) + " " + typeof ( v ) + " " + typeof ( pow ) + " " + typeof ( w ) );
   }

   if( mod( 18, 7 ) !== 4 || v !== 100 || pow( 10, 2 ) !== 100 || w !== 10 )
   {
      throw new Error( "test import importOnce outer function 4 100 100 10" + mod( 18, 7 ) + " " + v + " " + pow( 10, 2 ) + " " + w );
   }

   // remove file
   removeFile( importScopeFile );
   removeFile( importOnceScopeFile );
}
function createScopeTestFile ()
{
   var file = new File( importScopeFile );
   file.write( "function mod( a, b ) { return a % b ; } var v = 100" );
   file.close();
   file = new File( importOnceScopeFile );
   file.write( "function pow( x, y ) { return Math.pow( x, y ); } var w = 10" );
   file.close();
}

function testImportScope ()
{
   import( importScopeFile );
   var funcType = typeof ( mod );
   var varType = typeof ( v );
   if( funcType !== "function" || varType !== "number" )
   {
      throw new Error( "testImportScope check import func and var type function number" + funcType + " " + varType );
   }
   if( mod( 4, 3 ) !== 1 || v !== 100 )
   {
      throw new Error( "testImportScope check import func and var" + "1 100" + mod( 4, 3 ) + " " + v );
   }
}

function testImportOnceScope ()
{
   importOnce( importOnceScopeFile );
   var funcType = typeof ( pow );
   var varType = typeof ( w );
   if( funcType !== "function" || varType !== "number" )
   {
      throw new Error( "testImportOnceScope check importOnce func and var type function number" + funcType + " " + varType );
   }
   if( pow( 2, 3 ) !== 8 || w !== 10 )
   {
      throw new Error( "testImportOnceScope check importOnce func and var 8 10" + pow( 2, 3 ) + " " + w );
   }
}