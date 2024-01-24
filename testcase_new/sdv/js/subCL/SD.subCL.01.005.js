/************************************************************************
*@Description:	seqDB-818:主表和子表指定多个分区键字符类型进行切分,
               在主表做insert_SD.subCL.01.005
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

      var mainCSName = COMMCLNAME + "_maincs";
      var mainCLName = COMMCLNAME + "_maincl";
      var subCSName = COMMCLNAME + "_subcs";
      var subCLName = COMMCLNAME + "_subcl";
      var domainName = COMMCLNAME + "_domain";

      //create main&sub cl
      createDM( domainName );
      var maincl = createMainCL( mainCSName, mainCLName, domainName );
      var subcl = createSubCL( subCSName, subCLName, domainName );
      attachCL( maincl, subCSName, subCLName );

      //insert and check
      var istRecNum = 1000;
      insertRecs( maincl, istRecNum );
      checkResult( maincl, subcl, istRecNum );

      clean( mainCSName, subCSName );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
   }
}

function select2RG ()
{
   var dataRGInfo = commGetGroups( db );
   var rgsName = [];
   rgsName.push( dataRGInfo[0][0]["GroupName"] );
   rgsName.push( dataRGInfo[1][0]["GroupName"] );

   return rgsName;
}

function createDM ( domainName )
{
   dropDM( domainName, true, "drop domain in begin" );
   var dmGroups = select2RG();
   db.createDomain( domainName, dmGroups, { AutoSplit: true } );
}

function createMainCL ( mainCSName, mainCLName, domainName )
{
   println( "\n---Begin to create main cl" );

   commDropCS( db, mainCSName, true, "drop main cs in begin" );
   commCreateCS( db, mainCSName, false, "create main cs", { "Domain": domainName } );

   var option = { ShardingKey: { a: 1, b: 1 }, ShardingType: "range", IsMainCL: true };
   var cl = commCreateCL( db, mainCSName, mainCLName, option, true, false,
      "create mian cl" );
   return cl;
}

function createSubCL ( subCSName, subCLName, domainName )
{
   println( "\n---Begin to create sub cl" );

   commDropCS( db, subCSName, true, "drop main cs in begin" );
   commCreateCS( db, subCSName, false, "create main cs", { "Domain": domainName } );

   var option = { ShardingKey: { c: 1, d: 1 }, ShardingType: "hash" };
   var cl = commCreateCL( db, subCSName, subCLName, option,
      true, false, "create mian cl" );
   return cl;
}

function attachCL ( maincl, csName, clName )
{
   println( "\n---Begin to attach cl" );

   var option = {
      LowBound: { a: { $minKey: 1 }, b: { $minKey: 1 } },
      UpBound: { a: { $maxKey: 1 }, b: { $maxKey: 1 } }
   };
   maincl.attachCL( csName + "." + clName, option );
}

function insertRecs ( maincl, cnt )
{
   println( "\n---Begin to insert records" );

   for( i = 0; i < cnt; i++ )
   {
      maincl.insert( { a: i, b: i, c: i, d: i } );
   }
}

function checkResult ( maincl, subcl, totalCnt )
{
   println( "\n---Begin to check result after insert" );

   var mainclCnt = parseInt( maincl.count() );
   var subclCnt = parseInt( subcl.count() );

   //check maincl count 
   if( mainclCnt !== totalCnt )
   {
      throw buildException( "check maincl count", null, "maincl.count()",
         totalCnt, mainclCnt );
   }

   //compare maincl count to sum of all subcl count
   if( mainclCnt !== subclCnt )
   {
      println( "maincl count:" + mainclCnt + ", subcl count:" + subclCnt );
      throw buildException( "", null, "compare maincl count to sum of all subcl count",
         "equal", "no equal" );
   }

   //compare every records 
   var expCnt = 1;
   for( var k = 0; k < mainclCnt; k++ )
   {
      var fields = ['a', 'b', 'c', 'd'];
      for( var i in fields )
      {
         var field = fields[i];
         var matcher = {};
         matcher[field] = k;

         var cnt = maincl.find( matcher ).count();
         if( parseInt( cnt ) !== expCnt )
         {
            throw buildException( "compare every records", null,
               "maincl.find({a:" + k + "}).count()", expCnt, cnt );
         }
      }
   }
}

function clean ( mainCSName, subCSName )
{
   println( "\n---begin to clean" );

   commDropCS( db, mainCSName, false, "drop cs[" + mainCSName + "] in clean" );
   commDropCS( db, subCSName, false, "drop cs[" + subCSName + "] in clean" );
}

