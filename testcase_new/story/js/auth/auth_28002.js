/******************************************************************************
 * @Description   : seqDB-28002 :: 版本: 1 :: 创建监控用户执行SdbCollection类监控操作
 * @Author        : Tao Tang
 * @CreateTime    : 2022.09.26
 * @LastEditTime  : 2022.09.26
 * @LastEditors   : Tao Tang
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var testcaseID = 28002 ;
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
      db.analyze() ;


      var coordcs = coorddb.getCS( csName ) ;
      var coordcl = coordcs.getCL( clName ) ;

      coordcl.getDetail().toArray() ;
      coordcl.getIndexStat( idxname ) ;
      coordcl.listIndexes().toArray() ;
      coordcl.listLobs().toArray() ;
      coordcl.count() ;
      // v3.6 及以上版本才有这个函数
      // cl.snapshotIndexes() ;


      var datacs = datadb.getCS( csName ) ;
      var datacl = datacs.getCL( clName ) ;

      datacl.getDetail().toArray() ;
      datacl.getIndexStat( idxname ) ;
      datacl.listIndexes().toArray() ;
      datacl.listLobs().toArray() ;
      datacl.count() ;


      var cataCSName = "SYSCAT" ;
      var cataCLName = "SYSCOLLECTIONS" ;
      var cataCL = catadb.getCS( cataCSName ).getCL( cataCLName ) ;
      cataCL.getDetail().toArray() ;
      cataCL.listLobs().toArray() ;
      cataCL.count() ;
      cataCL.listIndexes().toArray() ;

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
