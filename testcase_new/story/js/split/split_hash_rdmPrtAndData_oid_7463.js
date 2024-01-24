/******************************************************************************
@Description : seqDB-7463: hash自动切分，分区键为不同数据类型，验证数据分布规则
                           type: oid
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

   var suffix = "_oid_7463";
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
   var expRecs = readyExpRecs( recordsArr );
   commCompareResults( cursor, expRecs );
   checkHashDistribution( groupNames, csName, clName, recordsNum );

   commDropCS( db, csName, false, "drop cs in the end." );
   commDropDomain( db, dmName, false );
}

function readyRdmRecs ( recordsNum ) 
{
   var recordsArr = [];
   for( var i = 0; i < recordsNum; i++ )
   {
      // rdmOid actuallly { "a": 0, "b": { "_str": "5e16825f873e0f8c7412bf20" } }, 
      // {"a":0,"b":{"$oid":"5e16825f873e0f8c7412bf20"}} after insert.
      var rdmOid = ObjectId();
      recordsArr.push( { "a": i, b: rdmOid } );
   }
   return recordsArr;
}

function readyExpRecs ( recordsArr )
{
   var expRecs = [];
   for( var i = 0; i < recordsArr.length; i++ )
   {
      var oid = recordsArr[i]["b"]["_str"];
      expRecs.push( { "a": i, b: { "$oid": oid } } );
   }
   return expRecs;
}