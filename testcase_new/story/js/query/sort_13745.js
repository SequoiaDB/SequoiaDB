/******************************************************************************
@Description : seqDB-13745:查询指定string类型字段排序（包含不带索引、带索引）
@Modify list :
               2015-01-16 pusheng Ding  Init
               2020-08-12 Zixian Yan    Modify
               2020-10-12 Xiaoni Huang  Modify
******************************************************************************/
testConf.clName = COMMCLNAME + "_13745";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var recordsNum = 1000;
   var recordsArr = new Array();
   var sortStringArr = new Array();
   readyRdmRecs( recordsNum, recordsArr, sortStringArr );
   sortStringArr.sort();
   cl.insert( recordsArr );

   // 不带索引
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { "b": 1 } );
   var expRecsArr = getExpRecs( sortStringArr );
   commCompareResults( cursor, expRecsArr );

   // 带索引
   cl.createIndex( "idx", { "b": -1 } );
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).hint( { "": "idx" } ).sort( { "b": 1 } );
   var expRecsArr = getExpRecs( sortStringArr );
   commCompareResults( cursor, expRecsArr );
}

function getExpRecs ( expStringArr )
{
   var expRecsArr = [];
   for( var i = 0; i < expStringArr.length; i++ )
   {
      expRecsArr.push( { "b": expStringArr[i] } );
   }
   return expRecsArr;
}

function readyRdmRecs ( recordsNum, recordsArr, sortStringArr ) 
{
   var baseStr = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
   for( var i = 0; i < recordsNum; i++ )
   {
      // get random string
      var str = "";
      var strLen = getRandomInt( 0, 10 );
      for( var j = 0; j < strLen; j++ )
      {
         var startLoc = getRandomInt( 0, baseStr.length );
         var subStr = baseStr.substring( startLoc, startLoc + 1 );
         str = str + subStr;
      }
      recordsArr.push( { "b": str } );

      sortStringArr.push( str );
   }
}

function getRandomInt ( min, max )
{
   var rdmVal = min + Math.round( Math.random() * ( max - min ), max );
   return rdmVal;
}
