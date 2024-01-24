/******************************************************************************
 * @Description   : seqDB-24091:先dropCL再dropCS，删除回收站CS项目检查CL项目
 * @Author        : liuli
 * @CreateTime    : 2021.04.13
 * @LastEditTime  : 2022.02.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_24091";
   var clName = "cl_24091";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcl = commCreateCL( db, csName, clName );
   var docs = [{ a: 1, b: 1 }, { a: 2, b: 2 }, { a: 3, b: 3 }];
   dbcl.insert( docs );

   // 先删除CL，再删除CS
   var dbcs = db.getCS( csName );
   dbcs.dropCL( clName );
   db.dropCS( csName );

   // 删除回收站CS项目
   var clRecycleName = getOneRecycleName( db, csName + "." + clName, "Drop" );
   var recycleName = getOneRecycleName( db, csName, "Drop" );
   db.getRecycleBin().dropItem( recycleName, true );

   // 检查回收站，CS项目和CL项目均被删除
   checkRecycleItem( clRecycleName );
   checkRecycleItem( recycleName );
}