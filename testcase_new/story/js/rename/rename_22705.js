/****************************************************
@description:   renameCL不需要等其他cl的事务操作结束
                testlink cases:  seqDB-22705
@modify list:
                2020-09-02 HaiLin Zhao init
****************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var allGroupName = commGetDataGroupNames( db );
   var groupName = allGroupName[0];
   var clNameOne = COMMCLNAME + "_22705_1";
   var clNameTwo = COMMCLNAME + "_22705_2";
   var clNameNew = COMMCLNAME + "_22705_new";
   commDropCL( db, COMMCSNAME, clNameOne );
   commDropCL( db, COMMCSNAME, clNameTwo );

   var next_db = new Sdb( COORDHOSTNAME, COORDSVCNAME );

   var opt = { "Group": groupName };

   commCreateCL( db, COMMCSNAME, clNameOne, opt, true, false, "create cl1 in begin" );
   commCreateCL( db, COMMCSNAME, clNameTwo, opt, true, false, "create cl2 in begin" );

   db.transBegin();
   db.getCS( COMMCSNAME ).getCL( clNameOne ).insert( { a: 1 } );
   next_db.getCS( COMMCSNAME ).renameCL( clNameTwo, clNameNew );
   checkRenameCLResult( COMMCSNAME, clNameTwo, clNameNew );

   db.transCommit();

   commDropCL( db, COMMCSNAME, clNameOne );
   commDropCL( db, COMMCSNAME, clNameNew );

   next_db.close();

}
