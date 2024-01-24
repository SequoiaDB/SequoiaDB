/************************************
*@Description: use rtrim:{field:<$ltrim:Value>},the field type is object type(include nested object)
*@author:      wuyan
*@createdate:  2019.3.4
***************************************/
main( test );
function test ()
{
   var clName = "selector_rtrim_17938";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var doc = [{ no: 1, test: { c: " Ad23#$%Ad dd ", d: "\tadgcd123\t" } },
   { no: 2, test: { a: { no: { str: "\rabcdef\r" } }, b: { str: "\n12adgaaa\naaB\n" } } }];
   dbcl.insert( doc );

   //the field is object 
   var findCondition1 = { no: 1 };
   var selectCondition1 = { "test.c": { $rtrim: 1 }, "test.d": { $rtrim: 1 } };
   var expRecs1 = [{ no: 1, test: { c: " Ad23#$%Ad dd", d: "\tadgcd123" } }];
   checkResult( dbcl, findCondition1, selectCondition1, expRecs1, { no: 1 } );

   //the field is nested object 
   var findCondition2 = { no: 2 };
   var selectCondition2 = { "test.a.no.str": { $rtrim: 1 }, "test.b.str": { $rtrim: 1 } };
   var expRecs2 = [{ no: 2, test: { a: { no: { str: "\rabcdef" } }, b: { str: "\n12adgaaa\naaB" } } }];
   checkResult( dbcl, findCondition2, selectCondition2, expRecs2, { no: 1 } );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the ending" );
}

