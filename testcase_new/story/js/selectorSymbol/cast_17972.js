/************************************
*@Description: use cast:{field:<$cast:Value>},the field type is nested object type
*@author:      wuyan
*@createdate:  2019.3.4
***************************************/
main( test );
function test ()
{
   var clName = "selector_cast_17972";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var doc = [{ no: 1, test: { c: "testa", d: true } },
   { no: 2, test: { a: { no: { num: { "$numberLong": "92233720368547758" } } }, b: { no: { $decimal: "-12.345" } } } }];
   dbcl.insert( doc );

   //the field is object 
   var findCondition1 = { no: 1 };
   var selectCondition1 = { "test.c": { $cast: "string" }, "test.d": { $cast: "bool" } };
   var expRecs1 = [{ no: 1, test: { c: "testa", d: true } }];
   checkResult( dbcl, findCondition1, selectCondition1, expRecs1, { no: 1 } );

   //the field is nested object 
   var findCondition2 = { no: 2 };
   var selectCondition2 = { "test.a.no.num": { $cast: "int64" }, "test.b.no": { $cast: "decimal" } };
   var expRecs2 = [{ no: 2, test: { a: { no: { num: { "$numberLong": "92233720368547758" } } }, b: { no: { $decimal: "-12.345" } } } }];
   checkResult( dbcl, findCondition2, selectCondition2, expRecs2, { no: 1 } );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the ending" );
}

