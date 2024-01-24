/* *****************************************************************************
@discretion: rename cs
             seqDB-16116 ���ӱ��ڶ���������ϣ��޸�cs�� 
@author��2018-10-13 chensiqin  Init
***************************************************************************** */
/*
 1���������ӱ��ڶ��cs�ϣ��ֱ������ֳ�����
       a.����ӱ�cl�ֲ��ڲ�ͬ����
 2���޸�cs����ִ�����ݲ������磺���룩��LOB���������������� 
 3�����cs��cl���գ������ļ��������ļ���LOB�ļ���LOBԪ�����ļ�
*/
main( test );
function test ()
{
   if( commGetGroupsNum( db ) < 2 )
   {
      return;
   }
   var csName1 = CHANGEDPREFIX + "_maincs16116_1";
   var mainClName = CHANGEDPREFIX + "maincl16116_1";

   var csName3 = CHANGEDPREFIX + "_subcs16116_1";
   var csName4 = CHANGEDPREFIX + "_subcs16116_2";
   var clName1 = CHANGEDPREFIX + "subcl16116_1";
   var clName2 = CHANGEDPREFIX + "subcl16116_2";
   var groups = commGetGroups( db );
   var groupName1 = groups[0][0].GroupName;
   var groupName2 = groups[1][0].GroupName;

   commDropCS( db, csName1, true, "drop CS " + csName1 );
   commDropCS( db, csName3, true, "drop CS " + csName3 );
   commDropCS( db, csName4, true, "drop CS " + csName4 );

   var varCS = commCreateCS( db, csName1, true, "create CS" );
   var varCL = commCreateCL( db, csName1, mainClName, { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range" }, true, false, "create main cl in the beginning" );

   //�ӱ�1
   var subCS = commCreateCS( db, csName3, true, "create CS1" );
   var subcl1 = commCreateCL( db, csName3, clName1, { ShardingKey: { a: 1 }, ShardingType: "hash", Partition: 1024, Group: groupName1 }, true, false, "create sub cl1 in the beginning" );

   //�ӱ�2
   var subcl2 = commCreateCL( db, csName3, clName2, { ShardingKey: { a: 1 }, ShardingType: "hash", Partition: 1024, Group: groupName2 }, true, false, "create sub cl2 in the beginning" );

   //����
   varCL.attachCL( csName3 + "." + clName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   varCL.attachCL( csName3 + "." + clName2, { LowBound: { a: 1000 }, UpBound: { a: 3000 } } );

   //�޸��ӱ�cs name
   db.renameCS( csName3, csName4 );

   var recordNums = 2000;
   insertData( varCL, recordNums );
   varCL.createIndex( "index16116", { no: 1 }, false );
   //check
   checkRenameCSResult( csName3, csName4, 2 );
   var cs = db.getCS( csName1 );
   var varCL = cs.getCL( mainClName );
   checkDatas( csName1, mainClName, recordNums );

   commDropCS( db, csName1, true, "drop CS " + csName1 );
   commDropCS( db, csName3, true, "drop CS " + csName3 );
   commDropCS( db, csName4, true, "drop CS " + csName4 );
}
function checkDatas ( csName, newCLName, expRecordNums )
{
   //check the record nums      
   var dbcl = db.getCS( csName ).getCL( newCLName );
   var count = dbcl.count();
   assert.equal( count, expRecordNums );
}