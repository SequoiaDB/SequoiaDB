/******************************************************************************
 * @Description   : seqDB-25652:truncate后renameCL，重命名恢复truncate项目，指定重命名为原始名称 
 * @Author        : liuli
 * @CreateTime    : 2022.03.28
 * @LastEditTime  : 2022.03.28
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25652";
   var clName = "cl_25652";
   var clNameNew = "cl_new_25652";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { ShardingKey: { a: 1 }, AutoSplit: true } );

   // 插入数据后执行truncate
   var docs = insertBulkData( dbcl, 1000 );
   dbcl.truncate();

   // renameCL
   dbcs.renameCL( clName, clNameNew );

   // 重命名恢复truncate项目，指定集合空间不是原始集合空间
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Truncate" );
   db.getRecycleBin().returnItemToName( recycleName, csName + "." + clName );

   // 校验renameCL后数据正确
   var dbcl = dbcs.getCL( clNameNew );
   var cursor = dbcl.find();
   commCompareResults( cursor, [] );

   // 校验重命名恢复truncate项目数据
   var dbcl = dbcs.getCL( clName );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}