/******************************************************************************
 * @Description   : seqDB-23781:dropCS后，修改域的自动切分属性，恢复CS
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
   var csName = "cs_23781";
   var clName = "cl_23781";
   var domainName = "domain_23781";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   commDropDomain( db, domainName );

   var groupNames = commGetDataGroupNames( db );
   var mydomain = db.createDomain( domainName, groupNames, { AutoSplit: true } );
   var dbcs = db.createCS( csName, { Domain: domainName } );
   var dbcl = dbcs.createCL( clName, { ShardingKey: { a: 1 } } );
   var docs = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   dbcl.insert( docs );

   // 删除CS后将domain改为 AutoSplit:false
   db.dropCS( csName );
   mydomain.alter( { AutoSplit: false } );
   var recycleName = getOneRecycleName( db, csName, "Drop" );
   db.getRecycleBin().returnItem( recycleName );

   // 恢复后检查数据正确性
   var cursor = dbcl.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs );
   checkRecycleItem( recycleName );

   // 删除CS后将domain改为 AutoSplit:true
   db.dropCS( csName );
   mydomain.alter( { AutoSplit: true } );
   var recycleName = getOneRecycleName( db, csName, "Drop" );
   db.getRecycleBin().returnItem( recycleName );

   // 恢复后检查数据正确性
   var cursor = dbcl.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs );
   checkRecycleItem( recycleName );

   commDropCS( db, csName );
   commDropDomain( db, domainName );
   cleanRecycleBin( db, csName );
}