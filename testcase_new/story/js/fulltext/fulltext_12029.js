/******************************************************************************
@Description :   seqDB-12029:更新记录，使记录中的索引字段存在/不存在
@Modify list :   2018-10-08  xiaoni Zhao  Init
******************************************************************************/
main( test );

function test ()
{

   if( commIsStandalone( db ) )
   {
      return;
   }

   var esOperator = new ESOperator();
   var dbOperator = new DBOperator();
   var clName = COMMCLNAME + "_ES_12029";
   var queryCond = '{ "query" : { "match_all" : {} }, "size" : "20" }';
   var findCond = { "": { "$Text": { "query": { "match_all": {} } } } };
   var selectorCond = { a: "", b: "" };
   var textIndexName = "a_12029";

   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   commCreateIndex( dbcl, textIndexName, { a: "text", b: "text" } );

   insertData( dbcl );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 3 );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   var expectResult = esOperator.findFromES( esIndexNames[0], queryCond ).sort( compare( 'a', compare( 'b' ) ) );
   var actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond ).sort( compare( 'a', compare( 'b' ) ) );
   checkResult( expectResult, actResult );

   updateData( dbcl );
   dbcl.insert( { a: "new", b: "new" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 4 );

   expectResult = new Array( { "a": "a", "b": "b" },
      { "a": "a" },
      { "a": "a2", "b": "b2" },
      { "a": "new", "b": "new" } );
   actResult = esOperator.findFromES( esIndexNames[0], queryCond ).sort( compare( 'a', compare( 'b' ) ) );
   checkResult( expectResult, actResult );

   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function insertData ( dbcl )
{
   dbcl.insert( { _id: 1, a: "a1", b: "b1" } );
   dbcl.insert( { _id: 2 } );
   dbcl.insert( { _id: 3, a: "a3", b: "b3" } );
   dbcl.insert( { _id: 4, a: "a4", b: "b4" } );
}

function updateData ( dbcl )
{
   dbcl.update( { $unset: { a: "", b: "" } }, { _id: 1 } );
   dbcl.update( { $set: { a: "a2", b: "b2" } }, { _id: 2 } );
   dbcl.update( { $set: { a: "a", b: "b" } }, { _id: 3 } );
   dbcl.update( { $replace: { a: "a" } }, { _id: 4 } );
}
