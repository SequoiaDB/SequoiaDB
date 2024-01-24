/***************************************************************************************************
 * @Description: Insert flag SDB_INSERT_REPLACEONDUP_ID 测试
 * @ATCaseID: insert_flag_at_4
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
 *    批量记录插入场景下，测试insert flag:SDB_INSERT_REPLACEONDUP_ID 及 insert option：ReplaceOnDupID的行为是否符合预期。
 * 测试步骤：
 *    1. 创建cs和cl，cl默认创建'$id'索引
 *    2. cl为'a'字段创建唯一索引'aIdx'
 *    3. 向cl中插入一条记录rec
 *    4. 设置 insert flag: SDB_INSERT_REPLACEONDUP_ID，向cl中插入三条记录，校验返回结果
 *       （三条记录中的第三条与rec有相同'_id'字段和'a'字段的记录，其余记录没有冲突字段）
 *    5. 清空cl
 *    6. 向cl中插入记录rec
 *    7. 设置 insert flag: SDB_INSERT_REPLACEONDUP_ID，向cl中插入三条记录，校验返回结果
 *       （三条记录中的第三条与rec有相同'a'字段的记录，其余记录没有冲突字段）
 *    8. 清空cl
 *    9. 向cl中插入记录rec
 *    10. 设置 insert option: {ReplaceOnDupID:true}，向cl中插入三条记录，校验返回结果
 *       （第一条记录与rec的'_id'字段相同；第二条记录与rec'_id'字段不同，但和第一条记录的'a'字段相同；第三条没有冲突字段）
 *    11. 清空cl
 *    12. 删除cs
 * 期望结果：
 *    1. 步骤4中的前两条记录未见冲突正常插入，第三条记录因'_id'字段重复被覆盖，
 *       返回结果InsertedNum为2，DuplicatedNum为1，ModifiedNum为1，总记录数为3
 *    2. 步骤7中的前两条记录未见冲突正常插入，第三条记录因'a'字段重复而报错-38，
 *       插入的记录数为2，总记录数为3
 *    3. 步骤10的第一条记录因'_id'重复而替换了原有的rec记录，替换后第二条记录因'a'字段与第一条记录重复而报错-38，
 *       插入的记录数为0，总记录数为1，rec记录被替换为插入的第一条记录
 *    
 **************************************************************************************************/

 main(test);
 function test() {
   var csName = "insert_flag_at_2_cs";
   var clName = "insert_flag_at_2_cl";

   commDropCS( db, csName, true, "init env" );

   var cl = db.createCS( csName ).createCL( clName );

   cl.createIndex( "aIdx", {a:1}, true );
   var rec = { "_id":1, "a": 1 };
   cl.insert( rec );

   var r1 = { "_id":11, "a":11 };
   var r2 = { "_id":22, "a":22, "b":22 };
   var r3 = { "_id":1, "a":1, "b":33 };
   var data = [];
   data.push( r1 );
   data.push( r2 );
   data.push( r3 );
   var res = cl.insert( data, SDB_INSERT_REPLACEONDUP_ID ).toObj();
   var exp = { "InsertedNum": 2, "DuplicatedNum":1, "ModifiedNum":1 };
   assert.equal( res, exp );
   var recordCount = cl.find().count();
   assert.equal( recordCount, 3 );

   cl.truncate();

   cl.insert( rec )

   r1 = { "_id":11, "a":11 };
   r2 = { "_id":22, "a":22, "b":22 };
   r3 = { "_id":33, "a":1, "b":33 }
   data = [];
   data.push( r1 );
   data.push( r2 );
   data.push( r3 );
   assert.tryThrow( SDB_IXM_DUP_KEY, function () {
      res = cl.insert( data, SDB_INSERT_REPLACEONDUP_ID );
   });

   recordCount = cl.find().count();
   assert.equal( recordCount, 3 );

   cl.truncate();

   cl.insert( rec )

   r1 = { "_id":1, "a":11 };
   r2 = { "_id":22, "a":11, "b":22 };
   r3 = { "_id":33, "a":33, "b":33 }
   data = [];
   data.push( r1 );
   data.push( r2 );
   data.push( r3 );
   assert.tryThrow( SDB_IXM_DUP_KEY, function () {
      res = cl.insert( data, {ReplaceOnDupID:true} );
   });
   recordCount = cl.find().count();
   assert.equal( recordCount, 1 );

   var actRecord = JSON.parse( cl.find({_id:1}).current() );
   assert.notEqual( r1, rec );
   assert.equal( actRecord, r1 );

   commDropCS( db, csName, true, "drop env" );
 }