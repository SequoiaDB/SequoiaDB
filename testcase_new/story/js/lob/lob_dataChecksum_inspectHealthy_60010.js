/******************************************************************************
 * @Description   : seqDB-60010:健康lob数据inspect基线(异常用例的对照组)
 *                  开启write后putLob(一次性流式写入,含尾部不满页的每一页
 *                  都是"整页一次写完"=全写页),离线inspect应:Fail=0,所有页
 *                  计入 Pass(NoCRC 仅出现在RMW部分覆盖写,见java用例60012)。
 *                  使用独立CS,停单节点跑inspect后恢复。
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

   var csName = "lobcrc60010_cs";
   var clName = "lobcrc60010_cl";
   var fullCL = csName + "." + clName;
   var testFile = CHANGEDPREFIX + "lob60010.file";
   var cmd = new Cmd();
   var groupName = null;
   var node = null;

   commDropCS( db, csName, true, "clean before test" );
   commCreateCS( db, csName, true, "create cs" );
   var cl = commCreateCL( db, csName, clName, {}, false, true, "create cl" );

   try
   {
      db.updateConf( { "lobdatachecksum": "write" } );

      // 3整页(满写,带crc) + 尾部不满页(非全写,NoCRC)
      cmd.run( "head -c " + ( 262144 * 3 + 500 ) + " /dev/urandom > " + testFile );
      cl.putLob( testFile );

      groupName = commGetCLGroups( db, fullCL )[0];
      node = lobGetGroupNodes( db, groupName )[0];
      var dnode = db.getRG( groupName ).getNode( node.HostName, node.svcname );

      dnode.stop();
      var out;
      try
      {
         out = lobInspectNode( node.HostName, node.dbpath, csName );
      }
      finally
      {
         dnode.start();
         sleep( 3000 );
      }

      var m = out.match( /Page CRC Check\s*:\s*Pass\s+(\d+),\s*Fail\s+(\d+),\s*NoCRC\s+(\d+)/ );
      assert.notEqual( null, m, "inspect未输出Page CRC Check行:\n" + out );
      var pass = parseInt( m[1] );
      var fail = parseInt( m[2] );
      var nocrc = parseInt( m[3] );
      assert.equal( 0, fail, "健康数据不应有坏页 Fail=" + fail + "\n" + out );
      assert.equal( true, pass >= 1, "全写页应计入Pass Pass=" + pass + "\n" + out );
      // putLob 全部为全写页,NoCRC 应为 0(NoCRC 仅出现在 RMW,见 60012)
      assert.equal( 0, nocrc, "putLob 数据不应产生 NoCRC 页 NoCRC=" + nocrc + "\n" + out );
      // 不应出现坏页明细行
      assert.equal( false, /crc mismatch/.test( out ),
                    "健康数据出现坏页明细行\n" + out );
   }
   finally
   {
      if( null != groupName && null != node )
      {
         try { db.getRG( groupName ).getNode( node.HostName, node.svcname ).start(); }
         catch( e ) {}
         sleep( 2000 );
      }
      db.updateConf( { "lobdatachecksum": "write" } );
      commDropCS( db, csName, true, "clean after test" );
      cmd.run( "rm -rf " + testFile );
   }
}
