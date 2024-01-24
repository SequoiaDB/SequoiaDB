/************************************
*@Description: use upper:{field:<$upper:Value>},the field type is object type(include nested object)
*@author:      wuyan
*@createdate:  2019.3.4
***************************************/
main( test );
function test ()
{
   var clName = "selector_upper_17936";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var doc = [{ no: 1, test: { c: "AdaEEcds123#$%Ad ddddd", d: "aggaag~!@abcds123aga " } },
   { no: 2, test: { a: { no: { str: "adg3344dgAECcvdgasfgdf" } }, b: { str: "testadAAgsdasdg#$$sdga" } } }];
   dbcl.insert( doc );

   //the field is object 
   var findCondition1 = { no: 1 };
   var selectCondition1 = { "test.c": { $upper: 1 }, "test.d": { $upper: 1 } };
   var expRecs1 = [{ no: 1, test: { c: "ADAEECDS123#$%AD DDDDD", d: "AGGAAG~!@ABCDS123AGA " } }];
   checkResult( dbcl, findCondition1, selectCondition1, expRecs1, { no: 1 } );

   //the field is nested object 
   var findCondition2 = { no: 2 };
   var selectCondition2 = { "test.a.no.str": { $upper: 1 }, "test.b.str": { $upper: 1 } };
   var expRecs2 = [{ no: 2, test: { a: { no: { str: "ADG3344DGAECCVDGASFGDF" } }, b: { str: "TESTADAAGSDASDG#$$SDGA" } } }];
   checkResult( dbcl, findCondition2, selectCondition2, expRecs2, { no: 1 } );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the ending" );
}

