/************************************
*@Description: use lower:{field:<$lower:Value>},the field type is object type(include nested object)
*@author:      wuyan
*@createdate:  2019.3.4
***************************************/
main( test );
function test ()
{
   var clName = "selector_lower_17935";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var doc = [{ no: 1, test: { c: "ABBCcds123#$%Ad ddddd", d: "AGSAAAD~!@abAcds123aga" } },
   { no: 2, test: { a: { no: { str: "AADCDEF3344dgabacvdgasdg fgdf" } }, b: { str: "TESTSAAaas#$$sdga" } } }];
   dbcl.insert( doc );

   //the field is object 
   var findCondition1 = { no: 1 };
   var selectCondition1 = { "test.c": { $lower: 1 }, "test.d": { $lower: 1 } };
   var expRecs1 = [{ no: 1, test: { c: "abbccds123#$%ad ddddd", d: "agsaaad~!@abacds123aga" } }];
   checkResult( dbcl, findCondition1, selectCondition1, expRecs1, { no: 1 } );

   //the field is nested object 
   var findCondition2 = { no: 2 };
   var selectCondition2 = { "test.a.no.str": { $lower: 1 }, "test.b.str": { $lower: 1 } };
   var expRecs2 = [{ no: 2, test: { a: { no: { str: "aadcdef3344dgabacvdgasdg fgdf" } }, b: { str: "testsaaaas#$$sdga" } } }];
   checkResult( dbcl, findCondition2, selectCondition2, expRecs2, { no: 1 } );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the ending" );
}

