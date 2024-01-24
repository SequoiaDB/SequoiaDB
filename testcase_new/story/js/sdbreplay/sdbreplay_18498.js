/************************************************************************
*@Description: seqDB-18498: tables.fields配置多个字段，部分字段不存在 
*@Author: 2019-7-4  xiaoni zhao init
************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var csName = COMMCSNAME;
   var clName = "clName_18498";
   var groupNames = getDataGroupNames();

   var cl = readyCL( csName, clName, { Group: groupNames[0] } );

   var expDataArr = [];
   var minLSN = getMinLSN( groupNames );
   for( var i = 0; i < 100; i++ )
   {
      cl.insert( { a: i, b: i } );
      cl.insert( { a: 100 + i, b: 100 + i } );
      cl.update( { $set: { b: 200 + i } }, { b: i } );
      cl.remove( { a: 100 + i } );

      expDataArr.push( '"I","' + i + '","' + i + '","defaultValue"' );
      expDataArr.push( '"I","' + ( 100 + i ) + '","' + ( 100 + i ) + '","defaultValue"' );
      expDataArr.push( '"B","' + i + '","' + i + '","defaultValue"' );
      expDataArr.push( '"A","' + i + '","' + ( 200 + i ) + '","defaultValue"' );
      expDataArr.push( '"D","' + ( 100 + i ) + '","' + ( 100 + i ) + '","defaultValue"' );
   }

   var rtCmd = getRemoteCmd( groupNames[0] );
   initTmpDir( rtCmd );

   try
   {
      var confName = "sdbreplay_18498.conf";
      getOutputConfFile( groupNames[0], csName, clName, confName );
      configOutputConfFile( rtCmd, csName, clName );

      var clNameArr = [csName + "." + clName];
      var filter = filter = '\'{CL: ["' + clNameArr + '"], MinLSN:' + minLSN + ' }\'';;
      execSdbReplay( rtCmd, groupNames[0], clNameArr, undefined, undefined, undefined, undefined, undefined, filter );

      checkCsvFile( rtCmd, clName, expDataArr );

      cleanCL( csName, clName );
      cleanFile( rtCmd );
   } catch( e )
   {
      backupFile( rtCmd, clName );
      throw e;
   }
}
function configOutputConfFile ( rtCmd, csName, clName )
{
   var targetConfPath = tmpFileDir + csName + "." + clName + ".conf";
   rtCmd.run( "sed -i 's/csName.clName_source/" + csName + "." + clName + "/g' " + targetConfPath );
   rtCmd.run( "sed -i 's/csName.clName_target/" + csName + "." + clName + "/g' " + targetConfPath );
}