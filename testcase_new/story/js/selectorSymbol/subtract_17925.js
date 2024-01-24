/************************************
*@Description: use subtract:{field:{$add:xx}},the field type is object type(include nested object)
*@author:      wuyan
*@createdate:  2019.3.2
**************************************/
main( test );
function test ()
{
   var clName = "selector_subtract_17925";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var doc = [{ no: 1, test: { c: 2147483648, d: -1145.934 } },
   { no: 2, test: { a: { no: { num: { "$numberLong": "92233720368547758" } } }, b: { no: { $decimal: "-12.345" } } } }];
   dbcl.insert( doc );

   //the field is object 
   var findCondition1 = { no: 1 };
   var selectCondition1 = { "test.c": { $subtract: 0.2 }, "test.d": { $subtract: { "$numberLong": "3000000000" } } };
   var expRecs1 = [{ no: 1, test: { c: 2147483647.8, d: -3000001145.934 } }];
   checkResult( dbcl, findCondition1, selectCondition1, expRecs1, { no: 1 } );

   //the field is nested object 
   var selectCondition2 = { "test.a.no.num": { $subtract: 2100 }, "test.b.no": { $subtract: -110 } };
   var expRecs2 = [{ no: 1, test: { c: 2147483648, d: -1145.934 } },
   { no: 2, test: { a: { no: { num: { "$numberLong": "92233720368545658" } } }, b: { no: { $decimal: "97.655" } } } }];
   checkResult( dbcl, null, selectCondition2, expRecs2, { no: 1 } );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the ending" );
}
