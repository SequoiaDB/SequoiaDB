/************************************
*@Description: 切分表修改名后，执行数据操作
*@author:      luweikang
*@createdate:  2018.10.12
*@testlinkCase:seqDB-16144
**************************************/

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   if( commGetGroupsNum( db ) < 2 )
   {
      return;
   }
   var oldcsName = CHANGEDPREFIX + "_16144_oldcs";
   var newcsName = CHANGEDPREFIX + "_16144_newcs";
   var clName = CHANGEDPREFIX + "_16144_CL";

   var groupNames = getGroupName( db, true );
   var sourceGroup = groupNames[0][0];
   var targetGroup = groupNames[1][0];

   var cs = commCreateCS( db, oldcsName, false, "create cs in begine" );
   var options = { ShardingType: "hash", ShardingKey: { a: 1 }, Group: sourceGroup }
   var cl = commCreateCL( db, oldcsName, clName, options, false, false, "create cl in the begin" );

   //insert 1000 data, split 50 to target group
   insertData( cl, 1000 );
   cl.split( sourceGroup, targetGroup, 50 );

   db.renameCS( oldcsName, newcsName );

   checkRenameCSResult( oldcsName, newcsName, 1 );

   cl = db.getCS( newcsName ).getCL( clName );

   //insert 1000 data, and check data
   insertData( cl, 1000 );
   //update ($set: {no:10086}) 2000 data, and check data
   updateData( cl );
   //delete no < 500 data, and check data
   deleteData( cl );

   //create index and drop index，check results
   cl.createIndex( "noIndex", { no: 1 }, false );
   cl.createIndex( "phoneIndex", { phone: 1 }, false );
   cl.dropIndex( "noIndex" );
   var indexArr = ['$id', "$shard", 'phoneIndex'];
   var cur = cl.listIndexes();
   while( cur.next() )
   {
      var index = cur.current().toObj();
      var name = index.IndexDef.name;
      assert.notEqual( indexArr.indexOf( name ), -1 );
   }

   commDropCS( db, newcsName, true, "clean cs---" );
}

function updateData ( cl )
{
   cl.update( { $set: { no: 10086 } } );
   var recordNum = cl.count( { no: 10086 } );
   assert.equal( recordNum, 2000 );
}

function deleteData ( cl )
{
   cl.remove( { a: { $lt: 500 } } );
   var recordNum = cl.count();
   assert.equal( recordNum, 1000 );
}