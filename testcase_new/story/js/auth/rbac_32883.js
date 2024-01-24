/******************************************************************************
 * @Description   : seqDB-32883:createRole接口参数校验
 * @Author        : tangtao
 * @CreateTime    : 2023.08.31
 * @LastEditTime  : 2023.08.31
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dbname = "db_32883";
   var roleName1 = "role_32883_1";
   var roleName2 = "role_32883_2";

   cleanRole( roleName1 );
   cleanRole( roleName2 );

   try
   {
      // 1、createRole指定role为合法参数：符合格式的object类型
      var role = {
         Role: roleName1, Privileges: [{
            Resource: { cs: dbname, cl: "" },
            Actions: ["find"]
         }]
      };
      db.createRole( role );

      // 2、createRole指定role为非法参数： a、非object类型 b、不包含role.Role
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.createRole( roleName1 );
      } );

      var role = { Privileges: [{ Resource: { cs: dbname, cl: "" }, Actions: ["find"] }] };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.createRole( role );
      } );

      // 3、createRole指定role.Role为合法参数：string类型
      var role = {
         Role: roleName2, Privileges: [{
            Resource: { cs: dbname, cl: "" },
            Actions: ["find"]
         }]
      };
      db.createRole( role );
      db.dropRole( roleName2 );

      // 4、createRole指定role.Role为非法参数： a、超长字符，超过16M b、非string类型，如：1、true、array
      var role = {
         Role: 100, Privileges: [{
            Resource: { cs: dbname, cl: "" },
            Actions: ["find"]
         }]
      };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.createRole( role );
      } );

      var role = {
         Role: true, Privileges: [{
            Resource: { cs: dbname, cl: "" },
            Actions: ["find"]
         }]
      };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.createRole( role );
      } );

      var role = {
         Role: [1, 2, 3], Privileges: [{
            Resource: { cs: dbname, cl: "" },
            Actions: ["find"]
         }]
      };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.createRole( role );
      } );

      // 5、createRole指定role.Privileges为合法参数：符合格式的array类型
      var role = {
         Role: roleName2, Privileges: [{
            Resource: { cs: dbname, cl: "" },
            Actions: ["find"]
         }]
      };
      db.createRole( role );
      db.dropRole( roleName2 );

      // 6、createRole指定role.Privileges为非法参数： a、不包含Resource字段 b、不包含Actions字段 d、Resource指定内容不正确
      // e、Actions指定内容不正确 f、role.Privileges指定非object类型
      var role = { Role: roleName2, Privileges: [{ Resource: { cs: dbname, cl: "" } }] };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.createRole( role );
      } );

      var role = { Role: roleName2, Privileges: [{ Actions: ["find"] }] };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.createRole( role );
      } );

      var role = { Role: roleName2, Privileges: [{ Resource: 100, Actions: ["find"] }] };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.createRole( role );
      } );

      var role = {
         Role: roleName2, Privileges: [{
            Resource: { cs: dbname, cl: "" },
            Actions: ["test_action"]
         }]
      };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.createRole( role );
      } );

      var role = { Role: roleName2, Privileges: [100] };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.createRole( role );
      } );

      // 7、createRole指定role.Roles为合法参数：符合格式的array类型
      var role = {
         Role: roleName2, Privileges: [{
            Resource: { cs: dbname, cl: "" },
            Actions: ["find"]
         }], Roles: [roleName1]
      };
      db.createRole( role );
      db.dropRole( roleName2 );

      //8、createRole指定role.Roles为非法参数： a、非array类型，如：1、true、"string"
      var role = {
         Role: roleName2, Privileges: [{
            Resource: { cs: dbname, cl: "" },
            Actions: ["find"]
         }], Roles: { a: 1 }
      };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.createRole( role );
      } );

      var role = {
         Role: roleName2, Privileges: [{
            Resource: { cs: dbname, cl: "" },
            Actions: ["find"]
         }], Roles: true
      };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.createRole( role );
      } );

      var role = {
         Role: roleName2, Privileges: [{
            Resource: { cs: dbname, cl: "" },
            Actions: ["find"]
         }], Roles: "string"
      };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.createRole( role );
      } );

   } finally
   {
      cleanRole( roleName1 );
      cleanRole( roleName2 );
   }
}

