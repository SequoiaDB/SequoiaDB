import( "../lib/main.js" );

/************************************
*@Description: 创建主表，默认创建两个字表
*@author:      luweikang
*@createDate:  2019.8.7
**************************************/
function createMainCLAndAttachCL ( db, csName, mainCLName, clName, shardingFormat, subCLNum, beginBound, scope )
{
   if( shardingFormat == undefined ) { shardingFormat = "YYYYMMDD"; }
   if( subCLNum == undefined ) { subCLNum = 2; }
   if( beginBound == undefined )
   {
      switch( shardingFormat )
      {
         case "YYYYMMDD":
            beginBound = new Date().getFullYear() * 10000 + 101;
            break;
         case "YYYYMM":
            beginBound = new Date().getFullYear() * 100 + 1;
            break;
         case "YYYY":
            beginBound = new Date().getFullYear();
            break;
         default:
            beginBound = new Date().getFullYear() * 10000 + 101;
      }
   }
   else
   {
      var dateArr = beginBound.toString().split( '' );
      switch( shardingFormat )
      {
         case "YYYYMMDD":
            beginBound = parseInt( beginBound.toString(), 10 );
            break;
         case "YYYYMM":
            beginBound = parseInt( dateArr[0] + dateArr[1] + dateArr[2] + dateArr[3] + dateArr[4] + dateArr[5], 10 );
            break;
         case "YYYY":
            beginBound = parseInt( dateArr[0] + dateArr[1] + dateArr[2] + dateArr[3], 10 );
            break;
         default:
            beginBound = new Date().getFullYear() * 10000 + 101;
      }
   }

   if( scope == undefined ) { scope = 5; }

   var options = { "IsMainCL": true, "ShardingKey": { "date": 1 }, "LobShardingKeyFormat": shardingFormat, "ShardingType": "range" };
   var mainCL = commCreateCL( db, csName, mainCLName, options, true, false, "create main cl" );

   for( var i = 0; i < subCLNum; i++ )
   {
      var subCLName = clName + "_" + i;
      commCreateCL( db, csName, subCLName, { "ShardingKey": { "date": 1 }, "ShardingType": "hash", "AutoSplit": true } );
      var lowBound = { "date": ( parseInt( beginBound ) + i * scope ) + '' };
      var upBound = { "date": ( parseInt( beginBound ) + ( i + 1 ) * scope ) + '' };
      mainCL.attachCL( csName + "." + subCLName, { "LowBound": lowBound, "UpBound": upBound } );
   }

   return mainCL;
}

/************************************
*@Description: 构造临时文件用于插入lob
*@author:      luweikang
*@createDate:  2019.8.7
**************************************/
function makeTmpFile ( filePath, fileName, fileSize )
{
   if( fileSize == undefined ) { fileSize = 1024 * 100; }
   var fileFullPath = filePath + "/" + fileName;
   File.mkdir( filePath );

   var cmd = new Cmd();
   cmd.run( "dd if=/dev/zero of=" + fileFullPath + " bs=1c count=" + fileSize );
   var md5Arr = cmd.run( "md5sum " + fileFullPath ).split( " " );
   var md5 = md5Arr[0];
   return md5
}

/************************************
*@Description: 删除临时文件
*@author:      luweikang
*@createDate:  2019.8.7
**************************************/
function deleteTmpFile ( filePath )
{
   try
   {
      File.remove( filePath );
   } catch( e )
   {
      if( e.message != SDB_FNE )
      {
         throw e;
      }
   }
}

