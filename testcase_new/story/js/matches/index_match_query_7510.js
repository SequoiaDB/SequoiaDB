/*******************************************************************************
*@Description : create index and query. query match $type/$exists/$elemmathc/
*                                                   /$+标识符/$size/$regex
*                                                   $and/$not/$or
*@Modify list :
*               2014-5-20  xiaojun Hu  Init
                2020-08-13 Zixian Yan  Modify
*******************************************************************************/
testConf.clName = COMMCLNAME + "_7510";
main( test );

function test ( testPara )
{
   var insertNum = 10;
   var cl = testPara.testCL;
   // insert data
   idxAutoGenData( cl, insertNum );

   // create Index
   var idxName = "noIndex";
   var indexDef = { "no": 1, "no1": 1, "no2": -1, "no3": 1 };
   commCreateIndex( cl, idxName, indexDef );
   commCheckIndexConsistency( cl, idxName, true );

   // query data
   var queryCond = {
      $and: [{ "string": { "$type": 1, $et: 2 } },
      { "obj_id": { "$exists": 1 } },
      { "subobj": { "$elemMatch": { "obj": { "val": "sub" } } } },
      { "array.$1": "5arr5" },
      { "array": { "$size": 1, "$et": 4 } },
      {
         "string": {
            "$regex": "西边个喇嘛，东边个哑巴*",
            "$options": "i"
         }
      }]
   };
   idxQueryCheck( cl, queryCond, 1, idxName );

   var queryCond = {
      $not: [{
         $and: [{ "string": { "$type": 1, $et: 2 } },
         { "obj_id": { "$exists": 1 } },
         { "subobj": { "$elemMatch": { "obj": { "val": "sub" } } } },
         { "array.$1": "5arr5" },
         { "array": { "$size": 1, "$et": 4 } },
         {
            "string": {
               "$regex": "西边个喇嘛，东边个哑巴*",
               "$options": "i"
            }
         }]
      }]
   };
   idxQueryCheck( cl, queryCond, 9, idxName );

   var queryCond = {
      $or: [{
         $not: [{
            $and: [{ "string": { "$type": 1, $et: 2 } },
            { "obj_id": { "$exists": 1 } },
            { "subobj": { "$elemMatch": { "obj": { "val": "sub" } } } },
            { "array.$1": "5arr5" },
            { "array": { "$size": 1, "$et": 4 } },
            {
               "string": {
                  "$regex": "西边个喇嘛，东边个哑巴*",
                  "$options": "i"
               }
            }]
         }]
      },
      {
         $and: [{ "string": { "$type": 1, $et: 2 } },
         { "obj_id": { "$exists": 1 } },
         { "subobj": { "$elemMatch": { "obj": { "val": "sub" } } } },
         { "array.$1": "5arr5" },
         { "array": { "$size": 1, "$et": 4 } },
         {
            "string": {
               "$regex": "西边个喇嘛，东边个哑巴*",
               "$options": "i"
            }
         }]
      }
      ]
   };
   idxQueryCheck( cl, queryCond, 10, idxName );

}
