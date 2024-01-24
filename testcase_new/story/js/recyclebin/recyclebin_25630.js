/******************************************************************************
 * @Description   : seqDB-25630:重命名恢复CS项目 
 * @Author        : liuli
 * @CreateTime    : 2022.03.25
 * @LastEditTime  : 2022.03.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25630";
   var clName = "cl_25630";
   var csNameNew = "cs_new_25630";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   commDropCS( db, csNameNew );
   cleanRecycleBin( db, csNameNew );

   var dbcl = commCreateCL( db, csName, clName );
   var docs = insertBulkData( dbcl, 1000 );

   // 删除CS后重命名恢复
   db.dropCS( csName );
   var recycleName = getOneRecycleName( db, csName, "Drop" );
   var recycle = db.getRecycleBin();
   var returnName = recycle.returnItemToName( recycleName, csNameNew );
   checkRecycleItem( recycleName );

   // 校验返回名称
   assert.equal( returnName.toObj().ReturnName, csNameNew );

   // 校验原始获取的CL不可用
   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      dbcl.insert( { a: 1 } );
   } );

   // 获取重命名恢复后的CL并校验数据
   var dbcl = db.getCS( csNameNew ).getCL( clName );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );

   commDropCS( db, csNameNew );
   cleanRecycleBin( db, csNameNew );
}