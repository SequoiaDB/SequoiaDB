/***************************************************************************************************
 * @Description: Insert flag SDB_INSERT_REPLACEONDUP_ID 测试
 * @ATCaseID: insert_flag_at_3
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
 *    单条记录插入场景下，测试insert flag:SDB_INSERT_REPLACEONDUP_ID 及 insert option：ReplaceOnDupID的行为是否符合预期。
 * 测试步骤：
 *    1. 创建cs和cl，cl默认创建'$id'索引
 *    2. cl为'a'字段创建唯一索引'aIdx'
 *    3. 向cl中插入一条记录rec
 *    4. 设置 insert flag: SDB_INSERT_REPLACEONDUP_ID，向cl中插入一条与rec有相同'_id'字段和'a'字段的记录，校验返回结果
 *    5. 设置 insert option: {ReplaceOnDupID:true}，向cl中插入一条与rec有相同'_id'字段和'a'字段的记录，校验返回结果
 *    6. 设置 insert option: {ReplaceOnDupID:false}，向cl中插入一条与rec有相同'_id'字段和'a'字段的记录，校验返回结果
 *    7. 设置 insert flag: SDB_INSERT_REPLACEONDUP_ID，向cl中插入一条与rec有不同'_id'字段和相同'a'字段的记录，校验返回结果
 *    8. 删除cs
 * 期望结果：
 *    1. 步骤4执行不抛异常，返回结果DuplicatedNum为1, ModifiedNum为1，校验查询结果与所插记录相同。
 *    2. 步骤5执行不抛异常，返回结果DuplicatedNum为1, ModifiedNum为1，校验查询结果与所插记录相同。
 *    3. 步骤6执行报错-38，id索引冲突
 *    4. 步骤7执行报错-38，aIdx索引冲突
 *    
 **************************************************************************************************/

 main(test);
 function test() {
   var csName = "insert_flag_at_3_cs";
   var clName = "insert_flag_at_3_cl";

   commDropCS( db, csName, true, "init env" );

   var cl = db.createCS( csName ).createCL( clName );

   cl.createIndex( "aIdx", {a:1}, true );
   cl.insert( { "_id":1, "a": 1 } )

   var record = { "_id":1, "a":1, "b":1 };
   var res = cl.insert( record, SDB_INSERT_REPLACEONDUP_ID ).toObj();
   var exp = { "InsertedNum": 0, "DuplicatedNum":1, "ModifiedNum":1 };
   assert.equal( res, exp );
   var actRecord = JSON.parse( cl.find().current() );
   assert.equal( actRecord, record );

   record = { "_id":1, "a":1, "b":1, "c":1 };
   res = cl.insert( record, { ReplaceOnDupID:true } ).toObj();
   exp = { "InsertedNum": 0, "DuplicatedNum":1, "ModifiedNum":1 };
   assert.equal( res, exp );
   actRecord = JSON.parse( cl.find().current() );
   assert.equal( actRecord, record );

   record = { "_id":1, "a":1, "b":1, "c":1 };
   assert.tryThrow( SDB_IXM_DUP_KEY, function () {
     res = cl.insert( record, { ReplaceOnDupID:false } );
   });

   assert.tryThrow( SDB_IXM_DUP_KEY, function () {
     res = cl.insert( { "_id":666, "a":1, "b":1 }, SDB_INSERT_REPLACEONDUP_ID );
   });

   commDropCS( db, csName, true, "drop env" );
 }