/************************************
*@Description: use mod:{field:{$mod:xx}},the field type is object type(include nested object)
*@author:      wuyan
*@createdate:  2019.3.1
**************************************/
main( test );
function test ()
{
   var clName = "selector_mod_17923";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var doc = [{ no: 1, test: { c: -0.34, d: -1145.934 } },
   { no: 2, test: { a: { no: { num: -0.12345 } }, b: { no: { $decimal: "-12.345" } } } }];
   dbcl.insert( doc );

   //the field is object 
   var findCondition1 = { no: 1 };
   var selectCondition1 = { "test.c": { $mod: 0.2 }, "test.d": { $mod: 2 } };
   var expRecs1 = [{ no: 1, test: { c: -0.14, d: -1.933999999999969 } }];
   checkResult( dbcl, findCondition1, selectCondition1, expRecs1, { no: 1 } );

   //the field is nested object 
   var selectCondition2 = { "test.a.no.num": { $mod: 0.2 }, "test.b.no": { $mod: 10 } };
   var expRecs2 = [{ no: 1, test: { c: -0.34, d: -1145.934 } },
   { no: 2, test: { a: { no: { num: -0.12345 } }, b: { no: { $decimal: "-2.345" } } } }];
   checkResult( dbcl, null, selectCondition2, expRecs2, { no: 1 } );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the ending" );
}