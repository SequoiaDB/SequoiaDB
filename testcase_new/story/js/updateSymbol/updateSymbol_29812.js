/******************************************************************************
 * @Description   : seqDB-29812:update使用rename更新字段名为不存在的字段名
 * @Author        : ChengJingjing
 * @CreateTime    : 2022.01.10
 * @LastEditTime  : 2023.02.25
 * @LastEditors   : ChengJingjing
 ******************************************************************************/
testConf.clName = COMMCLNAME + "29812";
testConf.clOpt = { ReplSize: 0 };
main( test );

function test ( args )
{
   var cl = args.testCL;

   var docs = [];
   var recsNum = 1000;
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( {
         num: i, user: {
            name: "user_" + i,
            score: { "avg": 89, detail: [{ math: 88 }, { en: 89, cn: 90 }] }
         }
      } );
   }
   cl.insert( docs );

   // 更新普通字段名
   cl.update( { $rename: { "num": "usernum" } } );
   var expResult = [];
   for( var i = 0; i < recsNum; i++ )
   {
      expResult.push( {
         usernum: i, user: {
            name: "user_" + i,
            score: { "avg": 89, detail: [{ math: 88 }, { en: 89, cn: 90 }] }
         }
      } );
   }
   checkUpdatedCL( cl, expResult );

   // 更新一层嵌套对象字段名
   cl.update( { $rename: { "user.name": "username" } } );
   var expResult = [];
   for( var i = 0; i < recsNum; i++ )
   {
      expResult.push( {
         usernum: i, user: {
            username: "user_" + i,
            score: { "avg": 89, detail: [{ math: 88 }, { en: 89, cn: 90 }] }
         }
      } );
   }
   checkUpdatedCL( cl, expResult );

   // 更新多层嵌套对象字段名
   cl.update( { $rename: { "user.score.avg": "average" } } );
   var expResult = [];
   for( var i = 0; i < recsNum; i++ )
   {
      expResult.push( {
         usernum: i, user: {
            username: "user_" + i,
            score: { "average": 89, detail: [{ math: 88 }, { en: 89, cn: 90 }] }
         }
      } );
   }
   checkUpdatedCL( cl, expResult );

   // 更新嵌套数组字段名
   cl.update( { $rename: { "user.score.detail.1.en": "english" } } );
   var expResult = [];
   for( var i = 0; i < recsNum; i++ )
   {
      expResult.push( {
         usernum: i, user: {
            username: "user_" + i,
            score: { "average": 89, detail: [{ math: 88 }, { english: 89, cn: 90 }] }
         }
      } );
   }
   checkUpdatedCL( cl, expResult );

   // 更新多个字段
   cl.update( { $rename: { "user.score.detail.1.en": "english", "user.score.detail.1.cn": "chinese" } } );
   var expResult = [];
   for( var i = 0; i < recsNum; i++ )
   {
      expResult.push( {
         usernum: i, user: {
            username: "user_" + i,
            score: { "average": 89, detail: [{ math: 88 }, { english: 89, chinese: 90 }] }
         }
      } );
   }
   checkUpdatedCL( cl, expResult );

   // 更新符合条件的记录
   cl.update( { $rename: { "usernum": "num" } }, { "usernum": 1 } );
   var expResult = [];
   expResult.push( {
      num: 1, user: {
         username: "user_" + 1,
         score: { "average": 89, detail: [{ math: 88 }, { english: 89, chinese: 90 }] }
      }
   } );
   db.setSessionAttr( { PreferedInstance: "m" } );
   var cursor = cl.find( { "num": 1 } );
   commCompareResults( cursor, expResult );
   db.setSessionAttr( { PreferedInstance: "s" } );
   cursor = cl.find( { "num": 1 } );
   commCompareResults( cursor, expResult );
}

function checkUpdatedCL ( cl, expResult )
{
   db.setSessionAttr( { PreferedInstance: "m" } );
   var cursor = cl.find().sort( { usernum: 1 } );
   commCompareResults( cursor, expResult );
   db.setSessionAttr( { PreferedInstance: "s" } );
   cursor = cl.find().sort( { usernum: 1 } );
   commCompareResults( cursor, expResult );
}