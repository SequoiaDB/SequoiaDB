/************************************
*@Description: use default:{<fieldName>:<defaultValue>}, the field type is object type(include nested object)              
*@author:      wuyan
*@createdate:  2019.2.27
**************************************/
main( test );
function test ()
{
   var clName = "selector_default_17916";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var doc = [{ no: 1, test: true },
   { no: 2, test: { no: { "numberLong": "9223372036854775807" } } },
   { no: 3, test: { a: { name: "lily", age: 29 }, b: { name: "zhangsan", age: 30 } } }];
   dbcl.insert( doc );

   //test a: field is non-existent,specify default valude is object type   
   var selectCondition1 = { "object": { $default: { name: "test" } } };
   var findCondition1 = { no: 1 };
   var expRecs1 = [{ "object": { name: "test" } }];
   checkResult( dbcl, findCondition1, selectCondition1, expRecs1, { no: 1 } );

   //test b: specify default values for object type element  
   var selectCondition2 = { "test.a": { $default: { $decimal: "123.456" } } };
   var findCondition2 = { no: 2 };
   var expRecs2 = [{ test: { a: { $decimal: "123.456" } } }];
   checkResult( dbcl, findCondition2, selectCondition2, expRecs2, { no: 1 } );

   //test c: specify default values for nested object type element  
   var selectCondition3 = { "test.a.no": { $default: { $numberLong: "-9223372036854775808" } } };
   var findCondition3 = { no: 3 };
   var expRecs3 = [{ test: { a: { no: { $numberLong: "-9223372036854775808" } } } }];
   checkResult( dbcl, findCondition3, selectCondition3, expRecs3, { no: 1 } );

   //test insert data not changed
   checkResult( dbcl, null, null, doc, { no: 1 } );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the ending" );
}


