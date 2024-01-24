/******************************************************************************
*@Description: testcases for normal table
*@Modify list:
*              2015-5-8  xiaojun Hu   Init
******************************************************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "truncate_155";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop collection begin" );
   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, false );
   var tableName = COMMCSNAME + "." + clName;

   testTruncateCompareRemveDelete( db );
   testTruncateLoopOperation( db, cl, tableName )
   testTruncateMultiTimes( db, cl, tableName )
   commDropCL( db, COMMCSNAME, clName, true, true, "drop collection end" );
}

/*******************************************************************************
*@Description: 测试普通表中truncate和remove操作的异同
*@Input: collection.truncate()
*@Expectation: truncate清除数据及LOB数据， 并且清除数据页和LOB数据页
*              remove只清除数据, 不清除数据页
*              delete只清除LOB数据, 不清除LOB数据页
********************************************************************************/
function testTruncateCompareRemveDelete ( db )
{

   if( undefined == db ) { throw new Error("no sdb connect handle"); }
   var lobSize = 2049;
   var lobNumber = 5;
   var clName1 = CHANGEDPREFIX + "_truncate_tbl1_155";
   var clName2 = CHANGEDPREFIX + "_truncate_tbl2_155";
   var recordNumber = 5;
   var tableName1 = COMMCSNAME + "." + clName1;
   var tableName2 = COMMCSNAME + "." + clName2;
   var verfify1 = { "TotalDataPages": 1, "TotalLobPages": 5 };

   commDropCL( db, COMMCSNAME, clName1, true, true, "drop collection1 begin" );
   commDropCL( db, COMMCSNAME, clName2, true, true, "drop collection2 begin" );
   var cl1 = commCreateCL( db, COMMCSNAME, clName1, {}, true, false );
   var cl2 = commCreateCL( db, COMMCSNAME, clName2, {}, true, false );
   truncateVerify( db, tableName1 );
   truncateVerify( db, tableName2 );

   // pub Lob and insert record, record have cursor for lob
   var lobIDSet1 = truncatePutLob( cl1, lobSize, lobNumber );
   var lobIDSet2 = truncatePutLob( cl2, lobSize, lobNumber );
   truncateInsertRecord( cl1, recordNumber );
   truncateInsertRecord( cl2, recordNumber );
   db.sync();
   truncateVerify( db, tableName1, verfify1 );
   // remove cl1's records
   cl1.remove();
   assert.equal( cl1.count(), 0 );

   // delete cl1's lobs
   for( var i = 0; i < lobIDSet1.length; ++i )
   {
      cl1.deleteLob( lobIDSet1[i] );
   }

   assert.equal( cl1.listLobs().toArray().length, 0 );
   var verfify1 = { "TotalDataPages": 1 }
   truncateVerify( db, tableName1, verfify1 );

   // truncate
   cl1.truncate();
   cl2.truncate();
   assert.equal( cl2.count(), 0 );
   assert.equal( cl2.listLobs().toArray().length, 0 );

   truncateVerify( db, tableName1 );
   truncateVerify( db, tableName2 );

   commDropCL( db, COMMCSNAME, clName1, true, true, "drop collection1 end" );
   commDropCL( db, COMMCSNAME, clName2, true, true, "drop collection2 end" );
}

/*******************************************************************************
*@Description: 测试对表的多次写入与多次truncate的操作
*@Input: collection.truncate()
*@Expectation: 多次的truncate操作成功
********************************************************************************/
function testTruncateLoopOperation ( db, cl, tableName )
{
   var lobSize = 4097;
   var lobNumber = 3;
   var loopNum = 10;

   for( var i = 0; i < loopNum; ++i )
   {
      var cursor = truncatePutLob( cl, lobSize, lobNumber );
      for( var j = 0; j < cursor.length; ++j )
      {
         truncateInsertRecord( cl, 1, lobSize, { "LobOID": cursor[j] } );
      }
      // truncate
      cl.truncate();
      assert.equal( cl.count(), 0 );
      truncateVerify( db, tableName );
   }
}

/*******************************************************************************
*@Description: 测试对表的一次写入与多次truncate的操作
*@Input: collection.truncate()
*@Expectation: 多次的truncate操作成功
********************************************************************************/
function testTruncateMultiTimes ( db, cl, tableName )
{
   var lobSize = 4097;
   var lobNumber = 3;
   var loopNum = 10;
   truncateVerify( db, tableName );

   // insert 13 types
   var cursor = truncatePutLob( cl, lobSize, lobNumber );
   for( var i = 0; i < cursor.length; ++i )
   {
      truncateInsertRecord( cl, 1, lobSize, { "LobOID": cursor[i] } );
   }

   for( var i = 0; i < loopNum; ++i )
   {
      // truncate
      cl.truncate();
      assert.equal( cl.count(), 0 );
      truncateVerify( db, tableName );
   }
}
