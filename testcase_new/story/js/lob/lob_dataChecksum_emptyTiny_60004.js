/******************************************************************************
 * @Description   : seqDB-60004:开启crc校验后,空lob与极小lob的put/get数据完整
 *                  边缘场景:dataLen=0 及远小于一页的数据,不应误报crc错误
 * @Author        : Claude
 * @CreateTime    : 2026.07.02
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_60004";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   var testFile = CHANGEDPREFIX + "lob60004.file";
   var getTestFile = CHANGEDPREFIX + "lob60004Get.file";
   // 依次:空文件、1字节、几字节
   var sizes = [0, 1, 7, 100];
   var cmd = new Cmd();

   db.updateConf( { "lobdatachecksum": "write|read" } );

   try
   {
      for( var s = 0; s < sizes.length; ++s )
      {
         cmd.run( "rm -rf " + testFile + " " + getTestFile );
         cmd.run( "head -c " + sizes[s] + " /dev/urandom > " + testFile );
         var originMd5 = getMd5ForFile( testFile );

         var oid = cl.putLob( testFile );
         cl.getLob( oid, getTestFile, true );
         assert.equal( originMd5, getMd5ForFile( getTestFile ),
                       "size=" + sizes[s] );
         cl.deleteLob( oid );
      }
   }
   finally
   {
      db.updateConf( { "lobdatachecksum": "write" } );
      cmd.run( "rm -rf " + testFile + " " + getTestFile );
   }
}
