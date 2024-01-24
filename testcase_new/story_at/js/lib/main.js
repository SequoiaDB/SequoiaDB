/*******************************************************************************
*@Description: JavaScript function main template
*@Modify list:
*   2019-11-12 wenjing Wang  Init
*******************************************************************************/

/*
入参配置项：
testConf.skipTest = true;                       跳过测试，在commlib.js中指定可跳过当前目录下的所有用例，用于屏蔽用例使用
testConf.skipStandAlone = true;                 跳过独立模式
testConf.skipOneGroup = true;                   跳过只有一个组的环境
testConf.skipGroupLessThanThree = true;         跳过数据组小于三个的环境，指定为true时skipOneGroup将被忽略不生效
testConf.skipOneDuplicatePerGroup = true;       跳过每组一个节点的环境
testConf.skipExistOneNodeGroup = true;          跳过存在一个节点复制组的环境，指定为true时skipOneDuplicatePerGroup将被忽略不生效
testConf.useSrcGroup = true;                    存储创建的 cl 所在组
testConf.useDstGroup = true;                    存储创建的 cl 不在的组
testConf.csName  = COMMCSNAME + "_xxx";         指定框架创建的 cs 名
testConf.csOpt = {};                            指定创建的 cs 配置项
testConf.clName = COMMCLNAME + "_xxx";          指定框架创建的 cl 名
testConf.clOpt = {};                            指定创建的 cl 配置项
testConf.testGroups = null;                     用于测试分组，对相同分组的用例统一修改配置(只能用于串行用例，或修改不影响其他用例的配置，如：setSessionAttr)

出参：
function test(testPara) {}
testPara.groups           获取数据组的信息，没有前置要求，可直接获取
testPara.testCS           获取创建的 cs，不指定 testConf.csName 则获取公共cs
testPara.testCL           获取创建的 cl，需要指定 testConf.clName
testPara.srcGroupName     获取创建的 cl 所在组，需要指定 testConf.clName,testConf.useSrcGroup = true
testPara.dstGroupNames    获取创建的 cl 不在的组，需要指定 testConf.clName,testConf.useDstGroup = true
*/

var testConf = {
   skipStandAlone: false, skipOneDuplicatePerGroup: false, skipOneGroup: false,
   useSrcGroup: false, useDstGroup: false, skipGroupLessThanThree: false, skipExistOneNodeGroup: false,
   skipTest: false, testGroups: null
};
// e.g. testConf.csName = COMMCSNAME, testConf.csOpt = {PageSize:4096} ;
// e.g. testConf.clName = COMMCLNAME, testConf.clOpt = {AutoSplit:true} ;
// e.g. testConf.useSrcGroup = true  设置为true获取CL所在组；设置true后在测试方法中获取，如test( arg ){ arg.srcGroupName ...}
// e.g. testConf.useDstGroup = true  设置为true返回CL所在组外的所有组，设置true后在测试方法中获取，如test( arg ){ arg.dstGroupNames ...}

var testPara = {};

var recycleBinConf = {};
var oneGroup = 1;
var nodeNum = 1;
var threeGroup = 3;
function checkEnv ( db, testConf )
{
   if( testConf.skipTest )
   {
      throw new Error( "skip test" );
   }
   if( testConf.skipStandAlone && commIsStandalone( db ) )
   {
      throw new Error( "standalone" );
   }
   testPara.groups = commGetGroups( db );
   if( testConf.skipGroupLessThanThree )
   {
      if( testPara.groups.length < threeGroup )
      {
         throw new Error( "group less than three" );
      }
   }
   else if( testConf.skipOneGroup )
   {
      if( testPara.groups.length === oneGroup )
      {
         throw new Error( "one data group" );
      }
   }

   if( testConf.skipExistOneNodeGroup )
   {
      for( var i = 0; i < testPara.groups.length; ++i )
      {
         if( testPara.groups[i].length - 1 == nodeNum )
         {
            break;
         }
      }

      if( i != testPara.groups.length )
      {
         throw new Error( "exist one node group" );
      }
   }
   else if( testConf.skipOneDuplicatePerGroup )
   {
      for( var i = 0; i < testPara.groups.length; ++i )
      {
         if( testPara.groups[i].length - 1 > nodeNum )
         {
            break;
         }
      }

      if( i === testPara.groups.length )
      {
         throw new Error( "one duplicate per group" );
      }
   }
}

function buildDomainContainGroups ()
{
   if( testConf.DomainUseGroupNum == undefined ) { testConf.DomainUseGroupNum = testPara.groups.length }
   var dmGroupNames = [];
   for( var i = 0; i < testPara.groups.length; ++i )
   {
      var groupName = testPara.groups[i][0].GroupName;
      if( groupName !== CATALOG_GROUPNAME && groupName !== COORD_GROUPNAME && groupName !== SPARE_GROUPNAME )
      {
         dmGroupNames.push( testPara.groups[i][0].GroupName );
      }

      if( dmGroupNames.length == testConf.DomainUseGroupNum )
      {
         break;
      }
   }
   return dmGroupNames;
}

