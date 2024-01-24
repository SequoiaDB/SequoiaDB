/***************************************************************************
@Description :seqDB-11983 :索引个数已达上限时创建全文索引   
@Modify list :
              2018-10-26  YinZhen  Create
****************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_ES_11983";
   dropCL( db, COMMCSNAME, clName, true, true );

   //创建64个索引
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   for( var i = 0; i < 63; i++ )
   {
      var obj = new Object();
      obj["a" + i] = 1;
      dbcl.createIndex( "a" + i, obj );
   }

   //在已创建64个索引的情况下，创建全文索引
   assert.tryThrow( SDB_DMS_MAX_INDEX, function()
   {
      dbcl.createIndex( "fullIndex_11983", { content: "text" } );
   } );

   var indexes = dbcl.listIndexes();
   var arrayIndexes = new Array();
   while( indexes.next() )
   {
      var index = indexes.current().toObj();
      arrayIndexes.push( index["IndexDef"]["name"] );
   }

   //检查listIndexes是否包含创建失败的索引名
   for( var i in arrayIndexes )
   {
      if( arrayIndexes[i] == "fullIndex_11983" )
      {
         throw new Error( "fullIndex_11983 exists,index in cl: " + arrayIndexes );
      }
   }
   dropCL( db, COMMCSNAME, clName, true, true );
}
