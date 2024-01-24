/* *****************************************************************************
@Description: seqDB-382:手工部署集群（独立模式部署)
@author:      Zhao Xiaoni
*******************************************************************************/
main( test );

function test()
{
   var port = generatePort( COORDSVCNAME );
   var db = createDb( COORDHOSTNAME, port, true );
   
   var clName = COMMCLNAME + "_382";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );
   
   insertData( cl, 1 );
   
   commDropCL( db, COMMCSNAME, clName, false, false );
   clean( db, COORDHOSTNAME, port, undefined, "standalone_data" );
   db.close();
}
