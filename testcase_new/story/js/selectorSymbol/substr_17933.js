/************************************
*@Description: use substr:{field:[<xx>,xx]},the field type is object type(include nested object)
*@author:      wuyan
*@createdate:  2019.3.4
**************************************/
main( test );
function test ()
{
   var clName = "selector_substr_17933";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var doc = [{ no: 1, test: { c: "abcds123#$%Adddddd", d: "aggaag~!@abcds123aga" } },
   { no: 2, test: { a: { no: { str: "adg3344dgabacvdgasdgfgdf" } }, b: { str: "testadgsdasdg#$$sdga" } } }];
   dbcl.insert( doc );

   //the field is object 
   var findCondition1 = { no: 1 };
   var selectCondition1 = { "test.c": { $substr: [5, 5] }, "test.d": { $substr: [-5, 16] } };
   var expRecs1 = [{ no: 1, test: { c: "123#$", d: "23aga" } }];
   checkResult( dbcl, findCondition1, selectCondition1, expRecs1, { no: 1 } );

   //the field is nested object 
   var findCondition2 = { no: 2 };
   var selectCondition2 = { "test.a.no.str": { $substr: 10 }, "test.b.str": { $substr: [-10, 5] } };
   var expRecs2 = [{ no: 2, test: { a: { no: { str: "adg3344dga" } }, b: { str: "sdg#$" } } }];
   checkResult( dbcl, findCondition2, selectCondition2, expRecs2, { no: 1 } );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the ending" );
}
