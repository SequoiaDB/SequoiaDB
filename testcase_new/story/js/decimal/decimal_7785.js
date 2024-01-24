/************************************
*@Description: main-sub table for decimal data,verify two part function:
               part1:attach bound is int type,and insert decimal data;
               part2:attach bound is decimal type,and insert decimal data; 
*@author:      zhaoyu
*@createdate:  2016.4.27
**************************************/
main( test )
function test ()
{
   //part1:attach bound is int type,and insert decimal data;

   //clean environment before test
   var mainCSName = COMMCSNAME + "_7785mcs"
   var mainCLName = COMMCLNAME + "_7785mcl";
   commDropCL( db, mainCSName, mainCLName );

   var subCSName = COMMCSNAME + "_7785scs";
   var subCLName1 = COMMCLNAME + "_7785scl1";
   commDropCL( db, subCSName, subCLName1 );

   var subCLName2 = COMMCLNAME + "_7785scl2";
   commDropCL( db, subCSName, subCLName2 );

   //check test environment before split
   //standalone can not split
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   //less two groups,can not split
   var allGroupName = getGroupName( db );
   if( 1 === allGroupName.length )
   {
      return;
   }

   //create main cl 
   var mainCLOption = { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range" };
   var dbcl = commCreateCL( db, mainCSName, mainCLName, mainCLOption );

   //create two sub cl
   var subCLOption1 = { ShardingKey: { a: 1 }, ShardingType: "hash", Partition: 8 };
   var dbsubcl_1 = commCreateCL( db, subCSName, subCLName1, subCLOption1 );

   var subCLOption2 = { ShardingKey: { b: 1 }, ShardingType: "range" };
   var dbsubcl_2 = commCreateCL( db, subCSName, subCLName2, subCLOption2 );

   //attach cl bound use int type
   attachOption1 = { LowBound: { a: -2147483648 }, UpBound: { a: 0 } };
   dbcl.attachCL( subCSName + "." + subCLName1, attachOption1 );

   attachOption2 = { LowBound: { a: 0 }, UpBound: { a: 2147483647 } };
   dbcl.attachCL( subCSName + "." + subCLName2, attachOption2 );

   //insert decimal data in bound ;
   var validDoc = [{ a: -2147483648 },
   { a: 0 },
   { a: 2147483646 },
   { a: { $decimal: "-2147483648" } },
   { a: { $decimal: "0" } },
   { a: { $decimal: "2147483646", $precision: [100, 2] } }];
   dbcl.insert( validDoc );

   //check decimal data in sub cl_1
   expRecs1 = [{ a: -2147483648 },
   { a: { $decimal: "-2147483648" } }];
   checkResult( dbsubcl_1, null, null, expRecs1, { _id: 1 } );

   //check decimal data in sub cl_2
   expRecs2 = [{ a: 0 },
   { a: 2147483646 },
   { a: { $decimal: "0" } },
   { a: { $decimal: "2147483646.00", $precision: [100, 2] } }];
   checkResult( dbsubcl_2, null, null, expRecs2, { _id: 1 } );

   //insert decimal data out of bound and check result
   var invalidDoc = [{ a: -2147483649 },
   { a: 2147483647 },
   { age: { $decimal: "-2147483649", $precision: [100, 2] } },
   { age: { $decimal: "2147483647" } }];
   invalidDataInsertCheckResult( dbcl, invalidDoc, -135 );

   //part2:attach bound is decimal type,and insert decimal data;

   //detach two sub cl
   dbcl.detachCL( subCSName + "." + subCLName1 );
   dbcl.detachCL( subCSName + "." + subCLName2 );

   //attach cl bound use decimal type 
   attachDecimalOption1 = { LowBound: { a: { $decimal: "-9223372036854775808", $precision: [100, 2] } }, UpBound: { a: 0 } };
   dbcl.attachCL( subCSName + "." + subCLName1, attachDecimalOption1 );

   attachDecimalOption2 = { LowBound: { a: { $decimal: "0", $precision: [100, 2] } }, UpBound: { a: { $decimal: "9223372036854775807" } } };
   dbcl.attachCL( subCSName + "." + subCLName2, attachDecimalOption2 );

   //insert decimal data in bound ;
   var validDecimalDoc = [{ a: -2147483648 },
   { a: 0 },
   { a: 2147483646 },
   { a: { $decimal: "-9223372036854775808", $precision: [100, 2] } },
   { a: { $decimal: "0" } },
   { a: { $decimal: "9223372036854775806" } },
   { a: { $decimal: "-4.7E-360" } },
   { a: { $decimal: "5.7E-400" } }];
   dbcl.insert( validDecimalDoc );

   //check decimal data in sub cl_1
   expDecimalRecs1 = [{ a: -2147483648 },
   { a: { $decimal: "-2147483648" } },
   { a: -2147483648 },
   { a: { $decimal: "-9223372036854775808.00", $precision: [100, 2] } },
   { a: { $decimal: "-0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000047" } }];
   checkResult( dbsubcl_1, null, null, expDecimalRecs1, { _id: 1 } );

   //check decimal data in sub cl_2
   expDecimalRecs2 = [{ a: 0 },
   { a: 2147483646 },
   { a: { $decimal: "0" } },
   { a: { $decimal: "2147483646.00", $precision: [100, 2] } },
   { a: 0 },
   { a: 2147483646 },
   { a: { $decimal: "0" } },
   { a: { $decimal: "9223372036854775806" } },
   { a: { $decimal: "0.00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000057" } }];
   checkResult( dbsubcl_2, null, null, expDecimalRecs2, { _id: 1 } );

   //insert decimal data out of bound and check result
   var invalidDecimalDoc = [{ age: { $decimal: "1.79E+400" } },
   { age: { $decimal: "-1.79E+500", $precision: [1000, 10] } }];
   invalidDataInsertCheckResult( dbcl, invalidDecimalDoc, -135 );
   commDropCS( db, mainCSName );
   commDropCS( db, subCSName );
}

