/************************************************************************
*@Description: seqDB-25601:sdbreplay导出MAPPING_DATE/DOUBLE/BINARY字段类型，
*              MAPPING_INT/LONG/DECIMAL 按 mth_cast 规则强制转换导出，
*              doubleQuote 开启/关闭；含同字段 update 跨类型及三条记录分别
*              insert NumberInt/NumberLong/NumberDecimal；
*              含 MAPPING_DOUBLE 默认 Ryu（省略 floatFmt）数值矩阵：
*              最短可还原 + %g 定点/科学计数；25602 覆盖自定义 floatFmt；
*              含 2^53 整数边界、1e-4/1e-5 切换边界、>1e15 大数等；
*              含 NaN/Inf 导出空串及 defaultValue；
*              含 truncatecl 导出 T 行（映射字段为空）；sdbreplay 走 archivelog
*@Author: 2026-6-17 init
************************************************************************/
main( test );

function test ()
{
   var clName;
   var rtCmd;
   var expDateStr;

   try
   {
      if( commIsStandalone( db ) )
      {
         return;
      }

      var groupNames = getDataGroupNames();
      var groupName;
      if ( typeof ( SDBREPLAY25601_GROUP ) != "undefined" )
      {
         groupName = SDBREPLAY25601_GROUP;
      }
      else
      {
         groupName = groupNames[getRandomInt( 0, groupNames.length )];
      }
      var csName = COMMCSNAME;
      clName = "cl25601_" + getRandomInt( 0, 100 );

      rtCmd = getRemoteCmd( groupName );
      initTmpDir( rtCmd );

      var cl = readyCL( csName, clName, { Group: groupName } );
      expDateStr = readyData( cl, rtCmd );

      var tmpConfName = "sdbreplay_25601.conf";
      getOutputConfFile( rtCmd, csName, clName, tmpConfName );
      configOutputConfFile( rtCmd, groupName, csName, clName );

      var fullCLName = csName + "." + clName;
      var clNameArr = [fullCLName];
      var confPath = tmpFileDir + fullCLName + ".conf";
      var filterBody = buildSdbReplay25601Filter( clNameArr );
      sleep( 15000 );
      execSdbReplayWithRetry( rtCmd, groupName, clNameArr, "archive", confPath,
         undefined, undefined, undefined, filterBody, 29 );

      checkResults( rtCmd, clName, expDateStr );

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

function readyData ( cl, rtCmd )
{
   var commonFields = {
      col_date1: { $date: "2000-01-01" },
      col_date2: { $date: "2000-01-01" },
      col_double1: 2.718281828,
      col_double2: 2.718281828,
      col_binary1: BinData( "AQIDBAUGBwg=", "0" ),
      col_binary2: BinData( "AQIDBAUGBwg=", "0" )
   };

   // col_cross: 同一文档 insert 为 NumberLong，update 为 NumberDecimal
   cl.insert( {
      id1: NumberLong( 1 ),
      id2: NumberLong( 1 ),
      col_cross: NumberLong( "9223372036854775807" ),
      col_date1: commonFields.col_date1,
      col_date2: commonFields.col_date2,
      col_double1: commonFields.col_double1,
      col_double2: commonFields.col_double2,
      col_binary1: commonFields.col_binary1,
      col_binary2: commonFields.col_binary2
   } );
   var dateStr1 = getDocDateStr( rtCmd, cl, { id1: NumberLong( 1 ) }, "col_date1" );

   cl.update( {
      $set: {
         col_cross: { $decimal: "42.5" },
         col_date1: { $date: "2020-06-15" },
         col_date2: { $date: "2020-06-15" },
         col_double1: -1.5,
         col_double2: -1.5,
         col_binary1: BinData( "YWJj", "0" ),
         col_binary2: BinData( "YWJj", "0" )
      }
   }, { id1: NumberLong( 1 ) } );
   var dateStr2 = getDocDateStr( rtCmd, cl, { id1: NumberLong( 1 ) }, "col_date1" );

   cl.remove( { id1: NumberLong( 1 ) } );

   // col_cross: 三条记录分别 insert NumberInt / NumberLong / NumberDecimal
   cl.insert( {
      id1: NumberLong( 2 ),
      id2: NumberLong( 2 ),
      col_cross: 42,
      col_date1: commonFields.col_date1,
      col_date2: commonFields.col_date2,
      col_double1: commonFields.col_double1,
      col_double2: commonFields.col_double2,
      col_binary1: commonFields.col_binary1,
      col_binary2: commonFields.col_binary2
   } );
   cl.insert( {
      id1: NumberLong( 3 ),
      id2: NumberLong( 3 ),
      col_cross: NumberLong( "9223372036854775807" ),
      col_date1: commonFields.col_date1,
      col_date2: commonFields.col_date2,
      col_double1: commonFields.col_double1,
      col_double2: commonFields.col_double2,
      col_binary1: commonFields.col_binary1,
      col_binary2: commonFields.col_binary2
   } );
   cl.insert( {
      id1: NumberLong( 4 ),
      id2: NumberLong( 4 ),
      col_cross: { $decimal: "12345.6789" },
      col_date1: commonFields.col_date1,
      col_date2: commonFields.col_date2,
      col_double1: commonFields.col_double1,
      col_double2: commonFields.col_double2,
      col_binary1: commonFields.col_binary1,
      col_binary2: commonFields.col_binary2
   } );

   // MAPPING_DOUBLE 特殊值（默认 Ryu，省略 floatFmt）
   // 使用 0.0 / 100.0 等字面量，避免 JS 将 0、100 存为 NumberInt 导致类型歧义
   insertDoubleSpecialRow( cl, commonFields, 5, 99.99 );
   insertDoubleSpecialRow( cl, commonFields, 6, 0.0 );
   insertDoubleSpecialRow( cl, commonFields, 7, 100.0 );
   insertDoubleSpecialRow( cl, commonFields, 8, 0.123456789 );
   insertDoubleSpecialRow( cl, commonFields, 9, 1.7e308 );
   insertDoubleSpecialRow( cl, commonFields, 10, -1.7e308 );
   insertDoubleSpecialRow( cl, commonFields, 11, 4.9e-324 );

   // NaN/Inf：无 defaultValue 导出空串；col_double2 配置了 defaultValue
   insertDoubleNanInfRow( cl, commonFields, 12, NaN, Infinity );
   insertDoubleNanInfRow( cl, commonFields, 13, -Infinity, NaN );

   // 2^53 安全整数边界
   insertDoubleSpecialRow( cl, commonFields, 14, 9007199254740992 );
   insertDoubleSpecialRow( cl, commonFields, 15, 9007199254740993 );
   insertDoubleSpecialRow( cl, commonFields, 16, -9007199254740992 );

   // 科学计数法切换（含 %g 边界 -4：1e-4 定点 / 1e-5 科学）
   insertDoubleSpecialRow( cl, commonFields, 17, 1e-7 );
   insertDoubleSpecialRow( cl, commonFields, 18, 0.000001 );
   insertDoubleSpecialRow( cl, commonFields, 19, 1e21 );
   insertDoubleSpecialRow( cl, commonFields, 20, 1000000 );
   insertDoubleSpecialRow( cl, commonFields, 21, 1e-4 );
   insertDoubleSpecialRow( cl, commonFields, 22, 1e-5 );

   // |val| > 1e15 大数
   insertDoubleSpecialRow( cl, commonFields, 23, 1e16 );
   insertDoubleSpecialRow( cl, commonFields, 24, 123456789012345 );
   insertDoubleSpecialRow( cl, commonFields, 25, 1234567890123456 );

   cl.truncate();

   try
   {
      db.sync();
   }
   catch( e )
   {
      println( "db.sync warning: " + e );
   }
   return { dateStr1: dateStr1, dateStr2: dateStr2 };
}

function buildSdbReplay25601Filter ( clNameArr )
{
   var clList = [];
   for( var i = 0; i < clNameArr.length; i++ )
   {
      clList.push( '"' + clNameArr[i] + '"' );
   }
   return '{CL: [' + clList.join( ', ' ) + '], OP: ["insert", "update", "delete", "truncatecl"] }';
}

function insertDoubleNanInfRow ( cl, commonFields, id, colDouble1, colDouble2 )
{
   cl.insert( {
      id1: NumberLong( id ),
      id2: NumberLong( id ),
      col_cross: 42,
      col_date1: commonFields.col_date1,
      col_date2: commonFields.col_date2,
      col_double1: colDouble1,
      col_double2: colDouble2,
      col_binary1: commonFields.col_binary1,
      col_binary2: commonFields.col_binary2
   } );
}

function insertDoubleSpecialRow ( cl, commonFields, id, colDouble )
{
   cl.insert( {
      id1: NumberLong( id ),
      id2: NumberLong( id ),
      col_cross: 42,
      col_date1: commonFields.col_date1,
      col_date2: commonFields.col_date2,
      col_double1: colDouble,
      col_double2: colDouble,
      col_binary1: commonFields.col_binary1,
      col_binary2: commonFields.col_binary2
   } );
}

function getDocDateStr ( rtCmd, cl, filter, fieldName )
{
   var proj = {};
   proj[fieldName] = "";
   var cursor = cl.find( filter, proj );
   var doc = cursor.current().toObj();
   cursor.close();

   var dateObj = doc[fieldName];
   if( dateObj == undefined )
   {
      throw new Error( "field " + fieldName + " not found in doc: " + tojson( doc ) );
   }

   var millis;
   if( typeof( dateObj ) == "object" && dateObj.$date != undefined )
   {
      millis = new Date( dateObj.$date ).getTime();
   }
   else
   {
      millis = new Date( dateObj ).getTime();
   }

   if( isNaN( millis ) )
   {
      throw new Error( "invalid date value: " + tojson( dateObj ) );
   }

   var second = parseInt( millis / 1000 );
   return rtCmd.run( 'date -d@"' + second + '" "+%Y-%m-%d"' ).split( "\n" )[0];
}

function buildNumCrossSuff ( intStr, longStr, decimalStr )
{
   // col_cross 三列：MAPPING_INT / MAPPING_LONG / MAPPING_DECIMAL
   // 按 mth_cast 强制转换，三列输出可以不同
   return '"' + intStr + '",' + longStr + ',"' + decimalStr + '"';
}

// col_cross 期望值（与 rplCastNumberToString / mth_cast 一致）
function getColCrossExp ( kind )
{
   var expMap = {
      // NumberLong(9223372036854775807): INT 溢出 -> 0
      longMax: {
         int: "0",
         long: "9223372036854775807",
         decimal: "9223372036854775807"
      },
      // NumberDecimal(42.5): INT/LONG toLong 四舍五入 -> 43
      decimal42_5: {
         int: "43",
         long: "43",
         decimal: "42.5"
      },
      // NumberInt(42)
      int42: {
         int: "42",
         long: "42",
         decimal: "42"
      },
      // NumberDecimal(12345.6789): INT/LONG toLong 四舍五入 -> 12346
      decimal12345: {
         int: "12346",
         long: "12346",
         decimal: "12345.6789"
      }
   };
   if( expMap[kind] == undefined )
   {
      throw new Error( "unknown col_cross expect kind: " + kind );
   }
   return expMap[kind];
}

// MAPPING_DOUBLE 期望值（默认 Ryu：shortest + %g-like 定点/科学计数）
function getDoubleCsvExp ( kind )
{
   var expMap = {
      pi: "2.718281828",
      neg1_5: "-1.5",
      decimal99: "99.99",
      zero: "0.0",
      int100: "100.0",
      frac6: "0.123456789",
      maxDouble: "1.7e+308",
      minNegDouble: "-1.7e+308",
      subnormal: "5e-324",
      // 2^53 安全整数边界（9007199254740993 在 double 中与 2^53 相同）
      intMax: "9007199254740992.0",
      intMaxPlus1: "9007199254740992.0",
      intMaxNeg: "-9007199254740992.0",
      // 科学计数法切换
      sciSmall: "1e-07",
      micro: "1e-06",
      sciLarge: "1e+21",
      million: "1000000.0",
      // %g 边界：X=-4 定点，X=-5 科学
      eNeg4: "0.0001",
      eNeg5: "1e-05",
      // |val| > 1e15
      e16: "1e+16",
      int15: "123456789012345.0",
      int16: "1234567890123456.0",
      // NaN/Inf 无 defaultValue 时导出空串
      nanEmpty: "",
      infEmpty: "",
      // col_double2 配置 defaultValue: "DFT_DOUBLE"
      nanDefault: "DFT_DOUBLE"
   };
   if( expMap[kind] == undefined )
   {
      throw new Error( "unknown double csv expect kind: " + kind );
   }
   return expMap[kind];
}

function buildSuffStr ( id1, id2, dateStr, colDouble1, colBinary, colCrossExp, colDouble2 )
{
   if( typeof ( colDouble2 ) == "undefined" )
   {
      colDouble2 = colDouble1;
   }
   return '"' + id1 + '",' + id2 + ','
      + buildNumCrossSuff( colCrossExp.int, colCrossExp.long, colCrossExp.decimal ) + ','
      + '"' + dateStr + '",' + dateStr + ','
      + '"' + colDouble1 + '",' + colDouble2 + ','
      + '"' + colBinary + '",' + colBinary;
}

// truncate T 行：映射字段取自空对象，输出空串（doubleQuote 字段为 ""）
function buildTruncateSuffStr ()
{
   return '"",,"",,"","",,"",,"",';
}

function checkResults ( rtCmd, clName, expDateStr )
{
   var crossLongMax = getColCrossExp( "longMax" );
   var crossDec42_5 = getColCrossExp( "decimal42_5" );
   var crossInt42 = getColCrossExp( "int42" );
   var crossDec12345 = getColCrossExp( "decimal12345" );
   var dPi = getDoubleCsvExp( "pi" );
   var dNeg15 = getDoubleCsvExp( "neg1_5" );

   // id1=1: insert NumberLong(max) -> update NumberDecimal(42.5) -> delete
   var suffStr1 = buildSuffStr( 1, 1, expDateStr.dateStr1, dPi, "AQIDBAUGBwg=",
      crossLongMax );
   var suffStr2 = buildSuffStr( 1, 1, expDateStr.dateStr2, dNeg15, "YWJj", crossDec42_5 );

   // id1=2/3/4: 纯 insert，分别 NumberInt / NumberLong(max) / NumberDecimal
   var suffInt = buildSuffStr( 2, 2, expDateStr.dateStr1, dPi, "AQIDBAUGBwg=",
      crossInt42 );
   var suffLong = buildSuffStr( 3, 3, expDateStr.dateStr1, dPi, "AQIDBAUGBwg=",
      crossLongMax );
   var suffDecimal = buildSuffStr( 4, 4, expDateStr.dateStr1, dPi, "AQIDBAUGBwg=",
      crossDec12345 );

   // id1=5~11: MAPPING_DOUBLE 特殊值纯 insert
   var suffD99 = buildSuffStr( 5, 5, expDateStr.dateStr1, getDoubleCsvExp( "decimal99" ),
      "AQIDBAUGBwg=", crossInt42 );
   var suffD0 = buildSuffStr( 6, 6, expDateStr.dateStr1, getDoubleCsvExp( "zero" ),
      "AQIDBAUGBwg=", crossInt42 );
   var suffD100 = buildSuffStr( 7, 7, expDateStr.dateStr1, getDoubleCsvExp( "int100" ),
      "AQIDBAUGBwg=", crossInt42 );
   var suffDFrac = buildSuffStr( 8, 8, expDateStr.dateStr1, getDoubleCsvExp( "frac6" ),
      "AQIDBAUGBwg=", crossInt42 );
   var suffDMax = buildSuffStr( 9, 9, expDateStr.dateStr1, getDoubleCsvExp( "maxDouble" ),
      "AQIDBAUGBwg=", crossInt42 );
   var suffDMinNeg = buildSuffStr( 10, 10, expDateStr.dateStr1, getDoubleCsvExp( "minNegDouble" ),
      "AQIDBAUGBwg=", crossInt42 );
   var suffDSub = buildSuffStr( 11, 11, expDateStr.dateStr1, getDoubleCsvExp( "subnormal" ),
      "AQIDBAUGBwg=", crossInt42 );

   // id1=12: NaN/Inf 无 defaultValue -> 空串（col_double1 带引号 ""，col_double2 无引号空）
   var dEmpty = getDoubleCsvExp( "nanEmpty" );
   var suffNanInf = buildSuffStr( 12, 12, expDateStr.dateStr1, dEmpty,
      "AQIDBAUGBwg=", crossInt42, getDoubleCsvExp( "infEmpty" ) );
   // id1=13: col_double2 配置 defaultValue，NaN -> DFT_DOUBLE
   var suffNanDft = buildSuffStr( 13, 13, expDateStr.dateStr1, dEmpty,
      "AQIDBAUGBwg=", crossInt42, getDoubleCsvExp( "nanDefault" ) );

   // id1=14~25: 高优先级 double 边界值
   var suffIntMax = buildSuffStr( 14, 14, expDateStr.dateStr1, getDoubleCsvExp( "intMax" ),
      "AQIDBAUGBwg=", crossInt42 );
   var suffIntMaxPlus1 = buildSuffStr( 15, 15, expDateStr.dateStr1, getDoubleCsvExp( "intMaxPlus1" ),
      "AQIDBAUGBwg=", crossInt42 );
   var suffIntMaxNeg = buildSuffStr( 16, 16, expDateStr.dateStr1, getDoubleCsvExp( "intMaxNeg" ),
      "AQIDBAUGBwg=", crossInt42 );
   var suffSciSmall = buildSuffStr( 17, 17, expDateStr.dateStr1, getDoubleCsvExp( "sciSmall" ),
      "AQIDBAUGBwg=", crossInt42 );
   var suffMicro = buildSuffStr( 18, 18, expDateStr.dateStr1, getDoubleCsvExp( "micro" ),
      "AQIDBAUGBwg=", crossInt42 );
   var suffSciLarge = buildSuffStr( 19, 19, expDateStr.dateStr1, getDoubleCsvExp( "sciLarge" ),
      "AQIDBAUGBwg=", crossInt42 );
   var suffMillion = buildSuffStr( 20, 20, expDateStr.dateStr1, getDoubleCsvExp( "million" ),
      "AQIDBAUGBwg=", crossInt42 );
   var suffENeg4 = buildSuffStr( 21, 21, expDateStr.dateStr1, getDoubleCsvExp( "eNeg4" ),
      "AQIDBAUGBwg=", crossInt42 );
   var suffENeg5 = buildSuffStr( 22, 22, expDateStr.dateStr1, getDoubleCsvExp( "eNeg5" ),
      "AQIDBAUGBwg=", crossInt42 );
   var suffE16 = buildSuffStr( 23, 23, expDateStr.dateStr1, getDoubleCsvExp( "e16" ),
      "AQIDBAUGBwg=", crossInt42 );
   var suffInt15 = buildSuffStr( 24, 24, expDateStr.dateStr1, getDoubleCsvExp( "int15" ),
      "AQIDBAUGBwg=", crossInt42 );
   var suffInt16 = buildSuffStr( 25, 25, expDateStr.dateStr1, getDoubleCsvExp( "int16" ),
      "AQIDBAUGBwg=", crossInt42 );

   // id1=1 含 update/delete 有 I/B/A/D；后续纯 insert 仅输出 I 行；最后 truncate 输出 T 行
   var suffTruncate = buildTruncateSuffStr();
   var expDataArr = ['I,' + suffStr1,
   'B,' + suffStr1,
   'A,' + suffStr2,
   'D,' + suffStr2,
   'I,' + suffInt,
   'I,' + suffLong,
   'I,' + suffDecimal,
   'I,' + suffD99,
   'I,' + suffD0,
   'I,' + suffD100,
   'I,' + suffDFrac,
   'I,' + suffDMax,
   'I,' + suffDMinNeg,
   'I,' + suffDSub,
   'I,' + suffNanInf,
   'I,' + suffNanDft,
   'I,' + suffIntMax,
   'I,' + suffIntMaxPlus1,
   'I,' + suffIntMaxNeg,
   'I,' + suffSciSmall,
   'I,' + suffMicro,
   'I,' + suffSciLarge,
   'I,' + suffMillion,
   'I,' + suffENeg4,
   'I,' + suffENeg5,
   'I,' + suffE16,
   'I,' + suffInt15,
   'I,' + suffInt16,
   'T,' + suffTruncate];
   checkCsvFile( rtCmd, clName, expDataArr );
}
