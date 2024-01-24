LOCK_IS = "IS";
LOCK_IX = "IX";
LOCK_S = "S";
LOCK_SIX = "SIX";
LOCK_U = "U";
LOCK_X = "X";
LOCK_Z = "Z";

/*******************************************************************************
@Description : 比较查询返回的结果（游标）与预期结果( 数组 )是否一致
@Modify list : 2018-10-15 zhaoyu init
*******************************************************************************/
function checkRec ( rc, expRecs )
{
   //get actual records to array
   var actRecs = [];
   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() );
   }

   //check count
   assert.equal( actRecs.length, expRecs.length );

   //check every records every fields
   for( var i in expRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];
      for( var f in expRec )
      {
         assert.equal( actRec[f], expRec[f] );
      }
   }

   //check every records every fields,actRecs as compare source
   for( var i in actRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];
      for( var f in actRec )
      {
         if( f == "_id" )
         {
            continue;
         }
         assert.equal( actRec[f], expRec[f] );
      }
   }
}


/************************************
*@Description: insert data
*@author:      wuyan
*@createDate:  2018.1.22
**************************************/
function insertData ( dbcl, number )
{
   if( undefined == number ) { number = 1000; }
   var docs = [];
   for( var i = 0; i < number; ++i )
   {
      var no = i;
      var a = i;
      var user = "test" + i;
      var phone = 13700000000 + i;
      var time = new Date().getTime();
      var doc = { no: no, a: a, customerName: user, phone: phone, openDate: time };
      //data example: {"no":5, customerName:"test5", "phone":13700000005, "openDate":1402990912105

      docs.push( doc );
   }
   dbcl.insert( docs );
   return docs;
}

/************************************
*@Description: 校验记录数
*@author:      luweikang
*@createDate:  2018.10.13
**************************************/
function checkCount ( dbcl, expRecordNums, options )
{
   if( options == undefined )
   {
      options = null;
   }
   var count = dbcl.count( options );
   assert.equal( count, expRecordNums );
}

/******************************************************************************
 * @description: 校验CL事务锁类型
 * @param {Sequoiadb} sdb
 * @param {string} lockMode   //预期的锁类型
 ******************************************************************************/
function checkCLLockType ( sdb, lockMode )
{
   var cursor = sdb.snapshot( SDB_SNAP_TRANSACTIONS_CURRENT );
   var mode = "";
   cursor.next();
   var lockList = cursor.current().toObj().GotLocks;
   for( var i = 0; i < lockList.length; i++ )
   {
      var lockObj = lockList[i];
      if( lockObj.CSID >= 0 && lockObj.CLID >= 0 && lockObj.CLID != 65535 && lockObj.ExtentID == -1 && lockObj.Offset == -1 )
      {
         mode = lockObj.Mode;
      }
   }
   cursor.close();
   assert.equal( mode, lockMode );
}

/******************************************************************************
 * @description: 校验事务锁是否升级
 * @param {Sequoiadb} sdb
 * @param {boolean} isLockEscalated   //预期的事务锁是否升级
 ******************************************************************************/
function checkIsLockEscalated ( sdb, isLockEscalated )
{
   var cursor = sdb.snapshot( SDB_SNAP_TRANSACTIONS_CURRENT );
   cursor.next();
   var actLockEscalated = cursor.current().toObj().IsLockEscalated;
   cursor.close();
   assert.equal( actLockEscalated, isLockEscalated );
}

/******************************************************************************
 * @description: 获取记录锁个数，仅限在事务中只对一个表进行操作的情况下
 * @param {Sequoiadb} sdb
 * @param {string} lockType  // 需要获取的记录锁类型
 * @return {int}
 ******************************************************************************/
function getCLLockCount ( sdb, lockType )
{
   var lockCount = 0;
   var cursor = sdb.snapshot( SDB_SNAP_TRANSACTIONS_CURRENT );
   cursor.next();
   var lockList = cursor.current().toObj().GotLocks;
   for( var i = 0; i < lockList.length; i++ )
   {
      var lockObj = lockList[i];
      if( lockObj.CSID >= 0 && lockObj.CLID >= 0 && lockObj.ExtentID >= 0 && lockObj.Offset >= 0 )
      {
         if( commCompareObject( lockObj.Mode, lockType ) )
         {
            lockCount++;
         }
      }
   }
   cursor.close();
   return lockCount;
}
