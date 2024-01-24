/******************************************************************************
*@Description : seqDB-18277:创建单键索引，指定NotNull，创建索引前后插入数据 
*@Author      : 2019-4-29  XiaoNi Huang
******************************************************************************/


main( test );

function test ()
{
   var clName = "cl_18277";
   var indexName = "idx";
   var validRecs1 = [{ a: 1, b: 1 }];
   var validRecs2 = [{ a: 1, b: 1 }, { b: 2 }, { a: null, b: 3 }]; // a contain: not exist, null 
   var invRecs = [{ b: 2 }, { a: null, b: 3 }];

   // ready cl
   commDropCL( db, COMMCSNAME, clName, true, true );
   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, false );

   /**************************** test1, create index[ NotNull:true ] -> insert ***************************/
   var NotNull = true;
   cl.createIndex( indexName, { a: 1 }, { NotNull: NotNull } );

   var valRecs = validRecs1;
   cl.insert( valRecs );
   for( i = 0; i < invRecs.length; i++ ) 
   {
      assert.tryThrow( SDB_IXM_KEY_NOTNULL, function()
      {
         cl.insert( invRecs[i] );
      } );
   }

   checkIndex( cl, indexName, NotNull );
   checkRecords( cl, valRecs );

   // clean index
   cl.dropIndex( indexName );
   cl.remove();


   /**************************** test2, create index[ NotNull:false ] -> insert ***************************/
   var NotNull = false;
   cl.createIndex( indexName, { a: 1 }, { NotNull: NotNull } );

   var valRecs = validRecs2;
   cl.insert( valRecs );

   checkIndex( cl, indexName, NotNull );
   checkRecords( cl, valRecs );

   // clean index
   cl.dropIndex( indexName );
   cl.remove();


   /**************************** test3, insert -> create index[ NotNull:true ]  ***************************/
   var NotNull = true;

   var valRecs = validRecs2;
   cl.insert( valRecs );

   // create index
   assert.tryThrow( SDB_IXM_KEY_NOTNULL, function()
   {
      cl.createIndex( indexName, { a: 1 }, { NotNull: NotNull } );
   } );

   // check results
   checkRecords( cl, valRecs );

   // clean index
   cl.remove();


   /**************************** test4, insert -> create index[ NotNull:false ]  ***************************/
   var NotNull = false;

   var valRecs = validRecs2;
   cl.insert( valRecs );
   cl.createIndex( indexName, { a: 1 }, { NotNull: NotNull } );
   checkRecords( cl, valRecs );

   // clean index
   cl.dropIndex( indexName );
   cl.remove();

   // clean env
   commDropCL( db, COMMCSNAME, clName, false, false );
}

function checkIndex ( cl, indexName, expNot ) 
{
   var indexDef = cl.getIndex( indexName ).toObj().IndexDef;
   var actNot = indexDef.NotNull;
   assert.equal( actNot, expNot );
}

function checkRecords ( cl, expRecs ) 
{
   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { b: 1 } );
   var actRecs = new Array();
   while( tmpRecs = rc.next() )
   {
      actRecs.push( tmpRecs.toObj() );
   }

   assert.equal( expRecs, actRecs );
}