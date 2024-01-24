/******************************************************************************
 * @Description   : seqDB-27797 :: 创建监控用户执行调用存储过程eval相关非监控操作  
 * @Author        : Wu Yan
 * @CreateTime    : 2022.09.24
 * @LastEditTime  : 2023.08.07
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

// SEQUOIADBMAINSTREAM-9798
// main( test );
function test ()
{
   try
   {
      var csName = "cs27797";
      var clName = "cl27797";
      var adminUser = "admin27797"
      var userName = "user27797";
      var passwd = "passwd27797";

      var dba = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      dba.createUsr( adminUser, passwd, { Role: "admin" } );
      dba.createUsr( userName, passwd, { Role: "monitor" } );

      var authDB = new Sdb( COORDHOSTNAME, COORDSVCNAME, userName, passwd );
      var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME, adminUser, passwd );
      //初始化db连接给存储过程使用
      var db = new Sdb( COORDHOSTNAME, COORDSVCNAME, adminUser, passwd );

      var procedureNames = [];
      //ddl      
      var pcdName1 = "createCSPcd";
      var pcdString1 = "db.createProcedure( function " + pcdName1 + "(" + ") {db.createCS('" + csName + "')} )";
      sdb.eval( pcdString1 );
      procedureNames.push( pcdName1 );
      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         authDB.eval( pcdName1 + "()" );
      } );

      //dml
      var pcdName2 = "createCLPcd";
      var pcdString2 = "db.createProcedure(function " + pcdName2 + "(" + "){db.getCS('" + COMMCSNAME + "').createCL('" + clName + "')})";
      sdb.eval( pcdString2 );
      procedureNames.push( pcdName2 );
      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         authDB.eval( pcdName2 + "()" );
      } );

      //dql     
      sdb.createCS( csName ).createCL( clName );
      var pcdName3 = "insertPcd";
      var pcdString3 = "db.createProcedure(function " + pcdName3 + "(" + "){db.getCS('" + csName + "').getCL('" + clName + "').insert( {a:1} )})";
      sdb.eval( pcdString3 );
      procedureNames.push( pcdName3 );
      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         authDB.eval( pcdName3 + "()" );
      } );

      //dql      
      var pcdName4 = "queryPcd";
      var pcdString4 = "db.createProcedure(function " + pcdName4 + "(" + "){db.getCS('" + csName + "').getCL('" + clName + "').find().toArray()})";
      sdb.eval( pcdString4 );
      procedureNames.push( pcdName4 );
      assert.tryThrow( SDB_NO_PRIVILEGES, function()
      {
         authDB.eval( pcdName3 + "()" );
      } );
      sdb.dropCS( csName );
   }
   finally
   {
      sdb.dropUsr( userName, passwd );
      sdb.dropUsr( adminUser, passwd );
      clearProcedures( sdb, procedureNames );
      authDB.close();
      sdb.close();
      db.close();
   }
}

function clearProcedures ( db, procedureNames )
{
   var procedureNum = procedureNames.length;
   if( procedureNum !== 0 )
   {
      for( var i = 0; i < procedureNum; i++ )
      {
         var name = procedureNames[i];
         db.removeProcedure( name );
      }
   }
}







