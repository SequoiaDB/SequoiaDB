/******************************************************************************
 * @Description   : seqDB-26625:打开mongroupmask，查询过程中关闭连接
 * @Author        : liuli
 * @CreateTime    : 2022.06.30
 * @LastEditTime  : 2022.06.30
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_26625";

main( test );

function test ( args )
{
   db.updateConf( { mongroupmask: "all:detail" } );
   var cl = args.testCL;
   var insertNum = 50000;
   var docs = [];
   for( var i = 0; i < insertNum; i++ )
   {
      docs.push( { a: i, b: i, c: i } );
   }
   cl.insert( docs );

   // 新建一个连接
   var db2 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   var cl2 = db2.getCS( COMMCSNAME ).getCL( testConf.clName );

   var cursor = cl2.find();
   cursor.next();
   db2.close();

   assert.tryThrow( SDB_NOT_CONNECTED, function()
   {
      while( cursor.next() ) { }
   } );
   cursor.close();

   var cursor = cl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );
   db.deleteConf( { mongroupmask: 1 } );
}
