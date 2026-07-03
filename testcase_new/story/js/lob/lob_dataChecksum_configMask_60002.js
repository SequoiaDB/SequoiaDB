/******************************************************************************
 * @Description   : seqDB-60002:lobdatachecksum掩码在线切换(write/read/空)均正常put/get
 * @Author        : Claude
 * @CreateTime    : 2026.07.02
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_60002";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   var testFile = CHANGEDPREFIX + "lob60002.file";
   var getTestFile = CHANGEDPREFIX + "lob60002Get.file";
   var masks = ["write", "write|read", "read", ""];

   try
   {
      lobGenerateFile( testFile );
      var originMd5 = getMd5ForFile( testFile );

      for( var m = 0; m < masks.length; ++m )
      {
         // 在线修改掩码，运行时生效
         db.updateConf( { "lobdatachecksum": masks[m] } );

         var oid = cl.putLob( testFile );
         cl.getLob( oid, getTestFile, true );
         assert.equal( originMd5, getMd5ForFile( getTestFile ),
                       "mask=" + masks[m] );
         cl.deleteLob( oid );
      }
   }
   finally
   {
      db.updateConf( { "lobdatachecksum": "write" } );
      var cmd = new Cmd();
      cmd.run( "rm -rf " + testFile );
      if( lobFileIsExist( getTestFile ) )
      {
         cmd.run( "rm -rf " + getTestFile );
      }
   }
}
