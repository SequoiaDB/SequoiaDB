/******************************************************************************
 * @Description   : seqDB-23779:dropCS后修改域新增数据组，恢复CS
 * @Author        : liuli
 * @CreateTime    : 2021.04.06
 * @LastEditTime  : 2022.02.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_23779";
   var clName = "cl_23779";
   var domainName = "domain_23779";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   commDropDomain( db, domainName );

   var groupNames = commGetDataGroupNames( db );
   var mydomain = db.createDomain( domainName, [groupNames[0], groupNames[1]], { AutoSplit: true } );
   var dbcs = db.createCS( csName, { Domain: domainName } );
   var dbcl = dbcs.createCL( clName, { ShardingKey: { a: 1 } } );
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test2" }, { a: 3, b: 234.3 }];
   dbcl.insert( docs );

   // 删除CS后在domain中新增数据组
   db.dropCS( csName );
   mydomain.alter( { Groups: groupNames } );

   // 恢复后检查数据正确性
   var recycleName = getOneRecycleName( db, csName, "Drop" );
   db.getRecycleBin().returnItem( recycleName );
   var cursor = dbcl.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs );

   checkRecycleItem( recycleName );

   commDropCS( db, csName );
   commDropDomain( db, domainName );
   cleanRecycleBin( db, csName );
}