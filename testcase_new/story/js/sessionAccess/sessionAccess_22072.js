/* *****************************************************************************
@description: seqDB-22072:设置会话访问属性preferedinstance/PreferedInstanceMode/PreferedStrict/PreferedPeriod的值，检查设置成功 
@author: 2018-1-29 wuyan  Init
***************************************************************************** */
testConf.skipStandAlone = true;

main( test );

function test ()
{
   //PreferedInstance为字母、PreferedInstanceMode为ordered、PreferedStrict为默认值、PreferedPeriod为默认值
   var options = { PreferedInstance: "M", PreferedInstanceMode: "ordered" };
   db.setSessionAttr( options );
   var expResult = { PreferedInstance: "M", PreferedInstanceMode: "ordered", PreferedStrict: false, PreferedPeriod: 60 };
   checkSessionAttr( db, expResult );

   //PreferedInstance为数字、PreferedInstanceMode为random、PreferedStrict为默认值、PreferedPeriod为-1
   var options = { PreferedInstance: 12, PreferedInstanceMode: "random", PreferedPeriod: -1 };
   db.setSessionAttr( options );
   var expResult = { PreferedInstance: 12, PreferedInstanceMode: "random", PreferedStrict: false, PreferedPeriod: -1 };
   checkSessionAttr( db, expResult );

   //PreferedInstance为数组、PreferedInstanceMode为默认值、PreferedStrict为true、PreferedPeriod为0
   var options = { PreferedInstance: [1, 2, 3], PreferedStrict: true, PreferedPeriod: 0 };
   db.setSessionAttr( options );
   var expResult = { PreferedInstance: [1, 2, 3], PreferedInstanceMode: "random", PreferedStrict: true, PreferedPeriod: 0 };
   checkSessionAttr( db, expResult );

   //PreferedInstance为数组、PreferedInstanceMode为默认值、PreferedStrict为false、PreferedPeriod为40
   var options = { PreferedInstance: [11, 22, 33], PreferedStrict: false, PreferedPeriod: 40 };
   db.setSessionAttr( options );
   var expResult = { PreferedInstance: [11, 22, 33], PreferedInstanceMode: "random", PreferedStrict: false, PreferedPeriod: 40 };
   checkSessionAttr( db, expResult );

   //PreferedPeriod为400000000000000000000000000
   var options = { PreferedPeriod: 400000000000000000000000000 };
   db.setSessionAttr( options );
   if( commIsArmArchitecture() )
   {
      var expResult = { PreferedInstance: [11, 22, 33], PreferedInstanceMode: "random", PreferedStrict: false, PreferedPeriod: 2147483647 };
   }
   else
   {
      var expResult = { PreferedInstance: [11, 22, 33], PreferedInstanceMode: "random", PreferedStrict: false, PreferedPeriod: -1 };
   }
   checkSessionAttr( db, expResult );
}

function checkSessionAttr ( db, expResult )
{
   var actResult = {};
   var object = db.getSessionAttr().toObj();
   actResult.PreferedInstance = object.PreferedInstance;
   actResult.PreferedInstanceMode = object.PreferedInstanceMode;
   actResult.PreferedStrict = object.PreferedStrict;
   actResult.PreferedPeriod = object.PreferedPeriod;
   assert.equal( actResult, expResult );
}
