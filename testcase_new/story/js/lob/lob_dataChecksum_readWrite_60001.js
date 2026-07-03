/******************************************************************************
 * @Description   : seqDB-60001:开启lob数据页crc校验(write|read)后put/get数据完整
 * @Author        : Claude
 * @CreateTime    : 2026.07.02
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_60001";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   var testFile = CHANGEDPREFIX + "lob60001.file";
   var getTestFile = CHANGEDPREFIX + "lob60001Get.file";
   var putNum = 10;

   // 开启写生成 + 读校验
   db.updateConf( { "lobdatachecksum": "write|read" } );

   try
   {
      lobGenerateFile( testFile );
      var originMd5 = getMd5ForFile( testFile );

      var oids = lobPutLob( cl, testFile, putNum );
      for( var i = 0; i < oids.length; ++i )
      {
         // 读路径开启crc校验，数据正确应不报错且内容一致
         cl.getLob( oids[i], getTestFile, true );
         assert.equal( originMd5, getMd5ForFile( getTestFile ) );
      }
   }
   finally
   {
      // 恢复默认配置
      db.updateConf( { "lobdatachecksum": "write" } );
      var cmd = new Cmd();
      cmd.run( "rm -rf " + testFile );
      if( lobFileIsExist( getTestFile ) )
      {
         cmd.run( "rm -rf " + getTestFile );
      }
   }
}
