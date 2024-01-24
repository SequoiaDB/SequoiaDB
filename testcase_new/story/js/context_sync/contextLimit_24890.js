/******************************************************************************
 * @Description   : 多个会话在同一节点中开启上下文超过限制   
 * @Author        : wu yan
 * @CreateTime    : 2021.12.30
 * @LastEditTime  : 2022.01.07
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_contextlimit_24890";

main( test );
function test ( testPara )
{
   var recordNum = 10000;
   var dbcl = testPara.testCL;

   insertData( dbcl, recordNum );
   var maxContextNum = 1000;
   var maxSessionContextNum = 100;
   var dbarray = [];
   try
   {
      db.updateConf( { maxcontextnum: maxContextNum } );
      var findNum = maxContextNum / maxSessionContextNum;
      for( var i = 0; i < findNum; i++ )
      {
         var db1 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
         dbarray.push( db1 );
         var dbcl1 = db1.getCS( COMMCSNAME ).getCL( testConf.clName );
         for( var j = 0; j < maxSessionContextNum; j++ )
         {
            var cursor = dbcl1.find();
            cursor.next();
         }
      }
      assert.tryThrow( SDB_DPS_CONTEXT_NUM_UP_TO_LIMIT, function()
      {
         dbcl1.findOne().toString();
      } );
   }
   finally
   {
      db.deleteConf( { maxcontextnum: 1 } );
      //关闭所有会话连接
      if( dbarray.length !== 0 )
      {
         for( var i in dbarray )
         {
            var db1 = dbarray[i];
            db1.close();
         }
      }
   }
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