/************************************
*@Description: 插入lob到主表中，lobNum是每个子表会插入的lob数，默认每个子表插入10个lob
*@author:      luweikang
*@createDate:  2019.8.7
**************************************/
function insertLob ( mainCL, filePath, format, scope, lobNum, subCLNum, beginDate )
{
   if( lobNum == undefined ) { lobNum = 10; }
   if( subCLNum == undefined ) { subCLNum = 2; }
   if( scope == undefined ) { scope = 5; }
   if( format == undefined ) { format = "YYYYMMDD" }

   var lobOids = [];
   if( beginDate != undefined )
   {
      var dateArr = beginDate.toString().split( '' );
      var year = parseInt( dateArr[0] + dateArr[1] + dateArr[2] + dateArr[3], 10 );
      var month = parseInt( dateArr[4] + dateArr[5], 10 );
      var day = parseInt( dateArr[6] + dateArr[7], 10 );
   }
   else
   {
      var year = new Date().getFullYear();
      var month = 1;
      var day = 1;
   }

   for( var i = 0; i < subCLNum; i++ )
   {
      for( var j = 0; j < lobNum; j++ )
      {
         var timestamp = null;
         switch( format )
         {
            case "YYYY":
               timestamp = ( year + i * scope ) + "-" + month + "-" + day + "-00.00.00.000000";
               break;
            case "YYYYMM":
               timestamp = year + "-" + ( month + i * scope ) + "-" + day + "-00.00.00.000000";
               break;
            case "YYYYMMDD":
               timestamp = year + "-" + month + "-" + ( day + i * scope ) + "-00.00.00.000000";
               break;
            default:
               timestamp = year + "-" + month + "-" + ( day + i * scope ) + "-00.00.00.000000";
         }
         var lobOid = mainCL.createLobID( timestamp );
         lobOids[i * lobNum + j] = mainCL.putLob( filePath, lobOid );
      }
   }
   return lobOids;
}

/************************************
*@Description: 读取lob并获取md5
*@author:      luweikang
*@createDate:  2019.8.7
**************************************/
function readLobs ( mainCL, lobOids )
{
   var clName = mainCL.toString().split( "." )[2];
   var lobFileMd5s = [];
   for( var i = 0; i < lobOids.length; i++ )
   {
      var cmd = new Cmd();
      var lobReadPath = WORKDIR + "/lob_" + clName + "_" + i;
      try
      {
         File.remove( lobReadPath );
      }
      catch( e ) { }
      mainCL.getLob( lobOids[i], lobReadPath );
      var md5Arr = cmd.run( "md5sum " + lobReadPath ).split( " " );
      var md5 = md5Arr[0];
      lobFileMd5s.push( md5 );
      File.remove( lobReadPath );
   }
   return lobFileMd5s;
}

/************************************
*@Description: 通过比较MD5检查插入的lob内容是否正确
*@author:      luweikang
*@createDate:  2019.8.7
**************************************/
function checkLobMD5 ( mainCL, lobOids, fileMD5 )
{
   var lobFileMd5s = readLobs( mainCL, lobOids );
   for( var i = 0; i < lobOids.length; i++ )
   {
      assert.equal( lobFileMd5s[i], fileMD5 );
   }
}

/************************************
*@Description: 获取主表下的所有子表
*@author:      luweikang
*@createDate:  2019.8.7
**************************************/
function getSubCLNames ( db, mainCLFullName )
{
   var clFullNames = [];
   var cur = db.snapshot( 8, { Name: mainCLFullName } );
   if( cur.next() )
   {
      var clInfo = cur.current();
      var cataInfo = clInfo.toObj().CataInfo
      for( i in cataInfo )
      {
         clFullNames.push( cataInfo[i].SubCLName );
      }
   }
   return clFullNames;
}

/************************************
*@Description: 删除lob并检查删除结果
*@author:      luweikang
*@createDate:  2019.8.7
**************************************/
function deleteLob ( cl, lobOids )
{
   var clName = cl.toString().split( "." )[2];
   for( i in lobOids )
   {
      cl.deleteLob( lobOids[i] );
      assert.tryThrow( SDB_FNE, function()
      {
         cl.getLob( lobOids[i], WORKDIR + "/" + clName + "_" + i );

      } );
   }
}

/************************************
*@Description: 检查主表lob落到子表区域
*@author:      luweikang
*@createDate:  2019.8.7
**************************************/
//TODO:建议优化该检测方法，从对应子表读取所有落入该子表的lob，同时需要检验lob内容正确性
function checkSubCLLob ( db, mainCLFullName, lobOids )
{
   var subCLNames = getSubCLNames( db, mainCLFullName );
   var clName = mainCLFullName.split( "." )[1];
   var lobNum = lobOids.length / subCLNames.length;
   for( i in subCLNames )
   {
      var nameArr = subCLNames[i].split( "." );
      var subCL = db.getCS( nameArr[0] ).getCL( nameArr[1] );
      var actlobNum = 0;
      var cur = subCL.listLobs();
      var tmpLobIds = [];
      while( cur.next() )
      {
         actlobNum++;
         tmpLobIds.push( cur.current() );
      }
      assert.equal( actlobNum, lobNum );
      for( var j = 0; j < lobNum; j++ )
      {
         var lobReadPath = WORKDIR + "/sublob_" + clName + "_" + ( i * lobNum + j );
         try
         {
            File.remove( lobReadPath );
         }
         catch( e )
         {
            if( e.message != SDB_FNE )
            {
               throw e;
            }
         }
         subCL.getLob( lobOids[i * lobNum + j], lobReadPath );
         File.remove( lobReadPath );
      }
   }
}

