/******************************************************************************
*@Description : Public function for testing split.
*@Modify list : 2014-6-17  xiaojun Hu  Init
                2015-10-15  Yan wu  modify
******************************************************************************/

var csName = COMMCSNAME;
var clName = COMMCLNAME;

// Get data group and split
function splitGroup ( db, csName, inserNum )
{
   try
   {
      var groups = new Array();
      groups = commGetGroups( db, "GroupName", "", true, true );
      var csGroup = new Array();
      csGroup = commGetCSGroups( db, csName );
      var dNum = groups.length - 1;
      var cs = db.getCS( csName );
      var cl = cs.getCL( clName );
      var conNum = inserNum / dNum;
      for( var i = 0; i < groups.length; ++i )
      {
         cl.split( csGroup[0], groups[i], { No: conNum } );
         conNum = conNum * 2;
      }
   }
   catch( e )
   {
      println( "Failed to split, rc = " + e );
      throw e;
   }
}

//Insert data to SequoiaDB,no check insert result
/************************************
*@Description: Insert data to SequoiaDB
*@author：wuyan 2015/10/14
**************************************/
function insertData ( db, csName, clName, insertNum )
{
   if( undefined == insertNum ) { insertNum = 1000; }

   try
   {
      var doc = [];
      var cs = db.getCS( csName );
      var cl = cs.getCL( clName );
      println( "--Begin to insert data" );
      for( var i = 0; i < insertNum; ++i )
      {
         var no = i;
         var user = "布斯" + i;
         var phone = 13700000000 + i;
         var date = new Date();
         var time = date.getTime();
         var card = 6800000000 + i;
         var no2 = i; //added for multi sharding key use
         /**********************************************************************
         data expampl : {"No":5, customerName:"布斯5","phoneNumber":13700000005,
                         "openDate":1402990912105,"cardID":6800000005}
         **********************************************************************/
         //println( "Start to deal string" ) ;
         var insertString = "{\"no\":" + no + ",\"customerName\":\"" + user +
            "\",\"phoneNumber\":" + phone + ",\"openDate\":" + time + ",\"cardID\":" + card + "}";
         //println( "String + " + insertString ) ;
         var insert = eval( "(" + insertString + ")" );
         // insert date        
         doc.push( insert );
      }
      cl.insert( doc );
      println( "--end insert data" )
   }
   catch( e )
   {
      println( "Failed to insert data to Sdb, rc = " + e );
      throw e;
   }
}


// Query the data
function queryData ( db, csName, clName )
{
   try
   {
      var cs = db.getCS( csName );
      var cl = cs.getCL( clName );
      /**********************************************************************
      data expampl : {"No":5, customerName:"布斯5","phoneNumber":13700000005,
                      "company":[MI5,GOLDEN5],"openDate":1402990912105,
                      "cardID":6800000005}
      query data and get quantity.
      **********************************************************************/
      sleep( 20 );
      var query =
         cl.find( {
            $and: [{
               "No": { $gte: 50 }, "phoneNumber": { $lt: 13700001000 },
               "customerName": { $nin: ["MI500", "GOLDEN500"] },
               "cardID": { $lte: 6800001111 },
               "openDate": { $gt: 1402990912105 }
            }]
         } ).count();
      if( 950 != query )
      {
         println( "Wrong query the data, count = " + query );
         throw "ErrQueryNum";
      }
      println( "Success to query the data" );
   }
   catch( e )
   {
      println( "Failed to query data, rc = " + e );
      throw e;
   }
}

// Update the data
function updateData ( db, csName, clName )
{
   try
   {
      var cs = db.getCS( csName );
      var cl = cs.getCL( clName );
      /**********************************************************************
      data expampl : {"No":5, customerName:"布斯5","phoneNumber":13700000005,
                      "company":[MI5,GOLDEN5],"openDate":1402990912105,
                      "cardID":6800000005}
      query data and get quantity.
      **********************************************************************/
      cl.update( {
         $inc: { "cardID": 5 }, $set: { "customerName": "布斯" },
         $unset: { "openDate": "" }, $addtoset: { company: [2, 3] },
         $pull_all: { "company": ["MI88", "GOLDEN88", 2, 3] }
      } );
      sleep( 20 );
      var count = cl.find( {
         "No": 88, "customerName": "布斯",
         "phoneNumber": 13700000088, "cardID": 6800000093,
         "company": ["MI88", "GOLDEN88", 2, 3]
      } ).count();
      if( 1 != count )
      {
         println( "Wrong update the data, count = " + count );
         throw "ErrUpdateData";
      }
      println( "Success to update the data." );
   }
   catch( e )
   {
      println( "Failed to update the data, rc = " + e );
      throw e;
   }
}