function getDstGroupName ( db )
{
   var srcGroupName = commGetCLGroups( db, testConf.csName + "." + testConf.clName );
   var dataGroupNames = commGetDataGroupNames( db );
   var dstGroupName = [];
   for( var i = 0; i < dataGroupNames.length; i++ )
   {
      if( srcGroupName.indexOf( dataGroupNames[i] ) === -1 )
      {
         dstGroupName.push( dataGroupNames[i] );
      }
   }
   return dstGroupName;
}

function createTestCS ( db, testConf )
{
   if( testConf.csName !== undefined )
   {
      if( testConf.csOpt !== undefined )
      {
         if( testConf.csOpt.Domain !== undefined )
         {
            testPara.dmGroupNames = buildDomainContainGroups();
            commCreateDomain( db, testConf.csOpt.Domain, testPara.dmGroupNames, testConf.domainOpt, true );
         }
         return commCreateCS( db, testConf.csName, true, "", testConf.csOpt );
      }
      else
      {
         return commCreateCS( db, testConf.csName, true );
      }
   }
   else
   {
      testConf.csName = COMMCSNAME;
      try
      {
         return db.getCS( testConf.csName );
      }
      catch( e )
      {
         if( commCompareErrorCode( e, SDB_DMS_CS_NOTEXIST ) )
         {
            println( "cs not exist" );
         }
         else
         {
            throw e;
         }
      }
   }
}

function createTestCL ( db, testConf )
{
   if( testConf.clName !== undefined )
   {
      var objCl = null;
      if( testConf.clName !== COMMCLNAME )
      {
         commDropCL( db, testConf.csName, testConf.clName, true, true );
      }

      if( testConf.clOpt !== undefined )
      {
         objCl = commCreateCL( db, testConf.csName, testConf.clName, testConf.clOpt, true, true );
      }
      else
      {
         objCl = commCreateCL( db, testConf.csName, testConf.clName );
      }

      if( testConf.useSrcGroup )
      {
         testPara.srcGroupName = commGetCLGroups( db, testConf.csName + "." + testConf.clName );
         if( testPara.srcGroupName.length == 1 )
         {
            testPara.srcGroupName = testPara.srcGroupName[0];
         }
      }

      if( testConf.useDstGroup )
      {
         testPara.dstGroupNames = getDstGroupName( db );
      }
      return objCl;
   }
}

function dropTestCS ( db, testConf )
{
   if( testConf.csName !== undefined &&
      testConf.csName !== COMMCSNAME )
   {
      commDropCS( db, testConf.csName, true );

      if( testConf.skipStandAlone !== true && testConf.csOpt !== undefined && testConf.csOpt.Domain !== undefined )
      {
         commDropDomain( db, testConf.csOpt.Domain, true );
      }
   }
}

function dropTestCL ( db, testConf )
{
   if( testConf.clName !== undefined &&
      testConf.clName !== COMMCLNAME &&
      testConf.csName === COMMCSNAME )
   {
      commDropCL( db, testConf.csName, testConf.clName, true, true );
   }
}

function initTestGroups ( db, testGroups )
{
   if( testGroups == null )
   {
      return;
   }
   else if( testGroups.indexOf( "recycleBin" ) > -1 )
   {
      // 将回收站配置设置为默认配置
      recycleBinConf = db.getRecycleBin().getDetail().toObj();
      db.getRecycleBin().alter( { "Enable": true, "ExpireTime": 4320, "MaxItemNum": 100, "MaxVersionNum": 2, "AutoDrop": false } );
   }
}

function finiTestGroups ( testGroups )
{
   var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   try
   {
      if( testGroups == null )
      {
         return;
      }
      else if( testGroups.indexOf( "recycleBin" ) > -1 )
      {
         // 恢复回收站配置为执行用例前的配置
         db.getRecycleBin().alter( recycleBinConf );
      }
   } finally
   {
      if( db !== undefined )
      {
         db.close();
      }
   }
}

function commonSetUp ( db, testConf )
{
   checkEnv( db, testConf );
   initTestGroups( db, testConf.testGroups );

   testPara.testCS = createTestCS( db, testConf );
   testPara.testCL = createTestCL( db, testConf );
}

function commonTearDown ( db, testConf )
{
   if( db !== undefined )
   {
      dropTestCL( db, testConf );
      dropTestCS( db, testConf );
      db.close();
   }
}

function main ()
{
   var isExecSuccess = false;
   try
   {
      commonSetUp( db, testConf );
      var numArgs = arguments.length;
      for( var i = 0; i < numArgs; ++i )
      {
         if( typeof ( arguments[i] ) === "function" )
         {
            arguments[i]( testPara );
         }
      }
      isExecSuccess = true;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         if( e.message === "standalone" ||
            e.message === "one data group" ||
            e.message === "one duplicate per group" ||
            e.message === "group less than three" ||
            e.message === "exist one node group" ||
            e.message === "skip test" )
         {
            return;
         }
         println( e.stack );
      }
      throw e;
   }
   finally
   {
      if( isExecSuccess )
      {
         commonTearDown( db, testConf );
      }
      finiTestGroups( testConf.testGroups );
   }
}
