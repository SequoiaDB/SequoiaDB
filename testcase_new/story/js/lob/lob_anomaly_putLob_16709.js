/******************************************************************************
*@Description: test putLob with specify oid
*@author:      wangkexin
*@createdate:  2018.11.23
*@testlinkCase: seqDB-16709:putLob，指定oid插入大对象
******************************************************************************/


main( test );

function test ()
{
   var clName = CHANGEDPREFIX + "_putlob16709";
   var testFile = CHANGEDPREFIX + "_lobTest16709.file";
   var getTestFile = CHANGEDPREFIX + "_lobTestGet16709.file";
   var cmd = new Cmd();
   var oid = "5bf7575bdc4e88fa3dd16709";

   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );

   lobGenerateFile( testFile );
   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, true, "create collection" );
   var md5Arr = cmd.run( "md5sum " + testFile ).split( " " );
   var md5 = md5Arr[0];

   // put lob with specify oid
   cl.putLob( testFile, oid );

   try
   {
      cl.getLob( oid, getTestFile, true );
      md5Arr = cmd.run( "md5sum " + getTestFile ).split( " " );
      getMd5 = md5Arr[0];
      assert.equal( getMd5, md5 );
      // delete lob
      cl.deleteLob( oid );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      cmd.run( "rm -rf " + testFile );
      cmd.run( "rm -rf " + getTestFile );
      commDropCL( db, COMMCSNAME, clName, true, true, "drop collection in the end, error" );
   }
}

