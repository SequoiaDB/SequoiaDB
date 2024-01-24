/******************************************************************************
*@Description : seqDB-18281:options参数校验
*@Author      : 2019-5-6  XiaoNi Zhao
******************************************************************************/

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var mainClName = "mainCl_18281_2";
   var subClName = "subCl_18281_2";
   var indexName = "idx";

   commDropCL( db, COMMCSNAME, mainClName );
   commDropCL( db, COMMCSNAME, subClName );

   var mainCl = commCreateCL( db, COMMCSNAME, mainClName, { IsMainCL: true, ShardingKey: { a: 1 } } );
   var subCl = commCreateCL( db, COMMCSNAME, subClName, { ShardingKey: { a: 1 } } );
   var fullName = COMMCSNAME + "." + subClName;
   mainCl.attachCL( fullName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );

   /**************************** test1, unique:0, enforced:0, NotNull:0 ***************************/
   mainCl.createIndex( indexName, { b: 1, c: 1 }, { unique: 0, enforced: 0, NotNull: 0 } );
   checkIndex( mainCl, indexName, false, false, false );
   var insertR1s = [{ a: 1 }, { a: 1 }, { a: 1, b: 1, c: 1 }, { a: 1, b: 1, c: 1 }];
   mainCl.insert( insertR1s );
   checkRecords( mainCl, insertR1s );
   mainCl.dropIndex( indexName );
   mainCl.remove();


   /**************************** test2, unique:1, enforced:1 ***************************/
   mainCl.createIndex( indexName, { a: 1, b: 1, c: 1 }, { unique: 1, enforced: 1, NotNull: 1 } );
   checkIndex( mainCl, indexName, true, true, true );

   var insertR1 = [{ a: 1, b: 1, c: 1 }];
   mainCl.insert( insertR1 );
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      mainCl.insert( insertR1 );
   } );

   var insertR2 = [{ a: 1, c: 1 }];
   assert.tryThrow( SDB_IXM_KEY_NOTNULL, function()
   {
      mainCl.insert( insertR2 );
   } );
   checkRecords( mainCl, insertR1 );
   mainCl.dropIndex( indexName );
   mainCl.remove();

   commDropCL( db, COMMCSNAME, mainClName, false, false, "Failed to drop mainCl in the end-condition" );
}

function checkIndex ( mainCl, indexName, expUni, expEnf, expNot ) 
{
   if( expUni == undefined ) { expUni = false };
   if( expEnf == undefined ) { expEnf = false };
   if( expNot == undefined ) { expNot = false };

   var indexDef = mainCl.getIndex( indexName ).toObj().IndexDef;
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