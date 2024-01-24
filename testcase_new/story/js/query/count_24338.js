/******************************************************************************
@Description : seqDB-24338:验证count只有一次消息交互

@Author      : xiaozhenfan
@CreateTime  : 2021.9.2
@LastEditTime: 2021.9.11
@LastEditors : xiaozhenfan
******************************************************************************/
testConf.clName = COMMCLNAME +"_24338";
main( test );

function test ( para )
{
   var cl = para.testCL;
   cl.insert({a: 1});
   var cursor = db.snapshot(SDB_SNAP_SESSIONS_CURRENT, {}, {ProcessEventCount: ""}); 
   var prevCount = cursor.next().toObj().ProcessEventCount;
   cl.count().valueOf() ;
   cursor = db.snapshot(SDB_SNAP_SESSIONS_CURRENT, {}, {ProcessEventCount: ""})
   var afterCount = cursor.next().toObj().ProcessEventCount;
   //count()执行后ProcessEventCount的增值应该为2
   var diff = 2;
   if (afterCount - prevCount != diff)
   {
       throw new Error("expect :" + diff + " real :" + (afterCount - prevCount)) ;
   }
   var prevList = afterCount ;
   db.list(SDB_SNAP_SESSIONS_CURRENT).next();
   cursor = db.snapshot(SDB_SNAP_SESSIONS_CURRENT, {}, {ProcessEventCount: ""}) ;
   var afterList = cursor.next().toObj().ProcessEventCount ;
   if (afterList - prevList != diff)
   {
       throw new Error("expect :" + diff + " real :" + (afterList - prevList)) ;
   }
}
