/******************************************************************************
 * @Description   : seqDB-25387:CL两次truncate，rename后dropCL
 * @Author        : liuli
 * @CreateTime    : 2022.02.23
 * @LastEditTime  : 2022.06.28
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_25387";
   var clName1 = "cl_25387_1";
   var clName2 = "cl_25387_2";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName1 );

   // CL执行truncate
   dbcl.insert( { a: 1 } );
   dbcl.truncate();

   // CL再次执行truncate
   dbcl.insert( { a: 2 } );
   dbcl.truncate();

   // renameCL后删除CL
   dbcs.renameCL( clName1, clName2 );
   var dbcl = dbcs.getCL( clName2 );
   dbcl.insert( { a: 3 } );
   assert.tryThrow( [SDB_RECYCLE_FULL], function() 
   {
      dbcs.dropCL( clName2 );
   } );

   var cursor = dbcl.find();
   commCompareResults( cursor, [{ a: 3 }] );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}