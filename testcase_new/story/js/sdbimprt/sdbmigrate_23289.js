/******************************************************************************
 * @Description   : seqDB-23289 :: 多库多表并发导出后导入，cl为分区表，数据文件为csv 
 * @Author        : Yu Fan
 * @CreateTime    : 2021.01.18
 * @LastEditTime  : 2021.10.12
 * @LastEditors   : liuli
 ******************************************************************************/
var domainName = "domain23289";
var csNames = ["cs223289A", "cs23289B", "cs23289C"];
var clNames = ["cl23289A1", "cl23289A2", "cl23289A3"];
var docs = new Array();
tmpFileDir += "/23289/";
var groups;
testConf.skipStandAlone = true;

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
   for( var i = 0; i < csNames.length; i++ )
   {
      var cs = db.getCS( csNames[i] );
      for( var j = 0; j < clNames.length; j++ )
      {
         var cl = cs.getCL( clNames[j] );
         cl.remove();
      }
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
   for( var i = 0; i < csNames.length; i++ )
   {
      var cs = db.getCS( csNames[i] );
      for( var j = 0; j < clNames.length; j++ )
      {
         var cl = cs.getCL( clNames[j] );
         var cursor = cl.find().sort( { a: 1 } );
         commCompareResults( cursor, docs );
      }
   }

   // 清理环境
   for( var i = 0; i < csNames.length; i++ )
   {
      commDropCS( db, csNames[i], true );
   }
   cmd.run( "rm -rf " + tmpFileDir );
}

function prepareCSCL ()
{
   for( var i = 0; i < csNames.length; i++ )
   {
      commDropCS( db, csNames[i], true );
   }
   commDropDomain( db, domainName, true );
   db.createDomain( domainName, [groups[0], groups[1]], { AutoSplit: true } );
   for( var i = 0; i < 200; i++ )
   {
      docs.push( { a: i, b: i * 2, c: i * 3 } );
   }

   for( var i = 0; i < csNames.length; i++ )
   {
      var cs = db.createCS( csNames[i], { Domain: domainName } );
      for( var j = 0; j < clNames.length; j++ )
      {
         var cl = cs.createCL( clNames[j], { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0 } );
         cl.insert( docs );
      }
   }
}

function prepareExportConf ()
{
   cmd.run( "rm -rf " + tmpFileDir );
   cmd.run( "mkdir -p " + tmpFileDir );
   var filename = tmpFileDir + "export23289.conf";
   var file = fileInit( filename );
   file.write( "[collections]\n" );
   file.write( "number=3\n" );
   for( var i = 1; i <= clNames.length; i++ )
   {
      file.write( "[collection" + i + "]\n" );
      file.write( "name=" + csNames[0] + "." + clNames[i - 1] + "\n" );
      file.write( "type=csv\n" );
      file.write( "fields=a,b,c\n" )
      file.write( "dir=" + tmpFileDir + "\n" );
   }

   file.write( "[collectionspaces]\n" );
   file.write( "number=2\n" );
   for( var i = 1; i <= csNames.length; i++ )
   {
      cmd.run( "mkdir -p " + tmpFileDir + csNames[i] );
      file.write( "[collectionspace" + i + "]\n" );
      file.write( "name=" + csNames[i] + "\n" );
      file.write( "type=csv\n" );
      file.write( "fields=a,b,c\n" )
      file.write( "dir=" + tmpFileDir + csNames[i] + "\n" );
   }
   return filename;
}

function prepareImportConf ()
{
   var filename = tmpFileDir + "import23289.conf";
   var file = fileInit( filename );
   file.write( "[collections]\n" );
   file.write( "number=3\n" );
   for( var i = 1; i <= clNames.length; i++ )
   {
      file.write( "[collection" + i + "]\n" );
      file.write( "name=" + csNames[0] + "." + clNames[i - 1] + "\n" );
      file.write( "headerline=true\n" );
      file.write( "type=csv\n" );
      file.write( "fields=a,b,c\n" )
      file.write( "file=" + tmpFileDir + csNames[0] + "." + clNames[i - 1] + "." + "0.csv,"
         + tmpFileDir + csNames[0] + "." + clNames[i - 1] + "." + "1.csv\n" );
   }

   file.write( "[collectionspaces]\n" );
   file.write( "number=2\n" );
   for( var i = 1; i <= csNames.length; i++ )
   {
      file.write( "[collectionspace" + i + "]\n" );
      file.write( "name=" + csNames[i] + "\n" );
      file.write( "headerline=true\n" );
      file.write( "type=csv\n" );
      file.write( "fields=a,b,c\n" )
      file.write( "dir=" + tmpFileDir + csNames[i] + "\n" );
   }
   return filename;
}
