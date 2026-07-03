/******************************************************************************
 * @Description   : seqDB-60008:lobdatachecksum掩码取值边缘处理
 *                  合法变体(乱序/重复)正常;非法token优雅降级为默认write不报错
 * @Author        : Claude
 * @CreateTime    : 2026.07.02
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_60008";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   var testFile = CHANGEDPREFIX + "lob60008.file";
   var getTestFile = CHANGEDPREFIX + "lob60008Get.file";
   var cmd = new Cmd();
   // 合法变体:乱序、重复,均应被规范化接受
   var validMasks = ["read|write", "write|write", "write|read|write"];
   // 非法token:内核记录"value error, use default"并回退默认write,不应报错
   var invalidMasks = ["xxx", "Write", "readwrite", "write|foo"];

   try
   {
      cmd.run( "head -c " + ( 262144 + 888 ) + " /dev/urandom > " + testFile );
      var originMd5 = getMd5ForFile( testFile );

      // 合法变体:配置成功且put/get正常
      for( var i = 0; i < validMasks.length; ++i )
      {
         db.updateConf( { "lobdatachecksum": validMasks[i] } );
         var oid = cl.putLob( testFile );
         cl.getLob( oid, getTestFile, true );
         assert.equal( originMd5, getMd5ForFile( getTestFile ),
                       "validMask=" + validMasks[i] );
         cl.deleteLob( oid );
         cmd.run( "rm -rf " + getTestFile );
      }

      // 非法值:优雅降级,不抛异常,系统功能不受影响
      for( var j = 0; j < invalidMasks.length; ++j )
      {
         db.updateConf( { "lobdatachecksum": invalidMasks[j] } );
         var oid2 = cl.putLob( testFile );
         cl.getLob( oid2, getTestFile, true );
         assert.equal( originMd5, getMd5ForFile( getTestFile ),
                       "invalidMask fallback=" + invalidMasks[j] );
         cl.deleteLob( oid2 );
         cmd.run( "rm -rf " + getTestFile );
      }
   }
   finally
   {
      db.updateConf( { "lobdatachecksum": "write" } );
      cmd.run( "rm -rf " + testFile + " " + getTestFile );
   }
}
