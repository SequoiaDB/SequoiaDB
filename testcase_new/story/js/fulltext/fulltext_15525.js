/******************************************************************************
@Description :   seqDB-15525:指定_id字段类型为不支持同步到ES端的类型插入记录
@Modify list :   2018-9-28  xiaoni Zhao  Init
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
   var clName = COMMCLNAME + "_ES_15525";
   var findCond = { "": { "$Text": { "query": { "match_all": {} } } } };
   var queryCond = "{\"query\" : {\"match_all\" : {}}}";
   var selectorCond = { a: "" };
   var textIndexName = "a_15525";

   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   insertDate( dbcl );

   commCreateIndex( dbcl, textIndexName, { a: "text" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 1 );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   var expectResult = esOperator.findFromES( esIndexNames[0], queryCond );
   var actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond );
   checkResult( expectResult, actResult );

   dbcl.remove();

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 0 );

   //insert again to test synchronize from cappedCL to ES
   insertDate( dbcl );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 1 );

   expectResult = esOperator.findFromES( esIndexNames[0], queryCond );
   actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond );
   checkResult( expectResult, actResult );

   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function insertDate ( dbcl )
{
   dbcl.insert( { _id: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" }, a: "binary" } );
   dbcl.insert( { _id: null, a: "null" } );
   dbcl.insert( { _id: { "$minKey": 1 }, a: "minKey" } );
   dbcl.insert( { _id: { "$maxKey": 1 }, a: "maxKey" } );
   dbcl.insert( { _id: 1, a: "a1" } );
}
