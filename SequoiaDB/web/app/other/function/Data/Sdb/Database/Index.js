//@ sourceURL=other/Index.js
// --------------------- Data.Database.Index ---------------------
var _DataDatabaseIndex = {} ;

//构建cl列表信息
_DataDatabaseIndex.buildClList = function( $scope, clList ){
   $scope.partitionCLNum = 0 ;
   $scope.clList = [] ;
   if( clList.length == 1 && clList[0]['Name'] == null ) clList = [] ;
   $.each( $scope.csList, function( index2, csInfo ){
      csInfo['clNum'] = 0 ;
   } ) ;
   $scope.clInfo = clList ;
   $.each( clList, function( index, clInfo ){
      if( clInfo['ErrNodes'] !== null && clInfo['Name'] === null )
      {
         $scope.ClTable['options']['text'] = {
            'default': $scope.autoLanguage( '有节点异常，数据可能不完整。' ) + $scope.autoLanguage( '显示 ? 条记录，一共 ? 条' ),
            'filterDefault': $scope.autoLanguage( '有节点异常，数据可能不完整。' ) +  $scope.autoLanguage( '显示 ? 条记录，符合条件的一共 ? 条' )
         } ;
         return true ;
      }
      var fullName = clInfo['Name'].split( '.' ) ;
      var csName = fullName[0] ;
      var clName = fullName[1] ;
      var clListIndex = -1 ;
      //查找cl列表是否已经存在该cl
      $.each( $scope.clList, function( index2, clInfo2 ){
         if( clName == clInfo2['Name'] && csName == clInfo2['csName'] )
         {
            clListIndex = index2 ;
            return false ;
         }
      } ) ;
      //存在则累加
      if( clListIndex >= 0 )
      {
         $scope.clList[clListIndex]['Record']                      += clInfo['TotalRecords'] ;
         $scope.clList[clListIndex]['TotalLobs']                   += clInfo['TotalLobs'] ;
         $scope.clList[clListIndex]['Info']['TotalLobs']           += clInfo['TotalLobs'] ;
         $scope.clList[clListIndex]['Info']['TotalRecords']        += clInfo['TotalRecords'] ;
         $scope.clList[clListIndex]['Info']['TotalDataPages']      += clInfo['TotalDataPages'] ;
         $scope.clList[clListIndex]['Info']['TotalIndexPages']     += clInfo['TotalIndexPages'] ;
         $scope.clList[clListIndex]['Info']['TotalLobPages']       += clInfo['TotalLobPages'] ;
         $scope.clList[clListIndex]['Info']['TotalDataFreeSpace']  += clInfo['TotalDataFreeSpace'] ;
         $scope.clList[clListIndex]['Info']['TotalIndexFreeSpace'] += clInfo['TotalIndexFreeSpace'] ;
         if( clInfo['GroupName'] != null )
         {
            $scope.clList[clListIndex]['GroupName'].push( { 'key': clInfo['GroupName'], 'value': index } ) ;
         }
      }
      //不存在则创建
      else
      {
         var shardingType, shardingTypeDesc ;
         if( clInfo['IsMainCL'] == true )
         {
            shardingType = $scope.autoLanguage( '垂直' ) ;
            shardingTypeDesc = $scope.autoLanguage( '垂直分区' ) ;
         }
         else
         {
            if( clInfo['ShardingType'] == 'range' )
            {
               ++$scope.partitionCLNum ;
               shardingType = $scope.autoLanguage( '水平' ) ;
               shardingTypeDesc = $scope.autoLanguage( '水平范围分区' ) ;
            }
            else if( clInfo['ShardingType'] == 'hash' )
            {
               ++$scope.partitionCLNum ;
               shardingType = $scope.autoLanguage( '水平' ) ;
               shardingTypeDesc = $scope.autoLanguage( '水平散列分区' ) ;
            }
            else
            {
               shardingType = $scope.autoLanguage( '普通' ) ;
               shardingTypeDesc = $scope.autoLanguage( '普通' ) ;
            }
         }
         var isHide = false ;
         var pageSize = 0 ;
         var lobPageSize = 0 ;
         var csId = -1 ;
         //计算cl数量
         $.each( $scope.csList, function( index2, csInfo ){
            if( csInfo['Name'] == csName )
            {
               csId = csInfo['i']
               ++csInfo['clNum'] ;
               if( csInfo['clNum'] > 5 )
               {
                  isHide = true ;
               }
               pageSize = parseInt( csInfo['Info']['PageSize'] ) ;
               lobPageSize = parseInt( csInfo['Info']['LobPageSize'] ) ;
               return false ;
            }
         } ) ;
         $scope.clList.push( {
            'csId': csId,
            'csName': csName,
            'Name': clName,
            'fullName': csName + '.' + clName,
            'type': shardingType,
            'typeDesc': shardingTypeDesc,
            'GroupName': ( clInfo['GroupName'] == null ? [] : [ { 'value': index, 'key': clInfo['GroupName'] } ] ),
            'Lob': clInfo['IsMainCL'] ? null : 0,
            'Record': clInfo['TotalRecords'],
            'TotalLobs': clInfo['IsMainCL'] ? null : clInfo['TotalLobs'],
            'Index': clInfo['Indexes'],
            'IsMainCL': clInfo['IsMainCL'],
            'hide': isHide,
            'pageSize': pageSize,
            'lobPageSize': lobPageSize,
            'MainCLName': clInfo['MainCLName'],
            'ShardingType': clInfo['ShardingType'],
            'Info': {
               'Name':                clName,
               'IsMainCL':            clInfo['IsMainCL'],
               'MainCLName':          clInfo['MainCLName'],
               'ShardingKey':         clInfo['ShardingKey'],
               'ShardingType':        clInfo['ShardingType'],
               'ID':                  clInfo['ID'],
               'LogicalID':           clInfo['LogicalID'],
               'Sequence':            clInfo['Sequence'],
               'GroupName':           clInfo['GroupName'],
               'Status':              clInfo['Status'],
               'Indexes':             clInfo['Indexes'],
               'TotalRecords':        clInfo['TotalRecords'],
               'TotalLobs':           clInfo['TotalLobs'],
               'TotalDataPages':      clInfo['TotalDataPages'],
               'TotalIndexPages':     clInfo['TotalIndexPages'],
               'TotalLobPages':       clInfo['TotalLobPages'],
               'TotalDataFreeSpace':  clInfo['TotalDataFreeSpace'],
               'TotalIndexFreeSpace': clInfo['TotalIndexFreeSpace'],
               'EnsureShardingIndex': clInfo['EnsureShardingIndex'],
               'ReplSize':            clInfo['ReplSize'],
               'LowBound':            null,
               'UpBound':             null,
               'Metadata Ratio':      '0%',
               'CataInfo':            clInfo['CataInfo'],
               'Attribute':           $scope.moduleMode == 'standalone' ? clInfo['Attribute'] : clInfo['AttributeDesc'],
               'CompressionType':     $scope.moduleMode == 'standalone' ? clInfo['CompressionType'] : clInfo['CompressionTypeDesc'],
               'HasDict':             clInfo['HasDict']
            }
         } ) ;
      }
      clInfo['Name'] = clName ;
      clInfo['TotalDataFreeSpace'] = fixedNumber( clInfo['TotalDataFreeSpace'], 2 ) + 'MB' ;
      clInfo['TotalIndexFreeSpace'] = fixedNumber( clInfo['TotalIndexFreeSpace'], 2 ) + 'MB' ;
   } ) ;
   $.each( $scope.clList, function( index, clInfo ){
      if( typeof( clInfo['Info']['TotalDataFreeSpace'] ) == 'number' )
      {
         clInfo['Info']['TotalDataFreeSpace'] = fixedNumber( clInfo['Info']['TotalDataFreeSpace'], 2 ) + 'MB' ;
      }
      if( typeof( clInfo['Info']['TotalIndexFreeSpace'] ) == 'number' )
      {
         clInfo['Info']['TotalIndexFreeSpace'] = fixedNumber( clInfo['Info']['TotalIndexFreeSpace'], 2 ) + 'MB' ;
      }
      if( typeof( clInfo['Info']['TotalIndexPages'] ) == 'number' && typeof( clInfo['Info']['TotalDataPages'] ) == 'number' && typeof( clInfo['Info']['TotalLobPages'] ) == 'number' && clInfo['Info']['TotalDataPages'] + clInfo['Info']['TotalLobPages'] > 0 )
      {
         clInfo['Info']['Metadata Ratio'] = fixedNumber( ( clInfo['Info']['TotalIndexPages'] * 100 * clInfo['pageSize'] ) / ( clInfo['Info']['TotalDataPages'] * clInfo['pageSize'] + clInfo['Info']['TotalLobPages'] * clInfo['lobPageSize'] ), 2 ) + '%' ;
      }
   } ) ;
   $scope.ClTable['body'] = $scope.clList ;
   $.each( $scope.ClTable['body'], function( index ){
      $scope.ClTable['body'][index]['i'] = index ;
      if( $scope.fullName != null && $scope.fullName.length > 0 && $scope.fullName == $scope.ClTable['body'][index]['fullName'] )
      {
         $scope.showCLInfo( $scope.ClTable['body'][index]['csId'], $scope.ClTable['body'][index]['i'] ) ;
      }
   } ) ;
}

