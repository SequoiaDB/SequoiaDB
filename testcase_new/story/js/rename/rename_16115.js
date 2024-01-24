/* *****************************************************************************
@discretion: rename cs
             seqDB-16115 �зֱ����зֵ���������飬�޸�cs�� 
@author��2018-10-13 chensiqin  Init
***************************************************************************** */
/*
1������cs��cl���������ݲ��ҷֵ����������
2���޸�cs����ִ�����ݲ�����LOB���������������� 
3�����cs��cl���գ������ļ��������ļ���LOB�ļ���LOBԪ�����ļ�
*/
main( test );
function test ()
{
   try
   {
      if( commGetGroupsNum( db ) < 2 )
      {
         return;
      }
      var csName1 = CHANGEDPREFIX + "_rename16115_1";
      var csName2 = CHANGEDPREFIX + "_rename16115_2";
      var clName1 = CHANGEDPREFIX + "renamecl16115_1";
      var fileName = CHANGEDPREFIX + "_lobtest16115.file";
      var groups = commGetGroups( db );
      var groupName1 = groups[0][0].GroupName;
      var groupName2 = groups[1][0].GroupName;
      //����cs cl
      commDropCS( db, csName1, true, "ignoreNotExist is true" );
      commDropCS( db, csName2, true, "ignoreNotExist is true" );
      var varCS1 = commCreateCS( db, csName1, true, "create CS" );
      var varCL = commCreateCL( db, csName1, clName1, { ShardingKey: { a: 1 }, ShardingType: "hash", Group: groupName1 }, true, false, "create cl in the beginning" );
      var recordNums = 2000;
      insertData( varCL, recordNums );
      varCL.split( groupName1, groupName2, 50 );
      db.renameCS( csName1, csName2 );
      checkRenameCSResult( csName1, csName2, 1 );
      var cs = db.getCS( csName2 );
      var varCL = cs.getCL( clName1 );
      var srcMd5 = createFile( fileName );
      var lobIdArr = putLobs( varCL, fileName );
      checkDatas( csName2, clName1, recordNums, srcMd5, lobIdArr );
      commDropCS( db, csName1, true, "ignoreNotExist is true" );
      commDropCS( db, csName2, true, "ignoreNotExist is true" );
   } finally
   {
      var cmd = new Cmd();
      cmd.run( "rm -rf *" + fileName );
   }
}

function checkDatas ( csName, newCLName, expRecordNums, srcMd5, expLobArr )
{
   //check the record nums      
   var dbcl = db.getCS( csName ).getCL( newCLName );
   var count = dbcl.count();
   assert.equal( count, expRecordNums );

   //check the lob
   checkLob( dbcl, expLobArr, srcMd5 );
}