/************************************
*@Description: 删除主表
*@author:      luweikang
*@createDate:  2019.8.7
**************************************/
function cleanMainCL ( db, mainCSName, mainCLName )
{
   var subCLNames = getSubCLNames( db, mainCSName + "." + mainCLName );
   commDropCL( db, mainCSName, mainCLName );
   for( i in subCLNames )
   {
      var name = subCLNames[i].split( "." );
      var csName = name[0];
      var clName = name[1];
      assert.tryThrow( SDB_DMS_NOTEXIST, function()
      {
         db.getCS( csName ).getCL( clName );
      } );
   }
}

/************************************
*@Description: 带匹配条件listLobs，然后比较listLobs结果
*@author:      wuyan
*@createDate:  2019.8.21
**************************************/
function listLobsAndCheckResult ( mainCL, condition, attrName, attrValue, matchSymbol )
{
   var listResult = mainCL.listLobs( SdbQueryOption().sort( { "Oid": 1 } ) );
   var expListResult = [];
   while( listResult.next() )
   {
      var listObj = listResult.current().toObj();
      var objValue = listObj[attrName];
      switch( matchSymbol )
      {
         case "$lt":
            if( objValue < attrValue )
            {
               expListResult.push( listObj );
            }
            break;
         case "$lte":
            if( objValue <= attrValue )
            {
               expListResult.push( listObj );
            }
            break;
         case "$gt":
            if( objValue > attrValue )
            {
               expListResult.push( listObj );
            }
            break;
         case "$gte":
            if( objValue >= attrValue )
            {
               expListResult.push( listObj );
            }
            break;
         case "$ne":
            if( objValue != attrValue )
            {
               expListResult.push( listObj );
            }
            break;
         case "$et":
            if( objValue == attrValue )
            {
               expListResult.push( listObj );
            }
            break;
         default:
            break;
      }
   }

   var actRecs = [];
   var rc = mainCL.listLobs( SdbQueryOption().cond( condition ).sort( { "Oid": 1 } ) );
   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() );
   }

   assert.equal( actRecs, expListResult );
}

/************************************
*@Description: listLobs指定选择符，默认指定字段为“Size”，然后比较listLobs结果
*@author:      wuyan
*@createDate:  2019.8.23
**************************************/
function listLobsWithSelCondAndCheckResult ( mainCL, selSymbol, selCondition, condition, modifyValue )
{
   if( condition == undefined ) { condition = {}; }
   if( modifyValue == undefined ) { modifyValue = 0; }
   var listResult = mainCL.listLobs( SdbQueryOption().cond( condition ).sort( { "Oid": 1 } ) );
   var expListResult = [];
   while( listResult.next() )
   {
      var listObj = listResult.current().toObj();
      var objValue = listObj["Size"];
      switch( selSymbol )
      {
         case "$include":
            expListResult.push( { "Size": objValue } );
            break;
         case "$add":
            listObj.Size = objValue + modifyValue;
            expListResult.push( listObj );
            break;
         case "$subtract":
            listObj.Size = objValue - modifyValue;
            expListResult.push( listObj );
            break;
         case "$multiply":
            listObj.Size = objValue * modifyValue;
            expListResult.push( listObj );
            break;
         case "$divide":
            listObj.Size = objValue / modifyValue;
            expListResult.push( listObj );
            break;
         default:
            expListResult.push( { "Size": objValue } );
            break;
      }
   }

   var actRecs = [];
   var rc = mainCL.listLobs( SdbQueryOption().cond( condition ).sel( selCondition ).sort( { "Oid": 1 } ) );
   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() );
   }
   assert.equal( actRecs, expListResult );

}

/************************************
*@Description: 比较结果不一致时，将预期结果集和实际结果集数据存到对应文件中
*@author:      wuyan
*@createDate:  2019.8.26
**************************************/
function saveResultToFile ( expResult, actResult, filePath )
{
   var cmd = new Cmd();
   var actResultFileName = filePath + "actResult";
   var expResultFileName = filePath + "expResult";
   var file = new File( actResultFileName );
   file.write( JSON.stringify( actResult ) );
   var file = new File( expResultFileName );
   file.write( JSON.stringify( expResult ) );
}

