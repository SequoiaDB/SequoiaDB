/*******************************************************************
*@Description : test import importOnce no permission file
*               seqDB-11908:使用import importOnce导入无权限的文件
*@author      : Liang XueWang 
*******************************************************************/
// js file with no permission
var noPermFile = WORKDIR + "/noPerm_11908.js"



main( test );

function test ()
{
   // check current user not root
   var user = currUser();
   if( user === "root" ) return;

   // create no perm file
   createNoPermFile();

   // import importOnce no perm file
   try
   {
      import( noPermFile );
      throw new Error( "should error" );
   } catch( e )
   {
      if( e != 	SDB_PERM )
      {
         throw e;
      }
   }

   try
   {
      importOnce( noPermFile );
      throw new Error( "should error" );
   } catch( e )
   {
      if( e != SDB_PERM )
      {
         throw e;
      }
   }

   // remove file
   removeFile( noPermFile );
}
function createNoPermFile ()
{
   var file = new File( noPermFile );
   file.write( "var a = 1;" );
   file.close();
   File.chmod( noPermFile, 0000 );
}
