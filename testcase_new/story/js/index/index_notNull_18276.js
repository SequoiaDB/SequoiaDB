/******************************************************************************
*@Description : seqDB-18276:新旧接口创建相同/不同索引 
*@Author      : 2019-4-29  XiaoNi Huang
******************************************************************************/


main( test );

function test ()
{
   var clName = "cl_18276";
   var indexName1 = "idx1";
   var indexName2 = "idx2";

   // ready cl
   commDropCL( db, COMMCSNAME, clName, true, true );
   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, false );

   /**************************** test1, different index ***************************/
   // old function
   var unique = false;
   var enforced = false;
   var sortBufferSize = 32;
   cl.createIndex( indexName1, { a: 1 }, unique, enforced, sortBufferSize );
   checkIndex( cl, indexName1, unique, enforced, NotNull );

   // new function
   var unique = true;
   var enforced = true;
   var NotNull = true;
   var options = { Unique: unique, Enforced: enforced, NotNull: NotNull, SortBufferSize: 128 };
   cl.createIndex( indexName2, { b: 1 }, options );
   checkIndex( cl, indexName2, unique, enforced, NotNull );

   // clean index
   cl.dropIndex( indexName1 );
   cl.dropIndex( indexName2 );


   /**************************** test2, -247 ***************************/

   cl.createIndex( indexName1, { a: 1 }, true );
   assert.tryThrow( SDB_IXM_REDEF, function()
   {
      cl.createIndex( indexName1, { a: 1 }, { Unique: true } );
   } );

   // clean index
   cl.dropIndex( indexName1 );


   /**************************** test3, -46 ***************************/
   cl.createIndex( indexName1, { a: 1 }, true );
   assert.tryThrow( SDB_IXM_EXIST, function()
   {
      cl.createIndex( indexName1, { b: 1 }, { Unique: true } );
   } );

   // clean index
   cl.dropIndex( indexName1 );


   /**************************** test4, -291 ***************************/
   cl.createIndex( indexName1, { a: 1 }, true );
   assert.tryThrow( SDB_IXM_EXIST_COVERD_ONE, function()
   {
      cl.createIndex( indexName2, { a: 1 }, { Unique: true } );
   } );

   // clean index
   cl.dropIndex( indexName1 );

   // clean env
   commDropCL( db, COMMCSNAME, clName, false, false );
}

function checkIndex ( cl, indexName, expUni, expEnf, expNot ) 
{
   if( typeof ( expNot ) == "undefined" ) { expNot = false; }
   var indexDef = cl.getIndex( indexName ).toObj().IndexDef;
   var actUni = indexDef.unique;
   var actEnf = indexDef.enforced;
   var actNot = indexDef.NotNull;
   if( actUni !== expUni || actEnf !== expEnf || actNot !== expNot )
   {
      var expResults = JSON.stringify( { unique: expUni, enforced: expEnf, NotNull: expNot } );
      var actResults = JSON.stringify( { unique: actUni, enforced: actEnf, NotNull: actNot } );
      throw new Error( "checkResult fail,", expResults, "  " + actResults );
   }
}