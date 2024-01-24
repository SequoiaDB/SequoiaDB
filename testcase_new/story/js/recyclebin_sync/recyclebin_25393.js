/******************************************************************************
 * @Description   : seqDB-25393:dropAll()接口参数校验
 * @Author        : liuli
 * @CreateTime    : 2022.03.02
 * @LastEditTime  : 2022.08.15
 * @LastEditors   : liuli
 ******************************************************************************/
// 合法参数其他用例可以覆盖
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_25393";
   var clName = "cl_25393";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var recycle = db.getRecycleBin();
   var dbcs = db.createCS( csName );
   var dbcl = dbcs.createCL( clName );
   dbcl.insert( { a: 1 } );
   dbcl.truncate();
   dbcs.dropCL( clName );
   db.dropCS( csName )

   // 指定 Async为 true,false
   recycle.dropAll( { Async: true } );
   assert.equal( recycle.count(), 0 );

   var dbcs = db.createCS( csName );
   var dbcl = dbcs.createCL( clName );
   dbcl.insert( { a: 1 } );
   dbcl.truncate();
   dbcs.dropCL( clName );
   db.dropCS( csName )
   recycle.dropAll( { Async: false } );
   assert.equal( recycle.count(), 0 );

   var dbcs = db.createCS( csName );
   var dbcl = dbcs.createCL( clName );
   dbcl.insert( { a: 1 } );
   dbcl.truncate();
   dbcs.dropCL( clName );
   db.dropCS( csName )

   // option 指定非法字段和非法值
   var options = [{ Async: "true" }, { Async: 1 }];
   for( var i in options )
   {
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         recycle.dropAll( options[i] );
      } );
   }

   assert.equal( recycle.count(), 3 );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}