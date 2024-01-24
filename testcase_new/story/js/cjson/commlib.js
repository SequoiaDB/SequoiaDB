/*******************************************************************************
*@Description :  common function
*@Modify list :
*                2016/7/28  wu yan Init
*******************************************************************************/

import( "../lib/main.js" );
import( "../lib/basic_operation/commlib.js" );

var tmpFileDir = '/tmp/cjsonOfSdbimprt/';
var cmd = new Cmd();
readyTmpDir();
var LocalPath = null;           // 当前目录       
var installDir = initPath();    // import工具所在目录
/* ****************************************************
@description: create cl
@return: cl
**************************************************** */
function readyCL ( csName, clName, optionObj )
{

   if( optionObj == undefined ) { optionObj = {}; }
   commDropCL( db, csName, clName );
   var cl = commCreateCL( db, csName, clName, optionObj );
   return cl;
}

/* ****************************************************
@description: create the tmpDir
**************************************************** */
function readyTmpDir ()
{
   cmd.run( "rm -rf " + tmpFileDir );
   cmd.run( "mkdir -p " + tmpFileDir );
}

/******************************************************************************
*@Description : initalize the global variable in the begninning.
                初始化全局变量LocalPath、installPath,获取sdbimprt所在目录
******************************************************************************/
function initPath ()
{
   var localPath = cmd.run( "pwd" ).split( "\n" )[0] + "/";
   println( "localPath   = " + localPath );
   var installPath = '';

   // 先取当前目录下的 bin/sdbimprt，不存在时，再取安装目录下的 bin/sdbimprt
   try
   {
      cmd.run( 'find ./bin/sdbimprt' ).split( '\n' )[0];
      installPath = localPath;
   }
   catch( e ) 
   {
      installPath = commGetInstallPath() + "/";
   }

   println( "instatllpath = " + installPath );
   return installPath;
}
/****************************************************
@description: check the result of import
@modify list:
              2016-7-22 yan WU init
****************************************************/
function checkCLData ( cl, expRecs )
{
   var rc = cl.find( {}, { _id: { $include: 0 } } );
   var recsArray = [];
   while( rc.next() )
   {
      recsArray.push( rc.current().toObj() );
   }
   //var expRecs = '[{"a":1},{"a":2}]';
   var actRecs = JSON.stringify( recsArray );
   if( actRecs !== expRecs )
   {
      throw new Error( "actRecs: " + actRecs + "\nexpRecs: " + expRecs );
   }

}

/******************************************************
@description: ready the datas of json file ,then import
@modify list:
              2016-7-22 yan WU init
******************************************************/
function importData ( csName, clName, imprtFile, datas )
{
   var file = new File( imprtFile );
   file.write( datas );
   var fileInfo = cmd.run( "cat " + imprtFile );
   println( imprtFile + "\n" + fileInfo );
   file.close();

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type json'
      + ' --file ' + imprtFile
      + ' --parsers 1 -j 1';
   //println( imprtOption );
   var rc = cmd.run( imprtOption );
   //println( rc );  
   return rc;
}

/******************************************************
@description: remove the removeTmpDir
@modify list:
              2016-7-22 yan WU init
******************************************************/
function removeTmpDir ()
{
   cmd.run( "rm -rf " + tmpFileDir );
}

/******************************************************
@description: check the errorLogMessage of the sdbimport.log
@modify list:
              2016-7-22 yan WU init
******************************************************/
function checkSdbimportLog ( matchInfos, expInfo )
{
   var logInfo = "";
   var result = cmd.run( matchInfos ).split( "\n" )[0];
   var check = result.split( ":" );
   if( check[0] === './sdbimport.log' )
   {
      var length = check.length;
      for( var i = 1; i < length; ++i )
      {
         if( i > 1 )
         {
            logInfo += ":";
         }
         logInfo += check[i];
      }
   }
   else
   {
      logInfo = result;
   }
   //println( "sdbimport.log: "+ logInfo );

   var actLogInfo = logInfo;
   if( expInfo !== actLogInfo )
   {
      throw new Error( "expInfo: " + expInfo + "\nactLogInfo: " + actLogInfo );
   }
}

/******************************************************
@description: check the sdbimport return Infos ,check the success and fail counts
@modify list:
              2016-7-22 yan WU init
******************************************************/
function checkImportReturn ( rc, parseFail, importRes )
{
   var rcObj = rc.split( "\n" );
   var expError = "Parsed failure: " + parseFail;
   var expImportedRecords = "Imported records: " + importRes;
   var actError = rcObj[1];
   var actImportedRecords = rcObj[4];
   if( expError !== actError || expImportedRecords !== actImportedRecords )
   {
      throw new Error( "expError: " + expError + " expImportedRecords: " + expImportedRecords + "\nactError: " + " actImportedRecords: " + actImportedRecords );
   }
}
