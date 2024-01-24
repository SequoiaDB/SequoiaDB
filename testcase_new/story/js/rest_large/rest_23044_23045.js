/************************************
*@Description: seqDB-23044:insert，集合指定 id hash 分区自动切分
*              seqDB-23045:使用 upsert 生成记录，集合指定 id hash 分区自动切分
*@Author:      2020/11/30  liuli
**************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.clName = COMMCLNAME + "_22044";
testConf.clOpt = { ShardingKey: { "_id": 1 }, AutoSplit: true };

main( test );

function test ( args )
{
   var cl = args.testCL;
   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_22044";
   var groupNames = commGetDataGroupNames( db );
   var expRecsNum = 1000;

   insert( csName, clName );
   checkDataDistribution( groupNames, csName, clName, expRecsNum );

   cl.remove();
   upsert( csName, clName );
   checkDataDistribution( groupNames, csName, clName, expRecsNum )
}

function insert ( csName, clName )
{
   var word = "insert";
   for( var i = 0; i < 1000; i++ )
   {
      tryCatch( ["cmd=" + word, "name=" + csName + '.' + clName, 'insertor={age:' + i + '}'], [0] );
   }
}

function upsert ( csName, clName )
{
   var word = "upsert";
   for( var i = 0; i < 1000; i++ )
   {
      tryCatch( ["cmd=" + word, "name=" + csName + '.' + clName, 'updator={ $set: { a:' + i + '}}', 'filter={age:{$gt:0}}'], [0] );
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