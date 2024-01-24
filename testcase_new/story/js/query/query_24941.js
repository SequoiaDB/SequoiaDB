/******************************************************************************
 * @Description   : seqDB-24941:删除主集合后新建同名普通集合，从另一个协调节点立刻插入数据
 * @Author        : 钟子明
 * @CreateTime    : 2022.01.8
 * @LastEditTime  : 2022.01.18
 * @LastEditors   : 钟子明
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.csName = COMMCSNAME + "_24941"

main( test );

function test () 
{
   var clName = COMMCLNAME + "_24941";
   //进行检测，如果协调节点数量小于2则跳过用例
   if( commGetGroupNodes( db, "SYSCoord" ).length < 2 )
   {
      return;
   }

   var groupName = commGetDataGroupNames( db )[0];
   createCL( db, groupName );
   var data = prepareData();

   var db2 = getAnotherCoord( db );

   commDropCL( db, testConf.csName, clName );
   commCreateCL( db, testConf.csName, clName, { ShardingKey: { a: 1 }, AutoSplit: true }, false, false );
   db2.getCS( testConf.csName ).getCL( clName ).insert( data );

   db2.close();
}

function createCL ( db1, groupName )
{
   var cl = commCreateCL( db1, testConf.csName, 'newcl1', { Group: groupName }, false, false );
   cl.enableSharding( { ShardingKey: { a: 1 }, AutoSplit: true } );
   cl = commCreateCL( db1, testConf.csName, 'newcl2', { Group: groupName }, false, false );
   cl.enableSharding( { ShardingKey: { a: 1 }, AutoSplit: true } );
   var maincl = commCreateCL( db1, testConf.csName, clName, { IsMainCL: true, ShardingKey: { b: 1 } }, false, false );
   maincl.attachCL( testConf.csName + '.newcl1', { LowBound: { b: 0 }, UpBound: { b: 5000 } } );
   maincl.attachCL( testConf.csName + '.newcl2', { LowBound: { b: 5000 }, UpBound: { b: 10000 } } );
}

function prepareData () 
{
   var data = [];

   for( i = 0; i < 10000; i++ ) 
   {
      data.push( { a: i, b: i } );
   }
   return data;
}

function getAnotherCoord ( db1 )
{
   var coordArray = commGetGroupNodes( db1, "SYSCoord" );
   var db2 = coordArray[0];
   if( db1.toString() == db2.HostName + ':' + db2.svcname )
   {
      db2 = coordArray[1];
   }
   return new Sdb( db2.HostName, db2.svcname );
}