/******************************************************************************
*@Description : Test the hint index common function.
*@Modify list :
*               2014-6-12   xiaojun Hu  Init
*               2014-11-10  xiaojun Hu  Change
******************************************************************************/
import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );


function lobFileIsExist ( fileName )
{
   var isExist = false;
   try
   {
      var cmd = new Cmd();
      cmd.run( "ls " + fileName );
      isExist = true;
   }
   catch( e )
   {
      if( 2 == e.message ) { isExist = false; }
   }
   return isExist;
}

function getMd5ForFile ( testFile )
{
   var cmd = new Cmd();
   var md5Arr = cmd.run( "md5sum " + testFile ).split( " " );
   var md5 = md5Arr[0];
   return md5;
}

function lobGenerateFile ( fileName, fileLine )
{
   if( undefined == fileLine )
   {
      fileLine = 1000;
   }

   var cnt = 0;
   while( true == lobFileIsExist( fileName ) )
   {
      File.remove( fileName );
      if( cnt > 10 ) break;
      cnt++;
      sleep( 10 );
   }

   if( 10 <= cnt )
      throw new Error( "failed to remove file: " + fileName );
   var file = new File( fileName );
   for( var i = 0; i < fileLine; ++i )
   {
      var record = '{ no:' + i + ', score:' + i + ', interest:["movie", "photo"], ' +
         '  major:"计算机软件与理论", dep:"计算机学院", ' +
         '  info:{name:"Holiday", age:22, sex:"男"} }';
      file.write( record );
   }
   if( false == lobFileIsExist( fileName ) )
      throw new Error( "NoFile: " + fileName );
}

function lobCreateCS ( db, DOMCSNAME, domName )
{
   var cs = db.createCS( DOMCSNAME, { "PageSize": 4096, "Domain": domName } );
   return cs;
}

function lobPutLob ( cl, lobFile, lobNum )
{
   if( undefined == lobNum )
   {
      lobNum = 10;
   }

   var oid = [];
   for( var i = 0; i < lobNum; ++i )
   {
      oid[i] = cl.putLob( lobFile );
   }
   // verify
   var cursor = cl.listLobs().toArray();
   assert.equal( lobNum, cursor.length );
   return oid;
}

// Insert data to SequoiaDB
function lobInsertDoc ( cl, recordNum )
{
   var docs = [];
   // insert 10000 records in CL
   for( var i = 0; i < recordNum; ++i )
   {
      docs.push(
         {
            no: i, score: i, interest: ["movie", "photo"],
            major: "计算机软件与理论", dep: "计算机学院",
            info: { name: "Holiday", age: 22, sex: "男" }
         } );

   }
   cl.insert( docs );
   var cnt = 0;
   do
   {
      ++cnt;
      sleep( 10 );
   }
   while( recordNum != cl.count() && 1000 < cnt );
   assert.equal( recordNum, cl.count() );
}

function lobSplit ( cl, srcGroup, dstGroup, firstCond, secondCond )
{
   if( undefined == secondCond ) { secondCond = "percent"; }
   if( "percent" != secondCond )
   {
      cl.split( srcGroup, dstGroup, firstCond, secondCond );
   }
   else
   {
      cl.split( srcGroup, dstGroup, firstCond );
   }
}

function lobGetAllGroupNames ( db )
{
   var RG = commGetGroups( db );
   var groupnames = [];
   for( var i = 0; i < RG.length; ++i )
   {
      groupnames.push( RG[i][0].GroupName );
   }
   return groupnames;
}

/******************************************************************************
 * 以下为 lob 数据页 CRC 校验相关的公共函数(供 hash/crc 边缘、异常用例复用)
 * 内核常量:DMS_HEADER_SZ = 65536(64K),默认 lob 页大小 = 262144(256K)。
 * lobd 文件布局:[64K header][pageID 0 .. n],pageID 0 为 meta,数据页从 1 起。
 ******************************************************************************/
var LOB_DMS_HEADER_SZ = 65536;    // 与内核 DMS_HEADER_SZ 一致
var LOB_PAGE_SIZE = 262144;       // 默认 lob 页大小 256K

