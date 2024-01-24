/************************************************************************
*@Description: seqDB-12224: 重放复制日志时过滤/指定cl  
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
   var clName1 = "clName_12224_1" + getRandomInt( 0, 100 );
   var clName2 = "clName_12224_2" + getRandomInt( 0, 100 );
   var groupNames = getDataGroupNames();

   var cl1 = readyCL( csName, clName1, { Group: groupNames[0] } );
   var cl2 = readyCL( csName, clName2, { Group: groupNames[0] } );

   var expDataArr = [];
   for( var i = 0; i < 100; i++ )
   {
      cl1.insert( { a: i } );
      cl1.insert( { a: 100 + i } );
      cl1.update( { $set: { a: 200 + i } }, { a: i } );
      cl1.remove( { a: 100 + i } );
      cl2.insert( { b: i } );
      cl2.insert( { b: 100 + i } );
      cl2.update( { $set: { b: 200 + i } }, { b: i } );
      cl2.remove( { b: 100 + i } );

      expDataArr.push( '"I","' + i + '"' );
      expDataArr.push( '"I","' + ( 100 + i ) + '"' );
      expDataArr.push( '"B","' + i + '"' );
      expDataArr.push( '"A","' + ( 200 + i ) + '"' );
      expDataArr.push( '"D","' + ( 100 + i ) + '"' );
   }

   var rtCmd = getRemoteCmd( groupNames[0] );
   initTmpDir( rtCmd );

   try
   {
      var fieldType = "MAPPING_INT";
      readyOutputConfFile( rtCmd, groupNames[0], csName, clName1, fieldType );

      //重放时指定/过滤cl
      var clNameArray = [csName + "." + clName1];
      var filter = '\'{CL: ["' + csName + '.' + clName1 + '","' + csName + '.' + clName2 + '"], ExclCL: ["' + csName + "." + clName2 + '"] }\'';
      execSdbReplay( rtCmd, groupNames[0], clNameArray, "replica", undefined, undefined, undefined, undefined, filter );
      checkCsvFile( rtCmd, clName1, expDataArr );

      cleanCL( csName, clName1 );
      cleanCL( csName, clName2 );
      cleanFile( rtCmd );
   } catch( e )
   {
      backupFile( rtCmd, clName1 );
      throw e;
   }
}