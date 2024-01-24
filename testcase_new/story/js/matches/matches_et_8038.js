/************************************************************************
*@Description:   seqDB-8038:使用$et查询，目标字段为数组且元素为数值型，不走索引查询
                     data type: array/object, and nested multiple
*@Author:  2016/5/16  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8038";

   var cl = readyCL( clName );

   var rawData = [{ int: [-2147483648, 2147483647] },
   { long: ["-9223372036854775808", "9223372036854775807"] }];
   insertRecs( cl, rawData );

   var arrRc = findRecs( cl, rawData, "array" );
   var objRc = findRecs( cl, rawData, "object" );

   checkResult( arrRc, objRc, rawData );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl, rawData )
{
   cl.insert( {
      a: 0, b: [{ b1: rawData[0]["int"][0] },
      { b2: rawData[0]["int"][1] }]
   } );
   cl.insert( {
      a: 1, b: {
         b1: { c1: { $numberLong: rawData[1]["long"][0] } },
         b2: { c2: { $numberLong: rawData[1]["long"][1] } }
      }
   } );
}

function findRecs ( cl, rawData, dataType )
{

   if( dataType === "array" )
   {
      var rc = cl.find( {
         "b.b1": { $et: rawData[0]["int"][0] },
         "b.b2": { $et: rawData[0]["int"][1] }
      } ).sort( { a: 1 } );
   }
   else if( dataType === "object" )
   {
      var rc = cl.find( {
         "b.b1.c1": { $et: { $numberLong: rawData[1]["long"][0] } },
         "b.b2.c2": { $et: { $numberLong: rawData[1]["long"][1] } }
      } ).sort( { a: 1 } );
   }

   return rc;
}

function checkResult ( arrRc, objRc, rawData )
{
   //-----------------------check result for dataType[array]---------------------

   var findRtn = new Array();
   while( tmpRecs = arrRc.next() )    //arrRc
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 1;
   assert.equal( findRtn.length, expLen );
   //compare records  ---$et: "array"
   if( findRtn[0]["b"][0]["b1"] !== rawData[0]["int"][0] ||   //expRecs--"b":[{"b1":-2147483648},{"b2":2147483647}]}]
      findRtn[0]["b"][1]["b2"] !== rawData[0]["int"][1] )  
   {
      throw new Error( "checkResult fail,[compare records]" +
         '["b.b1":' + rawData[0]["int"][0] + ',"b.b2":' + rawData[0]["int"][1] + "]" +
         '["b.b1":' + findRtn[0]["b"][0]["b1"] + ',"b.b2":' + findRtn[0]["b"][1]["b1"] + "]" );
   }

   //-----------------------check result for dataType[object]---------------------

   var findRtn = new Array();
   while( tmpRecs = objRc.next() )    //objRc
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 1;
   assert.equal( findRtn.length, expLen );
   //compare records  ---$et: "object"
   if( findRtn[0]["b"]["b1"]["c1"]["$numberLong"].toString() !== rawData[1]["long"][0] ||   //"b.b1.c1":{$numberlong:"xxxxxx"}
      findRtn[0]["b"]["b2"]["c2"]["$numberLong"].toString() !== rawData[1]["long"][1] )  
   {
      throw new Error( "checkResult fail,[compare records]" +
         '["b.b1.c1":' + rawData[1]["long"][0]
         + ',"b.b2.c2":' + rawData[1]["long"][1] + "]" +
         '["b.b1.c1":' + findRtn[0]["b"]["b1"]["c1"]["$numberLong"].toString()
         + ',"b.b2.c2":' + findRtn[0]["b"]["b1"]["c2"]["$numberLong"].toString() + "]" );
   }
}