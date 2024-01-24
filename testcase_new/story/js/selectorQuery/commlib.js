import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

/*******************************************************************************
*@Description : auto generate json record and insert to DB
*@Para:
*       cl: the handle of collection[object]
*       recordNum: the number of record[object]
*       addRecord1: extra record 1 that you define[object]
*       addRecord2: extra record 2 that you define[object]
*@Modify list :
*               2015-01-26  xiaojun Hu  Init
*******************************************************************************/
function selAutoGenData ( cl, recordNum, addRecord1, addRecord2, addRecord3, addRecord4 )
{
   if( undefined == recordNum ) { recordNum = 10; }
   var node_id = 1000;
   var svcname = 20000;
   for( var i = 1; i <= recordNum; ++i )
   {
      var nstLayer = 0;
      if( 0 == i % 3 )
         nstLayer = 3;
      else if( 0 == i % 5 )
         nstLayer = 5;
      else if( 0 == i % 7 )
         nstLayer = 7;
      else
         nstLayer = 1;
      var roleNum = parseInt( Math.random() * ( 3 - 0 ) + 0 );
      if( 0 == roleNum )
      {
         rgName = "DataGroup" + i;
         rgFolder = "data";
      }
      else if( 1 == roleNum )
      {
         rgName = "CoordGroup" + i;
         rgFolder = "coord";
      }
      else
      {
         rgName = "CatalogGroup" + i;
         rgFolder = "cata";
      }
      var mixID = node_id;
      var recordRG = new Array();
      for( var j = 0; j < parseInt( nstLayer ); ++j )
      {
         var hostnum = parseInt( j + 1 );
         var hostname = "Host_" + hostnum;
         var path = "/opt/sequoiadb/database/" + rgFolder + "/" + svcname;
         var sname0 = svcname;
         var sname1 = parseInt( svcname ) + 1;
         var sname2 = parseInt( svcname ) + 2;
         var sname3 = parseInt( svcname ) + 3;
         recordRG[j] = {
            "HostName": hostname,
            "dbpath": path,
            "Service": [{ "Type": 0, "Name": sname0 },
            { "Type": 1, "Name": sname1 },
            { "Type": 2, "Name": sname2 },
            { "Type": 3, "Name": sname3 }],
            "NodeID": node_id
         };
         node_id++;
         svcname += 10;
      }
      var maxID = parseInt( node_id ) + 1;
      var primNode = parseInt( Math.random() * ( ( maxID - 1 ) - mixID ) + mixID );
      var stat = parseInt( Math.random() * ( 1 - 0 ) + 0 );
      if( undefined == addRecord1 && undefined == addRecord2 )
      {
         var Record = {
            "Group": recordRG,
            "GroupID": i,
            "PrimaryNode": primNode,
            "Role": roleNum,
            "Status": stat,
            "Version": i
         };
      }
      else
      {
         var Record = {
            "Group": recordRG,
            "GroupID": i,
            "PrimaryNode": primNode,
            "Role": roleNum,
            "Status": stat,
            "Version": i,
            "ExtraField1": addRecord1,
            "ExtraField2": addRecord2,
            "ExtraField3": addRecord3,
            "ExtraField4": addRecord4
         };
      }
      cl.insert( Record );   // insert record
   }
   var cnt = 0;
   while( recordNum != cl.count() && 1000 > cnt )
   {
      cnt++;
      sleep( 3 );
   }
   assert.equal( cl.count(), recordNum );
}

function selMainQuery ( cl, condObj, selObj, expectResult )
{
   if( undefined == condObj ) { condObj = {}; }
   if( undefined == selObj ) { selObj = {}; }
   var condCnt = cl.find( condObj ).count();
   var selCnt = cl.find( condObj, selObj ).toArray();
   // simple verify
   assert.equal( condCnt, selCnt.length );

   return selCnt;
}

