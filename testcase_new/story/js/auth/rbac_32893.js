/******************************************************************************
 * @Description   : seqDB-32893:getUser接口参数校验
 * @Author        : tangtao
 * @CreateTime    : 2023.09.01
 * @LastEditTime  : 2023.09.01
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dbname = "db_32893";
   var roleName1 = "role_32893_1";
   var roleName2 = "role_32893_2";
   var userName = "user_32893";
   var passwd = "passwd_32893";

   cleanRole( roleName1 );
   cleanRole( roleName2 );

   var role = {
      Role: roleName1, Privileges: [{
         Resource: { cs: dbname, cl: "" },
         Actions: ["find"]
      }]
   };
   db.createRole( role );
   var role = {
      Role: roleName2, Privileges: [{
         Resource: { cs: dbname, cl: "" },
         Actions: ["insert", "update", "remove"]
      }]
   };
   db.createRole( role );

   db.createUsr( userName, passwd, { Roles: [roleName1] } );


   try
   {
      //1、getUser指定username为合法参数：string类型
      db.getUser( userName );

      //2、getUser指定username为非法参数：a、非string类型，如：1、true、array
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.getUser( 1 );
      } );

      //3、getUser指定options为合法参数：符合预期的object类型
      db.getUser( userName, { ShowPrivileges: true } );

      //4、getUser指定options为非法参数：a、非object类型，如："string"、1、true
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.getUser( userName, "string" );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.getUser( userName, 1 );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.getUser( userName, true );
      } );

      //5、getUser指定options.ShowPrivileges为合法参数：bool类型
      db.getUser( userName, { ShowPrivileges: true } );
      db.getUser( userName, { ShowPrivileges: false } );

      //6、getUser指定options.ShowPrivileges为非法参数：a、非bool类型，如："string"、1、array
      //自动修正为默认值
      db.getUser( userName, { ShowPrivileges: "string" } );

      db.getUser( userName, { ShowPrivileges: 1 } );

      db.getUser( userName, { ShowPrivileges: [] } );


   } finally
   {
      cleanRole( roleName1 );
      cleanRole( roleName2 );
      db.dropUsr( userName, passwd );
   }
}
