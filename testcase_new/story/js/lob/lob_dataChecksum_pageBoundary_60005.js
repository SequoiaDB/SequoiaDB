/******************************************************************************
 * @Description   : seqDB-60005:开启crc校验后,lob大小在页边界处put/get数据完整
 *                  边缘场景:正好整数页、整数页±1字节,校验全写页与尾部不满页
 * @Author        : Claude
 * @CreateTime    : 2026.07.02
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_60005";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   var testFile = CHANGEDPREFIX + "lob60005.file";
   var getTestFile = CHANGEDPREFIX + "lob60005Get.file";
   var page = 262144;   // 默认 lob 页大小 256K
   // 1页-1、正好1页、1页+1、正好2页、2页+1
   var sizes = [page - 1, page, page + 1, page * 2, page * 2 + 1];
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
