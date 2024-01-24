/******************************************************************************
 * @Description   : seqDB-23793:分区表且被切分到多个数据组，dropCL后恢复CL
 * @Author        : liuli
 * @CreateTime    : 2021.04.01
 * @LastEditTime  : 2022.02.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_23793";
testConf.clOpt = { ShardingKey: { a: 1 }, AutoSplit: true };

main( test );
function test ( args )
{
   var clName = testConf.clName;

   cleanRecycleBin( db, COMMCSNAME );
   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = args.testCL;

   var groupNames = commGetDataGroupNames( db );
   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   dbcl.insert( docs );
   var findData1 = getDataForGroups( groupNames, COMMCSNAME, clName );

   // 删除CL后恢复
   dbcs.dropCL( clName );
   var recycleName = getOneRecycleName( db, COMMCSNAME + "." + clName, "Drop" );
   db.getRecycleBin().returnItem( recycleName );
   checkRecycleItem( recycleName );

   // 校验恢复前和恢复后每组的数据量
   var findData2 = getDataForGroups( groupNames, COMMCSNAME, clName );
   assert.equal( findData1, findData2, "Before recovery " + findData1 + " ,after recovery " + findData2 + " ,data group order " + groupNames );
}

function getDataForGroups ( groupNames, csName, clName )
{
   var findData = [];
   for( var i = 0; i < groupNames.length; i++ )
   {
      var rgDB = null;
      try
      {
         rgDB = db.getRG( groupNames[i] ).getMaster().connect();
         var num = Number( rgDB.getCS( csName ).getCL( clName ).count() );
         findData.push( num );
      }
      finally
      {
         rgDB.close();
      }
   }
   return findData;
}