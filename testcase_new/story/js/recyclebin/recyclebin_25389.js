/******************************************************************************
 * @Description   : seqDB-25389:事务中强制恢复dropCL项目
 * @Author        : liuli
 * @CreateTime    : 2022.02.18
 * @LastEditTime  : 2022.02.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25389";
   var clName = "cl_25389";
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test2" }, { a: 3, b: 234.3 }];

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName );

   // 插入数据后删除CL
   dbcl.insert( { a: 1 } );
   dbcs.dropCL( clName );

   // 创建同名CL
   var dbcl = dbcs.createCL( clName );

   // 开启事务后插入数据
   db.transBegin();
   dbcl.insert( docs );

   // 恢复dropCL项目
   var recycleName = getOneRecycleName( db, csName + "." + clName );
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