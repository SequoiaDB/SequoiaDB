/************************************
*@Description: insert data
*@author:      wuyan
*@createDate:  2018.1.22
**************************************/
import( "../lib/main.js" );

function insertData ( dbcl, number )
{
   if( undefined == number ) { number = 1000; }
   var docs = [];
   for( var i = 0; i < number; ++i )
   {
      var no = i;
      var a = i;
      var user = "test" + i;
      var phone = 13700000000 + i;
      var time = new Date().getTime();
      var doc = { no: no, a: a, customerName: user, phone: phone, openDate: time };
      //data example: {"no":5, customerName:"test5", "phone":13700000005, "openDate":1402990912105

      docs.push( doc );
   }
   dbcl.insert( docs );
}

/************************************
*@Description: create file by lob
*@author:      wuyan
*@createDate:  2018.10.12
**************************************/
function createFile ( fileName, fileSize )
{
   if( undefined == fileSize ) { fileSize = "100K"; }
   var cmd = new Cmd();
   var str = "dd if=/dev/zero of=" + fileName + " bs=" + fileSize + " count=1";
   cmd.run( str );

   var md5 = cmd.run( "md5sum " + fileName ).split( " " )[0];
   return md5;
}

/************************************
*@Description: delete file
*@author:      luweikang
*@createDate:  2018.11.06
**************************************/
function deleteFile ( fileName )
{
   var cmd = new Cmd();
   cmd.run( "rm -rf " + fileName );
}

/************************************
*@Description: put lobs
*@author:      wuyan
*@createDate:  2018.10.12
**************************************/
function putLobs ( cl, fileName, lobNum )
{
   if( undefined == lobNum ) { lobNum = 1; }

   var lobIdArr = [];
   for( var i = 0; i < lobNum; i++ )
   {
      var lobId = cl.putLob( fileName );
      lobIdArr.push( lobId );
   }
   return lobIdArr;
}

/************************************
*@Description: delete the lob
*@author:      wuyan
*@createDate:  2018.10.12
**************************************/
function deleteLobs ( cl, lobIdArr )
{
   for( var i in lobIdArr )
   {
      cl.deleteLob( lobIdArr[i] );
   }
}


/************************************
*@Description: check the lob
*@author:      wuyan
*@createDate:  2018.10.12
**************************************/
function checkLob ( cl, expLobArr, srcMd5 )
{
   var rc = cl.listLobs();

   //check Available
   var lobArr = [];
   while( rc.next() )
   {
      var lobInfo = rc.current().toObj();
      var lobId = lobInfo["Oid"]["$oid"];
      var isNormal = lobInfo["Available"];
      lobArr.push( lobId );
      assert.equal( isNormal, true );
   }

   //check lob number 
   assert.equal( lobArr.length, expLobArr.length );

   lobArr.sort();
   expLobArr.sort();
   //check lob Id
   for( var i in expLobArr )
   {
      assert.equal( lobArr[i], expLobArr[i] );
   }

   //get lob and check md5sum
   var fileName = CHANGEDPREFIX + "_lobtest_getlob.file";
   for( var i in lobArr )
   {
      var lobId = lobArr[i];
      cl.getLob( lobId, fileName, true );

      var cmd = new Cmd();
      var desMd5 = cmd.run( "md5sum " + fileName ).split( " " )[0];
      assert.equal( desMd5, srcMd5 );
      cmd.run( "rm -rf " + fileName );
   }
}


/************************************
*@Description: check the new cl name 
*@author:      wuyan
*@createDate:  2018.10.12
**************************************/
function checkRenameCLResult ( csName, oldCLName, newCLName )
{
   var clFullName = csName + "." + newCLName;
   var getNewCLName = db.snapshot( SDB_SNAP_COLLECTIONS, { "Name": clFullName } ).current().toObj().Name;
   assert.equal( getNewCLName, clFullName );
   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      db.getCS( csName ).getCL( oldCLName );
   } );
}

/************************************
*@Description: check the new maincl name 
*@author:      wuyan
*@createDate:  2018.10.12
**************************************/
function checkRenameMainCLResult ( maincs, subcs, newMainCLName, oldMainCLName, subclName )
{

   var subclFullName = subcs + "." + subclName;
   var newMainCLFullName = maincs + "." + newMainCLName;
   var getMainCLName = db.snapshot( 8, { "Name": subclFullName } ).current().toObj().MainCLName;
   assert.equal( getMainCLName, newMainCLFullName );

   //check the old maincl is not exist
   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   { db.getCS( maincs ).getCL( oldMainCLName ); } );
}

/************************************
*@Description: get group name and service name .
*@author:      wuyan
*@createDate:  2018/10/12
**************************************/
function getGroupName ( db, mustBePrimary )
{
   var RGname = db.listReplicaGroups().toArray();
   var j = 0;
   var arrGroupName = Array();
   for( var i = 1; i != RGname.length; ++i )
   {
      var eRGname = eval( '(' + RGname[i] + ')' );
      if( 1000 <= eRGname["GroupID"] )
      {
         arrGroupName[j] = Array();
         var primaryNodeID = eRGname["PrimaryNode"];
         var groups = eRGname["Group"];
         for( var m = 0; m < groups.length; m++ )
         {
            if( true == mustBePrimary )
            {
               var nodeID = groups[m]["NodeID"];
               if( primaryNodeID != nodeID )
                  continue;
            }
            arrGroupName[j].push( eRGname["GroupName"] );
            arrGroupName[j].push( groups[m]["HostName"] );
            arrGroupName[j].push( groups[m]["Service"][0]["Name"] );
            break;
         }
         ++j;
      }
   }
   return arrGroupName;
}

