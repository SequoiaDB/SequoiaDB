/************************************
*@Description: seqDB-23046:upsert 使用 set 生成记录，集合指定 id hash 分区自动切分
*@Author:      2020/11/30  liuli
**************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.clName = COMMCLNAME + "_22046";
testConf.clOpt = { ShardingKey: { "_id": 1 }, AutoSplit: true };

main( test );

function test ( args )
{
   var cl = args.testCL;
   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_22046";;
   var groupNames = commGetDataGroupNames( db );
   var expRecsNum = 1000;

   upsert( cl );
   checkDataDistribution( groupNames, csName, clName, expRecsNum )
}

function upsert ( cl ) 
{
   for( var i = 0; i < 1000; i++ ) 
   {
      cl.upsert( { $set: { a: i } }, { age: { $gt: 0 } } );
   }
}

function checkDataDistribution ( groupNames, csName, clName, expRecsNum )
{
   // get number of records for each group
   var actTotalRecsNum = 0;
   for( var i = 0; i < groupNames.length; i++ )
   {
      var rgDB = null;
      try
      {
         rgDB = db.getRG( groupNames[i] ).getMaster().connect();
         var cnt = Number( rgDB.getCS( csName ).getCL( clName ).count() );
         if( cnt === 0 )
         {
            throw new Error( groupNames[i] + " has no data" );
         }
         actTotalRecsNum += cnt;
      }
      finally
      {
         rgDB.close();
      }
   }

   // check total number of records for all group
   if( expRecsNum !== actTotalRecsNum )
   {
      throw new Error( "expRecsNum = " + expRecsNum + ", actTotalRecsNum = " + actTotalRecsNum );
   }
}