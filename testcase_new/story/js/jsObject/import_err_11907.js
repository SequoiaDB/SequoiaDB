/*******************************************************************
*@Description : test import importOnce error
*               seqDB-11907:使用import importOnce导入不存在的文件
*               seqDB-11909:使用import importOnce导入非js文件
*@author      : Liang XueWang 
*******************************************************************/
// test import importOnce not exist file

main( test );
function test ()
{
   var notExistFile = WORKDIR + "/notExist_11907.js";

   try
   {
      import( notExistFile );
      throw new Error( "should error" );
   } catch( e )
   {
      if( e != SDB_FNE )
      {
         throw e;
      }
   }

   try
   {
      importOnce( notExistFile );
      throw new Error( "should error" );
   } catch( e )
   {
      if( e != SDB_FNE )
      {
         throw e;
      }
   }



   // test import importOnce not js file( sdblobtool file )
   var installPath = commGetInstallPath();
   var sdblobtoolFile = installPath + "/bin/sdblobtool";

   try
   {
      importOnce( sdblobtoolFile );
      throw new Error( "should error" );
   } catch( e )
   {
      if( e != SDB_SPT_EVAL_FAIL )
      {
         throw e;
      }
   }

   try
   {
      import( sdblobtoolFile );
      throw new Error( "should error" );
   } catch( e )
   {
      if( e != SDB_SPT_EVAL_FAIL )
      {
         throw e;
      }
   }
}
