/******************************************************************************
 * @Description   : seqDB-25631:重命名恢复CS项目，指定CS名已存在
 * @Author        : liuli
 * @CreateTime    : 2022.03.25
 * @LastEditTime  : 2022.03.26
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25631";
   var clName = "cl_25631";
   var csNameNew = "cs_new_25631";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   commDropCS( db, csNameNew );
   cleanRecycleBin( db, csNameNew );

   var dbcl = commCreateCL( db, csName, clName );
   var dbcl2 = commCreateCL( db, csNameNew, clName );
   insertBulkData( dbcl, 1000 );
   var docs = insertBulkData( dbcl2, 1000 );

   // 删除CS后重命名恢复，指定已存在的CS
   db.dropCS( csName );
   var recycleName = getOneRecycleName( db, csName, "Drop" );
   assert.tryThrow( SDB_DMS_CS_EXIST, function()
   {
      db.getRecycleBin().returnItemToName( recycleName, csNameNew );
   } );

   // 校验csNameNew中CL数据正确
   var cursor = dbcl2.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );

   // 删除csNameNew后重命名恢复CS
   db.dropCS( csNameNew );
   db.getRecycleBin().returnItemToName( recycleName, csNameNew );

   cleanRecycleBin( db, csName );
   commDropCS( db, csNameNew );
   cleanRecycleBin( db, csNameNew );
}