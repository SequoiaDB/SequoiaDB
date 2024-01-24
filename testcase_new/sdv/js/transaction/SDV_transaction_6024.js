/************************************************************************
*@Description:	seqDB-6024�����ӱ�ִ������������ع�����_SD.transaction.035
               �ֱ�ִ�д������ϡ��������񡢹����ӱ�����ɾ�ġ��ع�
*@Author:  		TingYU  2015/11/24
************************************************************************/
main();
function main ()
{
   var csName = COMMCSNAME;
   var mainclName = COMMCLNAME + "_maincl6024";
   var subclName = COMMCLNAME + "_subcl6024";

   try
   {
      if( commIsStandalone( db ) )
      {
         println( " Deploy mode is standalone!" );
         return;
      }
      if( commGetGroupsNum( db ) < 2 )
      {
         println( "This testcase needs at least 2 groups to split cl!" );
         return;
      }

      var groupsofSplit = select2RG();	             //{srcRG:'xx', tgtRG:'xx'
      var maincl = createMaincl( csName, mainclName );
      createSubcl( csName, subclName, groupsofSplit );

      execTransaction( beginTrans );

      attachCL( maincl, csName, subclName );

      var dataNum = 1000;
      var insert = new insertData( maincl, dataNum );
      var update = new updateData( maincl );
      var remove = new removeData( maincl );
      execTransaction( insert );
      checkResult( maincl, true, insert );
      execTransaction( update );
      checkResult( maincl, true, update );
      execTransaction( remove );
      checkResult( maincl, true, remove );

      execTransaction( rollbackTrans );
      checkResult( maincl, false, insert );

      clean( csName, mainclName );
      clean( csName, subclName );
   }
   catch( e )
   {
      throw e;
   }
}

function createMaincl ( csName, mainclName )
{
   println( "--create main cl" );

   var option = { ShardingKey: { mainSk: 1 }, ShardingType: "range", IsMainCL: true };
   commDropCL( db, csName, mainclName, true, true, "drop main cl in begin" );
   var cl = commCreateCL( db, csName, mainclName, option, true, false, "create mian cl in begin" );
   return cl;
}

function createSubcl ( csName, subclName, groupsOfSplit )
{
   println( "--create sub cl" );

   commDropCL( db, csName, subclName, true, true, "drop sub cl in begin" );
   var option = { ReplSize: 0, ShardingKey: { no: 1 }, ShardingType: "range", Group: groupsOfSplit.srcRG };
   var cl = commCreateCL( db, csName, subclName, option, true, false, "create sub cl in begin" );
   cl.split( groupsOfSplit.srcRG, groupsOfSplit.tgtRG, { no: 500 }, { no: MaxKey() } );
}

function attachCL ( maincl, csName, clName )
{
   println( "--attach cl" );

   var option = { LowBound: { mainSk: { $minKey: 1 } }, UpBound: { mainSk: { $maxKey: 1 } } };
   maincl.attachCL( csName + "." + clName, option );
} 