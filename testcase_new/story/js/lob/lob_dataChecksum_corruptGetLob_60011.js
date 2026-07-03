/******************************************************************************
 * @Description   : seqDB-60011:运行时read校验独立检出坏页(不依赖离线工具)
 *                  开启write|read写入多页lob,用公共函数lobCorruptDataPage
 *                  篡改该组所有副本的同一数据页,getLob应报
 *                  SDB_DMS_CORRUPTED_EXTENT。与60009的区别:仅验证运行时
 *                  读路径,不做停机态inspect,作为最小化的运行时检测用例。
 * @Author        : Claude
 * @CreateTime    : 2026.07.03
 ******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var csName = "lobcrc60011_cs";
   var clName = "lobcrc60011_cl";
   var fullCL = csName + "." + clName;
   var testFile = CHANGEDPREFIX + "lob60011.file";
   var getTestFile = CHANGEDPREFIX + "lob60011Get.file";
   var cmd = new Cmd();
   var corruptPageID = 1;   // page0为meta,篡改首个数据页
   var groupName = null;
   var nodes = null;

   commDropCS( db, csName, true, "clean before test" );
   commCreateCS( db, csName, true, "create cs" );
   var cl = commCreateCL( db, csName, clName, {}, false, true, "create cl" );

   try
   {
      db.updateConf( { "lobdatachecksum": "write|read" } );

      // 写入多页lob(4整页),保证首个数据页为满写页(带crc)
      cmd.run( "head -c " + ( 262144 * 4 ) + " /dev/urandom > " + testFile );
      var oid = cl.putLob( testFile );

      // 篡改前确认可正常读取
      cl.getLob( oid, getTestFile, true );
      cmd.run( "rm -rf " + getTestFile );

      groupName = commGetCLGroups( db, fullCL )[0];
      nodes = lobGetGroupNodes( db, groupName );

      // 公共函数:停机->dd篡改->起机,作用于该组所有副本(经sdbcm支持远端)
      lobCorruptDataPage( db, groupName, csName, corruptPageID );

      // 运行时getLob(read校验)应报数据页损坏
      assert.tryThrow( SDB_DMS_CORRUPTED_EXTENT, function()
      {
         cl.getLob( oid, getTestFile, true );
      } );
   }
   finally
   {
      // 兜底:确保所有节点已拉起
      if( null != groupName && null != nodes )
      {
         var rg2 = db.getRG( groupName );
         for( var k = 0; k < nodes.length; ++k )
         {
            try { rg2.getNode( nodes[k].HostName, nodes[k].svcname ).start(); }
            catch( e ) {}
         }
         sleep( 3000 );
      }
      db.updateConf( { "lobdatachecksum": "write" } );
      commDropCS( db, csName, true, "clean after test" );
      cmd.run( "rm -rf " + testFile + " " + getTestFile );
   }
}
