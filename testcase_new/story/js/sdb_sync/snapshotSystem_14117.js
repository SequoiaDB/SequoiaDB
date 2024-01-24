/******************************************************************************
 * @Description   : seqDB-14117:获取系统快照后检查/etc/mtab文件句柄泄露  
 * @Author        : Liang XueWang
 * @LastEditTime  : 2023.05.26
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
var cmd = new Cmd();

main( test );

function test ()
{
   db.snapshot( SDB_SNAP_SYSTEM );
   var pid = getCataPid();
   var fpNum = getFpNum( pid );
   assert.equal( fpNum, 0 );
}

// get local cata node pid
function getCataPid ()
{
   var remote = new Remote( COORDHOSTNAME, CMSVCNAME );
   var system = remote.getSystem();
   var cataSvcName = db.getCataRG().getMaster().getServiceName();
   var cursor = system.listProcess( {}, { cmd: "sequoiadb(" + cataSvcName + ") C" } );
   var obj = cursor.next().toObj();
   var pid = obj["pid"];
   remote.close();
   return pid;
}

// get process pid /etc/mtab fp num
function getFpNum ( pid )
{
   var command = "lsof -p " + pid + " | grep /etc/mtab | wc -l";
   var info = cmd.run( command ).split( "\n" );
   return info[info.length - 2];
}
