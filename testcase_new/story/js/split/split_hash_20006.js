/******************************************************************************
@description : seqDB-20006:hash切分表百分百切分多次，多个组多次切分覆盖边界值
@author : 2019-10-11   XiaoNi Huang  init
*******************************************************************************/
main( test );

function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   if( commGetGroupsNum( db ) < 3 )
   {
      return;
   }

   var groupNames = commGetDataGroupNames( db );
   var dmName = CHANGEDPREFIX + "_split_20006";
   var csName = CHANGEDPREFIX + "_split_20006";
   var clName = "cl";
   var recordsNum = 1000;

   commDropCS( db, csName, true, "drop cs in the begin" );
   commDropDomain( db, dmName, true );
   dm = db.createDomain( dmName, [groupNames[0], groupNames[1]] );
   var cs = db.createCS( csName, { "Domain": dmName } );
   cl = cs.createCL( clName, { "ShardingKey": { "id": 1 }, "ShardingType": "hash", "AutoSplit": true } );

   // insert records
   var recs = [];
   for( var i = 0; i < 1000; ++i ) 
   {
      recs.push( { "id": i, "name": "a" + i } );
   }
   cl.insert( recs );

   // alter domain, add group
   dm.alter( { "Groups": groupNames } );

   // split
   cl.split( groupNames[0], groupNames[2], 33 );
   cl.split( groupNames[1], groupNames[2], 33 );
   cl.split( groupNames[2], groupNames[0], 33 );

   // check results
   checkRecordsNum( cl, csName, clName, recordsNum, groupNames );
   checkShardingRange( csName, clName );

   commDropCS( db, csName, false, "drop cs in the end." );
   commDropDomain( db, dmName, false );
}

function checkRecordsNum ( cl, csName, clName, recordsNum, groupNames )
{
   // check total count
   var cnt = Number( cl.count() );
   assert.equal( recordsNum, cnt );

   // check count for each group
   var totalNodeRecsCnt = 0;
   for( var i = 0; i < groupNames.length; i++ ) 
   {
      var nodeDB = null;
      try 
      {
         var nodeDB = db.getRG( groupNames[i] ).getMaster().connect();
         var nodeRecsCnt = nodeDB.getCS( csName ).getCL( clName ).count();
         totalNodeRecsCnt += nodeRecsCnt;
      }
      finally
      {
         if( nodeDB !== null ) nodeDB.close();
      }
   }
   assert.equal( recordsNum, totalNodeRecsCnt );
}

function checkShardingRange ( csName, clName ) 
{
   var cataInfo = db.snapshot( 8, { "Name": csName + "." + clName } ).next().toObj().CataInfo;
   if( cataInfo[0]["LowBound"][""] !== 0 || cataInfo[0]["UpBound"][""] !== 1373
      || cataInfo[1]["LowBound"][""] !== 1373 || cataInfo[1]["UpBound"][""] !== 2048
      || cataInfo[2]["LowBound"][""] !== 2048 || cataInfo[2]["UpBound"][""] !== 3421
      || cataInfo[3]["LowBound"][""] !== 3421 || cataInfo[3]["UpBound"][""] !== 3651
      || cataInfo[4]["LowBound"][""] !== 3651 || cataInfo[4]["UpBound"][""] !== 4096 )
   {
      throw new Error( "CataInfo error after split, actual cataInfo = [" + JSON.stringify( cataInfo ) + "]" );
   }
}