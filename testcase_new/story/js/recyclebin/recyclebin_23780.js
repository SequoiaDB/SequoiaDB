/******************************************************************************
 * @Description   : seqDB-23780:dropCS后修改域删除数据组，恢复CS
 * @Author        : liuli
 * @CreateTime    : 2021.04.06
 * @LastEditTime  : 2022.11.04
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_23780";
   var clName = "cl_23780";
   var domainName = "domain_23780";

   var groupNames = commGetDataGroupNames( db );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   commDropDomain( db, domainName );

   var mydomain = db.createDomain( domainName, groupNames, { AutoSplit: true } );
   var dbcs = db.createCS( csName, { Domain: domainName } );
   var dbcl = dbcs.createCL( clName, { ShardingKey: { a: 1 } } );
   var docs = insertBulkData( dbcl, 1000 );

   // 删除CS后删除domain中集合所在的数据组
   db.dropCS( csName );
   mydomain.alter( { Groups: [groupNames[0]] } );

   // 恢复dropCS项目
   var recycleName = getOneRecycleName( db, csName, "Drop" );
   assert.tryThrow( SDB_CAT_GROUP_NOT_IN_DOMAIN, function()
   {
      db.getRecycleBin().returnItem( recycleName );
   } );

   // 将CS所在的组添加到domain中
   mydomain.alter( { Groups: groupNames } );

   // 恢复dropCS项目
   db.getRecycleBin().returnItem( recycleName );

   // 恢复后检查数据正确性
   var cursor = dbcl.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs );
   checkRecycleItem( recycleName );

   // 再次删除domain中集合所在的数据组
   assert.tryThrow( SDB_DOMAIN_IS_OCCUPIED, function()
   {
      mydomain.alter( { Groups: [groupNames[0]] } );
   } );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   commDropDomain( db, domainName );
}
