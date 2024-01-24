/******************************************************************************
 * @Description   : seqDB-23776:CS未关联域且为空，dropCS后恢复CS
 * @Author        : liuli
 * @CreateTime    : 2021.04.02
 * @LastEditTime  : 2022.06.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_23776";
   var clName = "cl_23776";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   commCreateCS( db, csName );

   // 删除CS后恢复CS，并检查回收站项目的正确性
   db.dropCS( csName );
   var recycleName = getOneRecycleName( db, csName, "Drop" );
   db.getRecycleBin().returnItem( recycleName );
   checkRecycleItem( recycleName );

   // 在CS上创建cl，插入数据并校验
   var dbcl = db.getCS( csName ).createCL( clName );
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test2" }, { a: 3, b: 234.3 }];
   dbcl.insert( docs );

   var cursor = dbcl.find();
   commCompareResults( cursor, docs );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}