/************************************
*@Description: listLobs指定query[index], 以下标的方式查询结果
*@author:      wuyan
*@createDate:  2019.8.26
**************************************/
function listLobsWithQueryAndCheckResult ( mainCL, queryIndex )
{
   var listResult = mainCL.listLobs( SdbQueryOption().sort( { "Oid": 1 } ) );
   var expListResult = [];
   var count = 0;
   while( listResult.next() )
   {
      var listObj = listResult.current().toObj();
      if( count == queryIndex )
      {
         expListResult.push( listObj );
         break;
      }
      count++;
   }

   var actRecs = [];
   var query = mainCL.listLobs( SdbQueryOption().sort( { "Oid": 1 } ) );
   actRecs.push( JSON.parse( query[queryIndex] ) );

   assert.equal( actRecs, expListResult );

}

/************************************
*@Description: listLobs指定limit返回指定记录条数，检查返回结果集
*@author:      wuyan
*@createDate:  2019.8.26
**************************************/
function listLobsWithLimitAndCheckResult ( mainCL, filePath, limitNum )
{
   var listResult = mainCL.listLobs( SdbQueryOption().sort( { "Oid": 1 } ) );
   var expListResult = [];
   var count = 0;
   while( listResult.next() )
   {
      var listObj = listResult.current().toObj();
      if( count < limitNum )
      {
         expListResult.push( listObj );
         break;
      }
      count++;
   }

   var actRecs = [];
   var rc = mainCL.listLobs( SdbQueryOption().limit( limitNum ).sort( { "Oid": 1 } ) );;
   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() );
   }

   for( var i in expListResult )
   {
      var actRec = actRecs[i];
      var expRec = expListResult[i];

      for( var f in expRec )
      {
         if( JSON.stringify( actRec[f] ) !== JSON.stringify( expRec[f] ) )
         {
            saveResultToFile( expListResult, actRecs, filePath );
            throw new Error( "rec ERROR, the limitNum=" + limitNum );
         }
      }
   }
}

/************************************
*@Description: listLobs指定skip返回指定记录条数，检查返回结果集
*@author:      wuyan
*@createDate:  2019.8.26
**************************************/
function listLobsWithSkipAndCheckResult ( mainCL, filePath, skipNum )
{
   var listResult = mainCL.listLobs( SdbQueryOption().sort( { "Oid": 1 } ) );
   var expListResult = [];
   var count = 0;
   while( listResult.next() )
   {
      var listObj = listResult.current().toObj();
      if( count >= skipNum )
      {
         expListResult.push( listObj );
      }
      count++;
   }

   var actRecs = [];
   var rc = mainCL.listLobs( SdbQueryOption().skip( skipNum ).sort( { "Oid": 1 } ) );;
   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() );
   }

   for( var i in expListResult )
   {
      var actRec = actRecs[i];
      var expRec = expListResult[i];

      for( var f in expRec )
      {
         if( JSON.stringify( actRec[f] ) !== JSON.stringify( expRec[f] ) )
         {
            saveResultToFile( expListResult, actRecs, filePath );
            throw new Error( "rec ERROR, the limitNum=" + limitNum );
         }
      }
   }
}

