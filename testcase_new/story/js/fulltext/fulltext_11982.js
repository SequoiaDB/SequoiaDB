/***************************************************************************
@Description :seqDB-11982 :全文索引个数已达上限时创建全文索引 
@Modify list :
              2018-10-25  YinZhen  Create
****************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_ES_11985";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //在已存在全文索引定义的集合中，再次创建全文索引
   dbcl.createIndex( "a_11982", { content: "text" } );
   assert.tryThrow( SDB_DMS_MAX_INDEX, function()
   {
      dbcl.createIndex( "b_11982", { about: "text" } );
   } );

   var indexes = dbcl.listIndexes();
   var arrayIndexes = new Array();
   while( indexes.next() )
   {
      arrayIndexes.push( indexes.current().toObj() );
   }

   //listIndexes不显示创建失败的集合
   if( arrayIndexes.length != 2 )
   {
      throw new Error( "more than 2 indexes: " + arrayIndexes.length );
   }
   for( var i in arrayIndexes )
   {
      if( arrayIndexes[i]["IndexDef"]["name"] != "$id" && arrayIndexes[i]["IndexDef"]["name"] != "a_11982" )
      {
         throw new Error( "index: $id or a_11982 is not exists." );
      }
   }

   var dbOperator = new DBOperator();
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "a_11982" );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