// Remove data
function removeData ( db, csName, clName )
{
   try
   {
      var cs = db.getCS( csName );
      var cl = cs.getCL( clName );
      /**********************************************************************
      data expampl : {"No":5, customerName:"布斯5","phoneNumber":13700000005,
                      "company":[MI5,GOLDEN5],"openDate":1402990912105,
                      "cardID":6800000005}
      query data and get quantity.
      **********************************************************************/
      cl.remove( { "No": { $gte: 89 } }, { "": "$id" } );
      var cnt = 0;
      sleep( 20 );
      do
      {
         var count = cl.count();
         if( 84 == count )
            break;
         ++cnt;
      } while( cnt <= 20 );
      if( 89 != count )
      {
         println( "Wrong remove the date, count = " + count );
         throw "ErrRemoveData";
      }
      println( "Success to remove the data." );
   }
   catch( e )
   {
      println( "Failed to remove the data, rc = " + e );
      throw e;
   }
}

// Get source group and destination group
function getTwoGroupSplit ( db, csName, clName, splitArg1, splitArg2 )
{
   try
   {
      // get collection
      var cs = db.getCS( csName );
      var cl = cs.getCL( clName );

      var listGroups = db.listReplicaGroups();
      var listGroupsArr = new Array();

      // Check over arguement "splitArg1" "splitArg2"
      if( "" == splitArg1 || undefined == splitArg1 )
      {
         println( "Wrong argument." );
         throw "ErrArg";
      }

      // argument : when the split is percent
      var argument = "";
      if( undefined == splitArg2 || "" == splitArg2 ) { argument = splitArg1; }
      // Get group where Collection Space located in
      while( listGroups.next() )
      {
         if( listGroups.current().toObj()["GroupID"] >= DATA_GROUP_ID_BEGIN )
         {
            listGroupsArr.push( listGroups.current().toObj()["GroupName"] );
         }
      }
      var groupNum = listGroupsArr.length;

      var snapShotCl = db.snapshot( SDB_SNAP_COLLECTIONS );
      var snapShotClName = new Array();
      var snapShotClGroup = new Array();
      var group = "";
      while( snapShotCl.next() )
      {
         snapShotClName.push( snapShotCl.current().toObj()["Name"] );
         snapShotClGroup.push( snapShotCl.current().toObj()["Details"][0]["GroupName"] );
      }
      var clname = csName + "." + clName;
      for( var i = 0; i < snapShotClGroup.length; i++ )
      {
         if( snapShotClName[i] == clname )
         {
            group = snapShotClGroup[i];
            break;
         }
      }
      if( "" == group )
      {
         println( "Failed to get Group where CL located in, snapshotCl = "
            + snapShotCl );
         throw "ErrGetGroup";
      }
      println( "The source group = " + group );
      // Get the other group where split to
      var groupSplit = "";
      var i = 0;
      do
      {
         if( group != listGroupsArr[i] )
         {
            groupSplit = listGroupsArr[i];
            break;
         }
         ++i;

      } while( i <= groupNum || i <= 8 );
      if( "" == groupSplit )
      {
         println( "Failed to get Split Group, Groups = " + listGroups );
         throw "ErrGetSplitGroup";
      }
      println( "The destination [split]group = " + groupSplit );
      //println( "Argument : " + argument ) ;
      if( "" == argument )
         cl.splitAsync( group, groupSplit, splitArg1, splitArg2 );
      else
         cl.splitAsync( group, groupSplit, argument );
      println( "Success to Split" );

   }
   catch( e )
   {
      println( "Failed to get the group " + e );
      throw e;
   }
}

/************************************
*@Description: split between the two groups,after split return nodeSvcName and nodeHostName 
*@author：wuyan 2015/10/14
**************************************/
function splitCl ( dbCL, csName, clName, condition1, condition2 )
{
   if( undefined == condition2 )

      var allGroupInfo = getGroupName( db, true )
   var srcGroupName = getSrcGroup( csName, clName );

   //get the nodesvcname of srcgroup into    
   var nodeSvcNames = [];
   var nodeHostNames = [];
   for( var i = 0; i != allGroupInfo.length; ++i )
   {
      if( srcGroupName == allGroupInfo[i][0] )
      {
         println( "allGroupInfo[i].GroupName=" + allGroupInfo[i][0] )
         nodeHostNames.push( allGroupInfo[i][1] );
         nodeSvcNames.push( allGroupInfo[i][2] );
         break;
      }
   }

   println( "--begin split" )


   for( var i = 0; i != allGroupInfo.length; ++i )
   {
      if( srcGroupName != allGroupInfo[i][0] )
      {
         try
         {
            dbCL.split( srcGroupName, allGroupInfo[i][0], condition1, condition2 );
            nodeHostNames.push( allGroupInfo[i][1] );
            nodeSvcNames.push( allGroupInfo[i][2] );
            break;
         }
         catch( e )
         {
            throw e;
         }
      }
   }
   println( "--end split" )
   return [nodeSvcNames, nodeHostNames];
}

