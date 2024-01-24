/************************************
*@Description: 重复按不同的键值进行数据切分
*@author:     Qiangzhong Deng 2015/10/20
**************************************/

var clName = CHANGEDPREFIX + "_cl";

main( db );
function main ( db )
{
   if( commIsStandalone( db ) ) return;

   commDropCL( db, COMMCSNAME, clName );

   var objCL = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { no: 1, no2: 1 }, ShardingType: "range" } );

   //prepare 3 groups:the first one is source gorup,the other 2 are target groups
   var tarGroupNum = 2;
   var testGroups = getSplitGroups( COMMCSNAME, clName, tarGroupNum );
   if( 1 === testGroups.length )
   {
      println( "--least two groups" );
      return;
   }

   var insertNum = 1000;
   insertData( db, COMMCSNAME, clName, insertNum );

   //split with first ShardingKey:no
   splitCL( objCL, testGroups[0].GroupName, testGroups[1].GroupName, { no: 500 } );

   var lastGroup = testGroups.pop();
   checkRangeClSplitResult( clName, testGroups );

   //split with second ShardingKey:no2
   splitCL( objCL, testGroups[0].GroupName, lastGroup.GroupName, { no2: 100 } );
   var newGroups = new Array( lastGroup );
   checkRangeClSplitResult( clName, newGroups );

   commDropCL( db, COMMCSNAME, clName );
}
