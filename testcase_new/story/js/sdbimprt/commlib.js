/*******************************************************************************
*@Description :  common function
*@Modify list :
*                2016/7/14  XiaoNi Huang Init
*******************************************************************************/
import( "../lib/main.js" );
import( "../lib/basic_operation/commlib.js" );

var testCaseDir = initTestCaseDir();
var tmpFileDir = WORKDIR + "/sdbimprt/";
var localPath = null;

var cmd = cmdInit();
var installDir = getInstallDir();   //eg:/opt/sequoiadb
readyTmpDir();

/* ****************************************************
@description: get testcase director
@return: testcase director
**************************************************** */
function initTestCaseDir ()
{
   if( typeof ( TESTCASEDIR ) == "undefined" ) 
   {
      var testCaseDir = './testcase_new/story/js/sdbimprt/';
   }
   else
   {
      //TESTCASEDIR default: ....../testcases/hlt/js_testcases/js/sdbimprt/
      var testCaseDir = TESTCASEDIR + '/';
   }
   return testCaseDir;
}

/* ****************************************************
@description: create cl
@return: cl
**************************************************** */
function readyCL ( csName, clName, optionObj, message )
{
   if( message == undefined ) { message = ""; }

   if( optionObj == undefined ) { optionObj = { ReplSize: 0 }; }

   commDropCL( db, csName, clName, true, true );

   var cl = commCreateCL( db, csName, clName, optionObj, true, true )
   return cl;
}

/* ****************************************************
@description: drop cl
**************************************************** */
function cleanCL ( csName, clName )
{
   commDropCL( db, csName, clName, false, false );
}

/* ****************************************************
@description: get dataRG Info
@parameter:
   [nameStr] "GroupName","HostName","svcname"
@return: groupArray
**************************************************** */
function getDataGroupsName ()
{
   var tmpArray = commGetGroups( db );
   var groupNameArray = new Array;
   for( i = 0; i < tmpArray.length; i++ )
   {
      groupNameArray.push( tmpArray[i][0].GroupName );
   }
   return groupNameArray;
}

/* ****************************************************
@description: ready tmp director
**************************************************** */
function readyTmpDir ()
{

   cmd.run( "rm -rf " + tmpFileDir );

   cmd.run( "mkdir -p " + tmpFileDir );
}

/* ****************************************************
@description: new Cmd
@return: cmd
**************************************************** */
function cmdInit ()
{
   var cmd = new Cmd();
   return cmd;
}

/* ****************************************************
@description: get install_dir of sequoiadb
@return: install_dir
**************************************************** */
function getInstallDir ()
{
   var localPath = cmd.run( "pwd" ).split( "\n" )[0] + "/";
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

   return installPath;
}

/* ****************************************************
@description: new File
@return: file
**************************************************** */
function fileInit ( fileName )
{
   var file = new File( fileName );
   return file;
}

/* ****************************************************
@description: turn to local time
@parameter:
   time: Timestamp with time zone to millisecond,eg:'1901-12-31T15:54:03.000Z'
   format: eg:%Y-%m-%d-%H.%M.%S.000000
@return: 
   localtime, eg: '1901-12-31-15.54.03.000000'
**************************************************** */
function turnLocaltime ( time, format )
{
   if( typeof ( format ) == "undefined" ) { format = "%Y-%m-%d"; };
   var msecond = new Date( time ).getTime();
   var second = parseInt( msecond / 1000 );  //millisecond to second
   var localtime = cmd.run( 'date -d@"' + second + '" "+' + format + '"' ).split( "\n" )[0];

   return localtime;
}

function importData ( csName, clName, importFile, type, fields, cast )
{
   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type ' + type
      + ' --file ' + importFile
      +' --insertnum ' + 10000;
   if( type == 'csv' ) 
   {
      imprtOption = imprtOption + ' --fields "' + fields + '"';
   }
   if( cast == true )
   {
      imprtOption = imprtOption + ' --cast ' + cast;
   }
   /*
   var command = "cat "+ importFile;
   var fileInfo = cmd.run( command );
   */
   var rc = cmd.run( imprtOption );
   var rcResults = rc.split( "\n" );

   return rcResults;
}

function exportData ( csName, clName, exportFile, type, fields, sort, otherParam )
{
   if( typeof ( sort ) == "undefined" ) { sort = "{a:1}"; }

   //remove export file
   cmd.run( "rm -rf " + exportFile );

   var exportOption = installDir + 'bin/sdbexprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type ' + type
      + ' --fields "' + fields + '"'
      + ' --sort "' + sort + '"'
      + ' --file ' + exportFile
      + ' ' + otherParam;
   var rc = cmd.run( exportOption );

   //cat exprt file
   var command = "cat " + exportFile;
   /*
   var fileInfo = cmd.run( command );
   */
}

function checkImportRC ( rcResults, expParseRecordsNum, expImportedRecordsNum, expParseFailureNum )
{
   if( typeof ( expParseFailureNum ) === "undefined" ) { expParseFailureNum = 0; }
   if( typeof ( expImportedRecordsNum ) === "undefined" ) { expImportedRecordsNum = expParseRecordsNum; }

   var expParseRecords = "Parsed records: " + expParseRecordsNum;
   var expParseFailure = "Parsed failure: " + expParseFailureNum;
   var expImportedRecords = "Imported records: " + expImportedRecordsNum;
   var actParseRecords = rcResults[0];
   var actParseFailure = rcResults[1];
   var actImportedRecords = rcResults[4];
   if( expParseRecords !== actParseRecords
      || expParseFailure !== actParseFailure
      || expImportedRecords !== actImportedRecords )
   {
      throw new Error( "importData fail,[sdbimprt results]" +
         "[" + expParseRecords + ", " + expParseFailure + ", " + expImportedRecords + "]" +
         "[" + actParseRecords + ", " + actParseFailure + ", " + actImportedRecords + "]" );
   }
}

function checkCLData ( cl, expRecsNum, expRecs, cond, message )
{
   if( typeof ( message ) === "undefined" ) 
   {
      message = "";
   }
   else
   {
      message = ", find type: " + message;
   }

   var rc;
   if( cond == "undefined" )
   {
      rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   }
   else 
   {
      rc = cl.find( cond, { _id: { $include: 0 } } ).sort( { a: 1 } );
   }

   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   // check count
   var actCnt = recsArray.length;
   assert.equal( actCnt, expRecsNum );

   // check records
   var actRecs = JSON.stringify( recsArray );
   assert.equal( actRecs, expRecs );
}

function checkExportData ( exportFile, expData )
{

   var rcData = cmd.run( "cat " + exportFile ).split( "\n" );
   var actData = JSON.stringify( rcData );
   assert.equal( actData, expData );

}

function checkResult ( cl, dataType, expResult )
{
   var rc = cl.find( { "a": { "$type": 2, "$et": dataType } } ).sort( { "_id": 1 } );
   var actResult = [];
   while( rc.next() )
   {
      actResult.push( rc.current().toObj() );
   }

   /*
   */
   assert.equal( actResult.length, expResult.length );

   for( var i in actResult )
   {
      assert.equal( actResult[i]['a'], expResult[i]['a'] );
   }
}
