/******************************************************************************
 * @Description   : seqDB-24885:catalog上上下文超时清理 
 * @Author        : Yao Kang
 * @CreateTime    : 2021.12.30
 * @LastEditTime  : 2022.04.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_24885";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   var cursor;
   var cursors = [];
   try
   {
      db.updateConf( { "contexttimeout": 1 } );
      insertData( cl, 10000 );

      for( var i = 0; i < 80; i++ )
      {
         var cursorCl = cl.find().sort( { no: 1 } );
         cursorCl.next();
         cursors.push( cursorCl );
      }

      cursor = db.list( SDB_LIST_CONTEXTS );
      cursor.next();
      sleep( 1000 * 60 * 3 );
      assert.tryThrow( SDB_RTN_CONTEXT_NOTEXIST, function()
      {
         while( cursor.next() ) { }
      } );
   } finally
   {
      //恢复默认配置
      db.deleteConf( { "contexttimeout": 1 } );
      cursor.close();
      for( var i = 0; i < cursors.length; i++ )
      {
         cursors[i].close();
      }
   }
}
