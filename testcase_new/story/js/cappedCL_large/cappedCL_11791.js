/************************************
*@Description:固定集合，记录未填满一个块，pop记录后，插入大于/小于原来记录大小的记录，再查询
*@author:      zhaoyu
*@createdate:  2017.7.12
*@testlinkCase: seqDB-11791
**************************************/
main( test );
function test ()
{
   var csName = COMMCSNAME + "_11791";
   commDropCS( db, csName, true, "drop CS in the beginning" );

   var csOption = { Capped: true };
   commCreateCS( db, csName, false, "", csOption );

   var clName = COMMCLNAME + "_11791";
   var clOption = { Capped: true, Size: 1024, AutoIndexId: false };
   var dbcl = commCreateCL( db, csName, clName, clOption, true, true );

   //插入定长记录刚好占用一个块大小
   var recordLength = 986839;
   var recordSize = recordLength + recordHeader;
   if ( recordSize % 4 != 0 ) 
   {   
      recordSize = recordSize + ( 4 - recordSize % 4 );
   }   
    
   var recordNum = Math.floor( 33554396 / recordSize );
   var string = "a";
   var insertRecords = insertFixedLengthDatas( dbcl, recordNum, recordLength, string );

   //检查id
   var expectIDs = [];
   for( var i = 0; i < recordNum; i++ )
   {
      expectIDs.push( i * recordSize );
   }
   checkLogicalID( dbcl, null, null, { _id: 1 }, null, null, expectIDs );

   //检查第一条及最后一条记录
   var firstExpRec = [];
   firstExpRec.push( insertRecords[0] );
   var skipNum = recordNum - 1;
   checkRecords( dbcl, null, null, { _id: -1 }, 1, skipNum, firstExpRec );

   var lastExpRec = [];
   lastExpRec.push( insertRecords[skipNum] );
   checkRecords( dbcl, null, null, { _id: -1 }, 1, null, lastExpRec );

   //逆向pop块尾的记录并插入同样大小的记录
   var logicalID = recordSize * ( recordNum - 1 );//块大小-块头大小
   dbcl.pop( { LogicalID: logicalID, Direction: -1 } );
   var expRecs = insertFixedLengthDatas( dbcl, 1, recordLength, string );

   var expectID = [];
   expectID.push( logicalID );
   checkLogicalID( dbcl, null, null, { _id: -1 }, 1, null, expectID );
   checkRecords( dbcl, null, null, { _id: -1 }, 1, null, expRecs );

   //逆向pop块尾的记录并插入比原来短的记录
   dbcl.pop( { LogicalID: logicalID, Direction: -1 } );
   var shortLength = 3;
   var expRecs = insertFixedLengthDatas( dbcl, 1, shortLength, string );

   //获取短记录的记录大小
   var shortRecordSize = shortLength + recordHeader;
   if ( shortRecordSize % 4 != 0 ) 
   {   
       shortRecordSize = shortRecordSize + ( 4 - shortRecordSize % 4 );
   }   

   checkLogicalID( dbcl, null, null, { _id: -1 }, 1, null, expectID );
   checkRecords( dbcl, null, null, { _id: -1 }, 1, null, expRecs );

   //逆向pop块尾的记录并插入比原来长的记录
   dbcl.pop( { LogicalID: logicalID, Direction: -1 } );
   var longLength = 1000003;
   var expRecs = insertFixedLengthDatas( dbcl, 1, longLength, string );

   var longRecordSize = longLength + recordHeader;
   if ( longRecordSize % 4 != 0 )
   {           
       longRecordSize = longRecordSize + ( 4 - longRecordSize % 4 );
   } 

   checkLogicalID( dbcl, null, null, { _id: -1 }, 1, null, expectID );
   checkRecords( dbcl, null, null, { _id: -1 }, 1, null, expRecs );

   //逆向pop块中的记录并插入与原来长度相等的记录
   var popNum = 12;
   var logicalID = getLogicalID( dbcl, null, null, { _id: 1 }, 1, popNum );
   dbcl.pop( { LogicalID: logicalID[0], Direction: -1 } );
   var expRecs = insertFixedLengthDatas( dbcl, ( recordNum - popNum ), recordLength, string );

   //比较插入后的第一条记录
   var firstExpRec = [];
   firstExpRec.push( expRecs[0] );
   checkLogicalID( dbcl, null, null, { _id: 1 }, 1, popNum, logicalID );
   checkRecords( dbcl, null, null, { _id: 1 }, 1, popNum, firstExpRec );

   //比较插入后的最后一条记录
   var expectID = [];
   var lastExpID = ( recordNum - 1 ) * recordSize;
   expectID.push( lastExpID );
   var lastExpRec = [];
   lastExpRec.push( expRecs[popNum - 1] );
   checkLogicalID( dbcl, null, null, { _id: -1 }, 1, null, expectID );
   checkRecords( dbcl, null, null, { _id: -1 }, 1, null, lastExpRec );

   //逆向pop块中的记录并插入比原来短的记录
   var logicalID = getLogicalID( dbcl, null, null, { _id: 1 }, 1, popNum );
   dbcl.pop( { LogicalID: logicalID[0], Direction: -1 } );
   var expRecs = insertFixedLengthDatas( dbcl, ( recordNum - popNum ), shortLength, string );

   //比较插入后的第一条记录
   var firstExpRec = [];
   firstExpRec.push( expRecs[0] );
   checkLogicalID( dbcl, null, null, { _id: 1 }, 1, popNum, logicalID );
   checkRecords( dbcl, null, null, { _id: 1 }, 1, popNum, firstExpRec );

   //比较插入后的最后一条记录
   var expectID = [];
   var lastExpID = logicalID[0] + ( recordNum - popNum - 1 ) * shortRecordSize;
   expectID.push( lastExpID );
   var lastExpRec = [];
   lastExpRec.push( expRecs[popNum - 1] );
   checkLogicalID( dbcl, null, null, { _id: -1 }, 1, null, expectID );
   checkRecords( dbcl, null, null, { _id: -1 }, 1, null, lastExpRec );

   //逆向pop块中的记录并插入比原来长的记录
   var logicalID = getLogicalID( dbcl, null, null, { _id: 1 }, 1, popNum );
   dbcl.pop( { LogicalID: logicalID[0], Direction: -1 } );
   var expRecs = insertFixedLengthDatas( dbcl, ( recordNum - popNum ), longLength, string );

   //比较插入后的第一条记录
   var firstExpRec = [];
   firstExpRec.push( expRecs[0] );
   checkLogicalID( dbcl, null, null, { _id: 1 }, 1, popNum, logicalID );
   checkRecords( dbcl, null, null, { _id: 1 }, 1, popNum, firstExpRec );

   //比较插入后的最后一条记录
   var lastExpID = logicalID[0] + ( recordNum - popNum - 1 ) * longRecordSize;
   var expectID = [];
   expectID.push( lastExpID );
   var lastExpRec = [];
   lastExpRec.push( expRecs[popNum - 1] );
   checkLogicalID( dbcl, null, null, { _id: -1 }, 1, null, expectID );
   checkRecords( dbcl, null, null, { _id: -1 }, 1, null, lastExpRec );

   //逆向pop块头的记录并插入比原来短的记录
   dbcl.pop( { LogicalID: 0, Direction: -1 } );
   var expRecs = insertFixedLengthDatas( dbcl, recordNum, shortLength, string );

   //比较插入后的第一条记录
   var expectID = [];
   var firstExpID = 0;
   expectID.push( firstExpID );
   var firstExpRec = [];
   firstExpRec.push( expRecs[0] );
   checkLogicalID( dbcl, null, null, { _id: 1 }, 1, null, expectID );
   checkRecords( dbcl, null, null, { _id: 1 }, 1, null, firstExpRec );

   //比较插入后的最后一条记录
   var expectID = [];
   var lastExpID = firstExpID + ( recordNum - 1 ) * shortRecordSize;
   expectID.push( lastExpID );
   var lastExpRec = [];
   lastExpRec.push( expRecs[recordNum - 1] );
   checkLogicalID( dbcl, null, null, { _id: -1 }, 1, null, expectID );
   checkRecords( dbcl, null, null, { _id: -1 }, 1, null, lastExpRec );

   //逆向pop块头的记录并插入比原来长的记录
   dbcl.pop( { LogicalID: 0, Direction: -1 } );
   var expRecs = insertFixedLengthDatas( dbcl, recordNum, longLength, string );

   //比较插入后的第一条记录
   var expectID = [];
   var firstExpID = 0;
   expectID.push( firstExpID );
   var firstExpRec = [];
   firstExpRec.push( expRecs[0] );
   checkLogicalID( dbcl, null, null, { _id: 1 }, 1, null, expectID );
   checkRecords( dbcl, null, null, { _id: 1 }, 1, null, firstExpRec );

   //比较插入后的最后一条记录
   var expectID = [];
   var lastExpID = firstExpID + ( recordNum - 1 ) * longRecordSize;
   expectID.push( lastExpID );
   var lastExpRec = [];
   lastExpRec.push( expRecs[recordNum - 1] );
   checkLogicalID( dbcl, null, null, { _id: -1 }, 1, null, expectID );
   checkRecords( dbcl, null, null, { _id: -1 }, 1, null, lastExpRec );

   //逆向pop块头的记录并插入跟原来长度相等的记录
   dbcl.pop( { LogicalID: 0, Direction: -1 } );
   var expRecs = insertFixedLengthDatas( dbcl, recordNum, recordLength, string );

   //检查id
   var expectIDs = [];
   for( var i = 0; i < recordNum; i++ )
   {
      expectIDs.push( i * recordSize );
   }
   checkLogicalID( dbcl, null, null, { _id: 1 }, null, null, expectIDs );

   //比较插入后的第一条记录
   var firstExpRec = [];
   firstExpRec.push( expRecs[0] );
   var skipNum = recordNum - 1;
   checkRecords( dbcl, null, null, { _id: -1 }, 1, skipNum, firstExpRec );

   var lastExpRec = [];
   lastExpRec.push( expRecs[skipNum] );
   checkRecords( dbcl, null, null, { _id: -1 }, 1, null, lastExpRec );

   //正向pop块头的记录
   dbcl.pop( { LogicalID: 0, Direction: 1 } );
   var expRecs = insertFixedLengthDatas( dbcl, 1, recordLength, string );
   var blockTailRec = recordNum - 2;

   var expectID = [];
   var blockSec = 33554396;//第2个块的起始位置
   expectID.push( blockSec );
   checkLogicalID( dbcl, null, null, { _id: -1 }, 1, null, expectID );
   checkRecords( dbcl, null, null, { _id: -1 }, 1, null, expRecs );

   //正向pop块中的记录
   var logicalID = getLogicalID( dbcl, null, null, { _id: 1 }, 1, popNum );
   dbcl.pop( { LogicalID: logicalID[0], Direction: 1 } );
   var expRecs = insertFixedLengthDatas( dbcl, popNum + 1, recordLength, string );
   blockTailRec = blockTailRec - popNum - 1;

   //比较插入后的第一条记录
   var expectID = [];
   var firstExpID = blockSec + recordSize;
   expectID.push( firstExpID );
   var firstExpRec = [];
   firstExpRec.push( expRecs[0] );
   checkLogicalID( dbcl, null, null, { _id: 1 }, 1, ( recordNum - popNum - 1 ), expectID );
   checkRecords( dbcl, null, null, { _id: 1 }, 1, ( recordNum - popNum - 1 ), firstExpRec );

   //比较插入后的最后一条记录
   var expectID = [];
   var lastExpID = firstExpID + popNum * recordSize;
   expectID.push( lastExpID );
   var lastExpRec = [];
   lastExpRec.push( expRecs[popNum] );
   checkLogicalID( dbcl, null, null, { _id: -1 }, 1, null, expectID );
   checkRecords( dbcl, null, null, { _id: -1 }, 1, null, lastExpRec );

   //正向pop块尾的记录
   var logicalID = getLogicalID( dbcl, null, null, { _id: 1 }, 1, blockTailRec );
   dbcl.pop( { LogicalID: logicalID[0], Direction: 1 } );
   var expRecs = insertFixedLengthDatas( dbcl, 1, recordLength, string );

   var expectID = [];
   expectID.push( lastExpID + recordSize );
   checkLogicalID( dbcl, null, null, { _id: -1 }, 1, null, expectID );
   checkRecords( dbcl, null, null, { _id: -1 }, 1, null, expRecs );

   commDropCS( db, csName, true, "drop CS in the end" );
}
