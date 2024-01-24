/******************************************************************************
@Description   seqDB-22704:插入数据后备节点执行analyze
@author  liyuanyue
@date  2020-9-2
******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = COMMCSNAME + "_22704";
   var clName = COMMCLNAME + "_22704";

   var groupName = commGetDataGroupNames( db )[0];
   var slaveDb = db.getRG( groupName ).getSlave().connect();
   var data = [];
   for( var i = 0; i < 5000; i++ )
   {
      data.push( { a: i, b: i, c: i } );
   }
   for( var i = 0; i < 50; i++ )
   {
      commDropCS( db, csName );
      var cl = commCreateCL( db, csName, clName, { Group: groupName } );
      cl.insert( data );
      try
      {
         slaveDb.analyze( { CollectionSpace: csName } );
         db.analyze( { CollectionSpace: csName } );
      } catch( e )
      {
         /*
            之所以不检验一致性而采用这种方法过滤错误是由于：
            出错可能导致 LSN 不一致，如果 LSN 检测不通过，不知道是错误导致的，还是其他用例回放 LSN 始终未同步导致的
         */
         // 可能由于备节点未同步cl报错
         if( !commCompareErrorCode( e, SDB_DMS_CS_NOTEXIST ) )
         {
            throw e;
         }
      }
   }
   commDropCS( db, csName );
}
