/******************************************************************************
 * @Description   : seqDB-27998: 版本: 1 :: 创建监控用户执行监控快照和列表类操作
 * @Author        : Tao Tang
 * @CreateTime    : 2022.09.26
 * @LastEditTime  : 2022.09.26
 * @LastEditors   : Tao Tang
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var testcaseID = 27998 ;
   var userNameAdmin = "admin_" + testcaseID ;
   var userNameMonitor = "monitor_" + testcaseID ;
   var optionAdmin = { Role : "admin" } ;
   var optionMonitor = { Role : "monitor" } ;
   var dataGroupName = commGetDataGroupNames( db )[0] ;


   try
   {
      // clean and create users
      cleanUsers( userNameMonitor ) ;
      cleanUsers( userNameAdmin ) ;

      db.createUsr(  userNameAdmin , userNameAdmin  , optionAdmin ) ;
      db.createUsr( userNameMonitor , userNameMonitor , optionMonitor ) ;

      var coorddb = new Sdb( COORDHOSTNAME, COORDSVCNAME, userNameMonitor, userNameMonitor ) ;
      var datadb = coorddb.getRG( dataGroupName ).getMaster().connect() ;
      var catadb = coorddb.getCataRG().getMaster().connect() ;


      snapshotOpr( coorddb ) ;
      listOpr( coorddb) ;

      snapshotOprForOtherNode( datadb ) ;
      listOprForOtherNode( datadb) ;

      snapshotOprForOtherNode( catadb ) ;
      listOprForOtherNode( catadb) ;


   }
   finally
   {
      cleanUsers( userNameMonitor ) ;
      cleanUsers( userNameAdmin ) ;
      coorddb.close() ;
      datadb.close() ;
      catadb.close() ;
   }

}
