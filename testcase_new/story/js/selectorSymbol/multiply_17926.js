/************************************
*@Description: use multiply:{field:{$add:xx}},the field type is object type(include nested object)
*@author:      wuyan
*@createdate:  2019.3.2
**************************************/
main( test );
function test ()
{
   var clName = "selector_multiply_17926";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var doc = [{ no: 1, test: { c: 21474836, d: -1145.934 } },
   { no: 2, test: { a: { no: { num: { "$numberLong": "300000001" } } }, b: { no: { $decimal: "-12.345" } } } }];
   dbcl.insert( doc );

   //the field is object 
   var findCondition1 = { no: 1 };
   var selectCondition1 = { "test.c": { $multiply: 3 }, "test.d": { $multiply: -0.2345 } };
   var expRecs1 = [{ no: 1, test: { c: 64424508, d: 268.721523 } }];
   checkResult( dbcl, findCondition1, selectCondition1, expRecs1, { no: 1 } );

   //the field is nested object 
   var selectCondition2 = { "test.a.no.num": { $multiply: 2.35 }, "test.b.no": { $multiply: 20 } };
   var expRecs2 = [{ no: 1, test: { c: 21474836, d: -1145.934 } },
   { no: 2, test: { a: { no: { num: 705000002.35 } }, b: { no: { $decimal: "-246.900" } } } }];
   checkResult( dbcl, null, selectCondition2, expRecs2, { no: 1 } );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the ending" );
}
