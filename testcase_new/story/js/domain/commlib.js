/******************************************************************************
*@Description : Public function for testing domain.
*@Modify list :
*               2014-6-17  xiaojun Hu  Init
******************************************************************************/
var csName = CHANGEDPREFIX + "_dom_cs";
var clName = CHANGEDPREFIX + "_dom_cl";
import( "../lib/main.js" );
import( "../lib/basic_operation/commlib.js" );

// Insert data to SequoiaDB
function insertData ( db, csName, clName, insertNum )
{
   if( undefined == insertNum ) { insertNum = 1000; }
   var data = [];
   var cs = db.getCS( csName );
   var cl = cs.getCL( clName );
   for( var i = 0; i < insertNum; ++i )
   {
      var no = i;
      var user = "布斯" + i;
      var phone = 13700000000 + i;
      var compa1 = "MI" + i;
      var compa2 = "GOLDEN" + i;
      var date = new Date();
      var time = date.getTime();
      var card = 6800000000 + i;
      /**********************************************************************
      data expampl : {"No":5, customerName:"布斯5","phoneNumber":13700000005,
                      "company":[MI5,GOLDEN5],"openDate":1402990912105,
                      "cardID":6800000005}
      **********************************************************************/
      var insertString = { "No": no, "customerName": user, "phoneNumber": phone, "company": [compa1, compa2], "openDate": time, "cardID": card };
      data.push( insertString );
   }
   //insert date
   cl.insert( data );

   // inspect the data is insert success or not
   var cnt = 0;
   do
   {
      var count = cl.count();
      if( insertNum == count )
         break;
      ++cnt;
   } while( cnt < 100 );
   if( insertNum != count )
   {
      throw new Error( "ErrNumRecord" );
   }
}

// Query the data
function queryData ( db, csName, clName )
{
   var cs = db.getCS( csName );
   var cl = cs.getCL( clName );
   /**********************************************************************
   data expampl : {"No":5, customerName:"布斯5","phoneNumber":13700000005,
                   "company":[MI5,GOLDEN5],"openDate":1402990912105,
                   "cardID":6800000005}
   query data and get quantity.
   **********************************************************************/
   var query =
      cl.find( {
         $and: [{
            "No": { $gte: 50 }, "phoneNumber": { $lt: 13700001000 },
            "customerName": { $nin: ["MI500", "GOLDEN500"] },
            "cardID": { $lte: 6800001111 },
            "openDate": { $gt: 1402990912105 }
         }]
      } ).count();
   assert.equal( 950, query, "ErrQueryNum" );
}

// Update the data
function updateData ( db, csName, clName )
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
   var count = cl.find( {
      "No": 88, "customerName": "布斯",
      "phoneNumber": 13700000088, "cardID": 6800000093,
      "company": ["MI88", "GOLDEN88", 2, 3]
   } ).count();
   if( 1 != count )
   {
      throw new Error( "ErrUpdateData" );
   }
}
// Remove data
function removeData ( db, csName, clName )
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
   do
   {
      var count = cl.count();
      if( 89 == count )
         break;
      ++cnt;
   } while( cnt <= 20 );
   if( 89 != count )
   {
      throw new Error( "ErrRemoveData" );
   }

}
// Get group from Sdb
function getGroup ( db )
{
   var listGroups = db.listReplicaGroups();
   var groupArray = new Array();
   while( listGroups.next() )
   {
      if( listGroups.current().toObj()['GroupID'] >= DATA_GROUP_ID_BEGIN )
      {
         groupArray.push( listGroups.current().toObj()["GroupName"] );
      }
   }
   return groupArray;

}

// Get domains from Sdb
function getDomains ( db )
{
   var listDom = db.listDomains();
   var domArray = new Array();
   while( listDom.next() )
   {
      domArray.push( listDom.current().toObj()["Name"] );
   }
   return domArray;
}

// Clear domain in the beginning or in the end
function clearDomain ( db, domName )
{
   var getDoms = new Array();
   getDoms = getDomains( db );
   if( 0 == getDoms.length )
   {
      return;
   }
   for( var i = 0; i < getDoms.length; ++i )
   {
      if( domName == getDoms[i] )
      {
         var domCSarr = new Array();
         var dom = db.getDomain( domName );
         domCS = dom.listCollectionSpaces();
         while( domCS.next() )
         {
            domCSarr.push( domCS.current().toObj()["Name"] );
         }
         if( "" != domCSarr )
         {
            for( var j = 0; j < domCSarr.length; ++j )
            {
               db.dropCS( domCSarr[j] );
            }
         }
         commDropDomain( db, getDoms[i] );
      }
   }
   // inspect the domain was droped or not
   var clearDom = new Array();
   clearDom = getDomains( db );
   for( var i = 0; i < clearDom.length; ++i )
   {
      if( domName == clearDom[i] )
      {
         throw new Error( "ErrClearDomain" );
      }
   }
}

// Inspect parameter : AutoSplit. {AutoSplit : true}
function inspectAutoSplit ( db, csName, clName, domName )
{
   // get group where collection locate in
   var catCL = csName + "." + clName;
   var lsCatCL = db.snapshot( 8, { Name: catCL } );
   //var lsCatCL = db.snapshot(8) ;
   var lsCatCLarr = new Array();
   while( lsCatCL.next() )
   {
      lsCatCLarr.push( lsCatCL.current().toObj()["CataInfo"][0]["GroupName"] );
   }
   var clGroup = lsCatCLarr[0];
   // get group in domain
   var domGroup = db.listReplicaGroups();
   var domGroupArr = new Array();
   while( domGroup.next() )
   {
      domGroupArr.push( domGroup.current().toObj()["GroupName"] );
   }
   for( var i = 0; i < domGroupArr.length; ++i )
   {
      var domRG = domGroupArr[i];
      if( clGroup == domRG )
      {
         // get master node in domain group
         var rg = db.getRG( domGroupArr[i] );
         var master = rg.getMaster();
         master = master.toString();
         var masterNode = new Array();
         mastNode = master.split( ":" );
         dataNode = new Sdb( mastNode[0], mastNode[1] );
         // inspect the count is greate than 0
         var cs = dataNode.getCS( csName );
         var cl = cs.getCL( clName );
         var k = 0;
         do
         {
            var count = cl.count();
            if( count > 0 )
               break;
            ++k;
         } while( k < 10 );
         if( count <= 0 )
         {
            throw new Error( "ErrDomAutoSplit" );
         }
      }
   }

}

// createCS and make sure located in one group
function getCSGroup ( db, csName, clName )
{
   var name = csName + "." + clName;
   var catQuery = db.snapshot( 8 );
   var csGroup = new Array();
   var catCsName = new Array();
   while( catQuery.next() )
   {
      var tmpObj = catQuery.current().toObj();
      if( typeof ( tmpObj["CataInfo"] ) == "undefined" ) { continue; } // main cl
      if( tmpObj["CataInfo"].length == 0 ) { continue; }
      csGroup.push( tmpObj["CataInfo"][0]["GroupName"] );
      catCsName.push( tmpObj["Name"] );
   }
   var group = "";
   for( var i = 0; i < catCsName.length; ++i )
   {
      if( name == catCsName[i] )
      {
         group = csGroup[i];
      }
   }
   if( "" == group )
   {
      throw new Error( "ErrGetCSGroup" );
   }
   return group;
}


// Inspect the run mode
function inspectRunMode ( db )
{
   try
   {
      db.listReplicaGroups();
      return "group";
   }
   catch( e )
   {
      if( -159 == e )
      {
         return "standalone";
      }
      else
      {
         throw e;
      }
   }
}
