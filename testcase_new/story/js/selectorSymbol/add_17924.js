/************************************
*@Description: use add:{field:{$add:xx}},the field type is object type(include nested object)
*@author:      wuyan
*@createdate:  2019.3.1
**************************************/
main( test );
function test ()
{
   var clName = "selector_add_17924";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var doc = [{ no: 1, test: { c: 12, d: -1145.934 } },
   { no: 2, test: { a: { no: { float: 1.2e+300 } }, b: { no: { $decimal: "-12.345" } } } }];
   dbcl.insert( doc );

   //the field is object 
   var findCondition1 = { no: 1 };
   var selectCondition1 = { "test.c": { $add: 0.2 }, "test.d": { $add: { "$numberLong": "3000000000" } } };
   var expRecs1 = [{ no: 1, test: { c: 12.2, d: 2999998854.066 } }];
   checkResult( dbcl, findCondition1, selectCondition1, expRecs1, { no: 1 } );

   //the field is nested object 
   var selectCondition2 = { "test.a.no.float": { $add: 1.3e+299 }, "test.b.no": { $add: 10 } };
   var expRecs2 = [{ no: 1, test: { c: 12, d: -1145.934 } },
   { no: 2, test: { a: { no: { float: 1.33e+300 } }, b: { no: { $decimal: "-2.345" } } } }];
   checkResult( dbcl, null, selectCondition2, expRecs2, { no: 1 } );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the ending" );
}
