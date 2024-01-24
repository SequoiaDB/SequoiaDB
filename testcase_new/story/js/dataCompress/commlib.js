/******************************************************************************
 * @Description   : 数据压缩公共方法
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2016.03.23
 * @LastEditTime  : 2023.02.08
 * @LastEditors   : liuli
 ******************************************************************************/
import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

function insertRecs1 ( cl, insertRecsNum )
{

   for( k = 0; k < insertRecsNum; k += 50000 )
   {
      var doc = [];
      for( i = 0 + k; i < 50000 + k; i++ )
      {
         doc.push( { atest: i, btest: i, ctest: "test" + i, dtest: "abcdefg890abcdefg890abcdefg890" } )
      };
      cl.insert( doc );
   }
}

function insertRecs2 ( cl, insertRecsNum )
{
   for( k = 0; k < insertRecsNum; k += 50000 )
   {
      var doc = [];
      for( i = 0 + k; i < 50000 + k; i++ )
      {
         doc.push( { INNER_NO: i, SA_ACCT_NO: i, EVT_ID: "lwy20120702" + i, IVC_NAME: "电子银行业务回单(付款)", OPEN_BRANCH_NAME: "中国民生银行福州闽江支行" } )
      };
      cl.insert( doc );
   }
}

function checkLzwAttributeByDataNode ( rgName, csName, clName, isCheckCompRatio )
{
   if( typeof ( isCheckCompRatio ) == "undefined" ) { isCheckCompRatio = false; }

   var rc = db.exec( "select NodeName from $SNAPSHOT_SYSTEM where GroupName='" + rgName + "'" );
   while( rc.next() )
   {
      var nodeName = rc.current().toObj()["NodeName"];
      var nodeDB = null;
      try
      {
         nodeDB = new Sdb( nodeName );
         var clInfo = nodeDB.snapshot( 4, { Name: csName + "." + clName } ).toArray();
         var details = JSON.parse( clInfo[0] ).Details[0];
         assert.equal( details.Attribute, "Compressed", "clInfo = " + JSON.stringify( clInfo ) );
         assert.equal( details.CompressionType, "lzw", "clInfo = " + JSON.stringify( clInfo ) );
         assert.equal( details.DictionaryCreated, true, "clInfo = " + JSON.stringify( clInfo ) );
         // 数据有压缩时检查压缩率 < 1
         if( isCheckCompRatio )
         {
            if( details.CurrentCompressionRatio >= 1 )
            {
               throw new Error( "Expected compression ratio < 1, actually = " + details.CurrentCompressionRatio
                  + ", clInfo = " + JSON.stringify( clInfo ) );
            }
         }
      }
      finally 
      {
         if( nodeDB != null ) nodeDB.close();
      }
   }
}

function waitDictionary ( db, csName, clName )
{
   var timeOut = 120;
   var doTime = 0;
   var nodeNames = commGetCLNodes( db, csName + "." + clName );
   for( var i in nodeNames )
   {
      nodeDB = new Sdb( nodeNames[i].HostName + ":" + nodeNames[i].svcname );
      var dictionaryCreated = false;
      while( ( doTime < timeOut ) && !dictionaryCreated )
      {
         var clInfo = nodeDB.snapshot( 4, { Name: csName + "." + clName } ).toArray();
         var details = JSON.parse( clInfo[0] ).Details[0];
         dictionaryCreated = details.DictionaryCreated;
         sleep( 1000 );
         doTime++;
      }
      nodeDB.close();
      if( doTime >= timeOut )
      {
         throw new Error( "check timeout, dictionaryCreated is " + dictionaryCreated + ", detailed information:" + JSON.stringify( clInfo ) );
      }
   }
}