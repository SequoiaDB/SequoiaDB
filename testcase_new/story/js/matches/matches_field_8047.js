/************************************************************************
*@Description:   seqDB-8047:使用$field查询，t1字段和t2字段为相同数据类型
                    cover all data type
*@Author:  2016/5/25  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8047";
   var cl = readyCL( clName );

   var rawData = insertRecs( cl );

   var findRecsArray = findRecs( cl );
   checkResult( findRecsArray, rawData );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl )
{

   var rawData = [{
      a: 0, a2: 0,
      int: -2147483648, int2: -2147483648,
      double: -1.7E+308, double2: -1.7E+308,
      null: null, null2: null,
      string: "test", string2: "test",
      bool: true, bool2: true,
      subObj: { "0": { c: "test" } }, subObj2: { "0": { c: "test" } },
      //array:  [2, {c:"test"}],  array2:  [2, {c:"test"}],  //$field can not match array
      long: { "$numberLong": "-9223372036854775808" }, long2: { "$numberLong": "-9223372036854775808" },
      decimal: { "$decimal": "111.001" }, decimal2: { "$decimal": "111.001" },
      oid: { "$oid": "123abcd00ef12358902300ef" }, oid2: { "$oid": "123abcd00ef12358902300ef" },
      regex: { "$regex": "^rg", "$options": "i" }, regex2: { "$regex": "^rg", "$options": "i" },
      binary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" }, binary2: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
      date: { "$date": "2038-01-18" }, date2: { "$date": "2038-01-18" },
      timestamp: { "$timestamp": "2038-01-18-23.59.59.999999" }, timestamp2: { "$timestamp": "2038-01-18-23.59.59.999999" }
   },
   {
      a: 1, a2: 2,
      int: -2147483648, int2: 2147483647,
      double: -1.7E+308, double2: 1.7E+308,
      null: null, null2: "",
      string: "test", string2: "hello",
      bool: true, bool2: false,
      subObj: { "0": { c: "test" } }, subObj2: { "0": { c: "hello" } },
      //array:  [2, {c:"test"}],  array2:  [2, {c:"hello"}], 
      long: { "$numberLong": "-9223372036854775808" }, long2: { "$numberLong": "9223372036854775807" },
      decimal: { "$decimal": "111.001" }, decimal2: { "$decimal": "222.002" },
      oid: { "$oid": "123abcd00ef12358902300ef" }, oid2: { "$oid": "123abcd00ef1235890230099" },
      regex: { "$regex": "^rg", "$options": "i" }, regex2: { "$regex": "^ab", "$options": "i" },
      binary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" }, binary2: { "$binary": "aGVsbG8gd39ybGQ=", "$type": "1" },
      date: { "$date": "2038-01-18" }, date2: { "$date": "1999-01-18" },
      timestamp: { "$timestamp": "2038-01-18-23.59.59.999999" }, timestamp2: { "$timestamp": "1999-01-18-23.59.59.999999" }
   }];
   cl.insert( rawData );

   return rawData;
}

function findRecs ( cl )
{
   dataType = ["int", "double", "null", "string",  //"array", 
      "long", "decimal", "oid", "regex", "binary", "date", "timestamp"];
   var rmNum = parseInt( Math.random() * dataType.length );

   //field variable
   var cond = new Object();
   var field1 = dataType[rmNum];
   cond[field1] = { $field: dataType[rmNum] + 2 };
   //condition of find
   //find
   var rc = cl.find( cond, { _id: { $include: 0 } } ).sort( { a: 1 } );

   var findRecsArray = [];
   while( tmpRecs = rc.next() )
   {
      findRecsArray.push( tmpRecs.toObj() );
   }
   return findRecsArray;
}

function checkResult ( findRecsArray, rawData )
{

   var expLen = 1;
   assert.equal( findRecsArray.length, expLen );
   var actRecs = JSON.stringify( findRecsArray[0] );
   var extRecs = JSON.stringify( rawData[0] );
   assert.equal( actRecs, extRecs );
}