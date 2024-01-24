/***************************************************************************************************
 * @Description: Insert flag SDB_INSERT_CONTONDUP_ID 测试
 * @ATCaseID: insert_flag_at_1
 * @Author: Liu Yuchen
 * @TestlinkCase: 无
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 02/14/2023 Liu Yuchen    Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：任意正常集群
 * 测试场景：
 *    单条记录插入场景下，测试insert flag:SDB_INSERT_CONTONDUP_ID 及 insert option：ContOnDupID的行为是否符合预期。
 * 测试步骤：
 *    1. 创建cs和cl，cl默认创建'$id'索引
 *    2. cl为'a'字段创建唯一索引'aIdx'
 *    3. 向cl中插入一条记录rec
 *    4. 设置 insert flag: SDB_INSERT_CONTONDUP_ID，向cl中插入一条与rec有相同'_id'字段和'a'字段的记录，校验返回结果
 *    5. 设置 insert option: {ContOnDupID:true}，向cl中插入一条与rec有相同'_id'字段和'a'字段的记录，校验返回结果
 *    6. 设置 insert option: {ContOnDupID:false}，向cl中插入一条与rec有相同'_id'字段和'a'字段的记录，校验返回结果
 *    7. 设置 insert flag: SDB_INSERT_CONTONDUP_ID，向cl中插入一条与rec有不同'_id'字段和相同'a'字段的记录，校验返回结果
 *    8. 同时设置两个insert option，例如：{ContOnDupID:true，ContOnDup:true}, 插入任意记录，校验返回结果
 *    9. 删除cs
 * 期望结果：
 *    1. 步骤4执行不抛异常，返回结果DuplicatedNum为1
 *    2. 步骤5执行不抛异常，返回结果DuplicatedNum为1
 *    3. 步骤6执行报错-38，id索引冲突
 *    4. 步骤7执行报错-38，aIdx索引冲突
 *    5. 步骤8执行报错-6，ContOnDupID和ContOnDup，ContOnDupID和ReplaceOnDup，ContOnDupID和ReplaceOnDupID，均不能重复设置
 *    
 **************************************************************************************************/

 main(test);
 function test() {
   var csName = "insert_flag_at_1_cs";
   var clName = "insert_flag_at_1_cl";

   commDropCS( db, csName, true, "init env" );

   var cl = db.createCS( csName ).createCL( clName );

   cl.createIndex( "aIdx", {a:1}, true );
   cl.insert( { "_id":1, "a": 1 } );

   var record =  { "_id":1, "a":1, "b":1 };
   var res = cl.insert( record, SDB_INSERT_CONTONDUP_ID ).toObj();
   var exp = { "InsertedNum": 0, "DuplicatedNum":1, "ModifiedNum":0 };
   assert.equal( res, exp );

   record = { "_id":1, "a":1, "b":1 };
   res = cl.insert( record, { ContOnDupID:true } ).toObj();
   exp = { "InsertedNum": 0, "DuplicatedNum":1, "ModifiedNum":0 };
   assert.equal( res, exp );

   record = { "_id":1, "a":1, "b":1 };
   assert.tryThrow( SDB_IXM_DUP_KEY, function () {
     res = cl.insert( record, { ContOnDupID:false } );
   });

   record = { "_id":666, "a":1, "b":1 };
   assert.tryThrow( SDB_IXM_DUP_KEY, function () {
     res = cl.insert( record, SDB_INSERT_CONTONDUP_ID );
   });

   assert.tryThrow( SDB_INVALIDARG, function () {
     res = cl.insert( record, { ContOnDupID:true , ContOnDup:true} );
   });

   assert.tryThrow( SDB_INVALIDARG, function () {
     res = cl.insert( record, { ReplaceOnDup:true, ContOnDupID:true } );
   });

   assert.tryThrow( SDB_INVALIDARG, function () {
    res = cl.insert( record, { ContOnDupID:true , ReplaceOnDupID:true } );
   });

   commDropCS( db, csName, true, "drop env" );
 }