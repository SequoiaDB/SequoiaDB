/************************************
*@Description: seqDB-19150:����ȫ�������������ֶ�Ϊ����Ԫ�أ�ɾ����¼��ȫ�������ֶ�
*@author:      zhaoyu
*@createdate:  2019.08.14
*@testlinkCase: seqDB-19150
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) ) { return; }

   var clName = COMMCLNAME + "_19150";
   var textIndexName = "textIndex_19150";
   dropCL( db, COMMCSNAME, clName, true, true );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   dbcl.createIndex( textIndexName, { "a.1": "text" } );
   var objs = new Array( { id: 1, a: "string1" },
      { id: 2, a: 1 },
      { id: 3, a: ["string1", "string2", "string3"] },
      { id: 4, a: [1, 2, 3] },
      { id: 5, a: [{ 0: "obj1" }, { 1: "obj2" }, { 2: "obj3" }] },
      { id: 6, a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
      { id: 7, a: { 1: "obj" } },
      { id: 8, a: [["string4", "string5", "string6"], ["string7", "string8", "string9"], ["string10", "string11", "string12"]] },
      { id: 9, a: [[4, 5, 6], [7, 8, 9], [10, 11, 12]] } );
   dbcl.insert( objs );
   dbcl.update( { $unset: { "a.1": "" } } );

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
