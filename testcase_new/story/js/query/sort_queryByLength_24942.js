/******************************************************************************
@Description : seqDB-24942:使用find查询，cursor.next()遍历从短到长的大量记录，不发生指针泄露
@Author      : 钟子明
@CreateTime  : 2022.1.11
 * @LastEditTime  : 2022.01.17
 * @LastEditors   : 钟子明
******************************************************************************/
testConf.skipStandAlone = true;
testConf.csName = COMMCSNAME + "_24942";
testConf.clName = COMMCLNAME + "_24942";
testConf.clOpt = { IsMainCL: true, ShardingKey: { a: 1 } };

main( test );

function test ( testPara ) 
{
   var groupName = commGetDataGroupNames( db )[0];

   var cs = testPara.testCS;
   var cl = testPara.testCL;
   for( i = 0; i < 2; i++ ) 
   {
      cs.createCL( 'newcl' + i, { Group: groupName } );
      cl.attachCL( testConf.csName + '.newcl' + i, { LowBound: { a: i }, UpBound: { a: i + 1 } } );
   }
   var data = [];
   for( j = 0; j < 10; j++ )
   {
      for( i = 0; i < 10000; i++ ) 
      {
         data.push( { a: i % 2, b: i + ( j * 10000 ), c: i + ( j * 10000 ) } );
      }
      cl.insert( data );
      data = [];
   }

   var largeRow = new Array( 10 * 1024 * 1024 ).join( "a" );
   data.push( { a: 0, b: 200000, c: largeRow } );
   largeRow = new Array( 10 * 1024 * 1024 ).join( "a" );
   data.push( { a: 1, b: 200000, c: largeRow } );
   cl.insert( data );

   var dataCursor = cl.find( {}, {} ).sort( { b: 1 } );

   var count = 0;
   while( dataCursor.next() )
   {
      count++;
   }
   if( count != 100002 )
   {
      throw new Error( 'loop count is different,expect 100002,actual ' + count );
   }
   dataCursor.close();
}