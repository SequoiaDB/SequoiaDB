/******************************************************************************
 * @Description   : seqDB-30334:分区数大于复制组数，集合切分之后查询数据
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.03.02
 * @LastEditTime  : 2023.03.06
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.clName = COMMCLNAME + "_30334";
testConf.clOpt = { ShardingKey: { a: 1 } };

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;

   // 目标组与源组来回切分，使分区数大于复制组
   dbcl.split( testPara.srcGroupName, testPara.dstGroupNames[0], 50 );
   dbcl.split( testPara.dstGroupNames[0], testPara.srcGroupName, 50 );

   var docs = [];
   for( var i = 0; i < 10000; i++ )
   {
      docs.push( { a: i } );
   }
   dbcl.insert( docs );

   var numbers = [];
   var expRecs = [];
   while( numbers.length < 20 )
   {
      var randomNumber = Math.floor( Math.random() * 10000 ) + 1;
      if( numbers.indexOf( randomNumber ) === -1 )
      {
         expRecs.push( { "a": randomNumber } );
         numbers.push( randomNumber );
      }
   }
   expRecs.sort( sortBy( 'a' ) );
   var actRecs = dbcl.find( { a: { "$in": numbers } } ).sort( { a: 1 } );
   commCompareResults( actRecs, expRecs );
}

function sortBy ( field )
{
   return function( a, b )
   {
      return a[field] > b[field];
   }
}