/******************************************************************************
 * @Description   : seqDB-32833:使用listRoles获取角色信息
 * @Author        : tangtao
 * @CreateTime    : 2023.08.31
 * @LastEditTime  : 2023.08.31
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.csName = COMMCSNAME + "_32832";
main( test );

function test ( testPara )
{
   var dbname = COMMCSNAME + "_32832";
   var user = "user_32832";
   var password = "passwd_32832";
   var roleName1 = "role_32832_1";
   var roleName2 = "role_32832_2";
   var roleName3 = "role_32832_3";
   var roleName4 = "role_32832_4";

   cleanRole( roleName1 );
   cleanRole( roleName2 );
   cleanRole( roleName3 );
   cleanRole( roleName4 );


   var actions1 = ["testCS", "testCL"];
   var actions2 = ["find", "insert", "update", "remove"];
   var actions3 = ["find", "getDetail", "createIndex", "dropIndex"];
   var actions4 = ["find", "insert", "update", "remove", "getDetail", "alterCL",
      "createIndex", "dropIndex", "truncate", "testCS", "testCL"];

   try
   {
      var role = {
         Role: roleName1, Privileges: [{
            Resource: { cs: dbname, cl: "" },
            Actions: actions1
         }]
      };
      db.createRole( role );
      var role = {
         Role: roleName2, Privileges: [{
            Resource: { cs: dbname, cl: "" },
            Actions: actions2
         }],
         Roles: [roleName1]
      };
      db.createRole( role );
      var role = {
         Role: roleName3, Privileges: [{
            Resource: { cs: dbname, cl: "" },
            Actions: actions3
         }],
         Roles: [roleName2]
      };
      db.createRole( role );

      var role = {
         Role: roleName4, Privileges: [{
            Resource: { cs: dbname, cl: "" },
            Actions: actions4
         }]
      };
      db.createRole( role );

      // 获取不存在的角色
      assert.tryThrow( SDB_AUTH_ROLE_NOT_EXIST, function()
      {
         db.getRole( "unExitName" );
      } );

      // 校验角色信息
      var roleInfo = db.getRole( roleName1 );
      assert.equal( roleInfo.toObj()["Role"], roleName1 );
      assert.equal( roleInfo.toObj()["InheritedRoles"], [] );
      assert.equal( roleInfo.toObj()["Privileges"], undefined );

      var roleInfo = db.getRole( roleName2, { ShowPrivileges: false } );
      assert.equal( roleInfo.toObj()["Role"], roleName2 );
      assert.equal( roleInfo.toObj()["InheritedRoles"], [roleName1] );
      assert.equal( roleInfo.toObj()["Privileges"], undefined );

      var roleInfo = db.getRole( roleName3, { ShowPrivileges: true } );
      assert.equal( roleInfo.toObj()["Role"], roleName3 );
      assert.equal( roleInfo.toObj()["InheritedRoles"].sort(), [roleName1, roleName2] );
      var actions = ["find", "insert", "update", "remove", "getDetail",
         "createIndex", "dropIndex", "testCL", "testCS"];
      assert.equal( roleInfo.toObj()["InheritedPrivileges"][0]["Actions"].sort(), actions.sort() );

      var roleInfo = db.getRole( roleName4, { ShowPrivileges: true } );
      assert.equal( roleInfo.toObj()["Role"], roleName4 );
      assert.equal( roleInfo.toObj()["InheritedRoles"], [] );
      var actions = ["alterCL", "find", "insert", "update", "remove",
         "getDetail", "createIndex", "dropIndex", "truncate", "testCL", "testCS"];
      assert.equal( roleInfo.toObj()["Privileges"][0]["Actions"].sort(), actions.sort() );
   }
   finally
   {
      cleanRole( roleName1 );
      cleanRole( roleName2 );
      cleanRole( roleName3 );
      cleanRole( roleName4 );
   }
}
