/*******************************************************************
*@Description : test import importOnce empty js file
*               seqDB-12815:使用import importOnce导入空文件
*@author      : Liang XueWang 
*******************************************************************/
// empty file to import
var emptyImportFile = WORKDIR + "/emptyImport_12815.js";
// empty file to importOnce    
var emptyImportOnceFile = WORKDIR + "/emptyImportOnce_12815.js";

main( test );

function test ()
{

   // creat empty import importOnce file
   createEmptyImportFile();
   createEmptyImportOnceFile();

   // test import empty file
   import( emptyImportFile );

   // test importOnce empty file
   importOnce( emptyImportOnceFile );

   // remove file   
   removeFile( emptyImportFile );
   removeFile( emptyImportOnceFile );
}
function createEmptyImportFile ()
{
   var file = new File( emptyImportFile );
   file.close();
}

function createEmptyImportOnceFile ()
{
   var file = new File( emptyImportOnceFile );
   file.close();
}