/************************************
*@Description: listLobs指定sort正序/逆序排序, 指定排序字段为size或者createTime
*@author:      wuyan
*@createDate:  2019.8.26
**************************************/
function listLobsWithSortAndCheckResult ( mainCL, filePath, sortCond, sortKey, sortOrder )
{
   if( sortOrder == undefined ) { sortOrder = 1; }

   var listResult = mainCL.listLobs();
   var expListResult = [];
   while( listResult.next() )
   {
      var listObj = listResult.current().toObj();
      expListResult.push( listObj );
   }

   var actRecs = [];
   var rc = mainCL.listLobs( SdbQueryOption().sort( sortCond ) );
   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() );
   }

   var compare = function( keyName, sort )
   {
      if( keyName == "Size" && sort == 1 )
      {
         //in ascending order of size
         return function( a, b )
         {
            //if size is equal, in acending order of createTime
            if( a.Size == b.Size )
            {
               if( a.CreateTime['$timestamp'] > b.CreateTime['$timestamp'] )
               {
                  return 1;
               }
               else
               {
                  return -1;
               }
            }
            else
            {
               return a.Size - b.Size;
            }
         }
      }
      else if( keyName == "Size" && sort == -1 )
      {
         //in descending order of size
         return function( a, b )
         {
            //if size is equal, in descending order of createTime
            if( a.Size == b.Size )
            {
               if( a.CreateTime['$timestamp'] > b.CreateTime['$timestamp'] )
               {
                  return -1;
               }
               else
               {
                  return 1;
               }
            }
            else
            {
               return b.Size - a.Size;
            }
         }
      }
      else if( keyName == "CreateTime" && sort == 1 )
      {
         return function( a, b )
         {
            if( a.CreateTime['$timestamp'] == b.CreateTime['$timestamp'] )
            {
               if( a.ModificationTime['$timestamp'] > b.ModificationTime['$timestamp'] )
               {
                  return 1;
               }
               else
               {
                  return -1;
               }
            }
            else if( a.CreateTime['$timestamp'] > b.CreateTime['$timestamp'] )
            {
               return 1;
            }
            else
            {
               return -1;
            }
         }
      }
      else if( keyName == "CreateTime" && sort == -1 )
      {
         return function( a, b )
         {
            if( a.CreateTime['$timestamp'] == b.CreateTime['$timestamp'] )
            {
               if( a.ModificationTime['$timestamp'] > b.ModificationTime['$timestamp'] )
               {
                  return -1;
               }
               else
               {
                  return 1;
               }
            }
            else if( a.CreateTime['$timestamp'] > b.CreateTime['$timestamp'] )
            {
               return -1;
            }
            else
            {
               return 1;
            }
         }
      }
      else
      {
         return 0;
      }
   }
   expListResult.sort( compare( sortKey, sortOrder ) );
   for( var i in expListResult )
   {
      var actRec = actRecs[i];
      var expRec = expListResult[i];

      for( var f in expRec )
      {
         if( JSON.stringify( actRec[f] ) !== JSON.stringify( expRec[f] ) )
         {
            saveResultToFile( expListResult, actRecs, filePath );
            throw new Error( "the cond=" + JSON.stringify( sortCond ) );
         }
      }
   }
}

/************************************
*@Description: listLobs指定sort/skip/limit条件查询, 指定排序字段为size和createTime，正序排序
*@author:      wuyan
*@createDate:  2019.8.26
**************************************/
function listLobsWithQueryOptionAndCheckResult ( mainCL, filePath, skipNum, limitNum )
{
   var listResult = mainCL.listLobs();
   var listResults = [];
   while( listResult.next() )
   {
      var listObj = listResult.current().toObj();
      listResults.push( listObj );
   }
   var compare = function()
   {
      //in ascending order of size
      return function( a, b )
      {
         //if size is equal, in acending order of createTime
         if( a.Size == b.Size )
         {
            if( a.CreateTime['$timestamp'] > b.CreateTime['$timestamp'] )
            {
               return 1;
            }
            else
            {
               return -1;
            }
         }
         else
         {
            return a.Size - b.Size;
         }
      }
   }
   //in ascending order of size, then slice the listResults.
   listResults.sort( compare() );
   var expListResult = listResults.slice( skipNum, limitNum );

   var actRecs = [];
   var rc = mainCL.listLobs( SdbQueryOption().skip( skipNum ).limit( limitNum ).sort( { "Size": 1, "CreateTime": 1 } ) );
   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() );
   }

   for( var i in expListResult )
   {
      var actRec = actRecs[i];
      var expRec = expListResult[i];

      for( var f in expRec )
      {
         if( JSON.stringify( actRec[f] ) !== JSON.stringify( expRec[f] ) )
         {
            saveResultToFile( expListResult, actRecs, filePath );
            throw new Error( "the limitNum=" + limitNum + "\nthe skipNum=" + skipNum );
         }
      }
   }
}

/************************************
*@Description: 获取cl切分的目标组名
*@author:      wuyan
*@createDate:  2019.8.21
**************************************/
function getTargetGroup ( csName, clName, srcGroupName )
{
   if( undefined == csName || undefined == clName )
   {
      throw new Error( "cs or cl name is undefined" );
   }
   var tableName = csName + "." + clName;
   var allGroupInfo = commGetGroups( db );
   var targetGroupName;
   for( var i = 0; i < allGroupInfo.length; ++i )
   {
      var groupName = allGroupInfo[i][0].GroupName;
      if( srcGroupName != groupName )
      {
         targetGroupName = groupName;
         break;
      }
   }
   return targetGroupName;
}

