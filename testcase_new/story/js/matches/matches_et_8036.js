/************************************************************************
*@Description:  seqDB-8036:使用$et查询，目标字段为数值型，不走索引查询
                     data type: int/long/double
*@Author:  2016/5/18  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8036";

   var cl = readyCL( clName );

   var rawData = [{ int: [-2147483648, 2147483647] },
   { long: ["-9223372036854775808", "9223372036854775807"] },
   { double: [-1.7E+308, 1.7E+308] }];
   insertRecs( cl, rawData );

   var intRc = findRecs( cl, rawData, "int" );
   var longRc = findRecs( cl, rawData, "long" );
   var doubleRc = findRecs( cl, rawData, "double" );

   checkResult( intRc, longRc, doubleRc, rawData );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl, rawData )
{
   cl.insert( {
      a: 0, b: rawData[0]["int"][0],
      c: rawData[0]["int"][1]
   } );
   cl.insert( {
      a: 1, b: { $numberLong: rawData[1]["long"][0] },
      c: { $numberLong: rawData[1]["long"][1] }
   } );
   cl.insert( {
      a: 2, b: rawData[2]["double"][0],
      c: rawData[2]["double"][1]
   } );
}

function findRecs ( cl, rawData, dataType )
{

   if( dataType === "int" )
   {
      var rc = cl.find( {
         b: { $et: rawData[0]["int"][0] },
         c: { $et: rawData[0]["int"][1] }
      } ).sort( { a: 1 } );
   }
   else if( dataType === "long" )
   {
      var rc = cl.find( {
         b: { $et: { $numberLong: rawData[1]["long"][0] } },
         c: { $et: { $numberLong: rawData[1]["long"][1] } }
      } ).sort( { a: 1 } );
   }
   else if( dataType === "double" )
   {
      var rc = cl.find( {
         b: { $et: rawData[2]["double"][0] },
         c: { $et: rawData[2]["double"][1] }
      } ).sort( { a: 1 } );
   }

   return rc;
}

function checkResult ( intRc, longRc, doubleRc, rawData )
{
   //-----------------------check result for dataType[int]---------------------

   var findRtn = new Array();
   while( tmpRecs = intRc.next() )  //incRc
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 1;
   assert.equal( findRtn.length, expLen );
   //compare records  ---$et: "int"
   if( findRtn[0]["b"] !== rawData[0]["int"][0] ||
      findRtn[0]["c"] !== rawData[0]["int"][1] )
   {
      throw new Error( "checkResult fail,[compare records]" +
         "[b:" + rawData[0]["int"][0] + ",c:" + rawData[0]["int"][1] + "]" +
         "[b:" + findRtn[0]["b"] + ",c:" + findRtn[0]["c"] + "]" );
   }

   //-----------------------check result for dataType[long]---------------------

   var findRtn = new Array();
   while( tmpRecs = longRc.next() )  //longRc
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 1;
   assert.equal( findRtn.length, expLen );
   //compare records  ---$et: "long"
   if( findRtn[0]["b"]["$numberLong"].toString() !== rawData[1]["long"][0] ||
      findRtn[0]["c"]["$numberLong"].toString() !== rawData[1]["long"][1] )
   {
      throw new Error( "checkResult fail,[compare records]" +
         "[b:" + rawData[1]["long"][0]
         + ",c:" + rawData[1]["long"][1] + "]" +
         "[b:" + findRtn[0]["b"]["$numberLong"].toString()
         + ",c:" + findRtn[0]["c"]["$numberLong"].toString() + "]" );
   }

   //-----------------------check result for dataType[double]---------------------

   var findRtn = new Array();
   while( tmpRecs = doubleRc.next() )  //doubleRc
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 1;
   assert.equal( findRtn.length, expLen );
   //compare records  ---$et: "double"
   if( findRtn[0]["b"] !== rawData[2]["double"][0] ||
      findRtn[0]["c"] !== rawData[2]["double"][1] )
   {
      throw new Error( "checkResult fail,[compare records]" +
         "[b:" + rawData[2]["double"][0] + ",c:" + rawData[2]["double"][1] + "]" +
         "[b:" + findRtn[0]["b"] + ",c:" + findRtn[0]["c"] + "]" );
   }
}