/************************************************************************
*@Description:    seqDB-8076:使用$ne查询，目标字段为非数值型，走索引查询
                     data type: null/string/bool/oid/regex/date/timestamp 
                     index:{b:1}
*@Author:  2016/5/18  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8076";
   var indexName = CHANGEDPREFIX + "_index";

   var cl = readyCL( clName );
   createIndex( cl, indexName );

   var dataType = ["null", "string", "bool", "oid", "regex", "date", "timestamp"];
   var rawData = [{ null: null },
   { string: "hello world" },
   { bool: true },
   { oid: { "$oid": "123abcd00ef12358902300ef" } },
   { regex: { "$regex": "^rg", "$options": "i" } },
   { date: { "$date": "2038-01-18" } },
   { timestamp: { "$timestamp": "2038-01-18-23.59.59.999999" } }];
   insertRecs( cl, rawData, dataType );

   var findRecsArray = findRecs( cl, rawData, dataType );

   checkResult( cl, findRecsArray, dataType, indexName );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function createIndex ( cl, indexName )
{

   cl.createIndex( indexName, { b: 1 } );
}

function insertRecs ( cl, rawData, dataType )
{

   for( i = 0; i < rawData.length; i++ )
   {
      cl.insert( { a: i, b: rawData[i][dataType[i]] } );
   }
}

function findRecs ( cl, rawData, dataType )
{

   var j = 0;
   var findRecsArray = [];
   for( i = 0; i < rawData.length; i++ )
   {

      var rc = cl.find( { b: { $ne: rawData[i][dataType[i]] } } ).sort( { a: 1 } );
      var tmpArray = [];
      while( tmpRecs = rc.next() )
      {
         tmpArray.push( tmpRecs.toObj() );
      }
      findRecsArray.push( tmpArray );;
   }
   return findRecsArray;
}

function checkResult ( cl, findRecsArray, dataType, indexName )
{
   //-------------------check index----------------------------

   //compare scanType
   var rc = cl.find( { b: { $ne: null } } ).sort( { a: 1 } ).hint( { '': '' } ).explain().current().toObj();
   if( rc["ScanType"] !== "ixscan" || rc["IndexName"] !== indexName )
   {
      throw new Error( "checkResult fail,[compare index]" +
         "[ScanType:ixscan,IndexName:" + indexName + "]" +
         "[ScanType:" + rc["ScanType"] + ",IndexName:" + rc["IndexName"] + "]" );
   }


   //compare scanType
   var rc = cl.find( { b: { $ne: null } } ).sort( { a: 1 } ).explain().current().toObj();
   if( rc["ScanType"] !== "tbscan" )
   {
      throw new Error( "checkResult fail,[compare tbscan]" +
         "[ScanType:tbscan]" +
         "[ScanType:" + rc["ScanType"] + ",IndexName:" + rc["IndexName"] + "]" );
   }

   //-------------------check records----------------------------

   for( i = 0; i < findRecsArray.length; i++ )
   {

      var expLen = 6;  //totalRecsCount:7, 6 after exec find["ne"]
      assert.equal( findRecsArray[i].length, expLen );

      for( j = 0; j < findRecsArray[i].length; j++ )
      {
         assert.notEqual( findRecsArray[i][j]["a"], i );
      }
   }
}
