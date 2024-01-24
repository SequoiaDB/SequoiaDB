/******************************************************************************
 * @Description   : seqDB-24677:修改主表名，非法名称校验
 * @Author        : liuli
 * @CreateTime    : 2021.11.23
 * @LastEditTime  : 2021.11.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var mainCLName = "mainCL_24677";
   var subCLName = "subCL_24677";

   commDropCL( db, COMMCSNAME, mainCLName );

   var dbcs = db.getCS( COMMCSNAME );
   var mainCL = dbcs.createCL( mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   dbcs.createCL( subCLName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );

   var docs = [{ a: 1, b: 1, c: 1 }];
   mainCL.insert( docs );

   // 修改表名为空串
   assert.tryThrow( [SDB_INVALIDARG], function() 
   {
      dbcs.renameCL( mainCLName, "" );
   } );

   // 修改表名以"$"开头
   assert.tryThrow( [SDB_INVALIDARG], function() 
   {
      dbcs.renameCL( mainCLName, "$none" );
   } );

   // 修改表名以"SYS"开头
   assert.tryThrow( [SDB_INVALIDARG], function() 
   {
      dbcs.renameCL( mainCLName, "SYSnone" );
   } );

   // 修改表名中包含"."
   assert.tryThrow( [SDB_INVALIDARG], function() 
   {
      dbcs.renameCL( mainCLName, "no.ne" );
   } );

   // 修改表名长度为128个字符
   var arr = new Array( 129 );
   var noneName = arr.join( "a" );
   assert.tryThrow( [SDB_INVALIDARG], function() 
   {
      dbcs.renameCL( mainCLName, noneName );
   } );

   dbcs.dropCL( mainCLName );
}