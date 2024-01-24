/************************************************************************
@Description : [seqDB-_13732] 关闭游标后，查看是否关闭了context
@Modify list : wang wenjing  Init
               Ting YU       modify
               2020-08-13  Zixian Yan Modify
************************************************************************/
testConf.clName = COMMCLNAME + "_13732";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   //insert records > 128M
   var rd = new commDataGenerator();
   var recs = rd.getRecords( 1000, "string", ['a', 'b', 'c'] );
   cl.insert( recs );

   var preSize = getSessionContextsSize();

   //cursor not close
   var cursor = cl.find();
   cursor.next();

   var hasCursorSize = getSessionContextsSize();

   //close cursor
   cursor.close();

   var closeCursorSize = getSessionContextsSize();
   assert.equal( closeCursorSize, preSize );
}

function getSessionContextsSize ()
{
   var contextSize = [];

   var rc = db.snapshot( SDB_SNAP_SESSIONS_CURRENT );
   while( rc.next() )
   {
      var obj = rc.current().toObj();
      contextSize.push( obj["Contexts"].length );
   }
   return contextSize;
}
