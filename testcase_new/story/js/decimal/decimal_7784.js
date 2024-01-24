/************************************
*@Description: hash split for decimal data,verify two part function:
               part1:split condition is int type,and insert decimal data;
               part2:split condition is decimal type,and insert decimal data; 
*@author:      zhaoyu
*@createdate:  2016.4.27
**************************************/
main( test )
function test ()
{
   var clName = COMMCLNAME + "_7784";
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName );

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

   //create cl for hash split
   var ClOption = { ShardingKey: { "age": 1 }, ShardingType: "hash" };
   var dbcl = commCreateCL( db, COMMCSNAME, clName, ClOption, true, true );

   //insert decimal data befor split;
   var doc = [{ age: { $decimal: "123" } },
   { age: { $decimal: "123", $precision: [10, 2] } },
   { age: 123 },
   { age: "123" },
   { age: { $numberLong: "123" } },
   { age: 123.5687 },
   { age: { $decimal: "123.5687", $precision: [10, 4] } },
   { age: { $decimal: "123.5687" } },
   { age: 0 },
   { age: { $decimal: "0" } },
   { age: { $decimal: "0", $precision: [10, 5] } },
   { age: { $decimal: "-5.94E-400" } },
   { age: { $decimal: "6.94E-500" } },
   { age: { $decimal: "-1.99E+700" } },
   { age: { $decimal: "2.99E+600" } },
   { age: { $decimal: "9" } },
   { age: { $decimal: "10" } },
   { age: { $decimal: "9999" } },
   { age: { $decimal: "10000" } }];
   dbcl.insert( doc );

   //split cl
   startCondition = { Partition: 2014 };
   splitGrInfo = ClSplitOneTimes( COMMCSNAME, clName, startCondition, null );

   //check decimal data in src group and des group
   var expRecsLength1 = 19;
   checkHashClSplitResult( db, clName, splitGrInfo, null, null, expRecsLength1, { _id: 1 } );

   //insert decimal data after split
   doc1 = [{ age: { $decimal: "2345" } },
   { age: { $decimal: "2345", $precision: [10, 5] } },
   { age: 2345 },
   { age: { $decimal: "5", $precision: [5, 2] } },
   { age: { $decimal: "100000", $precision: [10, 2] } }];
   dbcl.insert( doc1 );

   //check decimal data in src group and des group
   var expRecsLength2 = 24;
   checkHashClSplitResult( db, clName, splitGrInfo, null, null, expRecsLength2, { _id: 1 } );
   commDropCL( db, COMMCSNAME, clName );
}

