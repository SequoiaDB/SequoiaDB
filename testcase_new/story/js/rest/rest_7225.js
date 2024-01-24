/****************************************************
@description:   split, abnormal case
                testlink cases: seqDB-7225
@input:         create a cl with [Partition:1024]
                split cl by [splitendquery={Partition:128}]
@expectation:   lackSplitquery(): expect: return -6
@modify list:
                2015-4-7 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "_7225";
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
   var opt = { ShardingKey: { age: 1 }, ShardingType: "hash", Partition: 1024, Group: sourceGroup };
   var varCL = commCreateCL( db, csName, clName, opt, true, false, "create cl in begin" );
   lackSplitquery();
   commDropCL( db, csName, clName, false, false, "drop cl in clean" );
}

function lackSplitquery ()
{
   var word = "split";
   tryCatch( ["cmd=" + word, "name=" + csName + '.' + clName, 'source=' + sourceGroup, 'target=' + targetGroup, 'splitendquery={Partition:128}'], [-6], "Error occurs in " + getFuncName() );
}



