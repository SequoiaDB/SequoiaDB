/******************************************************************************
@Description : seqDB-7463: hash自动切分，分区键为不同数据类型，验证数据分布规则
                           type: date
                           数据量随机，cl partition随机，数据随机
@Author :
   2019-8-23   XiaoNi Huang  init
*******************************************************************************/
main( test );

function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   if( commGetGroupsNum( db ) < 2 )
   {
      return;
   }

   var suffix = "_date_7463";
   var dmName = CHANGEDPREFIX + "_dm" + suffix;
   var csName = CHANGEDPREFIX + "_cs" + suffix;
   var clName = CHANGEDPREFIX + "_cl" + suffix;
   var groups = commGetGroups( db, false, "", false, true, true );
   var groupNames = [groups[1][0].GroupName, groups[2][0].GroupName];
   var cl;
   var recordsNum = getRandomInt( 1000, 5000 );

   commDropCS( db, csName, true, "drop cs in the begin" );
   commDropDomain( db, dmName, true );

   // create domain / cs / cl
   db.createDomain( dmName, groupNames, { "AutoSplit": true } );
   var cs = db.createCS( csName, { "Domain": dmName } );
   var partition = getRandomPartition();
   cl = cs.createCL( clName, { "ShardingType": "hash", "ShardingKey": { b: 1 }, "Partition": partition } );

   // insert
   var recordsArr = readyRdmRecs( recordsNum );
   cl.insert( recordsArr );

   // check results
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { a: 1 } );
   commCompareResults( cursor, recordsArr );
   checkHashDistribution( groupNames, csName, clName, recordsNum );

   commDropCS( db, csName, false, "drop cs in the end." );
   commDropDomain( db, dmName, false );
}

function readyRdmRecs ( recordsNum, recordsArr ) 
{
   var recordsArr = [];
   for( var i = 0; i < recordsNum; i++ )
   {
      var rdmStdDate = getRandomStdDate();
      var rdmAbsMill = getRandomInt( 300000000000000, 500000000000000 );
      var tmpDate = [rdmStdDate, rdmAbsMill];
      var date = tmpDate[getRandomInt( 0, tmpDate.length - 1 )];
      recordsArr.push( { "a": i, "b": { "$date": date } } );
   }
   return recordsArr;
}