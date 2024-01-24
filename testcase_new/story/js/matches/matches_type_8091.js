/************************************************************************
*@Description:   seqDB-8091:使用$type查询，不走索引查询 
      base type:    16--int32; 1--double;    10--null; 2--string; 8--bool;   3--subObj; 4--array; 
      special type: 18--int64; 100--decimal; 7--oid;   11--regex; 5--binary; 9--date;  17--timestamp;
                    cover all data type
*@Author:  2016/5/21  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8091";

   var cl = readyCL( clName );

   //typeNum: 14
   var dataType = ["int", "double", "null", "string", "bool", "subObj", "array",
      "long", "decimal", "oid", "regex", "binary", "date", "timestamp"];
   var rawData = [{ int: -2147483648 },
   { double: -1.7E+308 },
   { null: null },
   { string: "test" },
   { bool: true },
   { subObj: { "0": { c: "test" } } },
   { array: [{ c: "test" }] },
   { long: { "$numberLong": "-9223372036854775808" } },
   { decimal: { "$decimal": "111.001" } },
   { oid: { "$oid": "123abcd00ef12358902300ef" } },
   { regex: { "$regex": "^rg", "$options": "i" } },
   { binary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } },
   { date: { "$date": "2038-01-18" } },
   { timestamp: { "$timestamp": "2038-01-18-23.59.59.999999" } }];
   insertRecs( cl, rawData, dataType );

   var numType = [16, 1, 10, 2, 8, 3, 4, 18, 100, 7, 11, 5, 9, 17];
   var findRecsArray = findRecs( cl, dataType, numType );

   checkResult( findRecsArray, rawData, dataType );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl, rawData, dataType )
{

   for( i = 0; i < rawData.length; i++ )
   {
      cl.insert( { a: i, b: rawData[i][dataType[i]] } );
   }
}

function findRecs ( cl, dataType, numType )
{

   var findRecsArray = [];
   for( i = 0; i < numType.length; i++ )
   {

      var rc = cl.find( { b: { $type: 1, $et: numType[i] } } ).sort( { a: 1 } );
      var tmpArray = [];
      while( tmpRecs = rc.next() )
      {
         tmpArray.push( tmpRecs.toObj() );
      }

      findRecsArray.push( tmpArray );
   }
   return findRecsArray;
}

function checkResult ( findRecsArray, rawData, dataType )
{

   for( i = 0; i < dataType.length; i++ )
   {

      var actLen = findRecsArray[i].length;
      var expLen = 1;
      assert.equal( actLen, expLen );

      if( i < 5 )
      {
         var actB = findRecsArray[i][0]["b"];
         var expB = rawData[i][dataType[i]];
      }
      else if( i === 5 )
      {
         var actB = findRecsArray[i][0]["b"].toString();
         var expB = rawData[i]["subObj"].toString();
      }
      else if( i > 5 )
      {
         var actB = findRecsArray[i][0]["b"].toString();
         var expB = rawData[i][dataType[i]].toString();
      }

      assert.equal( actB, expB );

   }
}