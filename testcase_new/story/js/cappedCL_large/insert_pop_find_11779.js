/************************************
*@Description:repeat insert/pop/find
*@author:      zhaoyu
*@createdate:  2017.7.14
*@testlinkCase: seqDB-11779
**************************************/
main( test );

function test ()
{
   var csName = COMMCSNAME + "_11779";
   commDropCS( db, csName, true, "drop CS in the beginning" );

   var csOption = { Capped: true };
   commCreateCS( db, csName, false, "", csOption );

   var clName = COMMCLNAME + "_11779";
   var clOption = { Capped: true, Size: 102400, AutoIndexId: false };
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
   var maxInsertNum = 100000;
   var expectNum = 0;

   //循环pop、查询、插入，logicaID随机
   var repeatNum = 2;
   for( var j = 0; j < repeatNum; j++ )
   {

      var insertNum = ( parseInt( 1 + Math.random() * ( maxInsertNum - 1 ) ) );

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
         expIDs.push( expID );
         nextExpID = expID + recordLength;
         if( nextExpID / 33554396 != blockID && blockID == Math.floor( nextExpID / 33554396 ) )
         {
            expIDs.pop();
            expID = 33554396 * blockID++;
            expIDs.push( expID );
            nextExpID = expID + recordLength;
         }

         //expIDs.push(expID);
         expID = nextExpID;
      }

      if( docs.length != 0 )
      {
         dbcl.insert( docs );
         docs = [];
      }

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
      var minRecordNum = 1;
      var maxRecordNum = expectNum;
      var skipNum = Math.floor( minRecordNum + Math.random() * ( maxRecordNum - minRecordNum ) );
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
