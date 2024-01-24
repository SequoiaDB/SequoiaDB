/******************************************************************************
 * @Description   : seqDB-25386:MaxVersionNum回收不同名同UniqueID项目
 * @Author        : liuli
 * @CreateTime    : 2022.02.23
 * @LastEditTime  : 2022.02.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_25386";
   var clName1 = "cl_25386_1";
   var clName2 = "cl_25386_2";
   var clName3 = "cl_25386_3";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName1 );

   // CL执行truncate
   dbcl.insert( { a: 1 } );
   dbcl.truncate();

   // renameCL后再次执行truncate
   dbcs.renameCL( clName1, clName2 );
   var dbcl = dbcs.getCL( clName2 );
   dbcl.insert( { a: 2 } );
   dbcl.truncate();

   // 再次renameCL后执行truncate
   dbcs.renameCL( clName2, clName3 );
   var dbcl = dbcs.getCL( clName3 );
   dbcl.insert( { a: 3 } );
   assert.tryThrow( [SDB_RECYCLE_FULL], function() 
   {
      dbcl.truncate();
   } );

   var cursor = dbcl.find();
   commCompareResults( cursor, [{ a: 3 }] );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}