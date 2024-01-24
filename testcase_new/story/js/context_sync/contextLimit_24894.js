/******************************************************************************
 * @Description   : seqDB-24894:节点cata平面开启上下文超过限制
 * @Author        : Zhang Yanan
 * @CreateTime    : 2021.12.31
 * @LastEditTime  : 2022.01.05
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var clName = COMMCLNAME + "cl_24894_"
   var clNum = 1001;
   for( var i = 0; i < clNum; i++ )
   {
      commDropCL( db, COMMCSNAME, clName + i, true, true );
   }
   for( var i = 0; i < clNum; i++ )
   {
      commCreateCL( db, COMMCSNAME, clName + i );
   }

   var maxContextnum = 1000;
   var config = { "maxcontextnum": maxContextnum };
   db.updateConf( config );

   var sdbs = [];
   try
   {
      for( var i = 0; i < 10; i++ )
      {
         var db1 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
         sdbs.push( db1 );
         for( var j = 0; j < 100; j++ )
         {
            var cursor = db1.list( SDB_LIST_COLLECTIONS );
            var obj = cursor.next();
         }
      }
      var db1 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      sdbs.push( db1 );
      var cursor = db1.list( SDB_LIST_COLLECTIONS );
      var obj = cursor.next();
   } finally
   {
      for( var i = 0; i < sdbs.length; i++ )
      {
         sdbs[i].close();
      }
      db.deleteConf( { "maxcontextnum": 1 } );
   }
   for( var i = 0; i < clNum; i++ )
   {
      commDropCL( db, COMMCSNAME, clName + i, false, false );
   }
}