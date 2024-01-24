/************************************************************************
*@Description:  	seqDB-820:主表和子表属于不同的CS,在主表做aggregate_SD.subCL.01.007
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
      var mainCSName = COMMCLNAME + "_mcs";
      var subCSName = COMMCLNAME + "_scs";
      var mainCLName = COMMCLNAME + "_mcl";
      var subCLName = COMMCLNAME + "_scl";

      println( "\n---Begin to drop cs/domain in the pre-condition." );
      commDropCS( db, subCSName, true, "Failed to drop subCS." );
      commDropCS( db, mainCSName, true, "Failed to drop mainCS." );
      dropDM( domainName, true, "Failed to drop domain." );

      createDM( domainName );
      commCreateCS( db, mainCSName, false, "Failed to create mainCS.", { Domain: domainName } );
      commCreateCS( db, subCSName, false, "Failed to create subCS.", { Domain: domainName } );

      var mainCL = createMainCL( mainCSName, mainCLName );
      createSubCL( subCSName, subCLName );
      attachCL( mainCL, subCSName, subCLName );

      var insertRecsNum = 100;
      insertRecs( mainCL, insertRecsNum );
      var aggre = aggregateRecs( mainCL );
      checkResult( mainCL, subCSName, subCLName, aggre );

      println( "\n---Begin to drop cs/domain in the end-condition." );
      commDropCS( db, subCSName, false, "Failed to drop subCS." );
      commDropCS( db, mainCSName, false, "Failed to drop mainCS." );
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

   var groupArray = getDataGroupsName();
   var options = {
      ShardingKey: { a: 1 }, ShardingType: "hash", Group: groupArray[0],
      ReplSize: 0, Compressed: true
   };
   var subCL = commCreateCL( db, csName, subCLName, options, false,
      true, "Failed to create subCL." );
   return subCL;
}

function attachCL ( mainCL, csName, subCLName )
{
   println( "\n---Begin to attach CL." );

   var options = { LowBound: { "a": -100 }, UpBound: { "a": 100 } };
   mainCL.attachCL( csName + "." + subCLName, options );
}

function insertRecs ( mainCL )
{
   println( "\n---Begin to insert records." );

   mainCL.insert( { a: 4, b: "test" } );
   mainCL.insert( { a: 1, b: "test" } );
   mainCL.insert( { a: 3, b: "hello", c: 0 } );
   mainCL.insert( { a: 2, b: "hello", c: 1 } );
   mainCL.insert( { a: 0, c: 2 } );
}

function aggregateRecs ( mainCL )
{
   println( "\n---Begin to upsert records." );

   var aggre = mainCL.aggregate( { $project: { a: 1, b: 1 } }, { $sort: { a: 1 } }, { $skip: 2 }, { $group: { _id: "$b" } }, { $match: { a: { $lt: 4 } } } );

   return aggre;
}

function checkResult ( mainCL, csName, subCLName, aggre )
{
   println( "\n---Begin to check result." );

   //get the records count in the mainCL
   var mainCLRecsCnt = mainCL.count();

   //get the records count in the data group
   var groupRecsCnt = new Array;
   var groupNameArray = getDataGroupsName();
   var tmpHost = db.getRG( groupNameArray[0] ).getMaster().toString();
   var dbRG = new Sdb( tmpHost );
   var groupRecsCnt = dbRG.getCS( csName ).getCL( subCLName ).count();

   //compare count of records
   var insertRecsNum = 5;   //Number of records inserted.
   if( parseInt( mainCLRecsCnt ) !== parseInt( groupRecsCnt ) || parseInt( groupRecsCnt ) !== insertRecsNum )
   {
      throw buildException( "checkResult", null, "[compare count of records]",
         "[mainCLRecsCnt:" + insertRecsNum + ",sumCnt:" + insertRecsNum + "]",
         "[mainCLRecsCnt:" + parseInt( mainCLRecsCnt ) + ",groupRecsCnt:" + parseInt( groupRecsCnt ) + "]" );
   }

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

   //compare aggregate records.  Expected results: {a:2, b: "hello"}   
   var expValue1 = 2;           //The expected value of field a.
   var expValue2 = "hello";     //The expected value of field b.
   var expValue3 = undefined;   //The expected value of field c.
   var a = aggre.current().toObj()["a"];
   var b = aggre.current().toObj()["b"];
   var c = aggre.current().toObj()["c"];
   if( a !== expValue1 || b !== expValue2 || c !== expValue3 ) 
   {
      throw buildException( "checkResult", null, "[compare aggregate records]",
         "[a:" + expValue1 + ",b:" + expValue2 + ",c:" + expValue3 + "]",
         "[a:" + a + ",b:" + b + ",c:" + c + "]" );
   }
}