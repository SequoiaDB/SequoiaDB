/************************************************************************
*@Description:	seqDB-6071�����ӱ��в������ݲ��ڷ�����Χ��_SD.transaction.045
               �ֱ�ִ�п�������ɾ�������벻���ӱ�������Χ�ڵļ�¼���ύ
*@Author:  		TingYU  2015/11/25
               wuyan 2017/1/6(�޸��ظ�ִ�лع�������) 
************************************************************************/
main();

function main ()
{
   var csName = COMMCSNAME;
   var mainclName = COMMCLNAME + "_maincl6071";
   var subclName = COMMCLNAME + "_subcl6071";

   try
   {
      var allGroupInfo = commGetGroups( db, true )
      if( 2 > allGroupInfo.length )
      {
         println( "only one group." );
         return;
      }

      var maincl = readyCL( csName, mainclName, subclName );

      //begin transaction and remove      
      var dataNum = 100;
      var insert = new insertData( maincl, dataNum );
      var remove = new removeData( maincl );
      execTransaction( insert, beginTrans, remove );
      checkResult( maincl, true, remove );

      //insert a record that is out of range
      try
      {
         maincl.insert( { mainSk: 101 } );
         throw buildException( "insert", "", " maincl.insert({mainSK:101})",
            -135, "did not throw any error" );
      }
      catch( e )
      {
         var expErr = -135;
         if( e !== expErr )
         {
            throw e;
         }
      }
      checkResult( maincl, false, remove );

      //commit
      try
      {
         execTransaction( commitTrans );
      }
      catch( e )
      {
         throw e;
      }
      checkResult( maincl, false, remove );

      clean( csName, mainclName );
      clean( csName, subclName );
   }
   catch( e )
   {
      throw e;
   }
}

function readyCL ( csName, mainclName, subclName )
{
   println( "--create maincl subcl" );

   commDropCL( db, csName, mainclName, true, true, "drop main cl in begin" );
   commDropCL( db, csName, subclName, true, true, "drop sub cl in begin" );

   var mainOpt = { ShardingKey: { mainSk: 1 }, ShardingType: "range", IsMainCL: true, ReplSize: 0 };
   var subOpt = { ReplSize: 0 };
   var maincl =
      commCreateCL( db, csName, mainclName, mainOpt, true, false, "create mian cl in begin" );
   commCreateCL( db, csName, subclName, subOpt, true, false, "create sub cl in begin" );

   println( "--attach cl" );

   var attaOpt = { LowBound: { mainSk: { $minKey: 1 } }, UpBound: { mainSk: 100 } };
   maincl.attachCL( csName + "." + subclName, attaOpt );

   return maincl;
}