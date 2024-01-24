/******************************************************************************
*@Description : seqDB-18280:垂直/水平分区表，创建索引指定NotNull 
*@Author      : 2019-5-6  XiaoNi Huang
******************************************************************************/

main( test );

function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   var mclName = "mcl_18280";
   var sclName = "scl_18280";
   var mIdxName = "mIdx";
   var sIdxName = "sIdx";

   // ready cl
   commDropCL( db, COMMCSNAME, mclName, true, true, "Failed to drop CL in the pre-condition." );
   commDropCL( db, COMMCSNAME, sclName, true, true, "Failed to drop CL in the pre-condition." );

   /************************* test1, main cl[ NotNull:true ], sub cl[ NotNull:false ] *******************/
   var mCL = commCreateCL( db, COMMCSNAME, mclName, { ShardingKey: { a: 1 }, IsMainCL: true }, true, true );
   mCL.createIndex( mIdxName, { b: 1 }, { NotNull: true } );
   // check index of main cl 
   mCL.getIndex( mIdxName );

   var sCL = commCreateCL( db, COMMCSNAME, sclName, { ShardingKey: { a: 1 } }, true, true );
   sCL.createIndex( sIdxName, { b: 1 }, { NotNull: false } );

   mCL.attachCL( COMMCSNAME + "." + sclName, { LowBound: { a: { $minKey: 0 } }, UpBound: { a: { $maxKey: 0 } } } );

   var recs = [{ a: 1, b: 1 }, { a: 2 }, { a: 3, b: null }];
   mCL.insert( recs );

   checkIndex( sCL, sIdxName, false );
   checkRecords( mCL, recs );

   // clean index
   sCL.dropIndex( sIdxName );
   mCL.remove();

   // clean env
   commDropCL( db, COMMCSNAME, mclName, false, false, "Failed to drop CL in the end-condition" );


   /************************* test2, main cl[ NotNull:false ], sub cl[ NotNull:true ] *******************/
   var mCL = commCreateCL( db, COMMCSNAME, mclName, { ShardingKey: { a: 1 }, IsMainCL: true }, true, true );
   mCL.createIndex( mIdxName, { b: 1 }, { NotNull: false } );

   var sCL = commCreateCL( db, COMMCSNAME, sclName, { ShardingKey: { a: 1 } }, true, true );
   sCL.createIndex( sIdxName, { b: 1 }, { NotNull: true } );

   mCL.attachCL( COMMCSNAME + "." + sclName, { LowBound: { a: { $minKey: 0 } }, UpBound: { a: { $maxKey: 0 } } } );

   var valRecs = [{ a: 1, b: 1 }];
   var invRecs = [{ a: 2 }, { a: 3, b: null }];
   mCL.insert( valRecs );
   for( i = 0; i < invRecs.length; i++ ) 
   {
      assert.tryThrow( SDB_IXM_KEY_NOTNULL, function()
      {
         mCL.insert( invRecs[i] );
      } );
   }

   checkIndex( mCL, mIdxName, false );
   checkIndex( sCL, sIdxName, true );
   checkRecords( mCL, valRecs );

   // clean env
   commDropCL( db, COMMCSNAME, mclName, false, false, "Failed to drop CL in the end-condition" );
}

function checkIndex ( cl, indexName, expNot ) 
{
   var indexDef = cl.getIndex( indexName ).toObj().IndexDef;
   var actNot = indexDef.NotNull;
   assert.equal( actNot, expNot );
}

function checkRecords ( cl, expRecs ) 
{
   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   var actRecs = new Array();
   while( tmpRecs = rc.next() )
   {
      actRecs.push( tmpRecs.toObj() );
   }

   assert.equal( expRecs, actRecs );
}