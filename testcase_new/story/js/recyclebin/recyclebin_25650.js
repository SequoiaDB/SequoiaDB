/******************************************************************************
 * @Description   : seqDB-25650:多次truncate，1个项目重命名恢复，1个项目直接恢复 
 * @Author        : liuli
 * @CreateTime    : 2022.03.28
 * @LastEditTime  : 2022.03.28
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25650";
   var clName = "cl_25650";
   var clNameNew = "cl_new_25650";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { ShardingKey: { a: 1 }, AutoSplit: true } );

   // 插入数据后执行truncate
   var docs1 = insertBulkData( dbcl, 1000 );
   dbcl.truncate();

   // 再次插入数据执行truncate
   var docs2 = insertBulkData( dbcl, 2000 );
   dbcl.truncate();

   // 获取回收站小红truncate项目，直接恢复第一个项目，重命名恢复第二个项目
   var recycleNames = getRecycleName( db, csName + "." + clName, "Truncate" );
   db.getRecycleBin().returnItem( recycleNames[0] );
   db.getRecycleBin().returnItemToName( recycleNames[1], csName + "." + clNameNew );

   // 校验数据
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs1 );
   var dbcl2 = dbcs.getCL( clNameNew );
   var cursor = dbcl2.find().sort( { a: 1 } );
   commCompareResults( cursor, docs2 );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}