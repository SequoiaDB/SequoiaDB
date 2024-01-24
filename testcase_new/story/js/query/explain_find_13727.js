/******************************************************************************
*@Description : seqDB-13727:访问计划带{Run:true/false}
*@Modify list :
*              2014-11-11 pusheng Ding  Init
*              2020-10-15 liuli
******************************************************************************/
testConf.clName = COMMCLNAME + "_13727";

main( test );

function test ( args )
{
   var cl = args.testCL;
   indexName = ""
   indexName_a = "a";
   scanType = "tbscan";
   scanType1 = "ixscan";

   //insert data
   commCreateIndex( cl, indexName_a, { a: 1 } );
   for( var i = 0; i < 5; i++ ) 
   {
      cl.insert( { a: i, b: i - 5, c: "abcdefghijkl" + i } );
   }

   //find().explain()
   //default is explain({Run:false})
   var explainObj = cl.find().explain();
   checkExplain( explainObj, indexName, scanType, 0, 0 );

   //find().explain({Run:true})
   var explainObj = cl.find().explain( { Run: true } );
   checkExplain( explainObj, indexName, scanType, 0, 5 );

   //find({ "a": 1 }).explain({Run:true})
   var explainObj = cl.find( { "a": 1 } ).explain( { Run: true } );
   checkExplain( explainObj, indexName_a, scanType1, 2, 1 );

   //find().explain({Run:false})
   //default is explain({Run:false})
   var explainObj = cl.find().explain( { Run: false } );
   checkExplain( explainObj, indexName, scanType, 0, 0 );
}

function checkExplain ( explainObj, indexName, scanType, indexRead, dataRead )
{
   assert.equal( explainObj.current().toObj().IndexName, indexName );
   assert.equal( explainObj.current().toObj().ScanType, scanType );
   assert.equal( explainObj.current().toObj().IndexRead, indexRead );
   assert.equal( explainObj.current().toObj().DataRead, dataRead );
   explainObj.close();
}
