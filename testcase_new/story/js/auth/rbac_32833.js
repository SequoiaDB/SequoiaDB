/******************************************************************************
 * @Description   : seqDB-32833:使用listRoles获取角色信息
 * @Author        : tangtao
 * @CreateTime    : 2023.08.31
 * @LastEditTime  : 2023.08.31
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.csName = COMMCSNAME + "_32833";
main( test );

function test ( testPara )
{
   var dbname = COMMCSNAME + "_32833";
   var user = "user_32833";
   var password = "passwd_32833";
   var roleName1 = "role_32833_1";
   var roleName2 = "role_32833_2";
   var roleName3 = "role_32833_3";
   var roleName4 = "role_32833_4";

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


      // 不指定options
      var roles = db.listRoles();
      while( roles.next() )
      {
         var roleInfo = roles.current();
         assert.equal( roleInfo.toObj()["Privileges"], undefined );
      }
      roles.close();

      // 指定ShowPrivileges为false
      var roles = db.listRoles( { ShowPrivileges: false } );
      while( roles.next() )
      {
         var roleInfo = roles.current();
         assert.equal( roleInfo.toObj()["Privileges"], undefined );
      }
      roles.close();

      // 指定ShowPrivileges为true
      var roles = db.listRoles( { ShowPrivileges: true } );
      while( roles.next() )
      {
         var roleInfo = roles.current();
         assert.notEqual( roleInfo.toObj()["Privileges"], undefined );
      }
      roles.close();

      // 指定ShowBuiltinRoles为false
      var roles = db.listRoles( { ShowBuiltinRoles: false } );
      while( roles.next() )
      {
         var roleInfo = roles.current();
         assert.equal( roleInfo.toObj()["Privileges"], undefined );
      }
      roles.close();

      // 指定ShowBuiltinRoles为true
      var roles = db.listRoles( { ShowBuiltinRoles: true } );
      var buildRoleCount = 0;
      while( roles.next() )
      {
         var roleInfo = roles.current();

         switch( roleInfo.toObj()["Role"] )
         {
            case "_root":
            case "_clusterAdmin":
            case "_clusterMonitor":
            case "_backup":
            case "_dbAdmin":
            case "_userAdmin":
            case "_exact.read":
            case "_exact.readWrite":
            case "_exact.admin":
               buildRoleCount++;
               break;
            default:
               break;
         }
      }
      assert.equal( buildRoleCount, 9 );
      roles.close();

      // 同时指定ShowPrivileges为true、ShowBuiltinRoles为true
      var roles = db.listRoles( { ShowPrivileges: true, ShowBuiltinRoles: true } );

      while( roles.next() )
      {
         var roleInfo = roles.current();
         assert.notEqual( roleInfo.toObj()["Privileges"], undefined );

         // 自建角色与内建角色各挑两个校验权限信息
         switch( roleInfo.toObj()["Role"] )
         {
            case roleName3:
               assert.equal( roleInfo.toObj()["InheritedRoles"].sort(), [roleName1, roleName2] );
               var actions = ["find", "insert", "update", "remove", "getDetail",
                  "createIndex", "dropIndex", "testCL", "testCS"];
               assert.equal( roleInfo.toObj()["InheritedPrivileges"][0]["Actions"].sort(),
                  actions.sort() );
               break;
            case roleName4:
               assert.equal( roleInfo.toObj()["InheritedRoles"], [] );
               var actions = ["alterCL", "find", "insert", "update", "remove",
                  "getDetail", "createIndex", "dropIndex", "truncate", "testCL", "testCS"];
               assert.equal( roleInfo.toObj()["Privileges"][0]["Actions"].sort(), actions.sort() );
               break;
            case "_clusterMonitor":
               assert.equal( roleInfo.toObj()["InheritedRoles"], [] );
               var actions = ["countBin", "getDetailBin", "listBin", "getRole", "listRoles",
                  "getUser", "eval", "list", "listCollectionSpaces", "resetSnapshot",
                  "snapshot", "traceStatus", "listProcedures", "getSequenceCurrentValue",
                  "getDCInfo"];
               assert.equal( roleInfo.toObj()["InheritedPrivileges"][0]["Actions"].sort(),
                  actions.sort() );
               break;
            case "_userAdmin":
               assert.equal( roleInfo.toObj()["InheritedRoles"], [] );
               var actions = ["createRole", "dropRole", "getRole", "listRoles", "updateRole",
                  "grantPrivilegesToRole", "revokePrivilegesFromRole", "grantRolesToRole",
                  "revokeRolesFromRole", "createUsr", "dropUsr", "getUser", "grantRolesToUser",
                  "revokeRolesFromUser", "invalidateUserCache"];
               assert.equal( roleInfo.toObj()["InheritedPrivileges"][0]["Actions"].sort(),
                  actions.sort() );
               break;
            default:
               break;
         }
      }
      roles.close();

   }
   finally
   {
      cleanRole( roleName1 );
      cleanRole( roleName2 );
      cleanRole( roleName3 );
      cleanRole( roleName4 );
   }
}