// Get group from Sdb
function getGroup ( db )
{
   try
   {
      var listGroups = db.listReplicaGroups();
      var groupArray = new Array();
      while( listGroups.next() )
      {
         if( listGroups.current().toObj()["GroupID"] >= DATA_GROUP_ID_BEGIN )
         {
            groupArray.push( listGroups.current().toObj()["GroupName"] );
         }
      }
      return groupArray;
   }
   catch( e )
   {
      println( "Failed to get groups from sdb, rc = " + e );
      throw e;
   }
}

/************************************
*@Description: check the conditions is ok before run testcase
*@author：wuyan 2015/10/14
*@parameter:
   noSplit: group name filter
   exceptCata : default true
   exceptCoord: default true
**************************************/
function runPreCheck ( clName, csName, isSplit )
{
   if( undefined == clName ) { clName = COMMCLNAME; }
   if( undefined == csName ) { csName = COMMCSNAME; }
   if( undefined == isSplit ) { isSplit = true; }
   if( true == commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }

   //less two groups no split
   var allGroupName = getGroupName( db, true );
   println( "allGroupName.length=" + allGroupName.length )
   if( 1 === allGroupName.length )
   {
      println( "--least two groups" );
      return;
   }
   //two groups 
   if( 2 === allGroupName.length && true !== isSplit )
   {
      println( "--least three groups" );
      return;
   }

   //@ clean before
   commDropCL( db, COMMCSNAME, clName, true, true,
      "drop CL in the beginning" );
}

/************************************
*@Description: get SrcGroup name,update getPG to getSrcGroup
*@author：wuyan 2015/10/14
**************************************/
function getSrcGroup ( csName, clName )
{
   try
   {
      var clFullName = csName + "." + clName;
      var clInfo = db.snapshot( 8, { Name: clFullName } );
      while( clInfo.next() )
      {
         var clInfoObj = clInfo.current().toObj();
         var srcGroupName = clInfoObj.CataInfo[0].GroupName;
      }
      println( csName + "." + clName + "'s source group: " + srcGroupName );

      return srcGroupName;
   }
   catch( e )
   {
      println( "failed to get source group, cl name: " + clFullName );
      throw e;
   }
}

/************************************
*@Description: get SrcGroup and TargetGroup info,the groups information
               include GroupName,HostName and svcname
*@author：wuyan 2015/10/14
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

/************************************
*@Description: get Group name and Service name
*@author：wuyan 2015/10/20
**************************************/
/*function getGroupName(db)
{
   try
   {
      var RGname = db.listReplicaGroups().toArray();
   }
   catch (e)
   {
      throw e;
   }
   var j = 0;
   var arrGroupName = Array();
   for ( var i=1 ; i != RGname.length ; ++i )
   {
      var eRGname = eval('('+RGname[i]+')') ;
      if( 1000 <= eRGname["GroupID"] )
      {
         arrGroupName[j] = Array();
         arrGroupName[j].push(eRGname["GroupName"]) ;
         arrGroupName[j].push( eRGname["Group"][0]["Service"][0]["Name"] );
         ++j;
      }
   }
   return arrGroupName;
}*/

