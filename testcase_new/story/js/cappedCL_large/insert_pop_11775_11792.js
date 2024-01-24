/************************************
*@Description:seqDB-11775:扩块/扩文件时插入记录/seqDB-11792:pop多个块的数据                 
*@author:      zhaoyu
*@createdate:  2017.7.17
*@testlinkCase: seqDB-11775/seqDB-11792
**************************************/
main( test );

function test ()
{
   var csName = COMMCSNAME + "_11775";
   commDropCS( db, csName, true, "drop CS in the beginning" );

   var csOption = { Capped: true };
   commCreateCS( db, csName, false, "", csOption );

   var clName = COMMCLNAME + "_11775";
   var clOption = { Capped: true, Size: 1024, AutoIndexId: false };
   var dbcl = commCreateCL( db, csName, clName, clOption, true, true );

   //获取主备节点
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary = db1.getCS( csName ).getCL( clName );
   var db2 = new Sdb( db );
   db2.setSessionAttr( { PreferedInstance: "s" } );
   var dbclSlave = db2.getCS( csName ).getCL( clName );

   var repeatNum = 30;
   var stringLength = 969;
   //获取每条记录的大小
   var recordSize = stringLength + recordHeader;
   if ( recordSize % 4 != 0 ) 
   {   
      recordSize = recordSize + ( 4 - recordSize % 4 );
   }   
   
   //循环插入                          
   for( var i = 0; i < repeatNum; i++ )
   {
      //插入1个块的记录
      insertNum = Math.floor( 33554396 / recordSize );
      var expectRecords = insertFixedLengthDatas( dbcl, insertNum, stringLength, "a" );
   }
   println( "--insert data success!" );
  
   //获取每个块塞满记录后所剩余的空间
   var remainSize = 33554396 - recordSize * insertNum;

   //计算多个块内的预期的_id值
   var expIDs = [];
   var expID = 0;
   for( var i = 0; i < repeatNum; i++ )
   {
      for( var j = 0; j < insertNum; j++ )
      {
         expIDs.push( expID );
         expID = expID + recordSize;
      }

      //跨块时，加上块尾的空隙
      var expID = expID + remainSize;
   }

   //检查主备节点一致
   checkConsistency( db, csName, clName );

   //校验多个块内的_id值
   checkLogicalID( dbclPrimary, null, null, { _id: 1 }, -1, 0, expIDs );
   checkLogicalID( dbclSlave, null, null, { _id: 1 }, -1, 0, expIDs );

   //插入记录使扩展文件后，检查记录数
   var expectCount = repeatNum * insertNum;
   checkCount( dbcl, null, expectCount );

   //逆向pop单个块
   var skipNum = ( repeatNum - 1 ) * insertNum;
   var logicalID = getLogicalID( dbcl, null, null, { _id: 1 }, 1, skipNum );
   dbcl.pop( { LogicalID: logicalID[0], Direction: -1 } );


   //计算多个块内的预期的_id值
   var expIDs = [];
   var expID = 0;
   for( var i = 0; i < repeatNum - 1; i++ )
   {
      for( var j = 0; j < insertNum; j++ )
      {
         expIDs.push( expID );
         expID = expID + recordSize;
      }

      //跨块时，加上块尾的空隙
      var expID = expID + remainSize;
   }

   //检查主备节点一致
   checkConsistency( db, csName, clName );

   //校验多个块内的_id值
   checkLogicalID( dbclPrimary, null, null, { _id: 1 }, -1, 0, expIDs );
   checkLogicalID( dbclSlave, null, null, { _id: 1 }, -1, 0, expIDs );

   //检查记录数
   expectCount = skipNum;
   checkCount( dbcl, null, expectCount );

   //逆向pop 2个块
   var skipNum = insertNum * ( repeatNum - 3 );
   var logicalID = getLogicalID( dbcl, null, null, { _id: 1 }, 1, skipNum );
   dbcl.pop( { LogicalID: logicalID[0], Direction: -1 } );


   //计算多个块内的预期的_id值
   var expIDs = [];
   var expID = 0;
   for( var i = 0; i < repeatNum - 3; i++ )
   {
      for( var j = 0; j < insertNum; j++ )
      {
         expIDs.push( expID );
         expID = expID + recordSize;
      }

      //跨块时，加上块尾的空隙
      var expID = expID + remainSize;
   }

   //检查主备节点一致
   checkConsistency( db, csName, clName );

   //校验多个块内的_id值
   checkLogicalID( dbclPrimary, null, null, { _id: 1 }, -1, 0, expIDs );
   checkLogicalID( dbclSlave, null, null, { _id: 1 }, -1, 0, expIDs );

   //检查记录数
   expectCount = skipNum;
   checkCount( dbcl, null, expectCount );

   //正向pop单个块
   var skipNum = insertNum - 1;
   var logicalID = getLogicalID( dbcl, null, null, { _id: 1 }, 1, skipNum );
   dbcl.pop( { LogicalID: logicalID[0], Direction: 1 } );


   //计算多个块内的预期的_id值
   var expIDs = [];
   var expID = 33554396;
   for( var i = 0; i < repeatNum - 4; i++ )
   {
      for( var j = 0; j < insertNum; j++ )
      {
         expIDs.push( expID );
         expID = expID + recordSize;
      }

      //跨块时，加上块尾的空隙
      var expID = expID + remainSize;
   }

   //检查主备节点一致
   checkConsistency( db, csName, clName );

   //校验多个块内的_id值
   checkLogicalID( dbclPrimary, null, null, { _id: 1 }, -1, 0, expIDs );
   checkLogicalID( dbclSlave, null, null, { _id: 1 }, -1, 0, expIDs );

   //检查记录数
   expectCount = expectCount - insertNum;
   checkCount( dbcl, null, expectCount );

   //正向pop 3个块
   var skipNum = insertNum * 3 - 1;
   var logicalID = getLogicalID( dbcl, null, null, { _id: 1 }, 1, skipNum );
   dbcl.pop( { LogicalID: logicalID[0], Direction: 1 } );

   //计算多个块内的预期的_id值
   var expIDs = [];
   var expID = 134217584;
   for( var i = 0; i < repeatNum - 7; i++ )
   {
      for( var j = 0; j < insertNum; j++ )
      {
         expIDs.push( expID );
         expID = expID + recordSize;
      }

      //跨块时，加上块尾的空隙
      var expID = expID + remainSize;
   }

   //检查主备节点一致
   checkConsistency( db, csName, clName );

   //校验多个块内的_id值
   checkLogicalID( dbclPrimary, null, null, { _id: 1 }, -1, 0, expIDs );
   checkLogicalID( dbclSlave, null, null, { _id: 1 }, -1, 0, expIDs );

   //检查记录数
   expectCount = expectCount - skipNum - 1;
   checkCount( dbcl, null, expectCount );

   commDropCS( db, csName, true, "drop CS in the end" );
   db1.close();
   db2.close();
}
