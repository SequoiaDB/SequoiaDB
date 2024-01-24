/******************************************************************************
 * @Description   : seqDB-24642:创建索引指定多个索引字段，更新多于7个字段的索引
 * @Author        : Wu Yan
 * @CreateTime    : 2021.11.17
 * @LastEditTime  : 2021.11.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24642";
main( test );

function test ( testPara )
{
   var indexNameA = "index24642a";
   var indexNameB = "index24642b";
   var cl = testPara.testCL;
   // 创建索引
   cl.createIndex( indexNameA, { testa: 1, testb: 1, testc: 1, testd: 1, teste: 1, testf: 1, testg: 1, testh: 1 } );
   cl.insert( { testa: 1 }, { testa: 2 } );

   //更新多于7个的索引字段,如更新testh字段
   var updateValue1 = "testupdateindexh";
   cl.update( { $set: { "testh": updateValue1 } } );
   cl.createIndex( indexNameB, { "testk": 1 } );

   //再次更新多于7个字段的索引，如更新testh字段
   var updateValue2 = "testupdateindexnewh";
   cl.update( { $set: { "testh": updateValue2 } } );

   //指定索引查询更新字段
   var expRecsA = [];
   var cursorA = cl.find( { "testa": 1, "testh": updateValue1 }, { "_id": { "$include": 0 } } ).hint( { "": indexNameA } );
   commCompareResults( cursorA, expRecsA );
   var expRecsB = [{ "testa": 1, "testh": updateValue2 }];
   var cursorB = cl.find( { "testa": 1, "testh": updateValue2 }, { "_id": { "$include": 0 } } ).hint( { "": indexNameA } );
   commCompareResults( cursorB, expRecsB );
}