//*****get GroupName ,return array**************
//eg : arrGroupName[?][0] == "GroupName"
//eg : arrGroupName[?][1] == "HostName"
//     arrGroupName[?][2] == "Service"
function getGroupName ( db, mustBePrimary )
{
   var RGname = null;
   try
   {
      RGname = db.listReplicaGroups().toArray();
   }
   catch( e )
   {
      throw e;
   }
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
*@Description: check the hash cl split result
*@author：wuyan 2015/10/20
*@parameter:
   dataNodeInfo:svcname and hostName
**************************************/
function checkHashClSplitResult ( clName, dataNodeInfo )
{
   var total = 0;
   for( var i = 0; i != dataNodeInfo.length; ++i )
   {
      try
      {
         var gdb = new Sdb( dataNodeInfo[i].HostName, dataNodeInfo[i].svcname );
         var cl = gdb.getCS( COMMCSNAME ).getCL( clName )
         var num = cl.count();
         println( "--the find count of " + dataNodeInfo[i].HostName + ":" + dataNodeInfo[i].svcname + " is " + num );
         total += num;

         if( Number( num ) === 0 )
         {
            throw buildException( "checkHashClSplitResult()", "check datasNum wrong", "count()", "!=0", num );
         }
      }
      catch( e )
      {
         throw e;
      }
      finally
      {
         if( gdb !== undefined )
         {
            gdb.close();
            gdb = undefined;
         }
      }
   }
   //check the insert datas total
   if( Number( total ) !== 1000 )
   {
      throw buildException( "checkHashClSplitResult()", "datas total wrong", "total count()", 1000, total );
   }
}

/************************************
*@Description: insert data after split
*@author：wuyan 2015/10/20
**************************************/
function insertDataAfterHashClSplit ( dbCL )
{
   var e = -1;
   try
   {
      dbCL.insert( { no: 1000, test: "intoTheTargetGroup" } )
      var num = dbCL.count( { no: 1000, test: "intoTheTargetGroup" } );
      if( Number( num ) !== 1 )
      {
         throw buildException( "insertDataAfterHashClSplit", "check datasNum", "check insertdata after split Result", 1, num );
      }
   }
   catch( e )
   {
      throw e;
   }
}

/************************************
*@Description: rangecl to split result,check the datas count of the splitgroups
*@author：wuyan 2015/10/20
*@parameter:
   dataNodeInfo:svcname and hostName
**************************************/
function checkRangeClSplitResult ( clName, dataNodeInfo )
{
   for( var i = 0; i != dataNodeInfo.length; ++i )
   {
      try
      {
         var gdb = new Sdb( dataNodeInfo[i].HostName, dataNodeInfo[i].svcname );

         var cl = gdb.getCS( COMMCSNAME ).getCL( clName )
         var num = cl.count( { $and: [{ no: { $gte: 500 * i } }, { no: { $lt: 500 * ( i + 1 ) } }] } );
         println( "--the find count of " + dataNodeInfo[i].HostName + ":" + dataNodeInfo[i].svcname + " is " + num );

         if( Number( num ) !== 500 )			
         {
            var operator = "count({$and:[{no:{$gte:" + 500 * i + "}},{no:{$lt:" + 500 * ( i + 1 ) + "}}]})";
            throw buildException( "checkRangeClSplitResult()", "count wrong", operator, 500, num )
         }
      }
      catch( e )
      {
         throw e;
      }
      finally
      {
         if( gdb !== undefined )
         {
            gdb.close();
            gdb = undefined;
         }
      }
   }
}

/************************************
*@Description: the rangecl,insert data after split
*@author：wuyan 2015/10/20
**************************************/
function insertDataAfterRangeClSplit ( dbCl, clName, dataNodeInfo )
{
   try
   {
      dbCl.insert( { no: 500, test: "intoTheTargetGroup" } )
      var gdb = new Sdb( dataNodeInfo[1].HostName, dataNodeInfo[1].svcname );
      var cl = gdb.getCS( COMMCSNAME ).getCL( clName )
      var num = cl.find( { no: 500, test: "intoTheTargetGroup" } ).count();
      if( Number( num ) !== 1 )
      {
         throw buildException( "insertDataAfterRangeClSplit()", -1, "check insertdata after split Result", 1, num );
      }
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      if( gdb !== undefined )
      {
         gdb.close();
         gdb = undefined;
      }
   }
}

/************************************
*@Description: check the datas count of the every splitgroup,comparison of actusl count values and expected count values
*@author：wuyan 2015/10/20
**************************************/
function checkSplitResultToEveryGr ( clName, dataNodeInfo, ArrResult )
{
   for( var i = 0; i != dataNodeInfo.length; ++i )
   {
      try
      {
         var gdb = new Sdb( dataNodeInfo[i].HostName, dataNodeInfo[i].svcname );
         var cl = gdb.getCS( COMMCSNAME ).getCL( clName )
         var num = cl.count();
         println( "--the find count of " + dataNodeInfo[i].HostName + ":" + dataNodeInfo[i].svcname + " is " + num )

         var expectResult = ArrResult[i];

         if( Number( num ) !== expectResult )			
         {
            throw buildException( "checkClSplitResult()", "count wrong", "count()", expectResult, num )
         }
      }
      catch( e )
      {
         throw e;
      }
      finally
      {
         if( gdb !== undefined )
         {
            gdb.close();
            gdb == undefined;
         }
      }
   }
}
/************************************
*@Description: split cl with different options
*@author：Qiangzhong Deng 2015/10/20
*@parameters:
             objectCL: a collection reference(the the clName string)
             sourceGroup:source group
             targetGroup:target group
             startCondition:start condition of split,if the typeof is number,then percentage split,if the typeof is object, 
             then range split
             endCondition:object of end condition
**************************************/
function splitCL ( objectCL, sourceGroup, targetGroup, startCondition, endCondition )
{
   if( typeof ( objectCL ) !== "object" )
   {
      throw "split commlib.js:splitCL: objectCL is not object";
   }
   try
   {
      if( typeof ( startCondition ) === "number" ) //percentage split
      {
         objectCL.split( sourceGroup, targetGroup, startCondition );
      }
      else if( typeof ( startCondition ) === "object" && endCondition === undefined ) //range split without end condition
      {
         objectCL.split( sourceGroup, targetGroup, startCondition );
      }
      else if( typeof ( startCondition ) === "object" && typeof ( endCondition ) === "object" ) //range split with end condition
      {
         objectCL.split( sourceGroup, targetGroup, startCondition, endCondition );
      }
      else
      {
         throw buildException( "wrong parameters:" +
            " objectCL: " + objectCL +
            " sourceGroup: " + sourceGroup +
            " targetGroup: " + targetGroup +
            " startCondition: " + startCondition +
            " endCondition: " + endCondition );
      }
   }
   catch( e )
   {
      throw e;
   }
}

/************************************
*@Description: get the informations of the srcGroups and targetGroups,then split cl with different options,
               only split 1 times
               return the informations of the srcGroups and targetGroups
*@author：Yan Wu 2015/10/26
*@parameters:
             startCondition:start condition of split,if the typeof is number,then percentage split,if the typeof is object, 
             then range split
             endCondition:object of end condition
*@return array[][] ex:
        [0]
           {"GroupName":"XXXX"}
           {"HostName":"XXXX"}
           {"svcname":"XXXX"}           
        [N]            
**************************************/
function ClSplitOneTimes ( csName, clName, startCondition, endCondition )
{
   try
   {
      var targetGroupNums = 1;
      var groupsInfo = getSplitGroups( csName, clName, targetGroupNums );
      var srcGrName = groupsInfo[0].GroupName;
      println( "srcGrName=" + srcGrName )
      var tarGrName = groupsInfo[1].GroupName;
      var CL = db.getCS( csName ).getCL( clName );
      println( "--begin split" )
      if( typeof ( startCondition ) === "number" ) //percentage split
      {
         CL.split( srcGrName, tarGrName, startCondition );
      }
      else if( typeof ( startCondition ) === "object" && endCondition === undefined ) //range split without end condition
      {
         CL.split( srcGrName, tarGrName, startCondition );
      }
      else if( typeof ( startCondition ) === "object" && typeof ( endCondition ) === "object" ) //range split with end condition
      {
         CL.split( srcGrName, tarGrName, startCondition, endCondition );
      }
      println( "--end split" )
   }
   catch( e )
   {
      throw e;
   }
   return groupsInfo;
}

/************************************
*@Description: get the informations of the srcGroups and targetGroups,then split cl with different options,
               larger 2 groups to split,from srcGroup to different targetGroups,only split 2 times
               return the informations of the srcGroups and targetGroups
*@author：Yan Wu 2015/10/26
*@parameters:
             startCondition:start condition of split,if the typeof is number,then percentage split,if the typeof is object,
             then range split
             endCondition:object of end condition
*@return array[][] ex:
        [0]
           {"GroupName":"XXXX"}
           {"HostName":"XXXX"}
           {"svcname":"XXXX"}
        [N]
**************************************/
/*function ClSplitManyTimes( csName, clName, startCondition, endCondition )
{
	try
	{
	   var targetGroupNums = 2;
      var groupsInfo = getSplitGroups(csName,clName,targetGroupNums);
      var srcGrName = groupsInfo[0].GroupName;
      var tarGrName = groupsInfo[1].GroupName;
      var CL = db.getCS(csName).getCL(clName);
      println("--begin split")
      var splitTimes=2;
      for( var i=1; i <= splitTimes; ++i)
	   {
		   if ( typeof(startCondition) === "number" ) //percentage split
		   {
			   CL.split( srcGrName, tarGrName, startCondition );
		   }
		   else if ( typeof(startCondition) === "object" && endCondition === undefined ) //range split without end condition
		   {
			   CL.split( srcGrName, tarGrName, startCondition );
		   }
   		else if ( typeof(startCondition) === "object" && typeof(endCondition) === "object" ) //range split with end condition
   		{
   			CL.split( srcGrName, tarGrName, startCondition, endCondition );
   		}
		println("--end split")
	}
	catch ( e )
	{
		throw e;
	}
	return groupsInfo;
}*/