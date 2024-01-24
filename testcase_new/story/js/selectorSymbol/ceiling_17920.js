/************************************
*@Description: use ceiling:{field:{$ceiling:1}},the field type is object type(include nested object)
*@author:      wuyan
*@createdate:  2019.3.1
**************************************/
main( test );
function test ()
{
   var clName = "selector_ceiling_17920";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var doc = [{ no: 1, test: { c: -0.34, d: 1.934 } },
   { no: 2, test: { a: { no: 0.12345 }, b: { no: { $decimal: "12.345" } } } }];
   dbcl.insert( doc );

   //the field is object 
   var findCondition1 = { no: 1 };
   var selectCondition1 = { "test.c": { $ceiling: 1 } };
   var expRecs1 = [{ no: 1, test: { c: 0, d: 1.934 } }];
   checkResult( dbcl, findCondition1, selectCondition1, expRecs1, { no: 1 } );

   //the field is nested object 
   var selectCondition2 = { "test.a.no": { $ceiling: 1 }, "test.b.no": { $ceiling: 1 } };
   var expRecs2 = [{ no: 1, test: { c: -0.34, d: 1.934 } },
   { no: 2, test: { a: { no: 1 }, b: { no: { $decimal: "13" } } } }];
   checkResult( dbcl, null, selectCondition2, expRecs2, { no: 1 } );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the ending" );
}
