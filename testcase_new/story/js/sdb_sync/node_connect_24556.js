
/******************************************************************************
 * @Description   : seqDB-24556:创建用户，创建SecureSdb对象指定用户密码，使用SecureSdb对象获取节点的连接
 * @Author        : xiaozhenfan
 * @CreateTime    : 2021.11.04
 * @LastEditTime  : 2022.09.02
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var isUseSsls = [];
   var cursor = db.snapshot( SDB_SNAP_CONFIGS, { GroupName: "SYSCoord" }, { usessl: "" } );
   while( cursor.next() )
   {
      var usessl = cursor.current().toObj()["usessl"];
      isUseSsls.push( usessl );
   }
   cursor.close();

   if( isUseSsls.indexOf( "FALSE" ) > -1 )
   {
      db.updateConf( { usessl: true }, { GroupName: "SYSCoord" } );
   }

   var userName = "sdbadmin";
   var passWrod = "sdbadmin";
   var isUsrExist = false;

   var db2 = new SecureSdb( COORDHOSTNAME, COORDSVCNAME, userName, passWrod )
   try
   {
      db2.createUsr( userName, passWrod );
      isUsrExist = true;
      var db1 = new SecureSdb( COORDHOSTNAME, COORDSVCNAME, userName, passWrod )
      var RGName = "SYSCatalogGroup";
      db1.getRG( RGName ).getMaster().connect();
   }
   finally
   {
      if( isUseSsls.indexOf( "FALSE" ) > -1 )
      {
         db.deleteConf( { usessl: 1 } );
      }
      if( isUsrExist )
      {
         db.dropUsr( userName, passWrod );
      }
      if( db1 != undefined )
      {
         db1.close()
      }
      if( db2 != undefined )
      {
         db2.close()
      }
   }
}