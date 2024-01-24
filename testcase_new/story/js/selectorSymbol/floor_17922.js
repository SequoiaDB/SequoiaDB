/************************************
*@Description: use floor:{field:{$floor:1}},the field type is object type(include nested object)
*@author:      wuyan
*@createdate:  2019.3.1
**************************************/
main( test );
function test ()
{
   var clName = "selector_floor_17922";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var doc = [{ no: 1, test: { c: -0.34, d: 1145.934 } },
   { no: 2, test: { a: { no: 0.12345 }, b: { no: { $decimal: "12.345" } } } }];
   dbcl.insert( doc );

   //the field is object 
   var findCondition1 = { no: 1 };
   var selectCondition1 = { "test.c": { $floor: 1 }, "test.d": { $floor: 1 } };
   var expRecs1 = [{ no: 1, test: { c: -1, d: 1145 } }];
   checkResult( dbcl, findCondition1, selectCondition1, expRecs1, { no: 1 } );

   //the field is nested object 
   var selectCondition2 = { "test.a.no": { $floor: 1 }, "test.b.no": { $floor: 1 } };
   var expRecs2 = [{ no: 1, test: { c: -0.34, d: 1145.934 } },
   { no: 2, test: { a: { no: 0 }, b: { no: { $decimal: "12" } } } }];
   checkResult( dbcl, null, selectCondition2, expRecs2, { no: 1 } );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the ending" );
}
