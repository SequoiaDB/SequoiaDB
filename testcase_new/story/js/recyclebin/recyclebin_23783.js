/******************************************************************************
 * @Description   : seqDB-23783:dropCS后删除关联的域，恢复CS
 * @Author        : liuli
 * @CreateTime    : 2021.04.06
 * @LastEditTime  : 2022.04.13
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_23783";
   var clName = "cl_23783";
   var domainName = "domain_23783";

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

   // 恢复dropCS项目
   var recycleName = getOneRecycleName( db, csName, "Drop" );
   db.getRecycleBin().returnItem( recycleName );
   checkRecycleItem( recycleName );

   // 恢复后校验数据
   var dbcl = db.getCS( csName ).getCL( clName );
   var cursor = dbcl.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs );

   // 校验CS没有使用domain
   var actDomainName = dbcs.getDomainName();
   assert.equal( actDomainName, "" );

   // 新建一个相同domain
   db.createDomain( domainName, groupNames, { AutoSplit: true } );
   // cs指定domain
   dbcs.alter( { Domain: domainName } );

   // 校验CS正确使用domain
   var actDomainName = dbcs.getDomainName();
   assert.equal( actDomainName, domainName );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   commDropDomain( db, domainName );
}