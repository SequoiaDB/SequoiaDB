/******************************************************************************
 * @Description   : seqDB-27803 :: 版本: 1 :: 重建同名admin用户更新用户角色，原角色连接执行监控操作和非监控操作
 * @Author        : Tao Tang
 * @CreateTime    : 2022.09.26
 * @LastEditTime  : 2022.09.26
 * @LastEditors   : Tao Tang
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var testcaseID = 27803 ;
   var userNameAdmin = "admin_" + testcaseID ;
   var userNameMonitor = "monitor_" + testcaseID ;
   var optionAdmin = { Role : "admin" } ;
   var optionMonitor = { Role : "monitor" } ;
   var dataGroupName = commGetDataGroupNames( db )[0] ;

   var csName = "seq_cs_" + testcaseID ;
   var clName = "seq_cl_" + testcaseID ;
   var cloption = { Group: dataGroupName } ;
   var idxname = "idx_" + testcaseID ;

   try
   {
      // clean and create users
      cleanUsers( userNameMonitor ) ;
      cleanUsers( userNameAdmin ) ;

      db.createUsr( userNameAdmin , userNameAdmin  , optionAdmin ) ;
      db.createUsr( userNameMonitor , userNameMonitor , optionAdmin ) ;

      var coorddb = new Sdb( COORDHOSTNAME, COORDSVCNAME, userNameMonitor, userNameMonitor ) ;

      cleanUsers( userNameMonitor ) ;
      db.createUsr( userNameMonitor , userNameMonitor , optionMonitor ) ;


      var cl = db.createCS( csName ).createCL( clName , cloption ) ;
      cl.createIndex( idxname, { 'name': 1 } ) ;
      cl.insert( { name : "tom"} ) ;

      coorddb.snapshot( SDB_SNAP_COLLECTIONSPACES ).toArray() ;

      coorddb.list( SDB_LIST_COLLECTIONSPACES ).toArray() ;

      var coordcs = coorddb.getCS( csName ) ;
      var coordcl = coordcs.getCL( clName ) ;

      coordcl.insert({ name : "jery"} ) ;


      var tmpname = clName + "_tmp" ;
      coordcs.createCL( tmpname , cloption ) ;

   }
   finally
   {
      cleanUsers( userNameMonitor ) ;
      cleanUsers( userNameAdmin ) ;
      coorddb.close() ;
      db.dropCS( csName ) ;
   }
}
