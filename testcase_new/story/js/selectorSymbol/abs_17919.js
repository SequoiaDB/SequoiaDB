/************************************
*@Description: use abs:{field:{$abs:1}},the field type is object type(include nested object)
*@author:      wuyan
*@createdate:  2019.3.1
**************************************/
main( test );
function test ()
{
   var clName = "selector_abs_17919";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var doc = [{ no: 1, test: { c: -23, d: -2.34 } },
   { no: 2, test: { a: { no: { "$numberLong": "-9223372036854775807" } }, b: { no: -2.45E+10 } } }];
   dbcl.insert( doc );

   //the field is obj 
   var findCondition1 = { no: 1 };
   var selectCondition1 = { "test.c": { $abs: 1 } };
   var expRecs1 = [{ no: 1, test: { c: 23, d: -2.34 } }];
   checkResult( dbcl, findCondition1, selectCondition1, expRecs1, { no: 1 } );

   //the field is nested obj
   var findCondition2 = { no: 2 };
   var selectCondition2 = { "test.a.no": { $abs: 1 } };
   var expRecs2 = [{ no: 2, test: { a: { no: { "$numberLong": "9223372036854775807" } }, b: { no: -2.45E+10 } } }];
   checkResult( dbcl, findCondition2, selectCondition2, expRecs2, { no: 1 } );

   //the field is nested obj,no findCondition
   var selectCondition3 = { "test.b.no": { $abs: 1 } };
   var expRecs3 = [{ no: 1, test: { c: -23, d: -2.34 } },
   { no: 2, test: { a: { no: { "$numberLong": "-9223372036854775807" } }, b: { no: 2.45E+10 } } }];
   checkResult( dbcl, null, selectCondition3, expRecs3, { no: 1 } );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the ending" );
}
