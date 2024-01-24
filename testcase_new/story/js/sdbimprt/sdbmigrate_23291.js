/******************************************************************************
 * @Description   : seqDB-23291 :: 多库多表并发导出后导入，数据文件包含json和csv 
 * @Author        : Yu Fan
 * @CreateTime    : 2021.01.14
 * @LastEditTime  : 2021.10.12
 * @LastEditors   : liuli
 ******************************************************************************/
var domainName = "domain23291";
var csName1 = "cs23291A";
var csName2 = "cs23291B";
var clNames1 = ["cl23291A1", "cl23291A2", "cl23291A3"];
var clNames2 = ["cl23291A1", "cl23291A2", "cl23291A3"];
var docs = new Array();
tmpFileDir += "/23291/";
var dataFileDir = tmpFileDir + "cl23291A1/"
testConf.skipStandAlone = true;
var groups;

main( test );
function test ( testPara )
{
   groups = commGetDataGroupNames( db );
   if( groups.length < 2 )
   {
      println( "数据组的个数小于2，跳过此用例！！！" );
      return;
   }

   // 准备集合并插入记录
   prepareCSCL();

   // 准备导出配置文件
   var exportConf = prepareExportConf();

   // 导出命令
   var command = installDir + "tools/sdbmigrate/bin/sdbexprt.sh" +
      " -s " + COORDHOSTNAME + " -p " + COORDSVCNAME +
      " --conf " + exportConf + " --jobs 3 --debug";
   // 执行导出
   cmd.run( command );

   // 删除数据，准备导入
   for( var i = 0; i < clNames1.length; i++ )
   {
      db.getCS( csName1 ).getCL( clNames1[i] ).remove();
   }
   for( var i = 0; i < clNames2.length; i++ )
   {
      db.getCS( csName2 ).getCL( clNames2[i] ).remove();
   }

   // 准备导入配置文件
   var importConf = prepareImportConf();
   // 导入命令
   var command = installDir + "tools/sdbmigrate/bin/sdbimprt.sh" +
      " -s " + COORDHOSTNAME + " -p " + COORDSVCNAME +
      " --conf " + importConf + " --jobs 3 --debug";
   // 执行导入
   cmd.run( command );

   // 检查结果
   for( var i = 0; i < clNames1.length; i++ )
   {
      var cursor = db.getCS( csName1 ).getCL( clNames1[i] ).find().sort( { a: 1 } );
      commCompareResults( cursor, docs );
   }
   for( var i = 0; i < clNames2.length; i++ )
   {
      var cursor = db.getCS( csName2 ).getCL( clNames2[i] ).find().sort( { a: 1 } );
      commCompareResults( cursor, docs );
   }
   // 清理环境
   commDropCS( db, csName1, true );
   commDropCS( db, csName2, true );
   commDropDomain( db, domainName, true );
   cmd.run( "rm -rf " + tmpFileDir );
}

function prepareCSCL ()
{
   commDropCS( db, csName1, true );
   commDropCS( db, csName2, true );
   commDropDomain( db, domainName, true )
   var domain = db.createDomain( domainName, [groups[0], groups[1]], { AutoSplit: true } );
   var cs1 = db.createCS( csName1, { Domain: domainName } );
   var cs2 = db.createCS( csName2, { Domain: domainName } );
   for( var i = 0; i < 200; i++ )
   {
      docs.push( { a: i, b: i * 2, c: i * 3 } );
   }

   for( var i = 0; i < clNames1.length; i++ )
   {
      var cl = cs1.createCL( clNames1[i], { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0 } );
      cl.insert( docs );
   }

   for( var i = 0; i < clNames2.length; i++ )
   {
      var cl = cs2.createCL( clNames2[i], { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0 } );
      cl.insert( docs );
   }
}

function prepareExportConf ()
{
   cmd.run( "rm -rf " + tmpFileDir );
   cmd.run( "mkdir -p " + tmpFileDir );
   cmd.run( "mkdir -p " + dataFileDir );
   var filename = tmpFileDir + "export23291.conf";
   var file = fileInit( filename );
   file.write( "[collections]\n" );
   file.write( "number=3\n" );
   for( var i = 1; i <= clNames1.length; i++ )
   {
      file.write( "[collection" + i + "]\n" );
      file.write( "name=" + csName1 + "." + clNames1[i - 1] + "\n" );
      if( i % 2 == 0 )
      {
         file.write( "type=json\n" );
      } else
      {
         file.write( "type=csv\n" );
         file.write( "fields=a,b,c\n" )
      }
      file.write( "dir=" + tmpFileDir + "\n" );
   }
   file.write( "[collectionspaces]\n" );
   file.write( "number=1\n" );
   file.write( "[collectionspace1]\n" );
   file.write( "name=" + csName2 + "\n" );
   file.write( "type=json\n" );
   file.write( "dir=" + dataFileDir + "\n" );
   return filename;
}

function prepareImportConf ()
{
   var filename = tmpFileDir + "import23291.conf";
   var file = fileInit( filename );
   file.write( "[collections]\n" );
   file.write( "number=3\n" );
   for( var i = 1; i <= clNames1.length; i++ )
   {
      file.write( "[collection" + i + "]\n" );
      file.write( "name=" + csName1 + "." + clNames1[i - 1] + "\n" );
      if( i % 2 == 0 )
      {
         file.write( "type=json\n" );
         file.write( "file=" + tmpFileDir + csName1 + "." + clNames1[i - 1] + "." + "0.json," +
            tmpFileDir + csName1 + "." + clNames1[i - 1] + "." + "1.json,\n" );
      } else
      {
         file.write( "type=csv\n" );
         file.write( "file=" + tmpFileDir + csName1 + "." + clNames1[i - 1] + "." + "0.csv," +
            tmpFileDir + csName1 + "." + clNames1[i - 1] + "." + "1.csv\n" );
         file.write( "headerline=true\n" )
         file.write( "fields=a,b,c\n" )
      }
   }
   file.write( "[collectionspaces]\n" );
   file.write( "number=1\n" );
   file.write( "[collectionspace1]\n" );
   file.write( "name=" + csName2 + "\n" );
   file.write( "type=json\n" );
   file.write( "dir=" + dataFileDir + "\n" );
   return filename;
}