//获取所有cs信息
_DataDatabaseIndex.getCSInfo = function( $scope, SdbRest ){
   var sql ;
   if( $scope.moduleMode == 'standalone' )
   {
      sql = 'SELECT Name, PageSize/1024, LobPageSize/1024, GroupName, TotalRecords, FreeDataSize/1048576, FreeIndexSize/1048576, FreeLobSize/1048576, FreeSize/1048576, MaxDataCapSize/1073741824, MaxIndexCapSize/1073741824, MaxLobCapSize/1073741824, TotalDataSize/1048576, TotalIndexSize/1048576, TotalLobSize/1048576, TotalSize/1048576, ErrNodes FROM $SNAPSHOT_CS ORDER BY Name' ;
   }
   else
   {
      sql = 'SELECT Name, PageSize/1024, LobPageSize/1024, GroupName, TotalRecords, FreeDataSize/1048576, FreeIndexSize/1048576, FreeLobSize/1048576, FreeSize/1048576, MaxDataCapSize/1073741824, MaxIndexCapSize/1073741824, MaxLobCapSize/1073741824, TotalDataSize/1048576, TotalIndexSize/1048576, TotalLobSize/1048576, TotalSize/1048576, ErrNodes FROM $SNAPSHOT_CS WHERE NodeSelect="master" ORDER BY Name' ;
   }
   function dataSizeFmt( value, str )
   {
      if( typeof( value ) == 'number' )
      {
         return fixedNumber( value, 2 ) + str ;
      }
      else
      {
         return value ;
      }
   }
   function dataPlus( value )
   {
      if( typeof( value ) == 'number' )
      {
         return value ;
      }
      else
      {
         return 0 ;
      }
   }
   //获取cs的信息
   SdbRest.Exec( sql, {
      'success': function( csList ){
         //过滤错误节点的信息
         var tmpCsList = csList ;
         var tmpSnapshotCSList = [] ; //记录下用快照查到的cs列表，用作性能优化
         csList = [] ;
         $.each( tmpCsList, function( index, csInfo ){
            if( csInfo['ErrNodes'] === null )
            {
               csList.push( csInfo ) ;
               tmpSnapshotCSList.push( csInfo['Name'] ) ;
            }
            else
            {
               $scope.CsTable['options']['text'] = {
                  'default': $scope.autoLanguage( '有节点异常，数据可能不完整。' ) + $scope.autoLanguage( '显示 ? 条记录，一共 ? 条' ),
                  'filterDefault': $scope.autoLanguage( '有节点异常，数据可能不完整。' ) +  $scope.autoLanguage( '显示 ? 条记录，符合条件的一共 ? 条' )
               } ;
            }
         } ) ;
         var data = { 'cmd': 'list collectionspaces', 'sort': JSON.stringify( { 'Name': 1 } ) } ;
         SdbRest.DataOperation( data, {
            'success': function( csList2 ){

               //构造cs汇总数据
               $scope.csList = [] ;
               {
                  var tmpSnapshotCSList2 = [] ;
                  $.each( csList, function( index, csInfo ){
                     var csListIndex = tmpSnapshotCSList2.indexOf( csInfo['Name'] ) ;
                     //存在则累加
                     if( csListIndex >= 0 )
                     {
                        $scope.csList[csListIndex]['TotalRecords']            += dataPlus( csInfo['TotalRecords'] ) ;
                        $scope.csList[csListIndex]['TotalSize']               += dataPlus( csInfo['TotalSize'] ) ;

                        $scope.csList[csListIndex]['Info']['TotalRecords']    += dataPlus( csInfo['TotalRecords'] ) ;
                        $scope.csList[csListIndex]['Info']['TotalDataSize']   += dataPlus( csInfo['TotalDataSize'] ) ;
                        $scope.csList[csListIndex]['Info']['FreeDataSize']    += dataPlus( csInfo['FreeDataSize'] ) ;
                        $scope.csList[csListIndex]['Info']['TotalIndexSize']  += dataPlus( csInfo['TotalIndexSize'] ) ;
                        $scope.csList[csListIndex]['Info']['FreeIndexSize']   += dataPlus( csInfo['FreeIndexSize'] ) ;
                        $scope.csList[csListIndex]['Info']['TotalLobSize']    += dataPlus( csInfo['TotalLobSize'] ) ;
                        $scope.csList[csListIndex]['Info']['FreeLobSize']     += dataPlus( csInfo['FreeLobSize'] ) ;
                        $scope.csList[csListIndex]['Info']['MaxDataCapSize']  += dataPlus( csInfo['MaxDataCapSize'] ) ;
                        $scope.csList[csListIndex]['Info']['MaxIndexCapSize'] += dataPlus( csInfo['MaxIndexCapSize'] ) ;
                        $scope.csList[csListIndex]['Info']['MaxLobCapSize']   += dataPlus( csInfo['MaxLobCapSize'] ) ;
                        $scope.csList[csListIndex]['Info']['TotalSize']       += dataPlus( csInfo['TotalSize'] ) ;
                        $scope.csList[csListIndex]['Info']['FreeSize']        += dataPlus( csInfo['FreeSize'] ) ;
                        if( csInfo['GroupName'] != null )
                        {
                           $scope.csList[csListIndex]['GroupName'].push( { 'key': csInfo['GroupName'], 'value': index } ) ;
                        }
                     }
                     //不存在则创建
                     else
                     {
                        var color = '' ;
                        var i = $scope.csList.length ;
                        if( i > 3 ) i = i % 4 ;
                        if( i == 0 ) color = 'green' ;
                        if( i == 1 ) color = 'yellow' ;
                        if( i == 2 ) color = 'blue' ;
                        if( i == 3 ) color = 'violet' ;
                        tmpSnapshotCSList2.push( csInfo['Name'] ) ;
                        $scope.csList.push( {
                           'Name': csInfo['Name'],
                           'clNum': 0,
                           'GroupName': ( csInfo['GroupName'] == null ? [] : [ { 'value': index, 'key': csInfo['GroupName'] } ] ),
                           'hide': false,
                           'color': color,
                           'show': false,
                           'TotalRecords': csInfo['TotalRecords'],
                           'TotalSize': csInfo['TotalSize'],
                           'Info': {
                              'Name':            csInfo['Name'],
                              'PageSize':        csInfo['PageSize'],
                              'LobPageSize':     csInfo['LobPageSize'],
                              'TotalRecords':    csInfo['TotalRecords'],
                              'FreeDataSize':    csInfo['FreeDataSize'],
                              'FreeIndexSize':   csInfo['FreeIndexSize'],
                              'FreeLobSize':     csInfo['FreeLobSize'],
                              'FreeSize':        csInfo['FreeSize'],
                              'MaxDataCapSize':  csInfo['MaxDataCapSize'],
                              'MaxIndexCapSize': csInfo['MaxIndexCapSize'],
                              'MaxLobCapSize':   csInfo['MaxLobCapSize'],
                              'TotalDataSize':   csInfo['TotalDataSize'],
                              'TotalIndexSize':  csInfo['TotalIndexSize'],
                              'TotalLobSize':    csInfo['TotalLobSize'],
                              'TotalSize':       csInfo['TotalSize']
                           }
                        } ) ;
                     }
                  } ) ;

                  //全部保留2位小数，增加单位（这是给列表用的, 汇总信息）
                  $.each( $scope.csList, function( index, csInfo ){
                     csInfo['TotalSize']               = dataSizeFmt( csInfo['TotalSize'], 'MB' ) ;

                     csInfo['Info']['PageSize']        = dataSizeFmt( csInfo['Info']['PageSize'], 'KB' ) ;
                     csInfo['Info']['LobPageSize']     = dataSizeFmt( csInfo['Info']['LobPageSize'], 'KB' ) ;
                     csInfo['Info']['TotalRecords']    = dataSizeFmt( csInfo['Info']['TotalRecords'], '' ) ;
                     csInfo['Info']['TotalDataSize']   = dataSizeFmt( csInfo['Info']['TotalDataSize'], 'MB' ) ;
                     csInfo['Info']['FreeDataSize']    = dataSizeFmt( csInfo['Info']['FreeDataSize'], 'MB' ) ;
                     csInfo['Info']['TotalIndexSize']  = dataSizeFmt( csInfo['Info']['TotalIndexSize'], 'MB' ) ;
                     csInfo['Info']['FreeIndexSize']   = dataSizeFmt( csInfo['Info']['FreeIndexSize'], 'MB' ) ;
                     csInfo['Info']['TotalLobSize']    = dataSizeFmt( csInfo['Info']['TotalLobSize'], 'MB' ) ;
                     csInfo['Info']['FreeLobSize']     = dataSizeFmt( csInfo['Info']['FreeLobSize'], 'MB' ) ;
                     csInfo['Info']['TotalSize']       = dataSizeFmt( csInfo['Info']['TotalSize'], 'MB' ) ;
                     csInfo['Info']['FreeSize']        = dataSizeFmt( csInfo['Info']['FreeSize'], 'MB' ) ;
                     csInfo['Info']['MaxDataCapSize']  = dataSizeFmt( csInfo['Info']['MaxDataCapSize'], 'GB' ) ;
                     csInfo['Info']['MaxIndexCapSize'] = dataSizeFmt( csInfo['Info']['MaxIndexCapSize'], 'GB' ) ;
                      csInfo['Info']['MaxLobCapSize']   = dataSizeFmt( csInfo['Info']['MaxLobCapSize'], 'GB' ) ;
                  } ) ;
               }

               //构造cs的数据
               $.each( csList2, function( index2, csInfo2 ){
                  if( tmpSnapshotCSList.indexOf( csInfo2['Name'] ) < 0 )
                  {
                     csList.push( csInfo2 ) ;   //构造cs在分区组的详细信息数据

                     {                          //构造cs汇总数据
                        var color = '' ;
                        var i = $scope.csList.length ;
                        if( i > 3 ) i = i % 4 ;
                        if( i == 0 ) color = 'green' ;
                        if( i == 1 ) color = 'yellow' ;
                        if( i == 2 ) color = 'blue' ;
                        if( i == 3 ) color = 'violet' ;
                        $scope.csList.push( {
                           'Name': csInfo2['Name'],
                           'clNum': 0,
                           'GroupName': [],
                           'hide': false,
                           'color': color,
                           'show': false,
                           'TotalRecords': 0,
                           'TotalSize': '0MB',
                           'Info': {
                              'Name':  csInfo2['Name']
                           }
                        } ) ;
                     }

                     tmpSnapshotCSList.push( csInfo2['Name'] ) ;
                  }
               } ) ;
               $scope.csInfo = csList ;

               //全部保留2位小数，增加单位（这是给右边的详细信息用的，没有汇总，只有单个组的）
               $.each( $scope.csInfo, function( index, csInfo ){
                  csInfo['PageSize']        = dataSizeFmt( csInfo['PageSize'], 'KB' ) ;
                  csInfo['LobPageSize']     = dataSizeFmt( csInfo['LobPageSize'], 'KB' ) ;
                  csInfo['TotalDataSize']   = dataSizeFmt( csInfo['TotalDataSize'], 'MB' ) ;
                  csInfo['FreeDataSize']    = dataSizeFmt( csInfo['FreeDataSize'], 'MB' ) ;
                  csInfo['TotalIndexSize']  = dataSizeFmt( csInfo['TotalIndexSize'], 'MB' ) ;
                  csInfo['FreeIndexSize']   = dataSizeFmt( csInfo['FreeIndexSize'], 'MB' ) ;
                  csInfo['TotalLobSize']    = dataSizeFmt( csInfo['TotalLobSize'], 'MB' ) ;
                  csInfo['FreeLobSize']     = dataSizeFmt( csInfo['FreeLobSize'], 'MB' ) ;
                  csInfo['TotalSize']       = dataSizeFmt( csInfo['TotalSize'], 'MB' ) ;
                  csInfo['FreeSize']        = dataSizeFmt( csInfo['FreeSize'], 'MB' ) ;
                  csInfo['MaxDataCapSize']  = dataSizeFmt( csInfo['MaxDataCapSize'], 'GB' ) ;
                  csInfo['MaxIndexCapSize'] = dataSizeFmt( csInfo['MaxIndexCapSize'], 'GB' ) ;
                  csInfo['MaxLobCapSize']   = dataSizeFmt( csInfo['MaxLobCapSize'], 'GB' ) ;
               } ) ;

               $scope.showCSInfo( 0 ) ;
               $scope.$apply() ;
               $scope.CsTable['body'] = $scope.csList ;
               $.each( $scope.CsTable['body'], function( index ){
                  $scope.CsTable['body'][index]['i'] = index ;
               } ) ;
               _DataDatabaseIndex.getCLInfo( $scope, SdbRest ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  _DataDatabaseIndex.getCSInfo( $scope, SdbRest ) ;
                  return true ;
               } ) ;
            }
         }, {
            'showLoading': false
         } ) ;
      },
      'failed': function( errorInfo ){
         _IndexPublic.createRetryModel( $scope, errorInfo, function(){
            _DataDatabaseIndex.getCSInfo( $scope, SdbRest ) ;
             return true ;
         } ) ;
      }
   },{
      'showLoading': false
   } ) ;
}

