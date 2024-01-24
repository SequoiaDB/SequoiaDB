/* *****************************************************************************
@discretion: rename cl
             seqDB-16072
@author��2018-10-15 chensiqin  Init
***************************************************************************** */
main( test );

function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   var csName = CHANGEDPREFIX + "_cs16072";
   var clName = CHANGEDPREFIX + "_cl16072";
   var newClName = CHANGEDPREFIX + "_newcl16072";
   var pcdName = "procedure16072"

   commDropCS( db, csName, true, "drop CS " + csName );
   assert.tryThrow( SDB_FMP_FUNC_NOT_EXIST, function()
   {
      db.removeProcedure( "test16072" );
   } );

   var cs = commCreateCS( db, csName, true, "create CS1" );
   var varCL = commCreateCL( db, csName, clName, {}, true, false, "create cl in the beginning" );

   var recordNums = 100;
   insertData( varCL, recordNums );

   //1.�����洢���̣�ָ��cl��ѯ
   db.createProcedure( function test16072 ( csName, clName ) { return db.getCS( csName ).getCL( clName ).count( { a: { $lt: 95 } } ); } );
   var ret = db.eval( 'test16072("' + csName + '", "' + clName + '")' );
   assert.equal( ret, 95 );

   //2.renameCL �ٴ�ִ�д洢���̲�ѯ
   cs.renameCL( clName, newClName );
   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      db.eval( 'test16072("' + csName + '", "' + clName + '")' );
   } );

   //3.�½�clָ��Ϊ�ɵ�cl���������¼���ٴ�ִ�д洢���̲�ѯ
   commCreateCL( db, csName, clName, {}, true, false, "create cl in the beginning" );
   insertData( varCL, 50 );
   ret = db.eval( 'test16072("' + csName + '", "' + clName + '")' );
   assert.equal( ret, 50 );

   db.removeProcedure( "test16072" );
   commDropCS( db, csName, true, "ignoreNotExist is true" );
}

function checkDatas ( csName, newCLName, expRecordNums )
{
   //check the record nums      
   var dbcl = db.getCS( csName ).getCL( newCLName );
   var count = dbcl.count();
   assert.equal( count, expRecordNums );

}
