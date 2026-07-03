/******************************************************************************
 * @Description   : seqDB-60007:read校验作用于"写入时未生成crc"的数据不误报
 *                  边缘场景:先关write写入无crc页,再开read校验读取,应正常(NoCRC跳过)
 * @Author        : Claude
 * @CreateTime    : 2026.07.02
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_60007";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   var testFile = CHANGEDPREFIX + "lob60007.file";
   var getTestFile = CHANGEDPREFIX + "lob60007Get.file";
   var cmd = new Cmd();

   try
   {
      // 1) 关闭校验写入:数据页不携带 crc
      db.updateConf( { "lobdatachecksum": "" } );
      cmd.run( "head -c " + ( 262144 * 2 + 500 ) + " /dev/urandom > " + testFile );
      var originMd5 = getMd5ForFile( testFile );
      var oid = cl.putLob( testFile );

      // 2) 仅开 read 校验读取:无 crc 的页应作 NoCRC 跳过,不报错
      db.updateConf( { "lobdatachecksum": "read" } );
      cl.getLob( oid, getTestFile, true );
      assert.equal( originMd5, getMd5ForFile( getTestFile ) );

      cl.deleteLob( oid );
   }
   finally
   {
      db.updateConf( { "lobdatachecksum": "write" } );
      cmd.run( "rm -rf " + testFile + " " + getTestFile );
   }
}
