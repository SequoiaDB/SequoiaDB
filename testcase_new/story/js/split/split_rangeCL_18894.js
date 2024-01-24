/************************************
*@description ：seqDB-18894:清理range表切分任务时创建同名hash分区表 
*@author ：2019-7-20 wuyan
**************************************/
main( test );

function test()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   if( commGetGroupsNum( db ) < 2 )
   {
      return;
   }

   var groupNames = commGetDataGroupNames( db );
   var srcGroupName = groupNames[0];
   var dstGroupName = groupNames[1];
   var csName = CHANGEDPREFIX + "_split_18894";
   var clName = "cl";
   var recsNum = 40000;

   // create cs/cl
   commDropCS( db, csName, true, "drop CS in the beginning." );
   var cs = db.createCS( csName );
   var cl = cs.createCL( clName, { ShardingKey: { a: 1 }, ShardingType: "range", Group: srcGroupName } );

   // insert
   var docs = [];
   for( var i = 0; i < recsNum; ++i )
   {
      var doc = { "a": i, "b": "test" + i };
      docs.push( doc );
   }
   cl.insert( docs );

   // split
   cl.splitAsync( srcGroupName, dstGroupName, { a: 1 }, { a: 40000 } );

   // drop cl, then create cl again
   cs.dropCL( clName );
   var cl2 = cs.createCL( clName, { ShardingKey: { a: 1 }, ShardingType: "hash", Group: dstGroupName } );

   // check result
   var docs2 = [{ a: 1 }];
   cl2.insert( docs2 );
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } );
   commCompareResults( cursor, docs2 );

   commDropCS( db, csName, true, "drop CS in the end." );
}
