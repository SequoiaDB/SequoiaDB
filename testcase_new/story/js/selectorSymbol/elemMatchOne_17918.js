/************************************
*@Description: use elemMatchOne,query the elements in the nested object that satisfy the criteria
*@author:      wuyan
*@createdate:  2019.2.27
**************************************/
main( test );
function test ()
{
   var clName = "selector_elemMatchOne_17918";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var doc = [{ no: 1, test: { c: 1 } },
   { no: 2, test: { name: "lili", age: 19 } },
   { no: 3, test: { a: { name: "lily", age: 29 }, b: { name: "zhangsan", age: 29 } } }];
   dbcl.insert( doc );

   var findCondition = { no: 3 };
   var selectCondition = { "test": { $elemMatchOne: { "a.age": 29 } } };
   var expRecs = [{ no: 3, test: { a: { name: "lily", age: 29 } } }];
   checkResult( dbcl, findCondition, selectCondition, expRecs, { no: 1 } );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the ending" );
}

