/******************************************************************************
 * @Description   : seqDB-60006:开启crc校验后truncateLob到非页对齐长度,数据完整
 *                  边缘场景:truncate产生的尾部不满页属"非全写页",不应误报crc错误
 * @Author        : Claude
 * @CreateTime    : 2026.07.02
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_60006";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   var testFile = CHANGEDPREFIX + "lob60006.file";
   var getTestFile = CHANGEDPREFIX + "lob60006Get.file";
   var expectFile = CHANGEDPREFIX + "lob60006Expect.file";
   var page = 262144;
   var fullSize = page * 3;         // 3 整页
   var truncLen = page * 2 + 12345; // 截断到非页对齐长度
   var cmd = new Cmd();

   db.updateConf( { "lobdatachecksum": "write|read" } );

   try
   {
      cmd.run( "head -c " + fullSize + " /dev/urandom > " + testFile );
      var oid = cl.putLob( testFile );

      // 截断到非页对齐长度,尾页变为非全写页
      cl.truncateLob( oid, truncLen );

      // 读回并与原文件前 truncLen 字节比较
      cl.getLob( oid, getTestFile, true );
      cmd.run( "head -c " + truncLen + " " + testFile + " > " + expectFile );
      assert.equal( getMd5ForFile( expectFile ), getMd5ForFile( getTestFile ),
                    "truncLen=" + truncLen );

      cl.deleteLob( oid );
   }
   finally
   {
      db.updateConf( { "lobdatachecksum": "write" } );
      cmd.run( "rm -rf " + testFile + " " + getTestFile + " " + expectFile );
   }
}
