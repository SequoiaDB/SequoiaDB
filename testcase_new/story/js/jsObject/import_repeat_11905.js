/*******************************************************************
*@Description : test repeat import importOnce js file
*               seqDB-11905:使用import重复导入文件
*               seqDB-11906:使用importOnce重复导入文件
*@author      : Liang XueWang 
*******************************************************************/


// js file to repeat import
var repeatImportFile = WORKDIR + "/repeatImport_11905.js";
// js file to repeat importOnce    
var repeatImportOnceFile = WORKDIR + "/repearImportOnce_11906.js";
var a = 0;
var b = 0;
main( test );
function test ()
{
   // creat repeat import importOnce file
   createRepeatImportFile();
   createRepeatImportOnceFile();

   // test repeat import
   import( repeatImportFile );
   import( repeatImportFile );
   assert.equal( a, 2 );

   // test repeat importOnce

   importOnce( repeatImportOnceFile );
   importOnce( repeatImportOnceFile );
   assert.equal( b, 1 );

   // remove repeat repeatOnce file   
   removeFile( repeatImportFile );
   removeFile( repeatImportOnceFile );
}
function createRepeatImportFile ()
{

   var file = new File( repeatImportFile );
   file.write( "a++; function sub( x, y) { return x - y ; }" );
   file.close();
}

function createRepeatImportOnceFile ()
{
   var file = new File( repeatImportOnceFile );
   file.write( "b++; function divide( x, y ) { return x / y ; }" );
   file.close();
}