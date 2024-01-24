/************************************************************************
*@Description:   seqDB-8029:使用$all查询，走索引查询 
                    cover all data type
*@Author:  2016/5/21  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8029";
   var indexName = CHANGEDPREFIX + "_index";
   var cl = readyCL( clName );
   createIndex( cl, indexName );

   //typeNum: 11
   var dataType = ["int", "double", "null", "string", "bool",
      "long", "oid", "regex", "binary", "date", "timestamp"];
   var rawData = [{ int: -2147483648 },
   { double: -1.7E+308 },
   { null: null },
   { string: "test" },
   { bool: true },
   { long: { "$numberLong": "-9223372036854775808" } },
   { oid: { "$oid": "123abcd00ef12358902300ef" } },
   { regex: { "$regex": "^rg", "$options": "i" } },
   { binary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } },
   { date: { "$date": "2038-01-18" } },
   { timestamp: { "$timestamp": "2038-01-18-23.59.59.999999" } }];
   insertRecs( cl, rawData, dataType );

   var rc = findRecs( cl, rawData, dataType );

   checkResult( rc, rawData, dataType, indexName );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function createIndex ( cl, indexName )
{

   cl.createIndex( indexName, { a: 1 } );
}

function insertRecs ( cl, rawData, dataType )
{

   var tmpValue = [];
   for( i = 0; i < rawData.length; i++ )
   {
      tmpValue.push( rawData[i][dataType[i]] );
   }
   cl.insert( { a: tmpValue } );
}

function findRecs ( cl, rawData, dataType )
{

   var tmpValue = [];
   for( i = 0; i < dataType.length; i++ )
   {
      tmpValue.push( rawData[i][dataType[i]] );
   }
   var rc = cl.find( { a: { $all: tmpValue } } ).sort( { a: 1 } );

   return rc;
}

function checkResult ( rc, rawData, dataType, indexName )
{
   //-------------------check index----------------------------

   //compare scanType
   var idx = rc.explain().current().toObj();
   if( idx["ScanType"] !== "ixscan" || idx["IndexName"] !== indexName )
   {
      throw new Error( "checkResult fail,[compare index]" +
         "[ScanType:ixscan,IndexName:" + indexName + "]" +
         "[ScanType:" + idx["ScanType"] + ",IndexName:" + idx["IndexName"] + "]" );
   }

   //-------------------check records----------------------------

   var findRecsArray = [];
   while( tmpRecs = rc.next() )
   {
      findRecsArray.push( tmpRecs.toObj() );
   }

   //compare number
   var expLen = 1;
   if( findRecsArray.length !== expLen )
   {
      throw new Error( "checkResult fail, [compare number]" +
         "[recsNum:" + expLen + "]" +
         "[recsNum:" + findRecsArray.length + "]" );
   }

   //compare records
   for( i = 0; i < rawData.length; i++ )
   {

      if( i < 5 )
      {
         var actA = findRecsArray[0]["a"][i];
         var expA = rawData[i][dataType[i]];
      }
      else
      {
         var actA = findRecsArray[0]["a"][i].toString();
         var expA = rawData[i][dataType[i]].toString();
      }

      if( actA !== expA )
      {
         throw new Error( "checkResult fail,[compare records]" +
            '["a": ' + expA + ']',
            '["a": ' + actA + ']' );
      }
   }
}
