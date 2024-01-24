/************************************
*@Description: use divide:{field:{$add:xx}},the field type is object type(include nested object)
*@author:      wuyan
*@createdate:  2019.3.2
**************************************/
main( test );
function test ()
{
   var clName = "selector_divide_17927";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var doc = [{ no: 1, test: { c: 2147483648, d: { "$numberLong": "9223372036854775807" } } },
   { no: 2, test: { a: { no: { num: 1.2e+300 } }, b: { no: { $decimal: "-12.300" } } } }];
   dbcl.insert( doc );

   //the field is object 
   var findCondition1 = { no: 1 };
   var selectCondition1 = { "test.c": { $divide: 3 }, "test.d": { $divide: { "$numberLong": "200" } } };
   var expRecs1 = [{ no: 1, test: { c: 715827882.66666666666666666666667, d: { "$numberLong": "46116860184273879" } } }];
   checkResult( dbcl, findCondition1, selectCondition1, expRecs1, { no: 1 } );

   //the field is nested object 
   var selectCondition2 = { "test.a.no.num": { $divide: 2.35 }, "test.b.no": { $divide: 3 } };
   var expRecs2 = [{ no: 1, test: { c: 2147483648, d: { "$numberLong": "9223372036854775807" } } },
   { no: 2, test: { a: { no: { num: 5.106382978723404e+299 } }, b: { no: { $decimal: "-4.1000000000000000" } } } }];
   checkResult( dbcl, null, selectCondition2, expRecs2, { no: 1 } );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the ending" );
}
