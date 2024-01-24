/******************************************************************************
 * @Description   : seqDB-23728 : 记录dmsRecord头中有Ovf标记时会assert
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.23
 * @LastEditTime  : 2021.03.29
 * @LastEditors   : Yi Pan
 ******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_cl23728";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;

   //插入数据
   cl.insert( { "_id": { "$oid": "6048721debc199adadc80583" } } );

   //更新数据
   cl.update( { "$set": { "id": 0 } }, { "id": { "$isnull": 1 } } );

   //创建索引
   cl.createIndex( "PRIMARY", { "id": 1 }, { "Unique": true, "NotNull": true, "NotArray": true } );

   //查询结果
   cl.find( {}, { "id": null } ).sort( { "id": 1 } ).hint( { "": "PRIMARY" } );

}