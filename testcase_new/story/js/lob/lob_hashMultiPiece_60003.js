/******************************************************************************
 * @Description   : seqDB-60003:新hash算法下多分片大lob的put/get/delete正确
 * @Author        : Claude
 * @CreateTime    : 2026.07.02
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_60003";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   var testFile = CHANGEDPREFIX + "lob60003.file";
   var getTestFile = CHANGEDPREFIX + "lob60003Get.file";

   try
   {
      // 生成较大文件，跨多个lobd页(多sequence)，覆盖新hash的多次入桶
      lobGenerateFile( testFile, 50000 );
      var originMd5 = getMd5ForFile( testFile );

      var oid = cl.putLob( testFile );
      cl.getLob( oid, getTestFile, true );
      assert.equal( originMd5, getMd5ForFile( getTestFile ) );

      // 删除后应查不到
      cl.deleteLob( oid );
      assert.equal( 0, cl.listLobs().toArray().length );
   }
   finally
   {
      var cmd = new Cmd();
      cmd.run( "rm -rf " + testFile );
      if( lobFileIsExist( getTestFile ) )
      {
         cmd.run( "rm -rf " + getTestFile );
      }
   }
}
