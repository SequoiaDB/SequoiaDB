/************************************************************************
*@Description:  seqDB-814:域指定自动切分,子表不指定自动切分,在主表做insert_SD.subCL.01.001
*           子表使用域中的AutoSplit
*@Author:   2015/10/10   huangxiaoni
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

      var domainName = COMMCLNAME + "_domain";
      var csName = COMMCLNAME + "_cs";
      var mainCLName = COMMCLNAME + "_mcl";
      var subCLName = COMMCLNAME + "_scl";

      println( "\n---Begin to drop cs/domain in the pre-condition." );
      commDropCS( db, csName, true, "Failed to drop CS." );
      dropDM( domainName, true, "Failed to drop domain." );

      createDM( domainName );
      commCreateCS( db, csName, false, "Failed to create CS.", { Domain: domainName } );

      var mainCL = createMainCL( csName, mainCLName );
      createSubCL( csName, subCLName );
      attachCL( csName, mainCL, subCLName );

      var insertRecsNum = 100;
      insertRecs( mainCL, insertRecsNum );
      checkResult( csName, mainCL, subCLName, insertRecsNum );

      println( "\n---Begin to drop cs/domain in the end-condition." );
      commDropCS( db, csName, false, "Failed to drop CS." );
      dropDM( domainName, false, "Failed to drop domain." );
   }
   catch( e )
   {
      throw e;
   }
}

function createDM ( domainName )
{
   println( "\n---Begin to create domain/cs." );

   var options = { AutoSplit: true };
   var groups = getDataGroupsName();
   db.createDomain( domainName, groups, options );
}

function createMainCL ( csName, mainCLName )
{
   println( "\n---Begin to create MainCL." );

   var options = { ShardingKey: { a: 1 }, IsMainCL: true };
   var mainCL = commCreateCL( db, csName, mainCLName, options, false,
      true, "Failed to create mainCL." );
   return mainCL;
}

function createSubCL ( csName, subCLName )
{
   println( "\n---Begin to create subCL." );

   var options = {
      ShardingKey: { a: 1 }, ShardingType: "hash",
      ReplSize: 0, Compressed: true
   };
   var subCL = commCreateCL( db, csName, subCLName, options, false,
      true, "Failed to create subCL." );
   return subCL;
}

function attachCL ( csName, mainCL, subCLName )
{
   println( "\n---Begin to attach CL." );

   var options = { LowBound: { "a": { $minKey: 1 } }, UpBound: { "a": { $maxKey: 1 } } };
   mainCL.attachCL( csName + "." + subCLName, options );
}

function insertRecs ( mainCL, insertRecsNum )
{
   println( "\n---Begin to insert records." );

   for( i = 0; i < insertRecsNum; i++ )
   {
      mainCL.insert( { a: i, b: i } );
   }
}

function checkResult ( csName, mainCL, subCLName, insertRecsNum )
{
   println( "\n---Begin to check result." );

   //get count of records in the mainCL
   var mainCLRecsCnt = mainCL.count();

   //get count of records in each group
   var groupRecsCnt = new Array;
   var groupNameArray = getDataGroupsName();
   for( i = 0; i < groupNameArray.length; i++ )
   {
      var tmpHost = db.getRG( groupNameArray[i] ).getMaster().toString();
      var dbRG = new Sdb( tmpHost );
      var eachGroupRecsCnt = dbRG.getCS( csName ).getCL( subCLName ).count();
      //The count of records not less than 1 in each group after AutoSplit
      if( parseInt( eachGroupRecsCnt ) < 1 )
      {
         throw buildException( "checkResult", null, "[compare count of records in each group after autoSplit]",
            "[eachGroupRecsCnt >= 1]",
            "[eachGroupRecsCnt:" + parseInt( eachGroupRecsCnt ) + "]" );
      }
      groupRecsCnt.push( eachGroupRecsCnt );
   }

   //get sum(groupRecsCnt)
   var sumCnt = 0;
   for( var j in groupRecsCnt )
   {
      sumCnt += groupRecsCnt[j];
   }

   //compare count of records
   if( parseInt( mainCLRecsCnt ) !== sumCnt || sumCnt !== insertRecsNum )
   {
      throw buildException( "checkResult", null, "[compare count of records]",
         "[mainCLRecsCnt:" + insertRecsNum + ",sumCnt:" + insertRecsNum + "]",
         "[mainCLRecsCnt:" + parseInt( mainCLRecsCnt ) + ",sumCnt:" + sumCnt + "]" );
   }

   // compare every records 
   for( k = 0; k < parseInt( mainCLRecsCnt ); k++ )
   {
      var everyRecsCnt = mainCL.find( { a: k } ).count();
      var expectCnt = 1;
      if( parseInt( everyRecsCnt ) !== expectCnt )
      {
         throw buildException( "checkResult", null, "[compare every records]",
            "[everyRecsCnt:" + expectCnt + "]",
            "[everyRecsCnt:" + parseInt( everyRecsCnt ) + "]" );
      }
   }
}