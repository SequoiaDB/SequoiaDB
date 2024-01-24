/******************************************************************************
 * @Description   : seqDB-27805 :: 版本: 1 :: 重建同名monitor用户更新用户角色，原角色连接执行监控操作和非监控操作
 * @Author        : Tao Tang
 * @CreateTime    : 2022.09.26
 * @LastEditTime  : 2023.08.07
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

// SEQUOIADBMAINSTREAM-9798
// main( test );

function test ()
{
   var testcaseID = 27805 ;
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
      db.createUsr( userNameMonitor , userNameMonitor , optionMonitor ) ;

      var coorddb = new Sdb( COORDHOSTNAME, COORDSVCNAME, userNameMonitor, userNameMonitor ) ;

      cleanUsers( userNameMonitor ) ;
      db.createUsr( userNameMonitor , userNameMonitor , optionAdmin ) ;


      var cl = db.createCS( csName ).createCL( clName , cloption ) ;
      cl.createIndex( idxname, { 'name': 1 } ) ;
      cl.insert( { name : "tom"} ) ;

      coorddb.snapshot( SDB_SNAP_COLLECTIONSPACES ).toArray() ;

      coorddb.list( SDB_LIST_COLLECTIONSPACES ).toArray() ;

      var coordcs = coorddb.getCS( csName ) ;
      var coordcl = coordcs.getCL( clName ) ;
      try
      {
         coordcl.insert({ name : "jery"} ) ;
         throw new Error( "monitor role can not insert data ." ) ;
      }
      catch(e)
      {
         if( e.message != SDB_NO_PRIVILEGES )
         {
            throw new Error( e ) ;
         }
      }

      try
      {
         tmpname = clName + "_tmp" ;
         coordcs.createCL( tmpname , cloption ) ;
         throw new Error( "monitor role can not create CL ." ) ;
      }
      catch(e)
      {
         if( e.message != SDB_NO_PRIVILEGES )
         {
            throw new Error( e ) ;
         }
      }

   }
   finally
   {
      cleanUsers( userNameMonitor ) ;
      cleanUsers( userNameAdmin ) ;
      coorddb.close() ;
      db.dropCS( csName ) ;
   }
}

