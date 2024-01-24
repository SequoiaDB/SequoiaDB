/******************************************************************************
 * @Description   : seqDB-26322:listCollections接口验证
 * @Author        : ZhangYanan
 * @CreateTime    : 2022.04.02
 * @LastEditTime  : 2022.04.02
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/

main( test );
function test ( testPara )
{
   var csName1 = "cs_26322_1";
   var csName2 = "cs_26322_2";
   var csName3 = "cs_26322_3";
   var clName1 = "cl_26322_1";
   var clName2 = "cl_26322_2";
   var clName3 = "cl_26322_3";

   //  清理集合空间
   commDropCS( db, csName1, true, "clear cs in the beginning." );
   commDropCS( db, csName2, true, "clear cs in the beginning." );
   commDropCS( db, csName3, true, "clear cs in the beginning." );

   var cs1 = db.createCS( csName1 );
   var cs2 = db.createCS( csName2 );
   db.createCS( csName3 );

   cs1.createCL( clName1 );
   cs2.createCL( clName2 );
   cs2.createCL( clName3 );

   tryCatch( ["cmd=list collections in collectionspace", "name=" + csName1], [0] );
   assert.equal( JSON.parse( infoSplit[1] ).Name, csName1 + "." + clName1, infoSplit );

   tryCatch( ["cmd=list collections in collectionspace", "name=" + csName2], [0] );
   assert.equal( JSON.parse( infoSplit[1] ).Name, csName2 + "." + clName2, infoSplit );
   assert.equal( JSON.parse( infoSplit[2] ).Name, csName2 + "." + clName3, infoSplit );

   tryCatch( ["cmd=list collections in collectionspace", "name=" + csName3], [0] );
   assert.equal( infoSplit.length, 1, infoSplit );

   commDropCS( db, csName1, false, "clear cs in the beginning." );
   commDropCS( db, csName2, false, "clear cs in the beginning." );
   commDropCS( db, csName3, false, "clear cs in the beginning." );
}