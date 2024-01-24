/*******************************************************************
*@Description : test nest import importOnce js file
*               seqDB-11904:使用import importOnce导入嵌套js文件
*@author      : Liang XueWang 
*******************************************************************/
// js file nest import, A import B
var nestImportFileA = WORKDIR + "/nestImportA_11904.js";
// js file nest import, B import A
var nestImportFileB = WORKDIR + "/nestImportB_11904.js";
// js file nest importOnce, A importOnce B
var nestImportOnceFileA = WORKDIR + "/nestImportOnceA_11904.js";
// js file nest importOnce, B importOnce A
var nestImportOnceFileB = WORKDIR + "/nestImportOnceB_11904.js";
var a = 0;
var b = 0;


main( test );

function test ()
{


   // create file
   createNestImportFile();
   createNestImportOnceFile();

   // test nest import
   import( nestImportFileA );
   assert.equal( a, 2 );

   // test nest importOnce
   importOnce( nestImportOnceFileA );
   assert.equal( b, 2 );

   // remove file
   removeFile( nestImportFileA );
   removeFile( nestImportFileB );

   removeFile( nestImportOnceFileA );
   removeFile( nestImportOnceFileB );

}


function createNestImportFile ()
{
   var file = new File( nestImportFileA );
   file.write( "a++; import( \"" + nestImportFileB + "\" )" );
   file.close();
   file = new File( nestImportFileB );
   file.write( "a++; import( \"" + nestImportFileA + "\" )" );
   file.close();

}

function createNestImportOnceFile ()
{
   var file = new File( nestImportOnceFileA );
   file.write( "b++; importOnce( \"" + nestImportOnceFileB + "\" )" );
   file.close();
   file = new File( nestImportOnceFileB );
   file.write( "b++; importOnce( \"" + nestImportOnceFileA + "\" )" );
   file.close();
}