//获取所有cl信息
_DataDatabaseIndex.getCLInfo = function( $scope, SdbRest )
{
   $scope.subCLNum = 0 ;
   $scope.mainCLNum = 0 ;
   var sql ;
   if( $scope.moduleMode == 'standalone' )
   {
      sql = 'SELECT t1.Name, t1.Details.ID, t1.Details.LogicalID, t1.Details.Sequence, t1.Details.Status,t1.Details.Attribute, t1.Details.Status,t1.Details.CompressionType, t1.Details.CompressionType, t1.Details.Indexes, t1.Details.TotalRecords, t1.Details.TotalLobs, t1.Details.TotalDataPages, t1.Details.HasDict, t1.Details.TotalIndexPages, t1.Details.TotalLobPages, t1.Details.TotalDataFreeSpace/1048576, t1.Details.TotalIndexFreeSpace/1048576, t1.ErrNodes FROM (SELECT * FROM $SNAPSHOT_CL split BY Details) AS t1 ORDER BY t1.Name' ;
   }
   else
   {
      sql = 'SELECT t1.Name, t1.Details.ID, t1.Details.LogicalID, t1.Details.Sequence, t1.Details.GroupName, t1.Details.Status, t1.Details.Indexes, t1.Details.TotalRecords, t1.Details.TotalLobs, t1.Details.TotalDataPages, t1.Details.HasDict, t1.Details.TotalIndexPages, t1.Details.TotalLobPages, t1.Details.TotalDataFreeSpace/1048576, t1.Details.TotalIndexFreeSpace/1048576, t1.IsMainCL, t1.MainCLName, t1.ShardingKey, t1.ShardingType, t1.ErrNodes FROM (SELECT * FROM $SNAPSHOT_CL WHERE NodeSelect="master" split BY Details) AS t1 ORDER BY t1.Name' ;
   }

   //合并cl数据
   function mergedData( clList, cataList ){
      $.each( cataList, function( index, cataInfo ){
         if( cataInfo['IsMainCL'] == true )
         {
            ++$scope.mainCLNum ;
            clList.push( cataInfo ) ;
            cataInfo['TotalRecords'] = 0 ;
            $.each( cataInfo['CataInfo'], function( index2, rangeInfo ){
               $.each( clList, function( index3, clInfo ){
                  if( clInfo['Name'] == rangeInfo['SubCLName'] )
                  {
                     cataInfo['TotalRecords'] += clInfo['TotalRecords'] ;
                  }
               } ) ;
               rangeInfo['LowBound'] = JSON.stringify( rangeInfo['LowBound'] ) ;
               rangeInfo['UpBound'] = JSON.stringify( rangeInfo['UpBound'] ) ;
               ++$scope.subCLNum ;
            } ) ;
         }
         else
         {
            $.each( clList, function( index2, clInfo ){
               if( clInfo['Name'] == cataInfo['Name'] )
               {
                  if( typeof( cataInfo['MainCLName'] ) == 'string' )
                  {
                     clInfo['MainCLName'] = cataInfo['MainCLName'] ;
                  }
                  if( typeof( cataInfo['ShardingType'] ) == 'string' )
                  {
                     clInfo['ShardingType'] = cataInfo['ShardingType'] ;
                     clInfo['ShardingKey'] = cataInfo['ShardingKey'] ;
                     clInfo['LowBound'] = [] ;
                     clInfo['UpBound'] = [] ;
                     clInfo['ReplSize'] = cataInfo['ReplSize'] ;
                     clInfo['EnsureShardingIndex'] = cataInfo['EnsureShardingIndex'] ;
                     $.each( cataInfo['CataInfo'], function( index3, groupInfo ){
                        if( groupInfo['GroupName'] == clInfo['GroupName'] )
                        {
                           clInfo['LowBound'].push( JSON.stringify( groupInfo['LowBound'] ) ) ;
                           clInfo['UpBound'].push( JSON.stringify( groupInfo['UpBound'] ) ) ;
                        }
                     } ) ;
                  }
                  if( typeof( cataInfo['ReplSize'] ) != 'undefined' )
                  {
                     clInfo['ReplSize'] = cataInfo['ReplSize'] ;
                  }
                  if( typeof( cataInfo['AttributeDesc'] ) != 'undefined' )
                  {
                     clInfo['AttributeDesc'] = cataInfo['AttributeDesc'] ;
                  }
                  if( typeof( cataInfo['CompressionTypeDesc'] ) != 'undefined' )
                  {
                     clInfo['CompressionTypeDesc'] = cataInfo['CompressionTypeDesc'] ;
                  }
               }
            } ) ;
         }
      } ) ;
      $scope.sourceClList = $.extend( true, [], clList ) ;
      var newClList = [] ;
      if( $scope.isHideSubCl == true )
      {
         $.each( clList, function( index, clInfo ){
            if( typeof( clInfo['MainCLName'] ) != 'string' )
            {
               newClList.push( clInfo ) ;
            }
         } ) ;
      }
      else
      {
         newClList = clList ;
      }
      return newClList ;
   }

   //查询成功执行的
   function success( clList ){
      _DataDatabaseIndex.buildClList( $scope, clList ) ;
   }

   //获取cl信息
   SdbRest.Exec( sql, {
      'success': function( clList ){
         if( $scope.moduleMode == 'standalone' )
         {
            success( clList ) ;
         }
         else
         {
            sql = 'select * from $SNAPSHOT_CATA' ;
            SdbRest.Exec( sql, {
               'success': function( cataList ){
                  var newList = mergedData( clList, cataList ) ;
                  success( newList ) ;
               },
               'failed': function( errorInfo ){
                  _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                     _DataDatabaseIndex.getCLInfo( $scope, SdbRest ) ;
                     return true ;
                  } ) ;
               }
            },{
               'showLoading': false
            } ) ;
         }
      },
      'failed': function( errorInfo ){
         _IndexPublic.createRetryModel( $scope, errorInfo, function(){
            _DataDatabaseIndex.getCLInfo( $scope, SdbRest ) ;
            return true ;
         } ) ;
      }
   },{
      'showLoading': false
   } ) ;
}
