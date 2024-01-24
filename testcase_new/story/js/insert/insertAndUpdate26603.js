/******************************************************************************
 * @Description   : seqDB-26603 ：普通表指定更新规则单条插入冲突记录，待插记录有/无_id，
 *                                更新规则里有/无_id.
 * @Author        : Lin Yingting
 * @CreateTime    : 2022.06.09
 * @LastEditTime  : 2022.06.14
 * @LastEditors   : Lin Yingting
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_26603";

main( test );

function test ( testPara )
{
   //普通表创建唯一索引，插入记录
   var cl = testPara.testCL;
   cl.createIndex( "idx_a", { a: 1 }, true, true );
   cl.insert( { a: 1, b: 1 } );
   var expId = cl.find().current().toObj()._id;

   //普通表指定FLG_INSERT_UPDATEONDUP，待插记录里带_id
   cl.insert( { a: 1, b: 2, _id: "123" }, SDB_INSERT_UPDATEONDUP ); 
   var actRes = cl.find().current().toObj();
   var expRes = { a: 1, b: 2, _id: expId };
   if( !commCompareObject( expRes, actRes ) )
   {
      throw new Error( "\nExpected:\n" + JSON.stringify( expRes ) + "\nactual:\n" + JSON.stringify( actRes ) );
   }

   //普通表指定FLG_INSERT_UPDATEONDUP，待插记录里不带_id
   cl.insert( { a: 1, b: 3 }, SDB_INSERT_UPDATEONDUP );
   actRes = cl.find().current().toObj();
   expRes = { a: 1, b: 3, _id: expId };
   if( !commCompareObject( expRes, actRes ) )
   {
      throw new Error( "\nExpected:\n" + JSON.stringify( expRes ) + "\nactual:\n" + JSON.stringify( actRes ) );
   }

   //普通表指定UpdateOnDup和更新规则，更新规则里没有_id
   cl.insert( { a: 1, b: 3 }, { UpdateOnDup: true, Update: { "$inc": { "a": 1 } } } );
   actRes = cl.find().current().toObj();
   expRes = { a: 2, b: 3, _id: expId };
   if( !commCompareObject( expRes, actRes ) )
   {
      throw new Error( "\nExpected:\n" + JSON.stringify( expRes ) + "\nactual:\n" + JSON.stringify( actRes ) );
   }

   //普通表指定UpdateOnDup和更新规则，更新规则里有_id
   cl.insert( { a: 2, b: 3 }, { UpdateOnDup: true, Update: { "$set": { "a": 3, "_id": "123" } } } );
   actRes = cl.find().current().toObj();
   expRes = { a: 3, b: 3, _id: "123" };
   if( !commCompareObject( expRes, actRes ) )
   {
      throw new Error( "\nExpected:\n" + JSON.stringify( expRes ) + "\nactual:\n" + JSON.stringify( actRes ) );
   }
}
