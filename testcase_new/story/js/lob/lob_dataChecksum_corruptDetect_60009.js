/******************************************************************************
 * @Description   : seqDB-60009:crc校验能检出被篡改的lob数据页(异常主场景)
 *                  开启write|read写入多页lob,直接dd篡改该组所有副本的某数据页:
 *                    1) 运行时getLob(read校验)报错 SDB_DMS_CORRUPTED_EXTENT
 *                    2) 离线sdbdmsdump inspect报告 Page CRC Check Fail>=1
 *                       且每坏页打印 expect/actual/dataLen 明细
 *                  使用独立CS,避免污染公共集合;全程恢复节点并drop cs。
 * @Author        : Claude
 * @CreateTime    : 2026.07.02
 ******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var csName = "lobcrc60009_cs";
   var clName = "lobcrc60009_cl";
   var fullCL = csName + "." + clName;
   var testFile = CHANGEDPREFIX + "lob60009.file";
   var getTestFile = CHANGEDPREFIX + "lob60009Get.file";
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

      // 写入多页lob(约1MB,>=4页),保证首个数据页为满写页(带crc)
      cmd.run( "head -c " + ( 262144 * 4 ) + " /dev/urandom > " + testFile );
      var oid = cl.putLob( testFile );

      // 篡改前确认可正常读取
      cl.getLob( oid, getTestFile, true );
      cmd.run( "rm -rf " + getTestFile );

      groupName = commGetCLGroups( db, fullCL )[0];
      nodes = lobGetGroupNodes( db, groupName );
      var rg = db.getRG( groupName );

      // 逐副本:停机 -> 篡改数据页 -> 停机态跑inspect断言检出 -> 起机
      // (dd/inspect 均经节点所在主机的 sdbcm 执行,支持远端节点)
      var inspectChecked = false;
      for( var i = 0; i < nodes.length; ++i )
      {
         var dnode = rg.getNode( nodes[i].HostName, nodes[i].svcname );
         dnode.stop();
         lobCorruptNodeDataPage( nodes[i].HostName, nodes[i].dbpath,
                                 csName, corruptPageID );

         // 离线inspect:应报Fail>=1且含 actual 明细(仅需在一个节点上断言)
         if( !inspectChecked )
         {
            var out = lobInspectNode( nodes[i].HostName, nodes[i].dbpath,
                                      csName );
            var fail = lobParseCrcFail( out );
            assert.notEqual( -1, fail, "inspect未输出Page CRC Check行:\n" + out );
            assert.equal( true, fail >= 1,
                          "inspect未检出坏页 Fail=" + fail + "\n" + out );
            assert.equal( true, /crc mismatch/.test( out ),
                          "inspect缺少坏页明细行\n" + out );
            assert.equal( true, /actual\[0x/.test( out ),
                          "inspect坏页明细缺少actual值\n" + out );
            inspectChecked = true;
         }
         dnode.start();
      }
      sleep( 5000 );   // 等待重新选主

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
