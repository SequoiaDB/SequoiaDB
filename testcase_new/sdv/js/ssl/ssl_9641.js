/***************************************************************************
/***************************************************************************
@Description :seqDB-9641:SSL功能开启，使用sdbexprt工具导出数据
@Modify list :
              2016-9-1  wuyan  Init
****************************************************************************/
var clName = CHANGEDPREFIX + "_9641";

main( dbs );
function main ( dbs )
{
   try
   {
      // drop collection in the beginning
      commDropCL( dbs, csName, clName, true, true, "drop collection in the beginning" );

      // create cs /cl
      var dbCL = commCreateCL( dbs, csName, clName, {}, true, true );

      //export datas
      var exprtFile = "/tmp/" + CHANGEDPREFIX + "_9641.json";
      exportData( dbCL, csName, clName, exprtFile )


      //dropCS
      commDropCS( dbs, csName, false, "seqDB-9639: dropCS failed" );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      dbs.close();
      //cmd.run( "rm -rf " + exprtFile ) ; 
      cmd.run( "rm -rf " + "./sdbexport.log" );
   }
}

/******************************************************
@description: exprt the data of cl
@modify list:
              2016-9-1 yan WU init
******************************************************/
function exportData ( dbcl, csName, clName, exprtFile, datas )
{
   println( "\n---Begin to insert data ." );
   dbcl.insert( { a: 1, b: "test" } );

   println( "\n---Begin to exprt data and check exec result." );
   var exprtOption = installDir + 'bin/sdbexprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName + ' --type json'
      + ' --file ' + exprtFile + ' --fields a,b'
      + ' --ssl true';
   println( exprtOption );
   var rc = cmd.run( exprtOption );
   println( rc );

   var actDatas = cmd.run( 'cat ' + exprtFile ).split( "\n" );
   var expDatas = '{ "a": 1, "b": "test" }';
   if( expDatas !== actDatas[0] )
   {
      throw buildException( "checkExprtData", null, "[sdbexprt results]",
         "[" + expDatas + "]",
         "[" + actDatas + "]" );
   }
}
