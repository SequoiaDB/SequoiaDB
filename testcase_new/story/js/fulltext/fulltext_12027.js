/******************************************************************************
@Description :   seqDB-12027:使用update接口更新全文索引字段
@Modify list :   2018-10-08  xiaoni Zhao  Init
******************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var dbOperator = new DBOperator();
   var clName = COMMCLNAME + "_ES_12027";
   var findCond = { "": { "$Text": { "query": { "match_all": {} } } } };
   var selectorCond = { a: "" };
   var textIndexName = "a_12072";

   dropCL( db, COMMCSNAME, clName, dbcl, dbcl );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   commCreateIndex( dbcl, textIndexName, { a: "text" } );

   insertData( dbcl );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 4 );

   var expectResult = dbOperator.findFromCL( dbcl, null, selectorCond ).sort( compare( 'a' ) );
   var actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond ).sort( compare( 'a' ) );
   checkResult( expectResult, actResult );

   updateData( dbcl );
   dbcl.insert( { a: "new" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 5 );

   expectResult = dbOperator.findFromCL( dbcl, null, selectorCond ).sort( compare( 'a' ) );
   actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond ).sort( compare( 'a' ) );
   checkResult( expectResult, actResult );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function insertData ( dbcl )
{
   dbcl.insert( { _id: 1, a: "text1" } );
   dbcl.insert( { _id: 2, a: "text2", b: [3, 4] } );
   dbcl.insert( { _id: 3, a: "text3", b: [3, 4] } );
   dbcl.insert( { _id: 4, a: "text4", b: [3, 4] } );
}

function updateData ( dbcl )
{
   dbcl.update( { $inc: { _id: 5 } }, { _id: 1 } );
   dbcl.update( { $addtoset: { b: [1, 2] } }, { _id: 2, b: { $exists: 1 } } );
   dbcl.update( { $pop: { b: 1 } }, { _id: 3 } );
   dbcl.update( { $push: { b: 1 } }, { _id: 4 } );
}
