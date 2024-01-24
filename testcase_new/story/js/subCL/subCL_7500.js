/******************************************************************************
@Description: seqDB-7500:对多个子表做切分后创建索引，删除跟索引字段匹配的记录
@modify list:
   2014-7-30   pusheng Ding  Init
   2019-4-15   xiaoni huang  modify
*******************************************************************************/

main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   if( commGetGroupsNum( db ) < 2 )
   {
      return;
   }
   db.setSessionAttr( { PreferedInstance: "M" } );

   var mclName = "mcl_7500";
   var sclName = "scl_7500";
   var groups = commGetGroups( db, false, "", false, true, true );
   var srcRG = groups[1][0].GroupName;
   var trgRG = groups[2][0].GroupName;

   // clear env
   commDropCL( db, COMMCSNAME, mclName, true, true, "drop mcl in the begin" );
   commDropCL( db, COMMCSNAME, sclName, true, true, "drop scl in the begin" );

   // create cs and cl, attach cl
   var mclOpt = { "ShardingKey": { a: 1 }, "IsMainCL": true };
   var mainCL = commCreateCL( db, COMMCSNAME, mclName, mclOpt, true, false );

   var sOpt = { ShardingKey: { a: 1 }, ShardingType: "hash", Group: srcRG };
   var subCL = commCreateCL( db, COMMCSNAME, sclName, sOpt, true, true );

   mainCL.attachCL( COMMCSNAME + "." + sclName, { LowBound: { a: 0 }, UpBound: { a: 100 } } );

   // insert
   var recordsNum = 100;
   var docs = [];
   for( var i = 0; i < recordsNum; ++i )
   {
      docs.push( { a: i } );
   }
   mainCL.insert( docs );

   // split and create index
   subCL.split( srcRG, trgRG, 50 );
   mainCL.createIndex( "idx", { b: 1 } );

   // CRUD
   // insert
   var docs = [];
   for( var i = 0; i < recordsNum; ++i )
   {
      docs.push( { a: i, b: i } );
   }
   mainCL.insert( docs );
   var cnt = mainCL.count( { b: { $exists: 1 } } );
   assert.equal( cnt, recordsNum );

   // remove
   mainCL.remove( { $and: [{ b: { $lt: 50 } }, { b: { $exists: 1 } }] } );
   subCL.remove( { $and: [{ b: { $gte: 50 } }, { b: { $exists: 1 } }] } );
   var cnt = mainCL.count( { b: { $exists: 1 } } );
   assert.equal( cnt, 0 );

   // count
   var cnt = mainCL.count();
   assert.equal( cnt, recordsNum );

   // clear env
   commDropCL( db, COMMCSNAME, mclName, true, false, "drop mcl in the end" );
}
