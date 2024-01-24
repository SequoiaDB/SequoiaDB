/************************************
*@Description: seqDB-19153:���ȫ�������������ֶ�Ϊ����Ԫ�أ�����ȫ��ȫ�������ֶ�Ϊ��string����
*@author:      zhaoyu
*@createdate:  2019.08.14
*@testlinkCase: seqDB-19153
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) ) { return; }

   var clName = COMMCLNAME + "_19153";
   var textIndexName = "textIndex_19153";
   dropCL( db, COMMCSNAME, clName, true, true );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   dbcl.createIndex( textIndexName, { "a.1": "text", "a.2": "text" } );
   var objs = new Array( { id: 1, a: "string1", b: "string" },
      { id: 2, a: 1, b: 1 },
      { id: 7, a: { 0: "obj3", 1: "obj4", 2: "obj5" }, b: { 0: "obj3", 1: "obj4", 2: "obj5" } } );
   dbcl.insert( objs );
   dbcl.update( { $set: { a: { 0: 1, 1: 2, 2: 3 } } } );

   var dbOpr = new DBOperator();
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 0 );
   var findCond = { "": { "$Text": { "query": { "match_all": {} } } } };
   var actResult = dbOpr.findFromCL( dbcl, findCond, { "a": { "$include": 1 } }, { _id: 1 } );
   var expResult = [];
   checkResult( expResult, actResult );

   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
