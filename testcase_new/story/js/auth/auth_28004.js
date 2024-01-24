/******************************************************************************
 * @Description   : seqDB-28004 :: 版本: 1 :: 创建监控用户执行SdbSequence类监控操作
 * @Author        : Tao Tang
 * @CreateTime    : 2022.09.26
 * @LastEditTime  : 2022.09.26
 * @LastEditors   : Tao Tang
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var testcaseID = 28004 ;
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


      var sequenceName = "seq" + testcaseID ;
      var increment = 1 ;
      dropSequence( db, sequenceName ) ;
      var sequence = db.createSequence( sequenceName, { Increment: increment } ) ;
      sequence.fetch( 10 ) ;


      var coordseq = coorddb.getSequence ( sequenceName ) ;
      var val = coordseq.getCurrentValue() ;
      assert.equal( val, 10 ) ;
   }
   finally
   {
      cleanUsers( userNameMonitor ) ;
      cleanUsers( userNameAdmin ) ;
      dropSequence( db, sequenceName ) ;
      coorddb.close() ;
      datadb.close() ;
      catadb.close() ;
   }
}
