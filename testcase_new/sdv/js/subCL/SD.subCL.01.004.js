/************************************************************************
*@Description:	seqDB-817:主表跟子表都为range分区,在主表做insert_SD.subCL.01.004
*@Author:  		TingYU  2015/10/22
************************************************************************/
main();

function main ()
{
   try
   {
      if( commIsStandalone( db ) )
      {
         println( " Deploy mode is standalone!" );
         return;
      }
      if( commGetGroupsNum( db ) < 2 )
      {
         println( "This testcase needs at least 2 groups to split sub cl!" );
         return;
      }

      var csName = COMMCSNAME; //maincl and subcl has the same cs
      var mainCLName = COMMCLNAME + "_maincl";
      var subCLName = COMMCLNAME + "_subcl";

      //create main&sub cl
      var maincl = createMainCL( csName, mainCLName );
      var groupsOfSplit = select2RG();
      createSubCL( csName, subCLName, groupsOfSplit );
      attachCL( maincl, csName, subCLName );

      //insert and check
      var istRecNum = 100;
      insertRecs( maincl, istRecNum );
      checkResult( maincl, csName, subCLName, groupsOfSplit, istRecNum );

      clean( csName, mainCLName, subCLName );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
   }
}

function createMainCL ( csName, mainCLName )
{
   println( "\n---Begin to create main cl" );

   commDropCL( db, csName, mainCLName, true, true, "drop main cl in begin" );
   var option = { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true };
   var cl = commCreateCL( db, csName, mainCLName, option, true, false,
      "create mian cl in begin" );
   return cl;
}

function select2RG ()
{
   var dataRGInfo = commGetGroups( db );
   var rgsName = {};
   rgsName.sourceRG = dataRGInfo[0][0]["GroupName"];
   rgsName.targetRG = dataRGInfo[1][0]["GroupName"];

   return rgsName;
}

function createSubCL ( csName, subCLName, groupsOfSplit )
{
   println( "\n---Begin to create sub cl" );

   commDropCL( db, csName, subCLName, true, true, "drop main cl in begin" );

   var option = { ShardingKey: { b: 1 }, ShardingType: "range", Group: groupsOfSplit.sourceRG };
   var cl = commCreateCL( db, csName, subCLName, option,
      true, false, "create sub cl in begin" );

   cl.split( groupsOfSplit.sourceRG, groupsOfSplit.targetRG, { b: 50 }, { b: MaxKey() } );

   return cl;
}

function attachCL ( maincl, csName, clName )
{
   println( "\n---Begin to attach cl" );

   var option = { LowBound: { "a": { $minKey: 1 } }, UpBound: { "a": { $maxKey: 1 } } };
   maincl.attachCL( csName + "." + clName, option );
}

function insertRecs ( maincl, cnt )
{
   println( "\n---Begin to insert records" );

   for( i = 0; i < cnt; i++ )
   {
      maincl.insert( { a: i, b: i } );
   }
}

function checkResult ( maincl, csName, subCLName, groupsOfSplit, totalCnt )
{
   println( "\n---Begin to check result" );

   //get maincl count
   var mainclCnt = parseInt( maincl.count() );

   //get every group count
   var groupsCnt = new Array;
   var tmpHost1 = db.getRG( groupsOfSplit.sourceRG ).getMaster().toString();
   var tmpHost2 = db.getRG( groupsOfSplit.targetRG ).getMaster().toString();
   var cnt1 = new Sdb( tmpHost1 ).getCS( csName ).getCL( subCLName ).count();
   var cnt2 = new Sdb( tmpHost2 ).getCS( csName ).getCL( subCLName ).count();
   groupsCnt.push( cnt1 );
   groupsCnt.push( cnt2 );

   //check maincl count 
   if( mainclCnt !== totalCnt )
   {
      throw buildException( "check maincl count", null, "maincl.count()",
         totalCnt, mainclCnt );
   }

   //check every group count
   var expCnt = totalCnt / 2;
   if( parseInt( cnt1 ) !== expCnt || parseInt( cnt2 ) !== expCnt )
   {
      throw buildException( "check every group count", null, "link to data node to count",
         sourceRG + ":50," + targetRG + ":50", sourceRG + ":" + expCnt + "," + targetRG + ":" + expCnt );
   }

   //compare maincl count to sum of every group count
   var sumCnt = 0;
   for( var j in groupsCnt )
   {
      sumCnt += groupsCnt[j];//get sum of every group count
   }
   if( mainclCnt !== sumCnt )
   {
      println( "maincl count:" + mainclCnt + ", sum count:" + sumCnt );
      throw buildException( "", null, "compare maincl count to sum of every group count",
         "maincl count equal to sum count:", "no equal" );
   }

   //compare every records 
   var expCnt = 1;
   for( var k = 0; k < mainclCnt; k++ )
   {
      var cnt = maincl.find( { a: k } ).count();
      if( parseInt( cnt ) !== expCnt )
      {
         throw buildException( "compare every records", null,
            "maincl.find({a:" + k + "}).count()", expCnt, cnt );
      }
   }
}

function clean ( csName, mainCLName, subCLName )
{
   println( "\n---begin to clean" );

   commDropCL( db, csName, subCLName, true, true, "drop sub cl in clean " );
   commDropCL( db, csName, mainCLName, true, true, "drop main cl in clean " );
}

