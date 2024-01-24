/******************************************************************************
 * @Description   : seqDB-9322:插入记录包含大量重复子串，且重复子串长度>255个字节
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2016.03.23
 * @LastEditTime  : 2023.02.08
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = CHANGEDPREFIX + "_cl_9322";
testConf.clOpt = { Compressed: true, CompressionType: "lzw", ReplSize: 0 };

main( test );
function test ( testPara )
{
   var rgName = testPara.srcGroupName;
   var csName = COMMCSNAME;
   var clName = testConf.clName;
   var cl = testPara.testCL;
   var insertRecsNum = 300000;
   var checkRecsNum = 3;

   // 构造随机数据
   var str1 = getRandomStr1();
   var str2 = getRandomStr2();
   var str3 = getRandomStr3();

   // 插入数据
   insertRecs( cl, insertRecsNum, str1, str2, str3 );

   // 等待字典构建
   waitDictionary( db, csName, clName );

   // 再次插入少量数据使数据压缩
   var insertRecsNum2 = 50000;
   insertRecs( cl, insertRecsNum2, str1, str2, str3 );

   // 检查结果，检查组内每个节点数据正确性
   checkLzwAttributeByDataNode( rgName, csName, clName, true );
   checkRecsByDataNode( rgName, csName, clName, insertRecsNum + insertRecsNum2, checkRecsNum, insertRecsNum );
}

function getRandomStr1 () 
{
   //str e.g: "000000000000.1111111111....."
   var data = ["0", "1", "2", "3", "4", "0", "5", "6", "7", "8", "9", "0"];
   var str1 = "";
   var tmpC1 = Math.floor( Math.random() * data.length );
   var str1 = "";
   for( var i = 0; i < 300; i++ )
   {
      str1 += data[tmpC1];
   }

   var tmpC2 = Math.floor( Math.random() * data.length );
   var str2 = "";
   for( var i = 0; i < 50; i++ )
   {
      str2 += data[tmpC2];
   }
   var str = str1 + "." + str2;

   return str;
}

function getRandomStr2 () 
{
   //str e.g: "12345555adbccccc......."
   var data = ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c"];
   var str = "";
   for( var i = 0; i < 300; i++ )
   {
      var c = Math.floor( Math.random() * data.length );
      str += data[c];
   }
   return str;
}

function getRandomStr3 () 
{
   //str e.g: "123455$%$%455adbccccc....."
   var strLen = getRandomInt( 254, 300 );
   var str = "";
   for( var i = 0; i < strLen; i++ )
   {
      var ascii = getRandomInt( 48, 127 ); // '0' -- '~'
      var c = String.fromCharCode( ascii );
      str += c;
   }
   return str;
}

function getRandomInt ( min, max )
{
   var range = max - min;
   var value = min + parseInt( Math.random() * range );
   return value;
}

function insertRecs ( cl, insertRecsNum, str1, str2, str3 )
{
   for( k = 0; k < insertRecsNum; k += 50000 )
   {
      var doc = [];
      for( i = 0 + k; i < 50000 + k; i++ )
      {
         doc.push( { "atest": str1 + i, "btest": str2 + i, "ctest": str3 + i, "num": i } )
      };
      cl.insert( doc )
   };
}

function checkRecsByDataNode ( rgName, csName, clName, insertRecsNum, checkRecsNum, insertRange )
{
   var rc = db.exec( "select NodeName from $SNAPSHOT_SYSTEM where GroupName='" + rgName + "'" );
   while( rc.next() )
   {
      var nodeName = rc.current().toObj()["NodeName"];
      var nodeDB = null;
      try
      {
         nodeDB = new Sdb( nodeName );
         var nodeCL = nodeDB.getCS( csName ).getCL( clName );
         // 检查数据总数
         var recsCnt = nodeCL.count();
         assert.equal( recsCnt, insertRecsNum );
         // 随机检查n条记录正确性
         for( j = 0; j < checkRecsNum; j++ )
         {
            var i = parseInt( Math.random() * insertRange );
            var cond = { "num": i };
            var recsCnt = nodeCL.find( cond ).count();
            if( recsCnt != 1 && recsCnt != 2 )
            {
               throw new Error( "expected result is 1 or 2, actual is " + recsCnt + " ,cond is :" + JSON.stringify( cond ) );
            }
         }
      }
      finally 
      {
         if( nodeDB != null ) nodeDB.close();
      }
   }
}