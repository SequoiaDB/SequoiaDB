/*******************************************************************************
*@Description :  common function
*@Modify list :
*                2016/7/14  XiaoNi Huang Init
*******************************************************************************/
import( "../lib/main.js" );

var testCaseDir = initTestCaseDir();
var tmpFileDir = '/tmp/sdbimprtTest/';
var LocalPath = null;

var cmd = cmdInit();
var installDir = getInstallDir();   //eg:/opt/sequoiadb
readyTmpDir();

/* ****************************************************
@description: get testcase director
@return: testcase director
**************************************************** */
function initTestCaseDir ()
{
   println( "\n---Begin to get testcase director." );

   if( typeof ( TESTCASEDIR ) == "undefined" ) 
   {
      var testCaseDir = './testcases/hlt/js_testcases/js/sdbimprt/';
   }
   else
   {
      //TESTCASEDIR default: ....../testcases/hlt/js_testcases/js/sdbimprt/
      var testCaseDir = TESTCASEDIR + '/';
   }
   println( "testCaseDir = " + testCaseDir );
   return testCaseDir;
}

/* ****************************************************
@description: get coord address
@return: coord address,e.g: "ubuntu-200-025:11810"
**************************************************** */
function getCoordAdrr ()
{
   println( "\n---Begin to get coord address." );

   var tmpInfo = db.exec( "select NodeName from $SNAPSHOT_SYSTEM where GroupName='SYSCoord'" )
      .current().toObj()["ErrNodes"];
   var nodeArray = [];
   for( i = 0; i < tmpInfo.length; i++ )
   {
      nodeInfo = tmpInfo[i]["NodeName"];
      nodeArray.push( nodeInfo );
   }
   return nodeArray;
}

/* ****************************************************
@description: create cl
@return: cl
**************************************************** */
function readyCL ( csName, clName, optionObj, message )
{
   if( message == undefined ) { message = ""; }
   println( "\n---Begin to create CL " + message + "." );

   if( optionObj == undefined ) { optionObj = { ReplSize: 0 }; }

   commDropCL( db, csName, clName, true, true,
      "Failed to drop CL in the pre-condition." );

   var cl = commCreateCL( db, csName, clName, optionObj, true, true,
      "Failed to create CL." )
   return cl;
}

/* ****************************************************
@description: drop cl
**************************************************** */
function cleanCL ( csName, clName )
{
   println( "\n---Begin to drop CL." );

   commDropCL( db, csName, clName, false, false,
      "Failed to drop CL in the end-condition" );
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
   println( "\n---Begin to ready tmpFileDir" );

   try
   {
      cmd.run( "rm -rf " + tmpFileDir );
   }
   catch( e )
   {
      println( "Failed to rm tmpFileDir[" + tmpFileDir + "]" );
      throw e;
   }

   try
   {
      cmd.run( "mkdir -p " + tmpFileDir );
   }
   catch( e )
   {
      println( "Failed to mkdir tmpFileDir[" + tmpFileDir + "]" );
      throw e;
   }
}

/* ****************************************************
@description: new Cmd
@return: cmd
**************************************************** */
function cmdInit ()
{
   try
   {
      var cmd = new Cmd();
      return cmd;
   }
   catch( e )
   {
      println( "Failed to init cmd." );
      throw e;
   }
}

/* ****************************************************
@description: get install_dir of sequoiadb
@return: install_dir
**************************************************** */
function getInstallDir ()
{
   try
   {
      var LocalPath = cmd.run( "pwd" ).split( "\n" )[0] + "/";
      println( "LocalPath = " + LocalPath );

      try
      {
         //get sequoiadb, if not exists to throw
         var tmpDir = cmd.run( 'find /etc/default/sequoiadb' );
         //get sequoiadb install_dir configurature item, if not exists to throw
         var tmpDir = cmd.run( 'find /etc/default/sequoiadb | xargs grep "INSTALL_DIR"' );
         //get sequoiadb director, if not exists to throw
         var tmpDir = cmd.run( 'find /etc/default/sequoiadb | xargs grep "INSTALL_DIR" |cut -d "=" -f 2' );
         var installPath = tmpDir.split( "\n" )[0] + "/";
      }
      catch( e )
      {
         ///etc/default/sequoiadb is not exists 
         var installPath = LocalPath;
         println( "instatllpath = " + installPath );
      }
      println( "instatllpath = " + installPath );

   }
   catch( e )
   {
      println( "failed to get global variable : cmd/LocalPath/installPath" + e );
      throw e;
   }

   return installPath;
}

/* ****************************************************
@description: new File
@return: file
**************************************************** */
function fileInit ( fileName )
{
   try
   {
      var file = new File( fileName );
      return file;
   }
   catch( e )
   {
      println( "Failed to init file." );
      throw e;
   }
}
