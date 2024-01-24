import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

function checkCollectionStat( cl, clName, version, IsDefault, IsExpired, AvgNumFields, SampleRecords, TotalRecords, TotalDataPages, TotalDataSize ){
   var actResult = cl.getCollectionStat().toObj() ;
   delete( actResult.StatTimestamp ) ;
   var expResult = {
      "Collection": COMMCSNAME + "." + clName,
      "InternalV": version,
      "IsDefault": IsDefault ,
      "IsExpired": IsExpired ,
      "AvgNumFields": AvgNumFields ,
      "SampleRecords": SampleRecords ,
      "TotalRecords": TotalRecords ,
      "TotalDataPages": TotalDataPages ,
      "TotalDataSize":TotalDataSize
   } ;
   if( !commCompareObject( expResult, actResult ) ){
      throw new Error( "\nExpected:\n" + JSON.stringify( expResult ) + "\nactual:\n" + JSON.stringify( actResult ) );
   }
}