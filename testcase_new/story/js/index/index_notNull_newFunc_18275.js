/******************************************************************************
*@Description : seqDB-18275:新增接口创建索引，覆盖所有参数 
*@Author      : 2019-4-29  XiaoNi Huang
******************************************************************************/


main( test );

function test ()
{
   var clName = "cl_18275";
   var indexName = "idx";

   // ready cl
   commDropCL( db, COMMCSNAME, clName, true, true );
   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, false );

   /**************************** test1, cover: all param ***************************/
   var unique = false;
   var enforced = false
   var NotNull = false;
   var options = { Unique: unique, Enforced: enforced, NotNull: NotNull, SortBufferSize: 32 };
   cl.createIndex( indexName, { a: 1 }, options );

   checkIndex( cl, indexName, unique, enforced, NotNull );

   // clean index
   cl.dropIndex( indexName );


   /**************************** test2, SortBufferSize:0 ***************************/
   var unique = true;
   var enforced = true;
   var NotNull = true;
   var options = { Unique: unique, Enforced: enforced, NotNull: NotNull, SortBufferSize: 32 };
   cl.createIndex( indexName, { a: 1 }, options );

   checkIndex( cl, indexName, unique, enforced, NotNull );

   // clean index
   cl.dropIndex( indexName );


   /**************************** test3, SortBufferSize < 0 ***************************/
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.createIndex( indexName, { a: 1 }, { SortBufferSize: -1 } );
   } );

   // clean env
   commDropCL( db, COMMCSNAME, clName, false, false );
}

function checkIndex ( cl, indexName, expUni, expEnf, expNot ) 
{
   var indexDef = cl.getIndex( indexName ).toObj().IndexDef;
   var actUni = indexDef.unique;
   var actEnf = indexDef.enforced;
   var actNot = indexDef.NotNull;
   if( actUni !== expUni || actEnf !== expEnf || actNot !== expNot )
   {
      var expResults = JSON.stringify( { unique: expUni, enforced: expEnf, NotNull: expNot } );
      var actResults = JSON.stringify( { unique: actUni, enforced: actEnf, NotNull: actNot } );
      throw new Error( "checkResult fail," + expResults + "  " + actResults );
   }
}