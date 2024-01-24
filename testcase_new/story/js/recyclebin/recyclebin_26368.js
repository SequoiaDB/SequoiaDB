/******************************************************************************
 * @Description   : seqDB-26368:dropCS后重建domain，恢复CS
 * @Author        : liuli
 * @CreateTime    : 2022.04.13
 * @LastEditTime  : 2022.04.13
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_26368";
   var clName = "cl_26368";
   var domainName = "domain_26368";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   commDropDomain( db, domainName );

   var groupNames = commGetDataGroupNames( db );
   db.createDomain( domainName, groupNames, { AutoSplit: true } );
   var dbcs = db.createCS( csName, { Domain: domainName } );
   var dbcl = dbcs.createCL( clName, { ShardingKey: { a: 1 } } );
   var docs = insertBulkData( dbcl, 10000 );

   // 删除CS后删除domain
   db.dropCS( csName );
   db.dropDomain( domainName );

   // 重建domain，指定一个group
   var domain = db.createDomain( domainName, [groupNames[0]], { AutoSplit: true } );
   // 恢复dropCS项目
   var recycleName = getOneRecycleName( db, csName, "Drop" );
   assert.tryThrow( SDB_CAT_GROUP_NOT_IN_DOMAIN, function()
   {
      db.getRecycleBin().returnItem( recycleName );
   } );

   // 将所有组加到domain中
   domain.alter( { Groups: groupNames } );

   // 恢复后校验数据
   db.getRecycleBin().returnItem( recycleName );
   var dbcl = db.getCS( csName ).getCL( clName );
   var cursor = dbcl.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs );

   // 校验CS使用domain正确
   var actDomainName = dbcs.getDomainName();
   assert.equal( actDomainName, domainName );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   commDropDomain( db, domainName );
}