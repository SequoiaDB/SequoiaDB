/******************************************************************************
 * @Description   : seqDB-28001 :: 版本: 1 :: 创建监控用户执行SdbCS类监控操作
 * @Author        : Tao Tang
 * @CreateTime    : 2022.09.26
 * @LastEditTime  : 2022.09.26
 * @LastEditors   : Tao Tang
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var testcaseID = 28001 ;
   var userNameAdmin = "admin_" + testcaseID ;
   var userNameMonitor = "monitor_" + testcaseID ;
   var optionAdmin = { Role : "admin" } ;
   var optionMonitor = { Role : "monitor" } ;
   var dataGroupName = commGetDataGroupNames( db )[0] ;

   var csName = "seq_cs_" + testcaseID ;
   var clName = "seq_cl_" + testcaseID ;
   var domainName = "seq_domain_" + testcaseID ;
   var domainOption = [ dataGroupName ] ;
   var csOption = { Domain : domainName } ;
   var idxname = "idx_" + testcaseID ;

   try
   {
      // clean and create users
      cleanUsers( userNameMonitor ) ;
      cleanUsers( userNameAdmin ) ;

      db.createUsr(  userNameAdmin , userNameAdmin  , optionAdmin ) ;
      db.createUsr( userNameMonitor , userNameMonitor , optionMonitor ) ;

      db.createDomain( domainName , domainOption ) ;
      var cl = db.createCS( csName , csOption ).createCL( clName ) ;
      cl.createIndex( idxname, { 'name': 1 } ) ;

      var coorddb = new Sdb( COORDHOSTNAME, COORDSVCNAME, userNameMonitor, userNameMonitor ) ;
      var datadb = coorddb.getRG( dataGroupName ).getMaster().connect() ;
      var catadb = coorddb.getCataRG().getMaster().connect() ;


      var cs = coorddb.getCS( csName ) ;
      var domainname = cs.getDomainName() ;
      cs.listCollections().toArray() ;

      cs = datadb.getCS( csName ) ;
      domainname = cs.getDomainName() ;
      cs.listCollections().toArray() ;

      var cataCSName = "SYSCAT" ;
      cs = catadb.getCS( cataCSName ) ;
      domainname = cs.getDomainName() ;
      cs.listCollections().toArray() ;

   }
   finally
   {
      cleanUsers( userNameMonitor ) ;
      cleanUsers( userNameAdmin ) ;
      coorddb.close() ;
      datadb.close() ;
      catadb.close() ;
      db.dropCS( csName ) ;
      db.dropDomain( domainName ) ;
   }
}
