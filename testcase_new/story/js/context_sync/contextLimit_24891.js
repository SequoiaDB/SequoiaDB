/******************************************************************************
 * @Description   : 不同cl操作在同一个节点中开启上下文超过限制   
 * @Author        : wu yan
 * @CreateTime    : 2021.12.30
 * @LastEditTime  : 2022.01.07
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_contextlimit_24891a";
testConf.useSrcGroup = true;

main( test );
function test ( testPara )
{
   var groupName = testPara.srcGroupName;
   var dbcl = testPara.testCL;
   var recordNum = 5000;
   var clName2 = COMMCLNAME + "_contextlimit_24891b";
   commDropCL( db, COMMCSNAME, clName2, true, true, "clear collection in the beginning" );
   var dbcl2 = commCreateCL( db, COMMCSNAME, clName2, { Group: groupName }, true, false, "create collection" );
   insertData( dbcl, recordNum );
   insertData( dbcl2, recordNum );

   var maxContextNum = 1001;
   var maxSessionContextNum = 1000;
   var dbArray = [];
   try
   {
      db.updateConf( { maxcontextnum: maxContextNum } );
      db.updateConf( { maxsessioncontextnum: maxSessionContextNum } );
      var findCurNum = 500;
      var db1 = findCursorOpr( COMMCSNAME, testConf.clName, findCurNum );
      dbArray.push( db1 );
      var db2 = findCursorOpr( COMMCSNAME, clName2, maxContextNum - findCurNum );
      dbArray.push( db2 );
      assert.tryThrow( SDB_DPS_CONTEXT_NUM_UP_TO_LIMIT, function()
      {
         dbcl.findOne().toString();
         //如果查询未报错则打印节点的上下文信息
         listContexts( db );
      } );
   }
   finally
   {
      db.deleteConf( { maxcontextnum: 1 } );
      db.deleteConf( { maxsessioncontextnum: 1 } );
      commDropCL( db, COMMCSNAME, clName2, true, true, "clear collection in the ending" );
      //关闭所有会话连接
      if( dbArray.length !== 0 )
      {
         for( var i in dbArray )
         {
            var db1 = dbArray[i];
            db1.close();
         }
      }
   }
}

function findCursorOpr ( csName, clName, findCurNum )
{
   var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   var dbcl = db.getCS( csName ).getCL( clName );
   for( var i = 0; i < findCurNum; i++ )
   {
      var cursor = dbcl.find();
      cursor.next();
   }
   return db;
}

function listContexts ( db )
{
   var cursor = db.list( SDB_LIST_CONTEXTS, {}, { "Contexts": { "$include": 0 } } );
   var actListInof = [];
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      actListInof.push( obj );
   }
   cursor.close();
   println( "---actList=" + JSON.stringify( actListInof ) );
}

