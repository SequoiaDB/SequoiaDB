/************************************************************************
*@Description:	seqDB-6023��������ִ�в���������ع�����_SD.transaction.034
*@Author:  		TingYU  2015/11/24
************************************************************************/
main();

function main ()
{
   var csName = COMMCSNAME + "_yt6023";
   var clName = COMMCLNAME + "_yt6023";

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

      //1.create splited cl
      var groupsofSplit = select2RG();	             //{srcRG:'xx', tgtRG:'xx'}
      var srcRG = groupsofSplit.srcRG;
      var tgtRG = groupsofSplit.tgtRG;
      var option = { ReplSize: 0, ShardingKey: { no: 1 }, ShardingType: "range", Group: srcRG };
      var cl = createSplitCL( csName, clName, option );

      //2.split
      var dataNum = 10000;
      println( "--split cl by range" );
      cl.split( srcRG, tgtRG, { no: dataNum / 2 }, { no: dataNum } );

      //3.insert
      var insert = new insertData( cl, dataNum );
      execTransaction( beginTrans, insert );
      var expSrcCnt = dataNum / 2;
      var expTgtCnt = dataNum / 2;
      checkSplitedCL( csName, clName, srcRG, tgtRG, expSrcCnt, expTgtCnt );

      //4.rollback
      execTransaction( rollbackTrans );
      checkResult( cl, false, insert );

      clean( csName, clName );
   }
   catch( e )
   {
      throw e;
   }
}

function createSplitCL ( csName, clName, option )
{
   println( "--ready cl" );

   commDropCL( db, csName, clName, true, true, "drop cl in begin" );
   var cl = commCreateCL( db, csName, clName, option, true, false, "create cl in begin" );

   return cl;
}

function checkSplitedCL ( csName, clName, srcRG, tgtRG, expSrcCnt, expTgtCnt )
{
   //get count of every groups
   var srcHostPort = db.getRG( srcRG ).getMaster().toString();
   var tgtHostPort = db.getRG( tgtRG ).getMaster().toString();
   var srcCnt = new Sdb( srcHostPort ).getCS( csName ).getCL( clName ).count();
   var tgtCnt = new Sdb( tgtHostPort ).getCS( csName ).getCL( clName ).count();
   srcCnt = Number( srcCnt );
   tgtCnt = Number( tgtCnt );

   //get total count
   var totalCnt = db.getCS( csName ).getCL( clName ).count();
   totalCnt = Number( totalCnt );

   //check every groups
   if( srcCnt !== expSrcCnt )
   {
      throw buildException( 'checkSplitedCL()', null,
         'new Sdb(' + srcHostPort + ').' + csName + '.' + clName + '.count()',
         expSrcCnt, srcCnt );
   }
   if( tgtCnt !== expTgtCnt )
   {
      throw buildException( 'checkSplitedCL()', null,
         'new Sdb(' + tgtHostPort + ').' + csName + '.' + clName + '.count()',
         expTgtCnt, tgtCnt );
   }

   //check total count
   var expTotalCnt = expSrcCnt + expTgtCnt;
   if( totalCnt !== expTotalCnt )
   {
      throw buildException( 'checkSplitedCL()', null,
         'db.' + csName + '.' + clName + '.count()',
         expTotalCnt, totalCnt );
   }
}