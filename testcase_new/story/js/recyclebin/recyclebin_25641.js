/******************************************************************************
 * @Description   : seqDB-25641:指定重命名恢复中集合空间不是原始集合空间 
 * @Author        : liuli
 * @CreateTime    : 2022.03.25
 * @LastEditTime  : 2022.03.26
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25641";
   var csNameNew = "cs_new_25641";
   var clName = "cl_25641";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var dbcl = commCreateCL( db, csName, clName );
   var docs = insertBulkData( dbcl, 1000 );

   // 删除CL
   dbcs.dropCL( clName );

   // 重命名恢复CL项目，指定CS不是原始CS
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Drop" );
   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      db.getRecycleBin().returnItemToName( recycleName, csNameNew + "." + clName );
   } );

   // 直接恢复CL，并校验数据
   db.getRecycleBin().returnItem( recycleName );
   var dbcl = db.getCS( csName ).getCL( clName );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}