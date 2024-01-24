/******************************************************************************
 * @Description   : seqDB-24165:创建过时的子表缓存，删除主表误发到不存在子表的数据组
 * @Author        : Yi Pan
 * @CreateTime    : 2021.05.07
 * @LastEditTime  : 2021.05.07
 * @LastEditors   : Yi Pan
 ******************************************************************************/
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csname = "maincs_24165"
   var mainclname = "maincl_24165"
   var subclname1 = "subcl_24165a"
   var subclname2 = "subcl_24165b"
   var subclname3 = "subcl_24165c"

   var coord = getCoords();
   if( coord.length < 2 )
   {
      return;
   }
   var db1 = new Sdb( coord[0] );
   var db2 = new Sdb( coord[1] );
   var groups = commGetDataGroupNames( db );
   commDropCS( db, csname );

   //连接1创建主表，挂载两个子表
   var cs = db1.createCS( csname );
   var mcl = cs.createCL( mainclname, { ShardingKey: { a: 1 }, ShardingType: 'range', IsMainCL: true } );
   cs.createCL( subclname1, { Group: groups[0] } );
   cs.createCL( subclname2, { Group: groups[1] } );
   mcl.attachCL( csname + "." + subclname1, { LowBound: { a: 0 }, UpBound: { a: 100 } } );
   mcl.attachCL( csname + "." + subclname2, { LowBound: { a: 100 }, UpBound: { a: 200 } } );

   //连接2删除子表1
   db2.getCS( csname ).dropCL( subclname1 );

   //连接1创建子表3，指定穿参与子表1相同
   cs.createCL( subclname3, { Group: groups[1] } );
   mcl.attachCL( csname + "." + subclname3, { LowBound: { a: 0 }, UpBound: { a: 100 } } );
   //将子表3重命名为子表1的name
   cs.renameCL( subclname3, subclname1 );

   //连接1删除主表cl
   cs.dropCL( mainclname );

   //清除环境
   db1.close();
   db2.close();
   commDropCS( db, csname );
}

function getCoords ()
{
   var cursor = db.snapshot( SDB_SNAP_CONFIGS, { "role": "coord" }, { "NodeName": "" } );
   var coord = [];
   while( cursor.next() )
   {
      coord.push( cursor.current().toObj().NodeName );
   }
   return coord;
}