/************************************
*@Description: get SrcGroup name,update getPG to getSrcGroup
*@author��wuyan 2018/10/12
**************************************/
function getSrcGroup ( csName, clName )
{

   var clFullName = csName + "." + clName;
   var clInfo = db.snapshot( 8, { Name: clFullName } );
   while( clInfo.next() )
   {
      var clInfoObj = clInfo.current().toObj();
      var srcGroupName = clInfoObj.CataInfo[0].GroupName;
   }
   return srcGroupName;
}

/************************************
*@Description: get SrcGroup and TargetGroup info,the groups information
               include GroupName,HostName and svcname
*@author��wuyan 2018/10/12
*@return array[][] ex:
        [0]
           {"GroupName":"XXXX"}
           {"HostName":"XXXX"}
           {"svcname":"XXXX"}
        [N]
           ...
**************************************/
function getSplitGroups ( csName, clName, targetGrMaxNums )
{
   var allGroupInfo = getGroupName( db, true );
   var srcGroupName = getSrcGroup( csName, clName );
   var splitGroups = new Array();
   if( targetGrMaxNums >= allGroupInfo.length - 1 )
   {
      targetGrMaxNums = allGroupInfo.length - 1;
   }
   var index = 1;

   for( var i = 0; i != allGroupInfo.length; ++i )
   {
      if( srcGroupName == allGroupInfo[i][0] )
      {
         splitGroups[0] = new Object();
         splitGroups[0].GroupName = allGroupInfo[i][0];
         splitGroups[0].HostName = allGroupInfo[i][1];
         splitGroups[0].svcname = allGroupInfo[i][2];
      }
      else
      {
         if( index > targetGrMaxNums )
         {
            continue;
         }
         splitGroups[index] = new Object();
         splitGroups[index].GroupName = allGroupInfo[i][0];
         splitGroups[index].HostName = allGroupInfo[i][1];
         splitGroups[index].svcname = allGroupInfo[i][2];
         index++;
      }
   }
   return splitGroups;

}

function splitCL ( dbcl, srcGroupName, dstGroupName )
{
   dbcl.split( srcGroupName, dstGroupName, 50 );
}

/************************************
*@Description: create maincl .
*@author:      wuyan
*@createDate:  2018/10/12
**************************************/
function createMainCL ( csName, mainCLName, shardingKey )
{
   var options = { ShardingKey: shardingKey, IsMainCL: true, ReplSize: 0 };
   var mainCL = commCreateCL( db, csName, mainCLName, options, false,
      true, "Failed to create mainCL." );
   return mainCL;
}

function createCL ( csName, clName, shardingKey, shardingType )
{
   if( typeof ( shardingType ) == "undefined" ) { shardingType = "hash"; }

   var options = { ShardingKey: shardingKey, ShardingType: shardingType, ReplSize: 0, Compressed: true };
   var dbcl = commCreateCL( db, csName, clName, options, true,
      true, "Failed to create cl." );
   return dbcl;
}


/************************************
*@Description: check the new cs name 
*@author:      luweikang
*@createDate:  2018.10.13
**************************************/
function checkRenameCSResult ( oldCSName, newCSName, clNum )
{
   if( undefined == clNum ) { clNum = 1; }
   //max cycle
   var maxTime = 5000;
   //current cycle
   var currentTime = 0;
   //sleep time��100 millisecond
   var intervalTime = 100;

   //because some dataNode none complete sync��so add retry for 5 seconds
   var clArray = getCSSnapshotCLArray( newCSName );
   while( clArray.length !== clNum && currentTime < maxTime )
   {
      sleep( 100 );
      currentTime += intervalTime;
      clArray = getCSSnapshotCLArray( newCSName );
   }
   assert.notEqual( currentTime, maxTime, "check cl num time out, it took five seconds " + clNum );

   //when the cl num expected results are met��check the cl name
   for( i = 0; i < clArray.length; i++ )
   {
      var csname = clArray[i].Name.split( "." )[0];
      assert.equal( csname, newCSName );
   }

   //check the old cl is not exist
   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      db.getCS( oldCSName );
   } );
}

function getCSSnapshotCLArray ( newCSName )
{
   var newCSObj = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { "Name": newCSName } ).current().toObj();
   var getNewCSName = newCSObj.Name;
   assert.equal( getNewCSName, newCSName );

   return newCSObj.Collection;
}

function checkRenameSubCLResult ( maincs, mainCLName, subcs, oldSubCLName, newSubCLName )
{
   var newSubCLFullName = subcs + "." + newSubCLName;
   var mainCLFullName = maincs + "." + mainCLName;
   var subCLInfo = db.snapshot( 8, { "Name": newSubCLFullName } ).current().toObj();
   var getMainCLName = subCLInfo.MainCLName;
   assert.equal( getMainCLName, mainCLFullName );

   var getSubCLName = subCLInfo.Name;
   assert.equal( getSubCLName, newSubCLFullName );

   //check the old subcl is not exist
   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      db.getCS( subcs ).getCL( oldSubCLName );
   } );
}
