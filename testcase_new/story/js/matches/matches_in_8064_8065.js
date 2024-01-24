/************************************************************************
*@Description:   seqDB-8064:使用$in查询，走索引查询
                 seqDB-8065:使用$in查询，指定多个不同数据类型的值
                    cover all data type
*@Author:  2016/5/20  xiaoni huang
*@Mender:  2020/10/16 Zixian Yan
************************************************************************/
testConf.clName = COMMCLNAME + "cl_8064_8065";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var indexName = "index_8064_8065";
   cl.createIndex( indexName, { b: 1 } );

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
                  { regex: { "$regex": "^rg", "$options": "" } },
                  { binary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } },
                  { date: { "$date": "2038-01-18" } },
                  { timestamp: { "$timestamp": "2038-01-18-23.59.59.999999" } },
                  { tmp: 1 }];

   var insertData = [];
   var tmpValue = [];
   for( i = 0; i < rawData.length; i++ )
   {
      var tmp = rawData[i][dataType[i]];
      tmpValue.push( tmp );
      insertData.push( { a: i, b: tmp } );
   }
   cl.insert( insertData );

   var rc = cl.find( { b: { $in: tmpValue } } ).sort( { a: 1 } ).hint( { '': '' } );
   checkResult( rc, rawData, dataType, indexName );
}

function checkResult ( rc, rawData, dataType, indexName )
{
   //---------------compare scanType & check index------------------------
   var idx = rc.explain().current().toObj();
   if( idx["ScanType"] != "ixscan" || idx["IndexName"] != indexName )
   {
      throw new Error( "CheckResult[compare index]:\n" +
         "[ScanType:ixscan,IndexName:" + indexName + "]\n"+
         "[ScanType:" + idx["ScanType"] + ", IndexName:" + idx["IndexName"] + "]" );
   }

   //-------------------check records----------------------------
   var findRecsArray = [];
   while( tmpRecs = rc.next() )
   {
      findRecsArray.push( tmpRecs.toObj() );
   }

   //compare number
   var expLen = 11;
   if( findRecsArray.length != expLen )
   {
      throw new Error( "checkResult[compare number]:\n" +
                       "Expected recsNum: " + expLen +
                       "\nActual recsNum: " + findRecsArray.length );
   }

   //compare records
   for( i = 0; i < findRecsArray.length; i++ )
   {
      if( i < 5 )
      {
         var actB = findRecsArray[i]["b"];
         var expB = rawData[i][dataType[i]];
      }
      else
      {
         var expB = rawData[i][dataType[i]].toString();
         var actB = findRecsArray[i]["b"].toString();
      }

      if( actB !== expB )
      {
         throw new Error( "checkResult[compare records]:\n" +
                          'Expected: ["b": ' + expB + ']'+
                          '\nActual: ["b": ' + actB + ']' );
      }
   }
}
