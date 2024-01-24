/***************************************************************************
/***************************************************************************
@Description :seqDB-9640:SSL功能开启，使用sdbimport导入数据
@Modify list :
              2016-8-31  wuyan  Init
****************************************************************************/
var clName = CHANGEDPREFIX + "_9640";

main( dbs );
function main ( dbs )
{
   try
   {
      // drop collection in the beginning
      commDropCL( dbs, csName, clName, true, true, "drop collection in the beginning" );

      // create cs /cl
      var dbCL = commCreateCL( dbs, csName, clName, {}, true, true );

      //import datas          
      var imprtFile = "/tmp/" + CHANGEDPREFIX + "_9640.json";
      var srcDatas = "{ _id:1,true : true,null:null,int :12345,int64:21474836470,double:123456789.123,string:'test'}";
      importData( csName, clName, imprtFile, srcDatas );

      //check the import result 
      var expRecs = '[{"_id":1,"true":true,"null":null,"int":12345,"int64":21474836470,"double":123456789.123,"string":"test"}]';
      checkCLData( dbCL, expRecs );

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
      cmd.run( "rm -rf " + imprtFile );
   }
}

/******************************************************
@description: ready the datas of json file ,then import
@modify list:
              2016-7-22 yan WU init
******************************************************/
function importData ( csName, clName, imprtFile, datas )
{
   println( "\n---Begin to ready data." );
   var file = new File( imprtFile );
   file.write( datas );
   var fileInfo = cmd.run( "cat " + imprtFile );
   println( imprtFile + "\n" + fileInfo );
   file.close();

   println( "\n---Begin to import data and check exec result." );

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type json' + ' --file ' + imprtFile
      + ' --ssl true';
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );
   return rc;
}
