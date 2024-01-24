/******************************************************************************
 * @Description   : seqDB-24895:会话在节点开启上下文超过限制（默认值） 
 * @Author        : Zhang Yanan
 * @CreateTime    : 2021.12.31
 * @LastEditTime  : 2022.01.05
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "cl_24895";

main( test );
function test ( testPara )
{
   var varCL = testPara.testCL;
   var maxSessioncontextnum = 100;
   insertData( varCL, 1000 );
   try
   {
      var db1 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      var dbcl = db1.getCS( COMMCSNAME ).getCL( testConf.clName );
      for( var j = 0; j < maxSessioncontextnum; j++ )
      {
         var cursor = dbcl.find();
         var obj = cursor.next();
      }
      assert.tryThrow( SDB_DPS_CONTEXT_NUM_UP_TO_LIMIT, function()
      {
         dbcl.findOne().toString();
         listContexts( db1 );
      } )
   } finally
   {
      db1.close();
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