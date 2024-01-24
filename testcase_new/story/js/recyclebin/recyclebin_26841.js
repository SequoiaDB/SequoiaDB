/******************************************************************************
 * @Description   : seqDB-26841:主子表属于不同CS，删除子表CS，不指定recursive删除回收站子表CS项目
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.08.24
 * @LastEditTime  : 2022.08.30
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var mainCSName = "mainCS_26841"
   var mainCLName = "mainCL_26841"
   var csName = "cs_26841";
   var clName = "cl_26841";

   commDropCS( db, mainCSName );
   commDropCS( db, csName );
   cleanRecycleBin( db, mainCSName );
   cleanRecycleBin( db, csName );

   var mainCS = commCreateCS( db, mainCSName );
   var mainCL = commCreateCL( db, mainCSName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, clName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( csName + "." + clName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } )

   mainCS.dropCL( mainCLName );
   db.dropCS( csName );

   var csRecycleName = getOneRecycleName( db, csName, "Drop" );
   assert.tryThrow( SDB_RECYCLE_CONFLICT, function()
   {
      db.getRecycleBin().dropItem( csRecycleName );
   } );

   // 指定recursive为true,验证回收站项目不存在
   db.getRecycleBin().dropItem( csRecycleName, true );
   checkRecycleItem( csRecycleName );

   // 删除主表所在cs，并清除回收站
   db.dropCS( mainCSName );
   var mainCSRecycleName = getOneRecycleName( db, mainCSName, "Drop" );
   var mainCLRecycleName = getOneRecycleName( db, mainCSName + "." + mainCLName, "Drop" );
   cleanRecycleBin( db, mainCSName );
   cleanRecycleBin( db, mainCLRecycleName );
   checkRecycleItem( mainCSRecycleName );
   checkRecycleItem( mainCLRecycleName );
}