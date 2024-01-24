/******************************************************************************
 * @Description   : seqDB-25374:事务过程中强制恢复truncate项目
 * @Author        : liuli
 * @CreateTime    : 2022.02.18
 * @LastEditTime  : 2022.06.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25374";
   var clName = "cl_25374";
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test2" }, { a: 3, b: 234.3 }];

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcl = commCreateCL( db, csName, clName );

   // 插入数据后执行truncate
   dbcl.insert( { a: 1 } );
   dbcl.truncate();

   // 开启事务后插入数据
   db.transBegin();
   dbcl.insert( docs );

   // 恢复truncate项目
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Truncate" );
   assert.tryThrow( SDB_DPS_INVALID_LOCK_UPGRADE_REQUEST, function()
   {
      db.getRecycleBin().returnItem( recycleName, { Enforced: true } );
   } )

   // 校验数据后提交事务
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
   db.transCommit();

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}