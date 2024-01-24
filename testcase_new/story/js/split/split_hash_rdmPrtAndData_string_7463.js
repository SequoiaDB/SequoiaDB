/******************************************************************************
@Description : seqDB-7463: hash自动切分，分区键为不同数据类型，验证数据分布规则
                           type: string
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

   var suffix = "_string_7463";
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
   // ready string
   var str = "";
   var baseStr = "大写ABCDEFGHIJKLMNOPQRSTUVWXYZ小写abcdefghijklmnopqrstuvwxyz数字0123456789中文";
   for( i = 0; i < 10; i++ ) 
   {
      str += baseStr;
   }

   // get random string
   for( var i = 0; i < recordsNum; i++ )
   {
      var startLoc = getRandomInt( 0, str.length );
      var endLoc = getRandomInt( 0, str.length - startLoc );
      var subStr = str.substring( startLoc, endLoc );
      recordsArr.push( { "a": i, "b": subStr } );
   }
   return recordsArr;
}