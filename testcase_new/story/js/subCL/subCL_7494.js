/*******************************************************************************
*@Description : attach multiple subCL, then insert, coverage of diffrent range
*@Modify List :
*   2015-03-09   xiaojun Hu   Init
*******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var SUBCLNAME1 = CHANGEDPREFIX + "_1";
   var SUBCLNAME2 = CHANGEDPREFIX + "_2";
   var SUBCLNAME3 = CHANGEDPREFIX + "_3";
   var SUBCLNAME4 = CHANGEDPREFIX + "_4";
   db.setSessionAttr( { PreferedInstance: "M" } );
   commDropCL( db, COMMCSNAME, SUBCLNAME1, true, true );
   commDropCL( db, COMMCSNAME, SUBCLNAME2, true, true );
   commDropCL( db, COMMCSNAME, SUBCLNAME3, true, true );
   commDropCL( db, COMMCSNAME, SUBCLNAME4, true, true );
   commDropCL( db, COMMCSNAME, CHANGEDPREFIX, true, true );
   db.setSessionAttr( { PreferedInstance: "M" } );
   var clOptionObj = {
      ShardingKey: { no: 1 }, ShardingType: "range", ReplSize: 0,
      Compressed: true, IsMainCL: true
   };
   var mainCL = commCreateCL( db, COMMCSNAME, CHANGEDPREFIX, clOptionObj,
      true, false );
   var subCL1 = commCreateCL( db, COMMCSNAME, SUBCLNAME1, {}, true, false );
   var subCL2 = commCreateCL( db, COMMCSNAME, SUBCLNAME2, {}, true, false );
   var subCL3 = commCreateCL( db, COMMCSNAME, SUBCLNAME3, {}, true, false );
   var subCL4 = commCreateCL( db, COMMCSNAME, SUBCLNAME4, {}, true, false );
   mainCL.attachCL( COMMCSNAME + "." + SUBCLNAME1, {
      LowBound: { no: 1 },
      UpBound: { no: 2500 }
   } );
   mainCL.attachCL( COMMCSNAME + "." + SUBCLNAME2, {
      LowBound: { no: 2500 },
      UpBound: { no: 5000 }
   } );
   mainCL.attachCL( COMMCSNAME + "." + SUBCLNAME3, {
      LowBound: { no: 5000 },
      UpBound: { no: 7500 }
   } );
   mainCL.attachCL( COMMCSNAME + "." + SUBCLNAME4, {
      LowBound: { no: 7500 },
      UpBound: { no: 10000 }
   } );
   // insert data
   for( var i = 1; i < 10000; ++i )
   {
      mainCL.insert( {
         no: i, "description": "testcase for main collection and sub " +
            "collection, testcase " + i
      } );
   }
   var explainQuery = mainCL.find( { $and: [{ no: { $gt: 5010 } }, { no: { $lt: 7490 } }] }
   ).explain( { Run: true } ).toArray();
   queryObj = JSON.parse( explainQuery );
   var clname = COMMCSNAME + "." + SUBCLNAME3;
   assert.equal( queryObj["SubCollections"][0]["Name"], clname );
   assert.equal( queryObj["SubCollections"][0]["ReturnNum"], 2479 );
   // clean in the end
   commDropCL( db, COMMCSNAME, SUBCLNAME1, false, false );
   commDropCL( db, COMMCSNAME, SUBCLNAME2, false, false );
   commDropCL( db, COMMCSNAME, SUBCLNAME3, false, false );
   commDropCL( db, COMMCSNAME, SUBCLNAME4, false, false );
   commDropCL( db, COMMCSNAME, CHANGEDPREFIX, false, false );
}