/******************************************************************************
*@Description: test putLob with incorrect oid
*@author:      wangkexin
*@createdate:  2018.11.20
*@testlinkCase: seqDB-16583:putLob, oid参数取值格式校验
******************************************************************************/


main( test );

function test ()
{
   var clName = CHANGEDPREFIX + "_putlob16583";
   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );

   var testFile = CHANGEDPREFIX + "_lobTest16583.file";
   lobGenerateFile( testFile );

   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, true, "create collection" );

   // put lob with incorrect oid
   try
   {
      var incorrectOid = "123456";
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         cl.putLob( testFile, incorrectOid );
      } );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      var cmd = new Cmd();
      // remove lobfile
      cmd.run( "rm -rf " + testFile );
      commDropCL( db, COMMCSNAME, clName, true, true, "drop collection in the end, error" );
   }
}

