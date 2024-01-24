import( "../lib/main.js" );
import( "../lib/basic_operation/commlib.js" );

//inspect the alter field is success or not.
function checkAlterResult ( clName, fieldName, expFieldValue, csName )
{
   if( csName == undefined ) { csName = COMMCSNAME; }
   var clFullName = csName + "." + clName;
   var cur = db.snapshot( 8, { "Name": clFullName } );
   var actualFieldValue = cur.current().toObj()[fieldName];
   assert.equal( expFieldValue, actualFieldValue );

}

//inspect the alter cs field is success or not.
function checkAlterCSResult ( csName, fieldName, expFieldValue )
{
   var rg = db.getRG( "SYSCatalogGroup" );
   var dbca = new Sdb( rg.getMaster() );
   var cur = dbca.SYSCAT.SYSCOLLECTIONSPACES.find( { "Name": csName } );
   while( cur.next() )
   {
      var tempinfo = cur.current().toObj();
      var actFieldValue = tempinfo[fieldName];
   }
   assert.equal( expFieldValue, actFieldValue );
}

/************************************
*@Description: check snapshot
*@author:      luweikang
*@createDate:  2018.4.25
**************************************/
function checkSnapshot ( db, snapType, csName, clName, field, expFieldValue )
{
   var clFullName = csName + "." + clName;
   var cursor = db.snapshot( snapType, { 'Name': clFullName } );
   var Obj = cursor.current().toObj();
   var actualFieldValue = Obj[field];
   assert.equal( expFieldValue, actualFieldValue );
}

/************************************
*@Description: get all groupName
*@author:      luweikang
*@createDate:  2018.4.25
**************************************/
function getGroupName ( db )
{
   var groupArr = db.listReplicaGroups();
   var dataRGNames = new Array();
   while( groupArr.next() )
   {
      var groupObj = groupArr.current().toObj();
      var groupID = groupObj.GroupID;
      if( groupID >= 1000 )
      {
         dataRGNames.push( groupObj.GroupName );
      }
   }
   return dataRGNames;
}

/************************************
*@Description: get Split Group
*@author:      luweikang
*@createDate:  2018.4.25
**************************************/
function getSplitGroup ( db, csName, clName )
{
   var clFullName = csName + "." + clName;
   var arr = getGroupName( db );
   var srcGroup = commGetCLGroups( db, clFullName )[0];
   var tarGroup = null;
   for( i in arr )
   {
      if( arr[i] !== srcGroup )
      {
         tarGroup = arr[i];
         break;
      }
   }
   var splitGroup = {};
   splitGroup.srcGroup = srcGroup;
   splitGroup.tarGroup = tarGroup;
   return splitGroup;
}

/************************************
*@Description: alter cl
*@author:      luweikang
*@createDate:  2018.4.25
**************************************/
function clSetAttributes ( cl, options )
{
   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      cl.setAttributes( options );
   } );
}

/* *****************************************************************************
@discription: insert data into cl
@author: wangkexin
@parameter
cl: the collection to be inserted
rownums: number of set data to be inserted
***************************************************************************** */
function insertData ( cl, rownums )
{
   var record = [];
   for( var i = 0; i < rownums; i++ )
   {
      record.push( { a: i, b: i, c: "sequoiadb alter test" } );
   }
   cl.insert( record );
}

/* *****************************************************************************
@discription: check split result from source group and target group
@author: wangkexin
@parameter
csName: the split collection space
clName: the split collection
srcGroupName: source group name
tarGroupName: target group name
expDataNum: Total number of data expected to be returned
***************************************************************************** */
function checkSplitResult ( csName, clName, srcGroupName, tarGroupName, expDataNum )
{
   var actDataNum = 0;

   var dataNode1 = new Sdb( db.getRG( srcGroupName ).getMaster() );
   var checkCL1 = dataNode1.getCS( csName ).getCL( clName );
   var recordNum1 = checkCL1.count();
   actDataNum = actDataNum + recordNum1;
   dataNode1.close();

   var dataNode2 = new Sdb( db.getRG( tarGroupName ).getMaster() );
   var checkCL2 = dataNode2.getCS( csName ).getCL( clName );
   var recordNum2 = checkCL2.count();
   assert.notEqual( recordNum2, 0 );

   actDataNum = actDataNum + recordNum2;
   dataNode2.close();
   assert.equal( actDataNum, expDataNum );
}

/* *****************************************************************************
@discription: check not split result from source group and target group
@author: wangkexin
@parameter
csName: the split collection space
clName: the split collection
srcGroupName: source group name
tarGroupName: target group name
expDataNum: Total number of data expected to be returned
***************************************************************************** */
function checkNotSplitResult ( csName, clName, srcGroupName, tarGroupName, expDataNum )
{
   var actDataNum = 0;

   var dataNode = new Sdb( db.getRG( srcGroupName ).getMaster() );
   var checkCL = dataNode.getCS( csName ).getCL( clName );
   actDataNum = actDataNum + checkCL.count();
   assert.equal( actDataNum, expDataNum );
   dataNode.close();

   var dataNode2 = new Sdb( db.getRG( tarGroupName ).getMaster() );
   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      dataNode2.getCS( csName ).getCL( clName );
   } );
}

function checkDomain ( db, domainName, expGroups, expAutoSplit, expAutoRebalance )
{
   var domainMsg = db.listDomains( { Name: domainName } ).current().toObj();
   actGroups = domainMsg.Groups;
   actAutoSplit = domainMsg.AutoSplit;
   actAutoRebalance = domainMsg.AutoRebalance;

   assert.equal( actGroups.length, expGroups.length );

   for( var i in expGroups )
   {
      var groupName = actGroups[i].GroupName;
      assert.notEqual( expGroups.indexOf( groupName ), -1 );
   }
   assert.equal( actAutoSplit, expAutoSplit );

   assert.equal( actAutoRebalance, expAutoRebalance );
}

function checkCL ( groupNames, csName, clName )
{
   assert.tryThrow( [-34, -23], function()
   {
      for( var i in groupNames )
      {
         var groupName = groupNames[i];
         var nodeName = db.getRG( groupName ).getMaster();
         var sdb = new Sdb( nodeName );
         sdb.getCS( csName ).getCL( clName );
      }
   } );
}

