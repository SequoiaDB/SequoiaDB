/*******************************************************************************
*@Description: JavaScript function main template
*@Modify list:
*   2019-11-12 wenjing Wang  Init
*******************************************************************************/

var testConf = {
   skipStandAlone: false, skipOneDuplicatePerGroup: false,
   skipOneGroup: false, clean: false, message: "", skip: false,
   useSrcGroup: false, useDstGroup: false
};
// e.g. testConf.csName = COMMCSNAME, testConf.csOpt = {PageSize:4096}} };
// e.g. testConf.clName:COMMCLNAME, testConf.clOpt:{AutoSplit:true} } ;
// e.g. testConf.useSrcGroup:true  返回源组，一般用于创建CL时指定组；设置true后在测试方法中获取源组，如test( arg ){ arg.srcGroupName ...}
// e.g. testConf.useDstGroup:true  返回目标组，一般用于切分；设置为true返回除源组外的所有组，在测试方法中获取源组，如test( arg ){ arg.dstGroupNames ...}

testConf.clean = CLEANFORFAIL;
var testPara = {};

var oneGroup = 1;
var nodeNum = 1;
function checkEnv ( db, testConf )
{
   if( testConf.skipStandAlone && commIsStandalone( db ) )
   {
      throw new Error( "standalone" );
   }

   testPara.groups = commGetGroups( db );
   if( testConf.skipOneGroup )
   {

      if( testPara.groups.length === oneGroup )
      {
         throw new Error( "one data group" );
      }
   }

   if( testConf.skipOneDuplicatePerGroup )
   {
      if( testPara.groups === undefined )
      {
         testPara.groups = commGetGroups( db );
      }

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

function createCommonCS ( db )
{
   return commCreateCS( db, COMMCSNAME, true );
}

function createCommonCL ( db )
{
   return commCreateCL( db, COMMCSNAME, COMMCLNAME, { ShardingType: 'hash', ShardingKey: { _id: 1 }, AutoSplit: true }, true, true );
}

function createDummyCL ( db )
{
   return commCreateCL( db, COMMCSNAME, COMMDUMMYCLNAME, { ShardingType: 'hash', ShardingKey: { _id: 1 }, AutoSplit: true }, true, true );
}

function buildDomainContainGroups ()
{
   if( testConf.DomainUseGroupNum === undefined ) { testConf.DomainUseGroupNum === testPara.groups.length }
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

var dataGroupNames = [];
function getAllDataGroupName ()
{
   if( dataGroupNames.length !== 0 )
   {
      return dataGroupNames;
   }

   for( var i = 0; i < testPara.groups.length; ++i )
   {
      var groupName = testPara.groups[i][0].GroupName;
      if( groupName !== CATALOG_GROUPNAME && groupName !== COORD_GROUPNAME
         && groupName !== SPARE_GROUPNAME )
      {
         dataGroupNames.push( groupName );
      }
   }

   return dataGroupNames;
}

function getSrcGroupName ()
{
   var dataGroupNames = getAllDataGroupName();
   var pos = Math.floor( Math.random() * dataGroupNames.length );
   return dataGroupNames[pos];
}

function getDstGroupName ( srcGroupName )
{
   var groupNames = [];
   var dataGroupNames = getAllDataGroupName();
   for( var i = 0; i < dataGroupNames.length; ++i )
   {
      if( dataGroupNames[i] !== srcGroupName )
      {
         groupNames.push( dataGroupNames[i] );
      }
   }
   return groupNames;
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
   }
}

function createTestCL ( db, testConf )
{
   if( testConf.clName !== undefined )
   {
      if( testConf.clName !== COMMCLNAME )
      {
         commDropCL( db, testConf.csName, testConf.clName, true, true, testConf.message );
      }

      if( testConf.clOpt !== undefined )
      {
         if( testConf.useSrcGroup )
         {
            testPara.srcGroupName = getSrcGroupName();
            testConf.clOpt.Group = testPara.srcGroupName;
         }

         if( testConf.useDstGroup )
         {
            testPara.dstGroupNames = getDstGroupName( testPara.srcGroupName );
         }
         return commCreateCL( db, testConf.csName, testConf.clName, testConf.clOpt, true, true );
      }
      else
      {
         return commCreateCL( db, testConf.csName, testConf.clName );
      }
   }
}

function dropTestCS ( db, testConf )
{
   if( testConf.csName !== undefined &&
      testConf.csName !== COMMCSNAME )
   {
      commDropCS( db, testConf.csName, true );

      if( testConf.csOpt.Domain !== undefined )
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
      commDropCL( db, testConf.csName, testConf.clName, true, true, testConf.message );
   }
}

function commonSetUp ( db, testConf )
{
   checkEnv( db, testConf );
   testPara.commonCS = createCommonCS( db );
   testPara.commonCL = createCommonCL( db );
   createDummyCL( db );

   testPara.testCS = createTestCS( db, testConf );
   testPara.testCL = createTestCL( db, testConf );
}

function commonTearDown ( db, testConf, isExecSuccess )
{
   if( db !== undefined )
   {
      if( isExecSuccess || testConf.clean )
      {
         dropTestCL( db, testConf );
         dropTestCS( db, testConf );
      }
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
            if( testConf.skip === false )
            {
               arguments[i]( testPara );
            }
         }
      }

      isExecSuccess = true;
   }
   catch( e )
   {
      if( e.constructor === Error )
      {
         if( e.message === "standalone" ||
            e.message === "one data group" ||
            e.message === "one duplicate per group" )
         {
            return;
         }
         println( e.stack );
      }
      throw e;
   }
   finally
   {
      commonTearDown( db, testConf, isExecSuccess );
   }
}
