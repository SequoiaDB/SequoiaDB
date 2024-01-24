/******************************************************************************
 * @Description   : seqDB-25651:指定重命名集合空间不是原始集合空间 
 * @Author        : liuli
 * @CreateTime    : 2022.03.28
 * @LastEditTime  : 2022.03.28
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25651";
   var csNameNew = "cs_new_25651";
   var clName = "cl_25651";
   var clNameNew = "cl_new_25651";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { ShardingKey: { a: 1 }, AutoSplit: true } );

   // 插入数据后执行truncate
   insertBulkData( dbcl, 1000 );
   dbcl.truncate();

   // 重命名恢复truncate项目，指定集合空间不是原始集合空间
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Truncate" );
   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      db.getRecycleBin().returnItemToName( recycleName, csNameNew + "." + clNameNew );
   } );

   // 校验数据
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, [] );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}