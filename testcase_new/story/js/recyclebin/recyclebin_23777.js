/******************************************************************************
 * @Description   : seqDB-23777:CS关联域且自动切分，CS下有分区表且被切分到多个组，dropCS后恢复CS
 * @Author        : liuli
 * @CreateTime    : 2021.04.02
 * @LastEditTime  : 2022.06.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_23777";
   var clName = "cl_23777";
   var domainName = "domain_23777";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   commDropDomain( db, domainName );

   var groupNames = commGetDataGroupNames( db );
   db.createDomain( domainName, groupNames, { AutoSplit: true } );
   var dbcs = db.createCS( csName, { Domain: domainName } );
   var dbcl = dbcs.createCL( clName, { ShardingKey: { a: 1 } } );
   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   dbcl.insert( docs );
   var findData1 = getDataForGroups( groupNames, csName, clName );

   // 删除CS后恢复
   db.dropCS( csName );
   var recycleName = getOneRecycleName( db, csName, "Drop" );
   db.getRecycleBin().returnItem( recycleName );
   checkRecycleItem( recycleName );

   // 恢复成功后校验数据分布正确性
   var findData2 = getDataForGroups( groupNames, csName, clName );
   assert.equal( findData1, findData2, "Before recovery " + findData1 + " ,after recovery " + findData2 + " ,data group order " + groupNames );

   // 校验数据正确性
   var dbcl = db.getCS( csName ).getCL( clName );
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );

   commDropCS( db, csName );
   commDropDomain( db, domainName );
   cleanRecycleBin( db, csName );
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