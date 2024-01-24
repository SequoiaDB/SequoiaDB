/************************************************************************
*@Description:     seqDB-8043:使用$et查询，目标字段与给定值为不同类型，走索引查询
                     data type: int/long, array/subObj
*@Author:  2016/5/18  xiaoni huang
*@bug:  ----------array--{"b.0.c":1}有索引和没索引查询结果不一致
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8043";
   var idx01 = CHANGEDPREFIX + "_index01";
   var idx02 = CHANGEDPREFIX + "_index02";

   var cl = readyCL( clName );
   createIndex( cl, idx01, idx02 );

   insertRecs( cl );

   var intRc = findRecs( cl, "int" );  // int/long
   var arrRc = findRecs( cl, "array" );  // array/subObj

   checkResult( intRc, arrRc, idx01, idx02 );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function createIndex ( cl, idx01, idx02 )
{

   cl.createIndex( idx01, { "b": 1 } );
   cl.createIndex( idx02, { "b.0.c": 1 } );
}

function insertRecs ( cl )
{

   cl.insert( [{ a: 0, b: 1 },
   { a: 1, b: NumberLong( 1 ) },
   { a: 2, b: [{ "c": 1 }] },
   { a: 3, b: { "0": { "c": 1 } } }] );
}

function findRecs ( cl, dataType )
{
   if( dataType === "int" )
   {
      var rc = cl.find( { b: { $et: 1 } } ).sort( { a: 1 } );
   }
   else if( dataType === "array" )
   {
      var rc = cl.find( { "b.0.c": { $et: 1 } } ).sort( { a: 1 } );
   }
   return rc;
}

function checkResult ( intRc, arrRc, idx01, idx02 )
{
   //-----------------------check result for dataType[int/long]---------------------
   //compare scanType

   var tmpExp = intRc.explain().current().toObj();  //incRc
   if( tmpExp["ScanType"] !== "ixscan" || tmpExp["IndexName"] !== idx01 )
   {
      throw new Error( "checkResult fail,[compare index]" +
         "[ScanType:ixscan,IndexName:" + idx01 + "]" +
         "[ScanType:" + tmpExp["ScanType"] + ",IndexName:" + tmpExp["IndexName"] + "]" );
   }

   //get results

   var findRtn = new Array();
   while( tmpRecs = intRc.next() )  //incRc
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 2;
   assert.equal( findRtn.length, expLen );
   //compare records
   if( findRtn[0]["b"] !== 1 || findRtn[1]["b"] !== 1 )
   {
      throw new Error( "checkResult fail,[compare records]" +
         "[b:" + 1 + ",b:" + 1 + "]" +
         "[b:" + findRtn[0]["b"] + ",b:" + findRtn[0]["b"] + "]" );
   }
   /*
   //-----------------------check result for dataType[array/subObj]---------------------
   //compare scanType
   
   var tmpExp = arrRc.explain().current().toObj();  //arrRc
   if( tmpExp["ScanType"] !== "ixscan" || tmpExp["IndexName"] !== idx02 )
   {
      throw new Error("checkResult fail,[compare index]" + 
                           "[ScanType:ixscan,IndexName:"+ idx02 +"]", 
                           "[ScanType:"+ tmpExp["ScanType"] +",IndexName:"+ tmpExp["IndexName"] +"]");
   }
   
   //get results
   
   var findRtn = new Array();
   while( tmpRecs = arrRc.next() )  //arrRc
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 2;
   if( findRtn.length !== expLen )
   {
      throw new Error("checkResultfail, [compare number]"+ 
                          "[recsNum:"+ expLen +"]",
                          "[recsNum:"+ findRtn.length +"]");
   }
   //compare records
   if( findRtn[0]["b"][0]["c"] !== 1 || findRtn[1]["b"][0]["c"] !== 1 )
   {
      throw new Error("checkResult fail,[compare records]" + 
                        "[b:"+ 1 +",b:"+ 1 +"]",
                        "[b:"+ findRtn[0]["b"][0]["c"] +",b:"+ findRtn[1]["b"][0]["c"] +"]");
   }
   */
}