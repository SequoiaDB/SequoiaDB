/******************************************************************************
 * @Description   : seqDB-33898:大记录的集合作为源执行插入数据
 * @Author        : tangtao
 * @CreateTime    : 2023.11.13
 * @LastEditTime  : 2023.11.13
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.csName = COMMCSNAME + "_qgm_33898";

main( test );
function test ( testPara )
{
   var csName = testConf.csName;
   var clName1 = COMMCLNAME + "_qgm_33898_1";
   var clName2 = COMMCLNAME + "_qgm_33898_2";

   var cs = testPara.testCS;
   cl1 = cs.createCL( clName1 );
   cl2 = cs.createCL( clName2 );
   var array = new Array( 1000000 );
   var string = array.join( 'a' );
   var recNum = 100;
   data = [];
   for( i = 0; i < recNum; i++ )
      data.push( { a: i, b: string } );
   cl1.insert( data );
   db.execUpdate( 'insert into ' + csName + '.' + clName2 + " select * from " +
      csName + '.' + clName1 );

   var count2 = cl2.count();
   assert.equal( count2, recNum );
}