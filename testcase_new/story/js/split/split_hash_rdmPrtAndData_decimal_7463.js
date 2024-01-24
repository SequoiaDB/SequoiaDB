/******************************************************************************
@Description : seqDB-7463: hash自动切分，分区键为不同数据类型，验证数据分布规则
                           type: decimal
                           数据量随机，cl partition随机，数据随机
@Author :
   2019-8-23   XiaoNi Huang  init
*******************************************************************************/
main( test );

function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   if( commGetGroupsNum( db ) < 2 )
   {
      return;
   }

   var suffix = "_decimal_7463";
   var dmName = CHANGEDPREFIX + "_dm" + suffix;
   var csName = CHANGEDPREFIX + "_cs" + suffix;
   var clName = CHANGEDPREFIX + "_cl" + suffix;
   var groups = commGetGroups( db, false, "", false, true, true );
   var groupNames = [groups[1][0].GroupName, groups[2][0].GroupName];
   var cl;
   var recordsNum = getRandomInt( 1000, 5000 );

   commDropCS( db, csName, true, "drop cs in the begin" );
   commDropDomain( db, dmName, true );

   // create domain / cs / cl
   db.createDomain( dmName, groupNames, { "AutoSplit": true } );
   var cs = db.createCS( csName, { "Domain": dmName } );
   var partition = getRandomPartition();
   cl = cs.createCL( clName, { "ShardingType": "hash", "ShardingKey": { b: 1 }, "Partition": partition } );

   // insert
   var recordsArr = readyRdmRecs( recordsNum );
   cl.insert( recordsArr );

   // check results
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { a: 1 } );
   var expRecsArr = readyExpRecs( recordsArr );
   commCompareResults( cursor, expRecsArr );
   checkHashDistribution( groupNames, csName, clName, recordsNum );

   commDropCS( db, csName, false, "drop cs in the end." );
   commDropDomain( db, dmName, false );
}

function readyRdmRecs ( recordsNum, recordsArr ) 
{
   var recordsArr = [];
   for( var i = 0; i < recordsNum; i++ )
   {
      // get baseDecimal, e.g:1.7E+308, get random 1.7
      var rdmBaseDecimal = getRandomNum( -9.9, 9.9 );
      // get baseDecimal, e.g:1.7E+308, get random [0, 400)
      var rdmBaseIndex = Math.floor( Math.random() * ( 400 ) );
      // get decimal
      var decimal = rdmBaseDecimal + "e+" + rdmBaseIndex;
      recordsArr.push( { "a": i, "b": { "$decimal": String( decimal ) } } );
   }
   return recordsArr;
}

function readyExpRecs ( recordsArr )
{
   var expRecsArr = [];
   for( var i = 0; i < recordsArr.length; i++ )
   {
      var oriStr = recordsArr[i]["b"]["$decimal"];
      // get int, e.g:1.234E+10, get 1
      var intStr = oriStr.substring( 0, oriStr.indexOf( "." ) );
      // get decimal, e.g:1.234E+10, get 234
      var decimalStr = oriStr.substring( oriStr.indexOf( "." ) + 1, oriStr.indexOf( "e" ) );
      // get index number, e.g:1.234E+10, get 10
      var indexStr = oriStr.substring( oriStr.indexOf( "+" ) + 1, oriStr.length );

      // e -> number, e.g: 1.234E+5, get 123400
      var numStr = "";
      if( decimalStr.length <= Number( indexStr ) )
      {
         // get number of zero, e.g: e.g: 1.234E+5 -> 123400, get number of zero is 2 (00)
         var zeroNum = Number( indexStr ) - decimalStr.length;
         var zeroStr = "";
         for( j = 0; j < zeroNum; j++ )
         {
            zeroStr += "0";
         }
         numStr = intStr + decimalStr + zeroStr;
      }

      // e -> number, e.g: 1.234E+2, get 123.4
      if( decimalStr.length > Number( indexStr ) )
      {
         // get int str, e.g: e.g: 1.234E+2 -> 123.4, get 123
         var newIntStr = intStr + decimalStr.substring( 0, Number( indexStr ) );
         // get decimal str, e.g: 1.234E+2 -> 123.4, get 4
         var newDecimalStr = decimalStr.substring( Number( indexStr ), decimalStr.length );
         // get decimal, e.g: e.g: 1.234E+2 -> 123.4, get 123.4
         numStr = newIntStr + "." + newDecimalStr;
      }

      var numStr = exceptInavalidZero( numStr );

      // get decimal records
      var newRecord = { "a": i, "b": { "$decimal": numStr } };
      expRecsArr.push( newRecord );
   }
   return expRecsArr;
}

function exceptInavalidZero ( numStr )
{
   // e.g:0.0001234e+5 -> numStr= 00012.34, get 12.34. or e.g:0001234, get 1234
   // e.g:0001234, get int 1234. or 00012.34, get 12
   var tmpIntStr = numStr;
   var tmpDecimalStr = "";
   if( numStr.indexOf( "." ) !== -1 )
   {
      // e.g:12.34, get 12
      tmpIntStr = numStr.substring( 0, numStr.indexOf( "." ) );
      // e.g:12.34, get .34
      tmpDecimalStr = numStr.substring( numStr.indexOf( "." ), numStr.length );
   }

   // get symbol, may be "-"
   var symbol = "";
   if( tmpIntStr.substring( 0, 1 ).indexOf( "-" ) === 0 )
   {
      symbol = "-";
      // e.g:-0012.34, get 0012.34
      tmpIntStr = tmpIntStr.substring( 1, tmpIntStr.length );
   }

   // e.g:0.1234 or 000.1234, e.g:0.1234
   if( Number( tmpIntStr ) === 0 )
   {
      tmpIntStr = "0";
   }
   else
   {
      // e.g:0012.34, get 12.34
      for( var j = 0; j < tmpIntStr.length; j++ )
      {
         var tmpNum = tmpIntStr.substring( j, j + 1 );
         if( tmpNum.indexOf( "0" ) !== 0 )
         {
            // get decimal 12.34
            tmpIntStr = tmpIntStr.substring( j, tmpIntStr.length );
            break;
         }
      }
   }

   numStr = symbol + tmpIntStr + tmpDecimalStr;

   return numStr;
}