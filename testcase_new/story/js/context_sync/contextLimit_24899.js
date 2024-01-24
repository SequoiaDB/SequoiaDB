/******************************************************************************
 * @Description   : seqDB-24899:节点cata平面开启上下文超过限制
 * @Author        : Zhang Yanan
 * @CreateTime    : 2021.12.31
 * @LastEditTime  : 2022.01.06
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var clName = COMMCLNAME + "cl_24894_"
   var clNum = 101;
   for( var i = 0; i < clNum; i++ )
   {
      commDropCL( db, COMMCSNAME, clName + i, true, true );
   }
   for( var i = 0; i < clNum; i++ )
   {
      commCreateCL( db, COMMCSNAME, clName + i );
   }

   var maxSessioncontextnum = 100;
   var config = { "maxsessioncontextnum": maxSessioncontextnum };
   db.updateConf( config );

   var sdbs = [];
   try
   {
      var db1 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      sdbs.push( db1 );
      for( var j = 0; j < maxSessioncontextnum + 1; j++ )
      {
         var cursor = db1.list( SDB_LIST_COLLECTIONS );
         var obj = cursor.next();
      }
   } finally
   {
      for( var i = 0; i < sdbs.length; i++ )
      {
         sdbs[i].close();
      }
      db.deleteConf( { "maxsessioncontextnum": 1 } );
   }

   for( var i = 0; i < clNum; i++ )
   {
      commDropCL( db, COMMCSNAME, clName + i, false, false );
   }
}