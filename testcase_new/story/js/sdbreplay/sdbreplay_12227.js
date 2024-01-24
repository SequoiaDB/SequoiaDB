/************************************************************************
*@Description: seqDB-12227:重放复制日志时指定LSN  
*@Author: 2019-7-3  xiaoni zhao init
************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var csName = COMMCSNAME;
   var clName = "clName_12227";
   var groupNames = getDataGroupNames();

   var cl = readyCL( csName, clName, { Group: groupNames[0] } );

   //get minLSN
   var cursor = db.list( SDB_SNAP_SYSTEM, { GroupName: groupNames[0] } );
   var svcName = cursor.current().toObj().Group[0].Service[0].Name;
   cursor = db.snapshot( 6, { ServiceName: svcName, RawData: true, IsPrimary: true } );
   var minLSN = cursor.current().toObj().CompleteLSN;

   var expDataArr = [];
   for( var i = 0; i < 100; i++ )
   {
      cl.insert( { a: i } );
      cl.insert( { a: 100 + i } );
      cl.update( { $set: { a: 200 + i } }, { a: i } );
      cl.remove( { a: 100 + i } );

      expDataArr.push( '"I","' + i + '"' );
      expDataArr.push( '"I","' + ( 100 + i ) + '"' );
      expDataArr.push( '"B","' + i + '"' );
      expDataArr.push( '"A","' + ( 200 + i ) + '"' );
      expDataArr.push( '"D","' + ( 100 + i ) + '"' );
   }

   cursor = db.snapshot( 6, { ServiceName: svcName, RawData: true, IsPrimary: true } );
   var maxLSN = cursor.current().toObj().CompleteLSN;

   for( var i = 0; i < 100; i++ )
   {
      cl.insert( { a: i } );
      cl.insert( { a: 100 + i } );
      cl.update( { $set: { a: 200 + i } }, { a: i } );
      cl.remove( { a: 100 + i } );
   }

   var rtCmd = getRemoteCmd( groupNames[0] );
   initTmpDir( rtCmd );

   try
   {
      var fieldType = "MAPPING_INT";
      readyOutputConfFile( rtCmd, groupNames[0], csName, clName, fieldType );

      var clNameArray = [csName + "." + clName];
      var filter = '\'{ MinLSN:' + minLSN + ', MaxLSN: ' + maxLSN + ' }\'';
      execSdbReplay( rtCmd, groupNames[0], clNameArray, "replica", undefined, undefined, undefined, undefined, filter );
      checkCsvFile( rtCmd, clName, expDataArr );

      cleanCL( csName, clName );
      cleanFile( rtCmd );
   } catch( e )
   {
      backupFile( rtCmd, clName );
      throw e;
   }
}