function selVerifyIncludeRet ( queryRet, selObj, includeValue, retNum )
{
   if( "object" != typeof ( selObj ) ) { selObj = JSON.parse( selObj ) }
   //if( "undefined" == typeof(retNum) ) { retNum = "NONUM" }
   //var record= queryRet.toArray() ;
   retNum = retNum.split( ":" );
   for( var k = 0; k < queryRet.length; ++k )
   {
      var recordObj = JSON.parse( queryRet[k] );
      var RetNum = new Array();
      var j = 0;
      for( var i in selObj )
      {
         var includeField = i;
         RetNum[j] = staticTraverseField( recordObj, includeField );
         j++;
      }
      for( var i = 0; i < retNum.length; ++i )
      {
         if( "NONUM" != retNum[i] &&
            parseInt( RetNum[i] ) == parseInt( retNum[i] ) )
            continue;
         else if( "NONUM" == retNum[i] && parseInt( RetNum[i] ) >= 1 )
            continue;
         else
         {
            throw new Error( "VerificationFailed" );
         }
      }
   }
}

function staticTraverseField ( selRetObj, includeField, initValue, passField )
{
   if( undefined == initValue ) { initValue = 0; }
   if( "object" != typeof ( selRetObj ) ) { selRetObj = JSON.parse( selRetObj ) }
   var existCnt = 0;
   var splitInc = includeField.split( "." );
   if( 0 != initValue )
   {
      var nestField = passField;
   }
   else
   {
      var nestField = selRetObj[splitInc[0]];
   }
   for( var i = initValue; i < splitInc.length; ++i )
   {
      var j = 0;
      var checkField = nestField;
      if( undefined != checkField )  // exp: {"GroupID":{$include:0}}
      {
         while( undefined != checkField[j] )
         {
            nestField = checkField[j];
            nestField = nestField[splitInc[i + 1]];
            ++j;
            if( undefined != nestField && i != ( splitInc.length - 2 ) )
            {
               existCnt = staticTraverseField( selRetObj, includeField,
                  ( i + 1 ), nestField );
               nestField = checkField;
            }
            else if( i == ( splitInc.length - 2 ) )
            {
               if( undefined != nestField )
               {
                  existCnt++;
               }
            }
            else
               continue;
         }
      }
      else
      {
         continue;
      }
      if( j > 0 )
      {
         nestField = selRetObj[splitInc[i + 1]];
         //continue ;
      }
      else
      {
         if( 0 < i )
         {
            nestField = nestField[splitInc[i]];
         }
         if( undefined != nestField && i == ( splitInc.length - 1 ) )
         {
            existCnt++;
         }
      }
   }
   return existCnt;
}

function selVerifyNonSelectorObj ( cl, retObj, condObj, selectObj )
{
   //if( "object" != typeof(retObj) ){ retObj = JSON.parse( retObj ); }
   if( "object" != typeof ( condObj ) ) { condObj = JSON.parse( condObj ); }
   if( "object" != typeof ( selectObj ) ) { selectObj = JSON.parse( selectObj ); }
   var query = cl.find( condObj ).toArray();
   for( var i = 0; i < query.length; ++i )
   {
      var queryObj = JSON.parse( query[i] );
      var queryStr = JSON.stringify( queryObj );
      var retObject = JSON.parse( retObj[i] );
      var retStr = JSON.stringify( retObject );
      for( var j in queryObj )
      {
         for( var m in retObject )
         {
            for( var k in selectObj )
            {
               var firstField = k.split( '.' );
               firstField = firstField[0];
               if( j == firstField )
               {
                  var fieldValue = JSON.stringify( queryObj[j] );
                  queryStr = queryStr.replace( fieldValue, "" );
               }
               if( m == firstField )
               {
                  var fieldValue = JSON.stringify( retObject[m] );
                  retStr = retStr.replace( fieldValue, "" );
               }
            }
         }
      }
      if( queryStr != retStr )
      {
         throw new Error( "NotEqualString" );
      }
      else
      {
         continue;
      }
   }
}
