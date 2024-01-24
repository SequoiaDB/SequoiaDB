/******************************************************************************
 * @Description   : seqDB-32892 :: createUsr新增字段参数校验
 * @Author        : Wang Xingming
 * @CreateTime    : 2023.09.05
 * @LastEditTime  : 2023.09.05
 * @LastEditors   : Wang Xingming
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dbname = "db_32892";
   var roleName = "role_32892";
   var name = "user_32892";
   var password = "password_32892";

   cleanRole( roleName );

   try
   {
      db.dropUsr( name, password );
   }
   catch( e )
   {
      if( e != SDB_AUTH_USER_NOT_EXIST )
      {
         throw e;
      }
   }

   var role = {
      Role: roleName, Privileges: [{
         Resource: { cs: dbname, cl: "" },
         Actions: ["find", "remove"]
      }]
   };
   db.createRole( role );

   // options.Roles字段指定非法参数
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createUsr( name, password, { Roles: [123] } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createUsr( name, password, { Roles: "string" } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createUsr( name, password, { Roles: true } );
   } );

   // options.Roles字段指定合法参数
   db.createUsr( name, password, { Roles: [roleName] } );

   cleanRole( roleName );

   try
   {
      db.dropUsr( name, password );
   }
   catch( e )
   {
      if( e != SDB_AUTH_USER_NOT_EXIST )
      {
         throw e;
      }
   }
}
