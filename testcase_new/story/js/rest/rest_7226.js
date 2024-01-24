/****************************************************
@description:   list groups, normal case
                testlink cases: seqDB-7226
@input:	
@expectation:   db.listReplicaGroups() result == rest [cmd=list groups]
@modify list:
                2015-4-3 Ting YU init   2016-3-16 XiaoNi Huang init
****************************************************/
var csName = COMMCSNAME;
var clName = COMMCLNAME + "_7226";

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   listgroupsAndCheck();
}

function listgroupsAndCheck ()
{
   var word = "list groups";
   tryCatch( ["cmd=" + word], [0], "listgroupsAndCheck: fail to run rest cmd=" + word );

   var returnByRest = info.slice( 14, info.length ).replace( /\s/g, "" );
   //14: a return message body '{ "errno": 0 }'
   var returnByDB = '';
   var rc = db.listReplicaGroups();
   while( rc.next() )
   {
      returnByDB += JSON.stringify( rc.current().toObj() );
   }

   if( returnByDB != returnByRest )
   {
      throw new Error( "returnByRest: " + returnByRest + "\nreturnByDB: " + returnByDB );
   }

}


