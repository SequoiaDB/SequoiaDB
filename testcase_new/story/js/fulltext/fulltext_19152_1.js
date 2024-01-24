/************************************
*@Description: seqDB-19152:���ȫ�������������ֶ�Ϊ����Ԫ�أ�����ȫ��ȫ�������ֶ�Ϊobj��valueΪstring(����Ԫ��)
*dbcl.update({$set:{a:[{0:"update1"}, {1:"update2"}, {2:"update3"}]}});---��֧�ֱ���-37
*@author:      zhaoyu
*@createdate:  2019.08.14
*@testlinkCase: seqDB-19152
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) ) { return; }

   var clName = COMMCLNAME + "_19152_3";
   var textIndexName = "textIndex_19152_3";
   dropCL( db, COMMCSNAME, clName, true, true );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   dbcl.createIndex( textIndexName, { "a.1": "text", "a.2": "text" } );
   var objs = new Array( { id: 1, a: "string1", b: "string" },
      { id: 2, a: 1, b: 1 },
      { id: 7, a: { 0: "obj3", 1: "obj4", 2: "obj5" }, b: { 0: "obj3", 1: "obj4", 2: "obj5" } } );
   dbcl.insert( objs );
   dbcl.update( { $set: { "a.0": "update1", "a.1": "update2", "a.2": "update3" } } );

   var dbOpr = new DBOperator();
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 1 );
   var findCond = { "": { "$Text": { "query": { "match_all": {} } } } };
   var actResult = dbOpr.findFromCL( dbcl, findCond, { "a": { "$include": 1 } }, { _id: 1 } );
   var expResult = [{ a: { 0: "update1", 1: "update2", 2: "update3" } }];
   checkResult( expResult, actResult );

   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