// 获取指定组内所有节点(含 dbpath),返回 [{HostName, svcname, dbpath}]
function lobGetGroupNodes ( db, groupName )
{
   var groups = commGetGroups( db );
   for( var i = 0; i < groups.length; ++i )
   {
      if( groups[i][0].GroupName === groupName )
      {
         var nodes = [];
         for( var j = 1; j < groups[i].length; ++j )
         {
            nodes.push( {
               HostName: groups[i][j].HostName,
               svcname: groups[i][j].svcname,
               dbpath: groups[i][j].dbpath
            } );
         }
         return nodes;
      }
   }
   throw new Error( "lobGetGroupNodes: group not found: " + groupName );
}

// 计算某数据页在 lobd 文件中的字节偏移(pageID>=1 为数据页)
function lobDataPageOffset ( pageID )
{
   if( undefined == pageID ) { pageID = 1; }
   return LOB_DMS_HEADER_SZ + LOB_PAGE_SIZE * pageID + 100;
}

// dd 篡改指定主机上 lobd 的某数据页 1 字节(几乎必然与原字节不同 -> CRC 失配)。
// 经该主机 sdbcm 的 Remote Cmd 执行,节点在远端也能正确注入。
function lobCorruptNodeDataPage ( hostName, dbpath, csName, pageID, seq )
{
   if( undefined == seq ) { seq = 1; }
   var offset = lobDataPageOffset( pageID );
   var lobdFile = dbpath + "/" + csName + "." + seq + ".lobd";
   var cmd = new Remote( hostName, CMSVCNAME ).getCmd();
   cmd.run( "printf '\\xFF' | dd of=" + lobdFile + " bs=1 seek=" + offset +
            " count=1 conv=notrunc 2>/dev/null" );
}

// 停节点 -> dd 篡改 lobd 指定数据页 1 字节 -> 起节点。作用于该组所有副本,
// 使运行时 getLob(开启 read 校验)在任意主节点上都能触发 CRC 报错。
// dd 经各节点所在主机的 sdbcm 执行(Remote),支持节点分布在远端主机。
function lobCorruptDataPage ( db, groupName, csName, pageID, seq )
{
   var nodes = lobGetGroupNodes( db, groupName );
   var rg = db.getRG( groupName );
   for( var i = 0; i < nodes.length; ++i )
   {
      var node = nodes[i];
      var dnode = rg.getNode( node.HostName, node.svcname );
      dnode.stop();
      lobCorruptNodeDataPage( node.HostName, node.dbpath, csName, pageID, seq );
      dnode.start();
   }
   // 等待节点重新拉起、主选出
   sleep( 5000 );
}

// 对单个节点(需已停机)运行 sdbdmsdump inspect(仅 lob),返回输出文本。
// 停机运行避免读到打开中的文件。-C true 显式开启 lob 数据页 crc 校验
// (默认关闭以免拖慢常规 inspect)。指定 csName 时用 -c 只检该 CS,
// 避免扫到其它/系统 CS 导致 "Page CRC Check" 多段、解析错行。
// sdbdmsdump 经节点所在主机的 sdbcm 执行(Remote),支持远端节点。
function lobInspectNode ( hostName, dbpath, csName )
{
   var installPath = commGetRemoteInstallPath( hostName, CMSVCNAME );
   var cmd = new Remote( hostName, CMSVCNAME ).getCmd();
   var cmdStr = installPath + "/bin/sdbdmsdump -d " + dbpath +
                " -a inspect -b true -C true";
   if( undefined != csName )
   {
      cmdStr += " -c " + csName;
   }
   var out = cmd.run( cmdStr );
   return out;
}

// 从 inspect 输出解析 "Page CRC Check : Pass N, Fail M, NoCRC K" 的 Fail 数,
// 解析失败返回 -1。
function lobParseCrcFail ( inspectOut )
{
   var m = inspectOut.match( /Page CRC Check\s*:\s*Pass\s+(\d+),\s*Fail\s+(\d+),\s*NoCRC\s+(\d+)/ );
   if( null == m ) { return -1; }
   return parseInt( m[2] );
}
