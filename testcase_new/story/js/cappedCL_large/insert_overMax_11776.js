/************************************
*@Description:insert data over Max
*@author:      zhaoyu
*@createdate:  2017.7.15
*@testlinkCase: seqDB-11776
**************************************/
main( test );

function test ()
{
   var csName = COMMCSNAME + "_11776";
   commDropCS( db, csName, true, "drop CS in the beginning" );

   var csOption = { Capped: true };
   commCreateCS( db, csName, false, "", csOption );

   var clName = COMMCLNAME + "_11776";
   var insertNum = 10000;
   var clOption = { Capped: true, Size: 1024, Max: insertNum, AutoIndexId: false, OverWrite: false };
   var dbcl = commCreateCL( db, csName, clName, clOption, true, true );

   //获取主备节点
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary = db1.getCS( csName ).getCL( clName );
   var db2 = new Sdb( db );
   db2.setSessionAttr( { PreferedInstance: "s" } );
   var dbclSlave = db2.getCS( csName ).getCL( clName );

   //获取随机长度的字符串
   var minLength = 1;
   var maxLength = 16 * 1024;
   var expIDs = [];
   var expID = 0;
   var nextExpID = 0;
   //var preExpID = 0;
   var blockID = 1;
   var docs = [];
   var expectNum = 0;

   var minRecordNum = 0;
   var maxRecordNum = insertNum - 1;
   var repeatNum = 10;
   for( var j = 0; j < repeatNum; j++ )
   {
      for( var i = 0; i < insertNum; i++ )
      {
         var stringLength = Math.ceil( minLength + Math.random() * ( maxLength - minLength ) );

         //计算不定长度记录的预期_id值
         var recordLength = stringLength + recordHeader;
         if( recordLength % 4 !== 0 )
         {
            recordLength += ( 4 - recordLength % 4 );
         }

         //生成不定长度的字符串记录
         var doc = new StringBuffer();
         doc.append( stringLength, "a" );
         var strings = doc.toString();

         docs.push( { a: strings } );
         if( docs.length % 1000 == 0 )
         {
            dbcl.insert( docs );
            docs = [];
         }

         //处理跨块的情况
         nextExpID = expID + recordLength;
         if( nextExpID / 33554396 != blockID && blockID == Math.floor( nextExpID / 33554396 ) )
         {
            expID = 33554396 * blockID++;
            nextExpID = expID + recordLength;
         }

         expIDs.push( expID );
         expID = nextExpID;
      }

      if( docs.length != 0 )
      {
         dbcl.insert( docs );
         docs = [];
      }

      assert.tryThrow( SDB_OSS_UP_TO_LIMIT, function()
      {
         dbcl.insert( { b: "a" } );
      } );

      //检查主备节点一致
      checkConsistency( db, csName, clName );

      //比较count结果
      expectNum = expectNum + insertNum;
      checkCount( dbclPrimary, null, expectNum );
      checkCount( dbclSlave, null, expectNum );

      //校验多个块内的_id值
      checkLogicalID( dbclPrimary, null, null, { _id: 1 }, -1, 0, expIDs );
      checkLogicalID( dbclSlave, null, null, { _id: 1 }, -1, 0, expIDs );

      //随机获取某条记录的logicalID
      var skipNum = Math.ceil( minRecordNum + Math.random() * ( maxRecordNum - minRecordNum ) );
      var logicalID = getLogicalID( dbcl, null, null, { _id: 1 }, 1, skipNum );

      //随机设置pop方向
      if( ( parseInt( Math.random() * 10 ) ) % 2 === 0 )
      {
         var direction = 1;
      } else
      {
         var direction = -1;
      }

      //执行pop
      dbcl.pop( { LogicalID: logicalID[0], Direction: direction } );

      //比较count结果
      if( direction == -1 )
      {
         expectNum = skipNum;
         expID = logicalID[0];
         blockID = Math.floor( expID / 33554396 ) + 1;
         expIDs.splice( skipNum );
      } else
      {
         expectNum = expectNum - skipNum - 1;
         expIDs = expIDs.splice( skipNum + 1 );
      }

      insertNum = maxRecordNum + 1 - expectNum;

      //检查主备节点一致
      checkConsistency( db, csName, clName );

      //比较count结果
      checkCount( dbclPrimary, null, expectNum );
      checkCount( dbclSlave, null, expectNum );

      //校验多个块内的_id值
      checkLogicalID( dbclPrimary, null, null, { _id: 1 }, -1, 0, expIDs );
      checkLogicalID( dbclSlave, null, null, { _id: 1 }, -1, 0, expIDs );
   }

   commDropCS( db, csName, true, "drop CS in the end" );
   db1.close();
   db2.close();
}
