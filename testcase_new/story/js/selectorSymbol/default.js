/************************************
*@Description: use default:{<fieldName>:<defaultValue>}, 
               1.field exists or not
               2.simple formart or stardard format  
               3.legal or illegal data
               4.many fields combination
*@author:      zhaoyu
*@createdate:  2016.7.14
*@testlinkCase: seqDB-5595/seqDB-5596/seqDB-5597/seqDB-5598/seqDB-5599/seqDB-8213
**************************************/
main( test );

function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data 
   var doc = [{ No: 1, name: "zhangsan", age: 18 },
   { No: 2, name: "lisi" },
   { No: 3, name: "wangwu" }];
   dbcl.insert( doc );

   //field is non-existent,command use simple format and standard format,seqDB-5595/seqDB-5596/seqDB-5599/seqDB-8213
   var selectCondition1 = { age: 20, age1: 30, age2: { $default: 40 } };
   var expRecs1 = [{ age: 18, age1: 30, age2: 40 },
   { age: 20, age1: 30, age2: 40 },
   { age: 20, age1: 30, age2: 40 }];
   checkResult( dbcl, null, selectCondition1, expRecs1, { No: 1 } );

   //field is non-existent,cover all type legal data,seqDB-5597
   var allTypeData = [-2147483648, 2147483647,
   { $numberLong: "-9223372036854775808" }, { $numberLong: "9223372036854775807" },
   -1.7E+308, 1.7E+308,
      "abc",
      true, false,
   { $oid: "123abcd00ef12358902300ef" },
   { $date: "1900-01-01" }, { "$date": "9999-12-31" },
   { $timestamp: "1902-01-01-00.00.00.000000" }, { "$timestamp": "2037-12-31-23.59.59.999999" },
   { $binary: "aGVsbG8gd29ybGQ=", "$type": "1" },
   { $regex: "^z", "$options": "i" },
   { subobj: "value" },
   { a: ["abc", 0, "def"] },
      null,
   MinKey(), MaxKey()];
   getRdmDataFromArrAndCheck( dbcl, allTypeData );

   //field is non-existent,cover all type illegal data,seqDB-5598
   var selectCondition2 = { age: { $numberLong: "-9223372036854775809" } };
   var expRecs2 = [{ age: 18 },
   { age: { $numberLong: "-9223372036854775808" } },
   { age: { $numberLong: "-9223372036854775808" } }];
   checkResult( dbcl, null, selectCondition2, expRecs2, { No: 1 } );

   var selectCondition3 = { age: { $numberLong: "9223372036854775808" } };
   var expRecs3 = [{ age: 18 },
   { age: { $numberLong: "9223372036854775807" } },
   { age: { $numberLong: "9223372036854775807" } }];
   checkResult( dbcl, null, selectCondition3, expRecs3, { No: 1 } );

   var selectCondition4 = { age: -1.8E+308 };
   var expRecs4 = [{ age: 18 },
   { age: -Infinity },
   { age: -Infinity }];
   checkResult( dbcl, null, selectCondition4, expRecs4, { No: 1 } );

   var selectCondition5 = { age: 1.8E+308 };
   var expRecs5 = [{ age: 18 },
   { age: Infinity },
   { age: Infinity }];
   checkResult( dbcl, null, selectCondition5, expRecs5, { No: 1 } );

   //then find all data,check result
   checkResult( dbcl, null, null, doc, { No: 1 } );
}

/************************************
*@Description: get random data from a arr,use default,then check result.
*@author:      zhaoyu 
*@createDate:  2016/7/14
*@parameters:               
**************************************/
function getRdmDataFromArrAndCheck ( dbcl, arr )
{
   var data = arr[Math.floor( Math.random() * arr.length )];
   if( data == "MinKey()" )
   {
      data = { $minKey: 1 }
   }
   else if( data == "MaxKey()" )
   {
      data = { $maxKey: 1 }
   }
   var rc = dbcl.find( {}, { age1: data } );
   var expRecs = [];
   var expArrLength = dbcl.count();
   for( var i = 0; i < expArrLength; i++ )
   {
      expRecs.push( { age1: data } )
   }
   checkRec( rc, expRecs );
}
