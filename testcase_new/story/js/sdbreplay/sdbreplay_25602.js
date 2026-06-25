/************************************************************************
*@Description: seqDB-25602:sdbreplay MAPPING_DOUBLE 的 floatFmt 对照：
*              省略=默认 Ryu；"%.4f"；显式 "%.16g"；"db2"；
*              含 NaN/Inf 与 defaultValue；sdbreplay 走 archivelog
*@Author: 2026-7-8 init
************************************************************************/
main( test );

function test ()
{
   var clName;
   var rtCmd;

   try
   {
      if( commIsStandalone( db ) )
      {
         return;
      }

      var groupNames = getDataGroupNames();
      var groupName;
      if ( typeof ( SDBREPLAY25602_GROUP ) != "undefined" )
      {
         groupName = SDBREPLAY25602_GROUP;
      }
      else
      {
         groupName = groupNames[getRandomInt( 0, groupNames.length )];
      }
      var csName = COMMCSNAME;
      clName = "cl25602_" + getRandomInt( 0, 100 );

      rtCmd = getRemoteCmd( groupName );
      initTmpDir( rtCmd );

      var cl = readyCL( csName, clName, { Group: groupName } );
      readyData( cl );

      var tmpConfName = "sdbreplay_25602.conf";
      getOutputConfFile( rtCmd, csName, clName, tmpConfName );
      configOutputConfFile( rtCmd, groupName, csName, clName );

      var fullCLName = csName + "." + clName;
      var clNameArr = [fullCLName];
      var confPath = tmpFileDir + fullCLName + ".conf";
      var filterBody = buildSdbReplay25602Filter( clNameArr );
      sleep( 8000 );
      // 5 I + 1 T
      execSdbReplayWithRetry( rtCmd, groupName, clNameArr, "archive", confPath,
         undefined, undefined, undefined, filterBody, 6 );

      checkResults( rtCmd, clName );

      cleanCL( csName, clName );
      cleanFile( rtCmd );
   }
   catch( e )
   {
      backupFile( rtCmd, clName );
      throw e;
   }
}

function configOutputConfFile ( rtCmd, groupName, csName, clName )
{
   var fullCLName = csName + "." + clName;
   var targetConfPath = tmpFileDir + fullCLName + ".conf";

   rtCmd.run( "sed -i 's/filePrefix_ori/test_" + groupName + "/g' " + targetConfPath );
   rtCmd.run( "sed -i 's/source_fullCLName_ori/" + fullCLName + "/g' " + targetConfPath );
   rtCmd.run( "sed -i 's/target_fullCLName_ori/" + fullCLName + "_new/g' " + targetConfPath );
}

function readyData ( cl )
{
   // 精简对照：99.99（Ryu vs %.16g）、%g 边界 1e-4/1e-5、NaN/Inf
   insertFmtRow( cl, 1, 99.99 );
   insertFmtRow( cl, 2, 1e-4 );
   insertFmtRow( cl, 3, 1e-5 );
   cl.insert( {
      id1: NumberLong( 4 ),
      id2: NumberLong( 4 ),
      col_default: NaN,
      col_f4: Infinity,
      col_g16: NaN,
      col_db2: Infinity
   } );
   cl.insert( {
      id1: NumberLong( 5 ),
      id2: NumberLong( 5 ),
      col_default: -Infinity,
      col_f4: NaN,
      col_g16: -Infinity,
      col_db2: NaN
   } );

   cl.truncate();

   try
   {
      db.sync();
   }
   catch( e )
   {
      println( "db.sync warning: " + e );
   }
}

function insertFmtRow ( cl, id, val )
{
   cl.insert( {
      id1: NumberLong( id ),
      id2: NumberLong( id ),
      col_default: val,
      col_f4: val,
      col_g16: val,
      col_db2: val
   } );
}

function buildSdbReplay25602Filter ( clNameArr )
{
   var clList = [];
   for( var i = 0; i < clNameArr.length; i++ )
   {
      clList.push( '"' + clNameArr[i] + '"' );
   }
   return '{CL: [' + clList.join( ', ' ) + '], OP: ["insert", "truncatecl"] }';
}

/* 省略 floatFmt：默认 Ryu */
function expRyu ( kind )
{
   var m = {
      d99: "99.99",
      eNeg4: "0.0001",
      eNeg5: "1e-05",
      empty: ""
   };
   if( m[kind] == undefined )
   {
      throw new Error( "unknown ryu expect: " + kind );
   }
   return m[kind];
}

/* floatFmt "%.4f" */
function expF4 ( kind )
{
   var m = {
      d99: "99.9900",
      eNeg4: "0.0001",
      eNeg5: "0.0000",
      empty: "",
      nanDefault: "DFT_DOUBLE"
   };
   if( m[kind] == undefined )
   {
      throw new Error( "unknown %.4f expect: " + kind );
   }
   return m[kind];
}

/* floatFmt "%.16g" */
function expG16 ( kind )
{
   var m = {
      d99: "99.98999999999999",
      eNeg4: "0.0001",
      eNeg5: "1e-05",
      empty: ""
   };
   if( m[kind] == undefined )
   {
      throw new Error( "unknown %.16g expect: " + kind );
   }
   return m[kind];
}

/* floatFmt "db2" => %+.14E */
function expDb2 ( kind )
{
   var m = {
      d99: "+9.99900000000000E+01",
      eNeg4: "+1.00000000000000E-04",
      eNeg5: "+1.00000000000000E-05",
      empty: ""
   };
   if( m[kind] == undefined )
   {
      throw new Error( "unknown db2 expect: " + kind );
   }
   return m[kind];
}

function buildSuffStr ( id1, id2, dDefault, dF4, dG16, dDb2 )
{
   return '"' + id1 + '",' + id2 + ','
      + '"' + dDefault + '",' + dF4 + ','
      + '"' + dG16 + '",' + dDb2;
}

function buildTruncateSuffStr ()
{
   return '"",,"",,"",';
}

function checkResults ( rtCmd, clName )
{
   var empty = expRyu( "empty" );
   var suff1 = buildSuffStr( 1, 1,
      expRyu( "d99" ), expF4( "d99" ), expG16( "d99" ), expDb2( "d99" ) );
   var suff2 = buildSuffStr( 2, 2,
      expRyu( "eNeg4" ), expF4( "eNeg4" ), expG16( "eNeg4" ), expDb2( "eNeg4" ) );
   var suff3 = buildSuffStr( 3, 3,
      expRyu( "eNeg5" ), expF4( "eNeg5" ), expG16( "eNeg5" ), expDb2( "eNeg5" ) );
   var suff4 = buildSuffStr( 4, 4,
      empty, expF4( "empty" ), empty, expDb2( "empty" ) );
   var suff5 = buildSuffStr( 5, 5,
      empty, expF4( "nanDefault" ), empty, expDb2( "empty" ) );
   var suffTruncate = buildTruncateSuffStr();

   var expDataArr = [
      'I,' + suff1,
      'I,' + suff2,
      'I,' + suff3,
      'I,' + suff4,
      'I,' + suff5,
      'T,' + suffTruncate
   ];
   checkCsvFile( rtCmd, clName, expDataArr );
}
