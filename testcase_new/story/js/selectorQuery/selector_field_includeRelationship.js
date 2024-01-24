/******************************************************************************
 * @Description   : fields have included relations
*                   $elemMatch/$elemMatchOne/$slice/$default/$include
 * @Author        : xiaojun Hu
 * @CreateTime    : 2015.01.29
 * @LastEditTime  : 2021.02.04
 * @LastEditors   : Lai Xuan
 ******************************************************************************/

main( test );

function test ()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true );

   var recordNum = 1;
   var addRecord1 = { "nest1": { "nest2": { "nest3": ["a", "b"] } } };
   var addRecord2 = [{ "nest1": [{ "nest2": [{ "nest3": ["a", "b"] }] }] }];
   var addRecord3 = [{ "nest1": [{ "nest2": [{ "nest3": ["A", "B", "C", "D", "E", "F", "G", "H"] }] }] }];
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, true );
   // auto generate data
   selAutoGenData( cl, recordNum, addRecord1, addRecord2, addRecord3 );

   /*【Test Point 1.1】 {$include:1} */
   var condObj = {};
   var selObj = {
      "ExtraField1.nest1.nest2.nest3": { "$include": 1 },
      "ExtraField1.nest1.nest2": { "$include": 1 },
      "ExtraField1.nest1": { "$include": 1 },
      "ExtraField1": { "$include": 1 },
      "Group.HostName": { "$include": 1 },
      "Group.Service.Name": { "$include": 1 },
      "Group.Service": { "$include": 1 },
      "Group": { "$include": 1 },
      "Group.NodeID": { "$include": 1 },
      "ExtraField2.nest1.nest2.nest3": { "$include": 1 },
      "ExtraField2.nest1.nest2": { "$include": 1 },
      "ExtraField2.nest1": { "$include": 1 },
      "ExtraField2": { "$include": 1 }
   };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   if( 20000 != retObj["Group"][0]["Service"][0]["Name"] ||
      20001 != retObj["Group"][0]["Service"][1]["Name"] ||
      20002 != retObj["Group"][0]["Service"][2]["Name"] ||
      20003 != retObj["Group"][0]["Service"][3]["Name"] ||
      "Host_1" != retObj["Group"][0]["HostName"] || 1000 != retObj["Group"][0]["NodeID"] ||
      "a,b" != retObj["ExtraField1"]["nest1"]["nest2"]["nest3"] ||
      "a,b" != retObj["ExtraField2"][0]["nest1"][0]["nest2"][0]["nest3"] )
   {
      throw new Error( "ErrReturnRecord$include/1" );
   }
   /*【Test Point 1.2】 {$include:0}*/
   var condObj = {};
   var selObj = {
      "ExtraField1.nest1.nest2.nest3": { "$include": 0 },
      "ExtraField1.nest1.nest2": { "$include": 0 },
      "ExtraField1.nest1": { "$include": 0 },
      "ExtraField1": { "$include": 0 },
      "Group.HostName": { "$include": 0 },
      "Group.Service.Name": { "$include": 0 },
      "Group.Service": { "$include": 0 },
      "Group": { "$include": 0 },
      "Group.NodeID": { "$include": 0 },
      "ExtraField2.nest1.nest2.nest3": { "$include": 0 },
      "ExtraField2.nest1.nest2": { "$include": 0 },
      "ExtraField2.nest1": { "$include": 0 },
      "ExtraField2": { "$include": 0 }
   };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   if( 1 != retObj["GroupID"] || 1 != retObj["Version"] ||
      "A,B,C,D,E,F,G,H" != retObj["ExtraField3"][0]["nest1"][0]["nest2"][0]["nest3"] )
   {
      throw new Error( "ErrReturnRecord$include/0" );
   }

   /*Test Point 2 $default*/
   var condObj = {};
   var selObj = {
      "ExtraField1.nest1.nest2.nest3": { "$default": 0 },
      "ExtraField1.nest1.nest2": { "$default": 0 },
      "ExtraField1.nest1": { "$default": 0 },
      "ExtraField1": { "$default": 0 },
      "Group.HostName": { "$default": 0 },
      "Group.Service.Name": { "$default": 0 },
      "Group.Service": { "$default": 0 },
      "Group": { "$default": 0 },
      "Group.NodeID": { "$default": 0 },
      "ExtraField2.nest1.nest2.nest3": { "$default": 0 },
      "ExtraField2.nest1.nest2": { "$default": 0 },
      "ExtraField2.nest1": { "$default": 0 },
      "ExtraField2": { "$default": 0 }
   };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   if( 20000 != retObj["Group"][0]["Service"][0]["Name"] ||
      20001 != retObj["Group"][0]["Service"][1]["Name"] ||
      20002 != retObj["Group"][0]["Service"][2]["Name"] ||
      20003 != retObj["Group"][0]["Service"][3]["Name"] ||
      "Host_1" != retObj["Group"][0]["HostName"] ||
      1000 != retObj["Group"][0]["NodeID"] ||
      "a,b" != retObj["ExtraField1"]["nest1"]["nest2"]["nest3"] ||
      "a,b" != retObj["ExtraField2"][0]["nest1"][0]["nest2"][0]["nest3"] )
   {
      throw new Error( "ErrReturnRecord$defult" );
   }

   /*Test Point 3 $slice*/
   var condObj = {};
   var selObj = {
      "ExtraField1.nest1.nest2.nest3": { "$slice": 1 },
      "ExtraField2.nest1.nest2.nest3": { "$slice": 1 },
      "ExtraField2.nest1.nest2": { "$slice": 1 },
      "ExtraField2.nest1": { "$slice": 1 },
      "ExtraField2": { "$slice": 1 },
      "Group.Service.Name": { "$slice": 1 },
      "Group.Service": { "$slice": 1 },
      "Group": { "$slice": 1 },
      "Group.NodeID": { "$slice": 1 },
      "ExtraField3.nest1.nest2.nest3": { "$slice": 5 },
      "ExtraField3.nest1.nest2": { "$slice": 1 },
      "ExtraField3.nest1": { "$slice": 1 },
      "ExtraField3": { "$slice": 1 }
   };
   var ret = selMainQuery( cl, condObj, selObj );
   // verify
   var retObj = JSON.parse( ret );
   if( "Host_1" != retObj["Group"][0]["HostName"] ||
      0 != retObj["Group"][0]["Service"][0]["Type"] ||
      20000 != retObj["Group"][0]["Service"][0]["Name"] ||
      1000 != retObj["Group"][0]["NodeID"] ||
      1 != retObj["GroupID"] ||
      "a" != retObj["ExtraField1"]["nest1"]["nest2"]["nest3"] ||
      "a" != retObj["ExtraField2"][0]["nest1"][0]["nest2"][0]["nest3"] ||
      "A,B,C,D,E" != retObj["ExtraField3"][0]["nest1"][0]["nest2"][0]["nest3"] )
   {
      throw new Error( "ErrReturnRecord$slice" );
   }
}



