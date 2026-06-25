/*******************************************************************
* @Description : test export with --floatfmt full
*                seqDB-25603:sdbexprt 指定 --floatfmt full，
*                            csv/json 导出 double 为最短往返文本；
*                            默认仍为 %.16g；--floatfmt 对照 %.4f/%.16g/db2；
*                            含 %g 边界 1e-4/1e-5 及 25601 精简边界矩阵；
*                            NaN/Inf 仍输出字面量；非法 floatfmt 失败
* @author      : Auto
*
*******************************************************************/
var csname = COMMCSNAME;

/* 精简 double 矩阵（对齐 sdbreplay 25601/25602） */
var MATRIX_KINDS = [
   "d99", "eNeg4", "eNeg5", "frac6",
   "pi", "sciSmall", "sciLarge", "intMax", "e16"
];

var SPECIAL_KINDS = [ "nan", "inf", "negInf" ];

main( test );

function test ()
{
   testFullMatrixCsv();
   testFullMatrixJson();
   testDefaultMatrixCsv();
   testDefaultMatrixJson();
   testFloatFmtF4Csv();
   testFloatFmtF4Json();
   testFloatFmtG16Csv();
   testFloatFmtG16Json();
   testFloatFmtDb2Csv();
   testFloatFmtDb2Json();
   testSpecialValuesCsv();
   testSpecialValuesJson();
   testInvalidFloatFmt();
}

function getDoubleMatrixDocs ( kinds )
{
   /* 0/100 等整型字面量会被存为 NumberInt，需用算术强制为 double */
   var valMap = {
      d99: 99.99,
      eNeg4: 1e-4,
      eNeg5: 1e-5,
      zero: 0 * 1.0,
      int100: 100 * 1.0,
      frac6: 0.123456789,
      pi: 2.718281828459045,
      sciSmall: 1e-7,
      sciLarge: 1e21,
      intMax: 9007199254740992,
      e16: 1e16,
      nan: 0 / 0,
      inf: 1 / 0,
      negInf: -1 / 0
   };
   var docs = [];
   for( var i = 0; i < kinds.length; i++ )
   {
      var kind = kinds[i];
      if( valMap[kind] == undefined )
      {
         throw new Error( "unknown matrix kind: " + kind );
      }
      docs.push( { a: valMap[kind] } );
   }
   return docs;
}

/* --floatfmt full：最短可还原 */
function getExpFull ( kind )
{
   var m = {
      d99: "99.99",
      eNeg4: "0.0001",
      eNeg5: "1e-05",
      zero: "0.0",
      int100: "100.0",
      frac6: "0.123456789",
      pi: "2.718281828459045",
      sciSmall: "1e-07",
      sciLarge: "1e+21",
      intMax: "9007199254740992.0",
      e16: "1e+16",
      nan: "NaN",
      inf: "Infinity",
      negInf: "-Infinity"
   };
   if( m[kind] == undefined )
   {
      throw new Error( "unknown full expect: " + kind );
   }
   return m[kind];
}

/* 默认 / 显式 --floatfmt %.16g */
function getExpG16 ( kind )
{
   var m = {
      d99: "99.98999999999999",
      eNeg4: "0.0001",
      eNeg5: "1e-05",
      zero: "0.0",
      int100: "100.0",
      frac6: "0.123456789",
      pi: "2.718281828459045",
      sciSmall: "1e-07",
      sciLarge: "1e+21",
      intMax: "9007199254740992.0",
      e16: "1e+16",
      nan: "NaN",
      inf: "Infinity",
      negInf: "-Infinity"
   };
   if( m[kind] == undefined )
   {
      throw new Error( "unknown %.16g expect: " + kind );
   }
   return m[kind];
}

/* --floatfmt %.4f */
function getExpF4 ( kind )
{
   var m = {
      d99: "99.9900",
      eNeg4: "0.0001",
      eNeg5: "0.0000",
      zero: "0.0000",
      int100: "100.0000",
      frac6: "0.1235",
      pi: "2.7183",
      sciSmall: "0.0000",
      sciLarge: "1000000000000000000000.0000",
      intMax: "9007199254740992.0000",
      e16: "10000000000000000.0000",
      nan: "NaN",
      inf: "Infinity",
      negInf: "-Infinity"
   };
   if( m[kind] == undefined )
   {
      throw new Error( "unknown %.4f expect: " + kind );
   }
   return m[kind];
}

/* --floatfmt db2 => %+.14E */
function getExpDb2 ( kind )
{
   var m = {
      d99: "+9.99900000000000E+01",
      eNeg4: "+1.00000000000000E-04",
      eNeg5: "+1.00000000000000E-05",
      zero: "+0.00000000000000E+00",
      int100: "+1.00000000000000E+02",
      frac6: "+1.23456789000000E-01",
      pi: "+2.71828182845905E+00",
      sciSmall: "+1.00000000000000E-07",
      sciLarge: "+1.00000000000000E+21",
      intMax: "+9.00719925474099E+15",
      e16: "+1.00000000000000E+16",
      nan: "NaN",
      inf: "Infinity",
      negInf: "-Infinity"
   };
   if( m[kind] == undefined )
   {
      throw new Error( "unknown db2 expect: " + kind );
   }
   return m[kind];
}

function buildCsvContent ( expFn, kinds )
{
   var lines = [ "a" ];
   for( var i = 0; i < kinds.length; i++ )
   {
      lines.push( expFn( kinds[i] ) );
   }
   return lines.join( "\n" ) + "\n";
}

function formatJsonNum ( expStr )
{
   if( expStr == "NaN" || expStr == "Infinity" || expStr == "-Infinity" )
   {
      return expStr;
   }
   return expStr;
}

