/****************************************************
*@Description: [seqDB-22690] Rest driver support interface of "SDB_UPDATE_ONE" & "SDB_DELETE_ONE"
               Rest 驱动接口支持 "SDB_UPDATE_ONE" & "SDB_DELETE_ONE"
*@Author:   2020-08-22  Zixian Yan
****************************************************/
testConf.csName = COMMCSNAME + "_22690";
testConf.clName = COMMCLNAME + "_22690";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var data = [{ "FirstName": "Zixian", "LastName": "Yan" },
   { "FirstName": "Travis", "LastName": "Yan" }];
   cl.insert( data );
   var cs_clName = testConf.csName + "." + testConf.clName;

   // Update { "FirstName": "Zixian", "LastName": "Yan" } -> { "FirstName": "Travis", "LastName": "Yan" }
   var expectResult = [{ "FirstName": "Travis", "LastName": "Yan" },
   { "FirstName": "Travis", "LastName": "Yan" }];
   var curPara = ["cmd=update", "name=" + cs_clName, "updator={\"$set\": {\"FirstName\":\"Travis\"} }", "filter={\"LastName\":\"Yan\"}", "flag=SDB_UPDATE_ONE"];
   runCurl( curPara );
   checkRec( cl.find(), expectResult );

   // // Delete { "FirstName": "Cayron", "LastName": "Shin-chan" }
   var expectResult = [{ "FirstName": "Travis", "LastName": "Yan" }];
   var curPara = ["cmd=delete", "name=" + cs_clName, "deletor={\"FirstName\":\"Travis\"}", "flag=SDB_DELETE_ONE"];
   runCurl( curPara );
   checkRec( cl.find(), expectResult );
}

function checkRec ( rc, expRecs )
{
   //get actual records to array
   var actRecs = [];
   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() );
   }

   //check count
   if( actRecs.length !== expRecs.length )
   {
      throw new Error( "\n\nactual recs in cl= " + JSON.stringify( actRecs ) + "\nexpect recs= " + JSON.stringify( expRecs )
         + "\n\ncheck count throw:\nExpected Result: " + expRecs.length + "\nActually Result: " + actRecs.length );
   }

   //check every records every fields
   for( var i in expRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];
      for( var f in expRec )
      {
         if( JSON.stringify( actRec[f] ) !== JSON.stringify( expRec[f] ) )
         {
            throw new Error( "\n\nerror occurs in " + ( parseInt( i ) + 1 ) + "th record, in field '" + f +
               "'\n\nactual recs in cl= " + JSON.stringify( actRecs ) + "\n\nexpect recs= " + JSON.stringify( expRecs ) );
         }
      }
   }
}
