/******************************************************************************
@Description: seqDB-7493:删除子表/子表所在CS，在主表做CRUD操作（jira-835）
@modify list:
   2014-5-5   xiaojun Hu   Init
   2019-4-15  xiaoni huang modify
*******************************************************************************/

main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   db.setSessionAttr( { PreferedInstance: "M" } );

   var mcsName = COMMCSNAME;
   var scsName1 = "scs_7493_1"
   var scsName2 = "scs_7493_2"
   var mclName = "mcl_7493";
   var sclName1 = "scl_1";
   var sclName2 = "scl_2";

   // clear env
   commDropCL( db, mcsName, mclName, true, true, "drop mcl in the begin" );
   commDropCS( db, scsName1, true, "drop main cs in the begin." );
   commDropCS( db, scsName2, true, "drop sub cs in the begin." );

   // create cs and cl, attach cl
   var mclOpt = { "ShardingKey": { a: 1 }, "IsMainCL": true };
   var mainCL = commCreateCL( db, COMMCSNAME, mclName, mclOpt, true, false );
   commCreateCL( db, scsName1, sclName1 );
   commCreateCL( db, scsName2, sclName2 );
   mainCL.attachCL( scsName1 + "." + sclName1, { LowBound: { a: 0 }, UpBound: { a: 100 } } );
   mainCL.attachCL( scsName2 + "." + sclName2, { LowBound: { a: 100 }, UpBound: { a: 200 } } );

   // insert records
   var recordsNum = 200;
   var docs = [];
   for( var i = 0; i < recordsNum; ++i )
   {
      docs.push( { a: i } );
   }
   mainCL.insert( docs );

   // drop sub cs
   commDropCS( db, scsName1, false, "drop sub cs1" );
   commDropCS( db, scsName2, false, "drop sub cs2" );

   // CRUD
   // insert
   assert.tryThrow( SDB_CAT_NO_MATCH_CATALOG, function()
   {
      mainCL.insert( { a: 1 } );
   } );

   // update
   mainCL.update( { $set: { a: 1 } } );
   var cnt = mainCL.count();
   assert.equal( cnt, 0 );

   // find   
   var cursor = mainCL.find( { a: 1 } );
   assert.equal( cursor.next(), undefined );

   // remove
   mainCL.remove( { a: 1 } );

   // clear env
   commDropCL( db, COMMCSNAME, mclName, false, false, "drop mcl in the end" );
}