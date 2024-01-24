/****************************************************
@description:   split, normal case
                testlink cases: seqDB-7220
@input:         insert a rec: {age:1}, split by [splitpercent=50]
@expectation:   sourceGroup: UpBound:{"age":1}
                targetGroup: LowBound:{"age":1} 
@modify list:
                2015-4-3 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "_7220";
var sourceGroup;
var targetGroup;

main( test );

function test ()
{
   var groupInfos = commGetGroups( db );
   if( commIsStandalone( db ) || groupInfos.length < 2 )
   {
      return;
   }
   commDropCL( db, csName, clName, true, true, "drop cl in begin" );
   sourceGroup = groupInfos[0][0].GroupName;
   targetGroup = groupInfos[1][0].GroupName;
   var opt = { ShardingKey: { age: 1 }, ShardingType: "range", Group: sourceGroup };
   var varCL = commCreateCL( db, csName, clName, opt, true, false, "create cl in begin" );
   varCL.insert( { age: 1 } );
   splitAndCheck();
   commDropCL( db, csName, clName, false, true, "drop cl in clean" );
}

function splitAndCheck ()
{
   var word = "split";
   tryCatch( ["cmd=" + word, "name=" + csName + '.' + clName, 'source=' + sourceGroup, 'target=' + targetGroup, 'splitpercent=50'], [0], "splitAndCheck: fail to run rest cmd=" + word );

   var obj = db.snapshot( 8, { Name: csName + '.' + clName } ).current().toObj().CataInfo;
   for( var i = 0; i < obj.length; i++ )
   {
      switch( obj[i].GroupName )
      {
         case targetGroup:
            for( var p in obj[i]["LowBound"] )
            {
               if( obj[i]["LowBound"][p] != 1 ) { throw new Error( "Fail to split by rest cmd=" + word + ",\ntargetGroup LowBound expect: 1, actual: " + obj[i]["LowBound"][p] ); }
               break;
            }
            break;
         case sourceGroup:
            for( var p in obj[i]["UpBound"] )
            {
               if( obj[i]["UpBound"][p] != 1 ) { throw new Error( "Fail to split by rest cmd=" + word + ",\nsourceGroup UpBound expect: 1, actual: " + obj[i]["UpBound"][p] ); }
               break;
            }
            break;
         default:
            throw new Error( "cl[" + csName + '.' + clName + "] is in group[" + obj[i].GroupName + "],\nexpect: it is in sourceGroup or targeGroup, actual: targeGroup[" + targetGroup + "] sourceGroup[" + sourceGroup + "]" );
      }
   }
}




