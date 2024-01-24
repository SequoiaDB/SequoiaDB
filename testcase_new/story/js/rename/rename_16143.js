/* *****************************************************************************
@discretion: rename cl
             seqDB-16143
@author��2018-10-15 chensiqin  Init
***************************************************************************** */
main( test );
function test ()
{
   if( commGetGroupsNum( db ) < 2 )
   {
      return;
   }
   var csName = CHANGEDPREFIX + "_cs16143";
   commDropCS( db, csName, true, "drop CS " + csName );
   var cs = commCreateCS( db, csName, true, "create CS1" );

   var clName = CHANGEDPREFIX + "_cl16143";
   var newClName = CHANGEDPREFIX + "_newcl16143";
   var groups = commGetGroups( db );
   var groupName1 = groups[0][0].GroupName;
   var groupName2 = groups[1][0].GroupName;
   var varCL = commCreateCL( db, csName, clName, { ShardingKey: { a: 1 }, ShardingType: "hash", Partition: 1024, Group: groupName1 }, true, false, "create cl in the beginning" );
   var recordNums = 2000;
   insertData( varCL, recordNums );
   varCL.split( groupName1, groupName2, 50 );
   cs.renameCL( clName, newClName );

   //��cl��ִ�в���
   varCL = cs.getCL( newClName );
   insertData( varCL, recordNums );
   checkDatas( csName, newClName, 2 * recordNums, { no: { $lt: 2000 } } );

   //����
   varCL.update( { $set: { no: 2 } } );
   checkDatas( csName, newClName, 2 * recordNums, { no: { $et: 2 } } );

   //ɾ��
   varCL.remove( { no: { $et: 2 } } );
   checkDatas( csName, newClName, 0, { no: { $et: 2 } } );

   commDropCS( db, csName, true, "drop CS " + csName );
}

function checkDatas ( csName, newClName, expRecordNums, cond )
{
   //check the record nums      
   var dbcl = db.getCS( csName ).getCL( newClName );
   var count = dbcl.count( cond );
   assert.equal( count, expRecordNums );
}
