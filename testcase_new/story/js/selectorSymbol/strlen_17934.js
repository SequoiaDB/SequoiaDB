﻿/************************************
*@Description: use strlen:{field:<$strlen:Value>},the field type is object type(include nested object)
*@author:      wuyan
*@createdate:  2019.3.4
**************************************/
main( test );
function test ()
{
   var clName = "selector_strlen_17934";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var doc = [{ no: 1, test: { c: "abcds123#$%Ad ddddd", d: "aggaag~!@abcds123aga " } },
   { no: 2, test: { a: { no: { str: " adg3344dgabacvdgasdg fgdf" } }, b: { str: "testadgsdasdg#$$sdga" } } }];
   dbcl.insert( doc );

   //the field is object 
   var findCondition1 = { no: 1 };
   var selectCondition1 = { "test.c": { $strlen: 1 }, "test.d": { $strlen: 1 } };
   var expRecs1 = [{ no: 1, test: { c: 19, d: 21 } }];
   checkResult( dbcl, findCondition1, selectCondition1, expRecs1, { no: 1 } );

   //the field is nested object 
   var findCondition2 = { no: 2 };
   var selectCondition2 = { "test.a.no.str": { $strlen: 1 }, "test.b.str": { $strlen: 1 } };
   var expRecs2 = [{ no: 2, test: { a: { no: { str: 26 } }, b: { str: 20 } } }];
   checkResult( dbcl, findCondition2, selectCondition2, expRecs2, { no: 1 } );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the ending" );
}
