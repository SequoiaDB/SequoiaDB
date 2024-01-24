/******************************************************************************
 * @Description   : 
 * @Author        : liuli
 * @CreateTime    : 2022.05.18
 * @LastEditTime  : 2022.06.02
 * @LastEditors   : liuli
 ******************************************************************************/
import( "../lib/main.js" );
import( "../lib/basic_operation/commlib.js" );

// 定义Linux状态码
var COMMAND_NOT_FOUND = 127;
var FILE_NOT_EXIST = 8;
var FILE_ALREADY_EXIST = 136;

var localPath = null;
var tmpFileDir = WORKDIR + "/sdbsecuretool/";

var cmd = cmdInit();
var installDir = getInstallDir();   //eg:/opt/sequoiadb

var testCaseDir = initTestCaseDir();

readyTmpDir();
/* ****************************************************
@description: ready tmp director
**************************************************** */
function readyTmpDir ()
{

   cmd.run( "rm -rf " + tmpFileDir );

   cmd.run( "mkdir -p " + tmpFileDir );
}

/* ****************************************************
@description: get testcase director
@return: testcase director
**************************************************** */
function initTestCaseDir ()
{
   if( typeof ( TESTCASEDIR ) == "undefined" ) 
   {
      var testCaseDir = './testcase_new/story/js/sdbsecuretool/';
   }
   else
   {
      //TESTCASEDIR default: ....../testcases/hlt/js_testcases/js/sdbsecuretool/
      var testCaseDir = TESTCASEDIR + '/';
   }
   return testCaseDir;
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

   // 先取当前目录下的 bin/sdbsecuretool，不存在时，再取安装目录下的 bin/sdbsecuretool
   try
   {
      cmd.run( 'find ./bin/sdbsecuretool' ).split( '\n' )[0];
      installPath = localPath;
   }
   catch( e ) 
   {
      installPath = commGetInstallPath() + "/";
   }

   return installPath;
}

