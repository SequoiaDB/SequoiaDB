/************************************************************************
*@Description: seqDB-12225: 重放时过滤/指定操作  
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
   var clName = "clName_12225_" + getRandomInt( 0, 100 );
   var groupNames = getDataGroupNames();

   var cl = readyCL( csName, clName, { Group: groupNames[0] } );

   var exclExpDataArr = [];
   var expDataArr = [];
   for( var i = 0; i < 100; i++ )
   {
      cl.insert( { a: i } );
      cl.insert( { a: 100 + i } );
      cl.update( { $set: { a: 200 + i } }, { a: i } );
      cl.remove( { a: 100 + i } );

      exclExpDataArr.push( '"I","' + i + '"' );
      exclExpDataArr.push( '"I","' + ( 100 + i ) + '"' );
      exclExpDataArr.push( '"D","' + ( 100 + i ) + '"' );
      expDataArr.push( '"I","' + i + '"' );
      expDataArr.push( '"I","' + ( 100 + i ) + '"' );
      expDataArr.push( '"B","' + i + '"' );
      expDataArr.push( '"A","' + ( 200 + i ) + '"' );
   }

   var rtCmd = getRemoteCmd( groupNames[0] );
   initTmpDir( rtCmd );

   try
   {
      var fieldType = "MAPPING_INT";
      readyOutputConfFile( rtCmd, groupNames[0], csName, clName, fieldType );

      //重放时过滤操作
      var clNameArray = [csName + "." + clName];
      var filter = '\'{CL: ["' + csName + "." + clName + '"], ExclOP: ["update"] }\'';
      execSdbReplay( rtCmd, groupNames[0], clNameArray, "replica", undefined, undefined, undefined, undefined, filter );
      checkCsvFile( rtCmd, clName, exclExpDataArr );

      cleanFile( rtCmd );

      var fieldType = "MAPPING_INT";
      readyOutputConfFile( rtCmd, groupNames[0], csName, clName, fieldType );

      //重放时指定操作
      var filter = '\'{CL: ["' + csName + "." + clName + '"], OP: ["insert", "update"] }\'';
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