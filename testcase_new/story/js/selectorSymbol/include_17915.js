/************************************
*@Description: use include:{field:<$include:Value>}, 
               1.field type is object type(include nested object)
               2.Value set 0 or !0                 
*@author:      wuyan
*@createdate:  2019.2.27
**************************************/
main( test );
function test ()
{
   var clName = "selector_include_17915";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var doc = [{ no: 1, test: { c: 1 } },
   { no: 2, test: { name: "lili", age: 19 } },
   { no: 3, test: { a: { name: "lily", age: 29 }, b: { name: "zhangsan", age: 30 } } }];
   dbcl.insert( doc );

   //test a: field is object ,value set 1    
   var selectCondition1 = { test: { $include: 1 } };
   var expRecs1 = [{ test: { c: 1 } },
   { test: { name: "lili", age: 19 } },
   { test: { a: { name: "lily", age: 29 }, b: { name: "zhangsan", age: 30 } } }];
   checkResult( dbcl, null, selectCondition1, expRecs1, { no: 1 } );

   //test b: field is elements in object,value set 1
   var selectCondition2 = { "test.name": { $include: 1 } };
   var findCondition2 = { no: 2 };
   var expRecs2 = [{ test: { name: "lili" } }];
   checkResult( dbcl, findCondition2, selectCondition2, expRecs2, { no: 1 } );

   //test c: field is elements in nested object,value set 1
   var selectCondition3 = { "test.b.name": { $include: 1 } };
   var findCondition3 = { no: 3 };
   var expRecs3 = [{ test: { b: { name: "zhangsan" } } }];
   checkResult( dbcl, findCondition3, selectCondition3, expRecs3, { no: 1 } );

   //test a1: field is object ,value set 0    
   var selectCondition4 = { test: { $include: 0 } };
   var expRecs4 = [{ no: 1 }, { no: 2 }, { no: 3 }];
   checkResult( dbcl, null, selectCondition4, expRecs4, { no: 1 } );

   //test b1: field is elements in object,value set 0
   var selectCondition5 = { "test.name": { $include: 0 } };
   var findCondition5 = { no: 2 };
   var expRecs5 = [{ no: 2, test: { age: 19 } }];
   checkResult( dbcl, findCondition5, selectCondition5, expRecs5, { no: 1 } );

   //test c1: field is elements in nested object,value set 0
   var selectCondition6 = { "test.b.name": { $include: 0 } };
   var findCondition6 = { no: 3 };
   var expRecs6 = [{ no: 3, test: { a: { name: "lily", age: 29 }, b: { age: 30 } } }];
   checkResult( dbcl, findCondition6, selectCondition6, expRecs6, { no: 1 } );

   //test insert data not changed
   checkResult( dbcl, null, null, doc, { no: 1 } );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the ending" );
}

