/******************************************************************************
@Description :   seqDB-15523:ES端_id字段长度限制验证
@Modify list :   2018-9-27  xiaoni Zhao  Init
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
   var clName = COMMCLNAME + "_ES_15523";
   var queryCond = '{ "query" : { "match_all" : {} }, "size" : "20" }';
   var findCond = { "": { "$Text": { "query": { "match_all": {} } } } };
   var selectorCond = { a: "" };
   var textIndexName = "a_15523";

   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   commCreateIndex( dbcl, textIndexName, { a: "text" } );

   var data = getData();
   dbcl.insert( { _id: data, a: "string" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 0 );

   dbcl.insert( { _id: 1, a: "a1" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 1 );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   var expectResult = esOperator.findFromES( esIndexNames[0], queryCond );
   var actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond );
   checkResult( expectResult, actResult )

   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function getData ()
{
   var data = "";
   for( var i = 0; i < 300; i++ )
   {
      data = data + "f";
   }
   return data;
}
