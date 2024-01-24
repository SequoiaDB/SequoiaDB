/*******************************************************************************
@Description : Query common functions
@Modify list : 2014-5-20  xiaojun Hu  Init
               2020-10-10 XiaoNi Huang
*******************************************************************************/
import( "./basic_operation/commlib.js" );
import( "./main.js" );

var csName = COMMCSNAME;
var clName = COMMCLNAME;

function checkRec ( rc, expRecs )
{
   //get actual records to array
   var actRecs = [];
   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() );
   }

   //check count
   assert.equal( actRecs.length, expRecs.length );

   //check every records every fields
   for( var i in expRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];
      for( var f in expRec )
      {
         assert.equal( actRec[f], expRec[f] );
      }
   }
}

/*******************************************************************************
*@Description : common functions
*@Modify list :
*              2016/7/11 huangxiaoni
*******************************************************************************/
function readyCL ( clName )
{

   commDropCL( db, COMMCSNAME, clName, true, true );

   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, false );
   return cl;
}

/*******************************************************************************
*@Description : insert data of various data types
*@Modify list :
*              2019/3/1 wangkexin
*******************************************************************************/
function readyData ( cl )
{
   var record = [{ _id: 1, Key: 123 }, { _id: 2, Key: { "$numberLong": "3000000000" } }, { _id: 3, Key: 123.456 }, { _id: 4, Key: { $decimal: "123.456" } }, { _id: 5, Key: "value" }, { _id: 6, Key: { "$oid": "123abcd00ef12358902300ef" } }, { _id: 7, Key: true }, { _id: 8, Key: { "$date": "2012-01-01" } }, { _id: 9, Key: { "$timestamp": "2012-01-01-13.14.26.124233" } }, { _id: 10, Key: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } }, { _id: 11, Key: { "$regex": "^张", "$options": "i" } }, { _id: 12, Key: { "subobj": "value" } }, { _id: 13, Key: ["abc", 0, "def"] }, { _id: 14, Key: null }, { _id: 15, Key: { "$minKey": 1 } }, { _id: 16, Key: { "$maxKey": 1 } }];
   cl.insert( record );
}