function buildJsonContent ( expFn, kinds )
{
   var lines = [];
   for( var i = 0; i < kinds.length; i++ )
   {
      lines.push( '{ "a": ' + formatJsonNum( expFn( kinds[i] ) ) + ' }' );
   }
   return lines.join( "\n" ) + "\n";
}

function runExportTest ( opts )
{
   var clname = COMMCLNAME + opts.clSuffix;
   var cl = commCreateCL( db, csname, clname );
   cl.insert( getDoubleMatrixDocs( opts.kinds ) );

   var outfile = tmpFileDir + opts.fileName;
   cmd.run( "rm -rf " + outfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + outfile +
      " --type " + opts.type +
      " --sort '{ _id: 1 }'" +
      " --fields a";
   if( opts.floatfmt != undefined )
   {
      command += " --floatfmt " + opts.floatfmt;
   }
   testRunCommand( command, opts.expRc );

   if( opts.expRc == undefined || opts.expRc == 0 )
   {
      if( opts.type == "csv" )
      {
         checkFileContent( outfile, buildCsvContent( opts.expFn, opts.kinds ) );
      }
      else
      {
         checkFileContent( outfile, buildJsonContent( opts.expFn, opts.kinds ) );
      }
   }

   cmd.run( "rm -rf " + outfile );
   commDropCL( db, csname, clname );
}

function testFullMatrixCsv ()
{
   runExportTest( {
      clSuffix: "_sdbexprt25603_full_csv",
      fileName: "sdbexprt25603_full_matrix.csv",
      type: "csv",
      kinds: MATRIX_KINDS,
      expFn: getExpFull,
      floatfmt: "full"
   } );
}

function testFullMatrixJson ()
{
   runExportTest( {
      clSuffix: "_sdbexprt25603_full_json",
      fileName: "sdbexprt25603_full_matrix.json",
      type: "json",
      kinds: MATRIX_KINDS,
      expFn: getExpFull,
      floatfmt: "full"
   } );
}

function testDefaultMatrixCsv ()
{
   runExportTest( {
      clSuffix: "_sdbexprt25603_default_csv",
      fileName: "sdbexprt25603_default_matrix.csv",
      type: "csv",
      kinds: MATRIX_KINDS,
      expFn: getExpG16
   } );
}

function testDefaultMatrixJson ()
{
   runExportTest( {
      clSuffix: "_sdbexprt25603_default_json",
      fileName: "sdbexprt25603_default_matrix.json",
      type: "json",
      kinds: MATRIX_KINDS,
      expFn: getExpG16
   } );
}

function testFloatFmtF4Csv ()
{
   runExportTest( {
      clSuffix: "_sdbexprt25603_f4_csv",
      fileName: "sdbexprt25603_f4_matrix.csv",
      type: "csv",
      kinds: MATRIX_KINDS,
      expFn: getExpF4,
      floatfmt: "%.4f"
   } );
}

function testFloatFmtF4Json ()
{
   runExportTest( {
      clSuffix: "_sdbexprt25603_f4_json",
      fileName: "sdbexprt25603_f4_matrix.json",
      type: "json",
      kinds: MATRIX_KINDS,
      expFn: getExpF4,
      floatfmt: "%.4f"
   } );
}

function testFloatFmtG16Csv ()
{
   runExportTest( {
      clSuffix: "_sdbexprt25603_g16_csv",
      fileName: "sdbexprt25603_g16_matrix.csv",
      type: "csv",
      kinds: MATRIX_KINDS,
      expFn: getExpG16,
      floatfmt: "%.16g"
   } );
}

function testFloatFmtG16Json ()
{
   runExportTest( {
      clSuffix: "_sdbexprt25603_g16_json",
      fileName: "sdbexprt25603_g16_matrix.json",
      type: "json",
      kinds: MATRIX_KINDS,
      expFn: getExpG16,
      floatfmt: "%.16g"
   } );
}

function testFloatFmtDb2Csv ()
{
   runExportTest( {
      clSuffix: "_sdbexprt25603_db2_csv",
      fileName: "sdbexprt25603_db2_matrix.csv",
      type: "csv",
      kinds: MATRIX_KINDS,
      expFn: getExpDb2,
      floatfmt: "db2"
   } );
}

function testFloatFmtDb2Json ()
{
   runExportTest( {
      clSuffix: "_sdbexprt25603_db2_json",
      fileName: "sdbexprt25603_db2_matrix.json",
      type: "json",
      kinds: MATRIX_KINDS,
      expFn: getExpDb2,
      floatfmt: "db2"
   } );
}

function testSpecialValuesCsv ()
{
   runExportTest( {
      clSuffix: "_sdbexprt25603_special_csv",
      fileName: "sdbexprt25603_special.csv",
      type: "csv",
      kinds: SPECIAL_KINDS,
      expFn: getExpFull,
      floatfmt: "full"
   } );
}

function testSpecialValuesJson ()
{
   runExportTest( {
      clSuffix: "_sdbexprt25603_special_json",
      fileName: "sdbexprt25603_special.json",
      type: "json",
      kinds: SPECIAL_KINDS,
      expFn: getExpFull,
      floatfmt: "full"
   } );
}

function testInvalidFloatFmt ()
{
   var clname = COMMCLNAME + "_sdbexprt25603_invalid";
   var cl = commCreateCL( db, csname, clname );
   cl.insert( [ { a: 1.5 } ] );

   var csvfile = tmpFileDir + "sdbexprt25603_invalid.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --fields a" +
      " --floatfmt trunc6";
   testRunCommand( command, 127 );

   cmd.run( "rm -rf " + csvfile );
   commDropCL( db, csname, clname );
}
