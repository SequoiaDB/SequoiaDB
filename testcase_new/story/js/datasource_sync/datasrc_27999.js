/******************************************************************************
 * @Description   : seqDB-27999:创建监控用户执行Sdb类监控操作
 * @Author        : Xu Mingxing
 * @CreateTime    : 2022.10.09
 * @LastEditTime  : 2022.11.03
 * @LastEditors   : Xu Mingxing
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var userName1 = "user_27999_1";
   var userName2 = "user_27999_2";
   var dataSrcName = "datasrc27999";
   var datasrcUrl = DSHOSTNAME + ":" + DSSVCNAME;
   db.createDataSource( dataSrcName, datasrcUrl );
   db.createUsr( userName1, userName1, { Role: "admin" } );
   db.createUsr( userName2, userName2, { Role: "monitor" } );
   try
   {
      var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME, userName2, userName2 );
      // 各对象列取操作
      sdb.listDataSources().toArray();
      // 各对象元数据操作
      sdb.getDataSource( dataSrcName );
   }
   finally
   {
      db.dropUsr( userName2, userName2 );
      db.dropUsr( userName1, userName1 );
      db.dropDataSource( dataSrcName );
      sdb.close();
   }
}