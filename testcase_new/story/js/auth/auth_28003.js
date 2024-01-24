/******************************************************************************
 * @Description   : seqDB-28003 :: 版本: 1 :: 创建监控用户执行SdbQuery类监控操作
 * @Author        : Tao Tang
 * @CreateTime    : 2022.09.26
 * @LastEditTime  : 2022.09.26
 * @LastEditors   : Tao Tang
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var testcaseID = 28003 ;
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

      db.createUsr(  userNameAdmin , userNameAdmin  , optionAdmin ) ;
      db.createUsr( userNameMonitor , userNameMonitor , optionMonitor ) ;

      var coorddb = new Sdb( COORDHOSTNAME, COORDSVCNAME, userNameMonitor, userNameMonitor ) ;
      var datadb = coorddb.getRG( dataGroupName ).getMaster().connect() ;
      var catadb = coorddb.getCataRG().getMaster().connect() ;


      var cl = db.createCS( csName ).createCL( clName , cloption ) ;
      cl.createIndex( idxname, { 'name': 1 } ) ;
      cl.insert( { name : "tom"} ) ;


      var coordcs = coorddb.getCS( csName ) ;
      var coordcl = coordcs.getCL( clName ) ;
      coordcl.find().count() ;


      var datacs = datadb.getCS( csName ) ;
      var datacl = datacs.getCL( clName ) ;
      datacl.find().count() ;


      var cataCSName = "SYSCAT" ;
      var cataCLName = "SYSCOLLECTIONS" ;
      var cataCL = catadb.getCS( cataCSName ).getCL( cataCLName ) ;
      cataCL.find().count() ;

   }
   finally
   {
      cleanUsers( userNameMonitor ) ;
      cleanUsers( userNameAdmin ) ;
      coorddb.close() ;
      datadb.close() ;
      catadb.close() ;
      db.dropCS( csName ) ;
   }
}
