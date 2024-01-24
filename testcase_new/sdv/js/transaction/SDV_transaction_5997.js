/* *****************************************************************************
@discretion: ���ӱ�ִ������������ύ����
@author��2015-11-18 wuyan  Init
***************************************************************************** */
main();
function main ()
{
   try
   {
      var csName = CHANGEDPREFIX + "_cs5997";

      if( commIsStandalone( db ) )
      {
         println( "skip standalone!" );
         return;
      }
      if( commGetGroupsNum( db ) < 2 )
      {
         println( "This testcase needs at least 2 groups to split sub cl!" );
         return;
      }

      var cl = createCL( csName );

      var dataNum = 1000;
      var insert = new insertData( cl, dataNum );
      //update data,then commmit transaction
      var update = new updateData( cl );
      var remove = new removeData( cl );
      execTransaction( beginTrans, insert, update );
      checkResult( cl, true, update );
      execTransaction( remove, commitTrans );
      checkResult( cl, true, remove );

      //@ clean end
      commDropCS( db, csName, false, "drop CS in the beginning" );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      if( undefined !== db )
      {
         db.close();
      }
   }
}


function createCL ( csName )
{
   try
   {
      var mainCLName = CHANGEDPREFIX + "_mcl5997";
      var subCLName = CHANGEDPREFIX + "_scl5997";

      var cs = commCreateCS( db, csName, true, "create cs in the beginning" );
      var mainCL = cs.createCL( mainCLName, {
         ShardingKey: { no: 1 }, ShardingType: "range", ReplSize: 0,
         Compressed: true, IsMainCL: true
      } );

      var subCL = cs.createCL( subCLName, {
         ShardingKey: { no: 1 }, ShardingType: "hash", ReplSize: 0,
         Compressed: true
      } );
      var options = { LowBound: { "no": 0 }, UpBound: { "no": 1000 } };
      mainCL.attachCL( csName + "." + subCLName, options );
      return mainCL;
   }
   catch( e )
   {
      throw e;
   }
}




