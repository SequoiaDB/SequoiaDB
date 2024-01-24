/******************************************************************************
 * @Description   : seqDB-32886 :: listRoles接口参数校验 
 * @Author        : Wang Xingming
 * @CreateTime    : 2023.09.05
 * @LastEditTime  : 2023.09.05
 * @LastEditors   : Wang Xingming
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dbname = "db_32886";
   var roleName = "role_32886";
   var sysName = ["_root", "_clusterAdmin", "_clusterMonitor", "_backup", "_dbAdmin", "_userAdmin", "_exact.read", "_exact.readWrite", "_exact.admin"];

   cleanRole( roleName );

   var role = {
      Role: roleName, Privileges: [{
         Resource: { cs: dbname, cl: "" },
         Actions: ["find", "remove"]
      }]
   };
   db.createRole( role );

   // listRoles指定options为合法参数
   var cursor = db.listRoles( { ShowPrivileges: false, ShowBuiltinRoles: true } );
   while( cursor.next() )
   {
      var cursorObj = cursor.current().toObj();
      if( cursorObj["Privileges"] != undefined )
      {
         throw new Error( JSON.stringify( cursorObj ) );
      }
   }
   cursor.close();

   var cursor = db.listRoles( { ShowPrivileges: true, ShowBuiltinRoles: true } );
   while( cursor.next() )
   {
      var cursorObj = cursor.current().toObj();
      if( cursorObj["Privileges"] == undefined )
      {
         throw new Error( JSON.stringify( cursorObj ) );
      }
   }
   cursor.close();

   var Roles = 0;
   var cursorStr = "";
   var cursor = db.listRoles( { ShowPrivileges: true, ShowBuiltinRoles: true } );
   while( cursor.next() )
   {
      Roles++;
      cursorStr += JSON.stringify( cursor.current().toObj() );
   }
   cursor.close();
   if( Roles < 10 )
   {
      throw new Error( cursorStr );
   }

   var Roles = 0;
   var cursor = db.listRoles( { ShowPrivileges: true, ShowBuiltinRoles: false } );
   while( cursor.next() )
   {
      Roles++;
      var cursorObj = cursor.current().toObj()["Role"];
      if( sysName.indexOf( cursorObj ) !== -1 )
      {
         throw new Error( JSON.stringify( cursorObj ) );
      }
   }
   cursor.close();

   // listRoles指定options为非法参数
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.listRoles( "string" );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.listRoles( 123 );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.listRoles( ['array'] );
   } );

   // listRoles指定options.ShowPrivileges为非法参数
   var Roles = 0;
   var cursor = db.listRoles( { ShowPrivileges: false, ShowBuiltinRoles: 123 } );
   while( cursor.next() )
   {
      Roles++;
      var cursorObj = cursor.current().toObj()["Role"];
      if( sysName.indexOf( cursorObj ) !== -1 )
      {
         throw new Error( JSON.stringify( cursorObj ) );
      }
   }
   cursor.close();

   var Roles = 0;
   var cursor = db.listRoles( { ShowPrivileges: false, ShowBuiltinRoles: "string" } );
   while( cursor.next() )
   {
      Roles++;
      var cursorObj = cursor.current().toObj()["Role"];
      if( sysName.indexOf( cursorObj ) !== -1 )
      {
         throw new Error( JSON.stringify( cursorObj ) );
      }
   }
   cursor.close();

   var Roles = 0;
   var cursor = db.listRoles( { ShowPrivileges: false, ShowBuiltinRoles: ['array'] } );
   while( cursor.next() )
   {
      Roles++;
      var cursorObj = cursor.current().toObj()["Role"];
      if( sysName.indexOf( cursorObj ) !== -1 )
      {
         throw new Error( JSON.stringify( cursorObj ) );
      }
   }
   cursor.close();

   // listRoles指定options.ShowBuiltinRoles为非法参数
   var cursor = db.listRoles( { ShowPrivileges: 123, ShowBuiltinRoles: true } );
   while( cursor.next() )
   {
      var cursorObj = cursor.current().toObj();
      if( cursorObj["Privileges"] != undefined )
      {
         throw new Error( JSON.stringify( cursorObj ) );
      }
   }
   cursor.close();

   var cursor = db.listRoles( { ShowPrivileges: "string", ShowBuiltinRoles: true } );
   while( cursor.next() )
   {
      var cursorObj = cursor.current().toObj();
      if( cursorObj["Privileges"] != undefined )
      {
         throw new Error( JSON.stringify( cursorObj ) );
      }
   }
   cursor.close();

   var cursor = db.listRoles( { ShowPrivileges: ['array'], ShowBuiltinRoles: true } );
   while( cursor.next() )
   {
      var cursorObj = cursor.current().toObj();
      if( cursorObj["Privileges"] != undefined )
      {
         throw new Error( JSON.stringify( cursorObj ) );
      }
   }
   cursor.close();

   cleanRole( roleName );
}
