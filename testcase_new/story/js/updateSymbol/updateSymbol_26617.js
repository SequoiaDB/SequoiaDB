/******************************************************************************
 * @Description   : seqDB-26617:更新符使用rename更新字段名
 * @Author        : liuli
 * @CreateTime    : 2022.06.13
 * @LastEditTime  : 2023.01.10
 * @LastEditors   : ChengJingjing
 ******************************************************************************/
 testConf.clName = COMMCLNAME + "26617";
 main( test );

 function test ( args )
 {
    var cl = args.testCL;

    var docs = [];
    var recsNum = 1000;
    for( var i = 0; i < recsNum; i++ )
    {
       docs.push( { num: i, user: { name: "user_" + i, score: { math: 88, en: 89, cn: 90 } } } );
    }
    cl.insert( docs );

    // 1.更新普通字段名
    // 1.1 更新为同名字段
    assert.tryThrow( [SDB_INVALIDARG], function()
    {
       cl.update( { $rename: { "num": "num" } } );
    } );

    // 1.2 更新为记录中已存在字段
    assert.tryThrow( [SDB_INVALIDARG], function()
    {
       cl.update( { $rename: { "num": "user" } } );
    } );

    // 2.更新一层嵌套对象字段名
    // 2.1 更新为同名字段
    assert.tryThrow( [SDB_INVALIDARG], function()
    {
       cl.update( { $rename: { "user.name": "name" } } );
    } );

    // 2.2 更新为记录中已存在字段
    assert.tryThrow( [SDB_INVALIDARG], function()
    {
       cl.update( { $rename: { "user.name": "score" } } );
    } );

    // 3.更新多层嵌套对象字段名
    // 3.1 更新为同名字段
    assert.tryThrow( [SDB_INVALIDARG], function()
    {
       cl.update( { $rename: { "user.score.cn": "cn" } } );
    } );

    // 3.2 更新为记录中已存在字段
    assert.tryThrow( [SDB_INVALIDARG], function()
    {
       cl.update( { $rename: { "user.score.cn": "en" } } );
    } );

    // 3.3 更新多个字段名,其中部分字段名为记录中已存在字段名
    assert.tryThrow( [SDB_INVALIDARG], function()
    {
       cl.update( { $rename: { "user.score.en": "cn", "user.score.cn": "chinese" } } );
    } );

    var cursor = cl.find().sort( { num: 1 } );
    commCompareResults( cursor, docs );
 }