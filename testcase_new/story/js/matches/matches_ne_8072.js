/************************************************************************
*@Description:  seqDB-8072:使用$ne查询，目标字段为数值型，走索引查询
                     data type: int/long/double;   index: {b:1, c:1}
*@Author:  2016/5/16  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8072";
   var indexName = CHANGEDPREFIX + "_index";

   var cl = readyCL( clName );
   createIndex( cl, indexName );

   var rawData = [{ int: [-2147483648, 2147483647] },
   { long: ["-9223372036854775808", "9223372036854775807"] },
   { double: [-1.7E+308, 1.7E+308] }];
   insertRecs( cl, rawData );

   var intRc = findRecs( cl, rawData, "int" );
   var longRc = findRecs( cl, rawData, "long" );
   var doubleRc = findRecs( cl, rawData, "double" );

   checkResult( intRc, longRc, doubleRc, rawData, indexName );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function createIndex ( cl, indexName )
{

   cl.createIndex( indexName, { b: 1, c: 1 } );
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
         b: { $ne: rawData[0]["int"][0] },
         c: { $ne: rawData[0]["int"][1] }
      } ).sort( { a: 1 } );
   }
   else if( dataType === "long" )
   {
      var rc = cl.find( {
         b: { $ne: { $numberLong: rawData[1]["long"][0] } },
         c: { $ne: { $numberLong: rawData[1]["long"][1] } }
      } ).sort( { a: 1 } );
   }
   else if( dataType === "double" )
   {
      var rc = cl.find( {
         b: { $ne: rawData[2]["double"][0] },
         c: { $ne: rawData[2]["double"][1] }
      } ).sort( { a: 1 } );
   }

   return rc;
}

function checkResult ( intRc, longRc, doubleRc, rawData, indexName )
{
   //
   ////compare scanType
   //var tmpExp = intRc.explain().current().toObj();
   //if( tmpExp["ScanType"] !== "ixscan" || tmpExp["IndexName"] !== indexName )
   //{
   //   throw new Error("checkResult", null, "[compare index]", 
   //                        "[ScanType:ixscan,IndexName:"+ indexName +"]", 
   //                        "[ScanType:"+ tmpExp["ScanType"] +",IndexName:"+ tmpExp["IndexName"] +"]");
   //}

   //-----------------------check result for dataType[int]---------------------

   var findRtn = new Array();
   while( tmpRecs = intRc.next() )  //incRc
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 2;
   assert.equal( findRtn.length, expLen );
   //compare records  ---$ne: "int"
   if( findRtn[0]["b"]["$numberLong"].toString() !== rawData[1]["long"][0] ||  //b:{$numberlong:"xxxxxx"}
      findRtn[1]["b"] !== rawData[2]["double"][0] )
   {
      throw new Error( "checkResult fail,[compare records]" +
         "[b:" + rawData[1]["long"][0] + ",b:" + rawData[2]["double"][0] + "]" +
         "[b:" + findRtn[0]["b"]["$numberLong"].toString() + ",b:" + findRtn[1]["b"] + "]" );
   }

   //-----------------------check result for dataType[long]---------------------

   var findRtn = new Array();
   while( tmpRecs = longRc.next() )  //longRc
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 2;
   assert.equal( findRtn.length, expLen );
   //compare records  ---$ne: "long"
   if( findRtn[0]["b"] !== rawData[0]["int"][0] ||
      findRtn[1]["b"] !== rawData[2]["double"][0] )
   {
      throw new Error( "checkResult fail,[compare records]" +
         "[b:" + rawData[0]["int"][0] + ",b:" + rawData[2]["double"][0] + "]" +
         "[b:" + findRtn[0]["b"] + ",b:" + findRtn[1]["b"] + "]" );
   }

   //-----------------------check result for dataType[double]---------------------

   var findRtn = new Array();
   while( tmpRecs = doubleRc.next() )  //doubleRc
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 2;
   assert.equal( findRtn.length, expLen );
   //compare records  ---$ne: "double"
   if( findRtn[0]["b"] !== rawData[0]["int"][0] ||
      findRtn[1]["b"]["$numberLong"].toString() !== rawData[1]["long"][0] )  //b:{$numberlong:"xxxxxx"}
   {
      throw new Error( "checkResult fail,[compare records]" +
         "[b:" + rawData[0]["int"][0] + ",b:" + rawData[1]["long"][0] + "]" +
         "[b:" + findRtn[0]["b"]["$numberLong"].toString() + ",b:" + findRtn[1]["b"] + "]" );
   }
}
