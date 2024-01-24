//@ sourceURL=Index.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Data.Database.Index.Ctrl', function( $scope, $location, $timeout, SdbFunction, SdbRest ){
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      if( clusterName == null || moduleType != 'sequoiadb' || moduleMode == null || moduleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      // == 初始化参数 ==
      //分区组列表
      $scope.GroupList = [] ;
      //集群名
      $scope.clusterName = clusterName ;
      //业务名
      $scope.moduleName = moduleName ;
      //业务模式
      $scope.moduleMode = moduleMode ;
      //收起cl列表时，最大显示cl数
      $scope.maxShowCLNum = 5 ;
      //当前选中的cs的ID
      $scope.csID = 0 ;
      //当前选中的cl的ID
      $scope.clID = 0 ;
      //cs列表
      $scope.csList = [] ;
      //cs详细信息
      $scope.csInfo = [] ;
      //原始cl列表
      $scope.sourceClList = [] ;
      //cl列表
      $scope.clList = [] ;
      //cl详细信息
      $scope.clInfo = [] ;
      //参数列表
      $scope.attr = {} ;
      //当前所选的group
      $scope.selectGroup = null ;
      //搜索内容
      $scope.find = '' ;
      //符合搜索的数量
      $scope.findNum = 0 ;
      //是否隐藏子集合
      $scope.isHideSubCl = SdbFunction.LocalData( 'SdbHidePartition' ) ? true : false ;
      //主表数量
      $scope.mainCLNum = 0 ;
      //子表数量
      $scope.subCLNum = 0 ;
      //分区表数量
      $scope.partitionCLNum = 0 ;
      //是否有自增字段
      $scope.HasAutoIncrement = false ;
      //右侧高度偏移量
      $scope.boxHeight = ( moduleMode == 'distribution' ) ? { 'offsetY': -335 } : { 'offsetY': -254 } ;
      //判断如果是Firefox浏览器的话，调整右侧表格高度
      var browser = SdbFunction.getBrowserInfo() ;
      if( browser[0] == 'firefox' )
      {
         $scope.boxHeight = ( moduleMode == 'distribution' ) ? { 'offsetY': -371 } : { 'offsetY': -281 } ;
      }
      //域列表
      var domainList = [] ;
      //集合空间列表
      $scope.CsTable = {
         'title': {
            'Name': $scope.autoLanguage( '集合空间' ),
            'TotalRecords': $scope.autoLanguage( '总记录数' ),
            'TotalSize': $scope.autoLanguage( '总大小' ),
            'clNum': $scope.autoLanguage( '集合' ),
            'GroupName.length': moduleMode == 'distribution' ? $scope.autoLanguage( '分区组' ) : false
         },
         'body': [],
         'options': {
            'width': {
               'Name': '40%',
               'TotalRecords': '15%',
               'TotalSize': '15%',
               'clNum': '15%',
               'GroupName.length': '15%'
            },
            'sort': {
               'Name': true,
               'TotalRecords': true,
               'TotalSize': true,
               'clNum': true,
               'GroupName.length': true
            },
            'max': 50,
            'filter': {
               'Name': 'indexof',
               'TotalRecords': 'number',
               'TotalSize': 'indexof',
               'clNum': 'number',
               'GroupName.length': 'number'
            }
         }
      } ;
      //集合列表
      $scope.ClTable = {
         'title': {
            'fullName': $scope.autoLanguage( '集合' ),
            'typeDesc': moduleMode == 'standalone' ? false : $scope.autoLanguage( '分区类型' ),
            'MainCLName': $scope.isHideSubCl == true || moduleMode == 'standalone' ? false : $scope.autoLanguage( '归属集合' ),
            'TotalLobs': $scope.autoLanguage( 'Lob数' ),
            'Record': $scope.autoLanguage( '记录数' ),
            'AutoIncrement': $scope.autoLanguage( '自增字段数' ),
            'Index': $scope.autoLanguage( '索引数' ),
            'TotalTbScan': $scope.autoLanguage( '表扫描数' ),
            'TotalIxScan': $scope.autoLanguage( '索引扫描数' )
         },
         'body': [],
         'options': {
            'sort': {
               'fullName': true,
               'typeDesc': true,
               'MainCLName': true,
               'TotalLobs': true,
               'Record': true,
               'AutoIncrement': true,
               'Index': true,
               'TotalTbScan': true,
               'TotalIxScan': true
            },
            'max': 50,
            'filter': {
               'fullName': 'indexof',
               'typeDesc' :[
                  { 'key': $scope.autoLanguage( '全部' ), 'value': '' },
                  { 'key': $scope.autoLanguage( '普通' ), 'value': $scope.autoLanguage( '普通' ) },
                  { 'key': $scope.autoLanguage( '水平范围分区' ), 'value': $scope.autoLanguage( '水平范围分区' ) },
                  { 'key': $scope.autoLanguage( '水平散列分区' ), 'value': $scope.autoLanguage( '水平散列分区' ) },
                  { 'key': $scope.autoLanguage( '垂直分区' ), 'value': $scope.autoLanguage( '垂直分区' ) }
               ],
               'MainCLName': 'indexof',
               'TotalLobs': 'number',
               'Record': 'number',
               'AutoIncrement': 'indexof',
               'Index': 'number',
               'TotalTbScan': 'number',
               'TotalIxScan': 'number'
            }
         }
      } ;
      //分离集合 弹窗
      $scope.DetachCL = {
         'config': {},
         'callback': {}
      } ;

      //获取域列表
      if( $scope.moduleMode == 'distribution' )
      {
         var getDomainList = function(){
            var data = { 'cmd': 'list domains' } ;
            SdbRest.DataOperation( data, {
               'success': function( domains ){
                  //存在域时才执行
                  if( domains.length > 0 )
                  {
                     domainList = domains ;
                  }
               },
               'failed': function( errorInfo ){
                  _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                     getDomainList() ;
                     return true ;
                  } ) ;
               }
            }, { 'showLoading': false } ) ;
         }
         getDomainList() ;
      }

      var setClTableTitle = function(){
         if( $scope.isHideSubCl == true || moduleMode == 'standalone' )
         {
            $scope.ClTable['title']['MainCLName'] = false ;
         }
         else
         {
            $scope.ClTable['title']['MainCLName'] = $scope.autoLanguage( '归属集合' ) ;
         }
      }

      setClTableTitle() ;

      //获取分区组列表
      if( $scope.moduleMode == 'distribution' )
      {
         var getGroupList = function(){
            var data = { 'cmd': 'list groups', 'sort': JSON.stringify( { 'GroupName': 1 } ) } ;
            SdbRest.DataOperation( data, {
               'success': function( groups ){
                  $.each( groups, function( index, groupInfo ){
                     if( groupInfo['Role'] == 0 )
                     {
                        $scope.GroupList.push( groupInfo ) ;
                     }
                  } ) ;
               },
               'failed': function( errorInfo ){
                  _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                     getGroupList() ;
                     return true ;
                  } ) ;
               }
            }, {
               'showLoading': false
            } ) ;
         } ;
         getGroupList() ;
      }
      
      //跳到记录页面
      $scope.gotoRecord = function( csIndex, clIndex ){
         var csName = $scope.csList[csIndex]['Name'] ;
         var clName = $scope.clList[clIndex]['Info']['Name'] ;
         var clType = '' ;
         if( $scope.clList[clIndex]['Info']['IsMainCL'] == true )
         {
            clType = 'main' ;
         }
         SdbFunction.LocalData( 'SdbCsName', csName ) ;
         SdbFunction.LocalData( 'SdbClName', clName ) ;
         SdbFunction.LocalData( 'SdbClType', clType ) ;
         $location.path( 'Data/SDB-Operate/Record' ).search( { 'r': new Date().getTime() } ) ;
      }

      //跳到Lob页面
      $scope.gotoLob = function( csIndex, clIndex ){
         var csName = $scope.csList[csIndex]['Name'] ;
         var clName = $scope.clList[clIndex]['Info']['Name'] ;
         var clType = '' ;
         if( $scope.clList[clIndex]['Info']['IsMainCL'] == true )
         {
            clType = 'main' ;
         }
         SdbFunction.LocalData( 'SdbCsName', csName ) ;
         SdbFunction.LocalData( 'SdbClName', clName ) ;
         SdbFunction.LocalData( 'SdbClType', clType ) ;
         $location.path( 'Data/SDB-Operate/Lobs' ).search( { 'r': new Date().getTime() } ) ;
      }

      //过滤CS和cl
      $scope.search = function( fullName ){
         $scope.findNum = 0 ;
         $scope.find = fullName ;
         var isNumber = !isNaN( fullName ) ;
         var len = fullName.length ;
         var pointIndex = fullName.indexOf( '.' ) ;
         fullName = trim( fullName ) ;
         if( len > 0 )
         {
            //xxx.???
            if( pointIndex > 0 )
            {
               //xxx.xxx
               if( pointIndex < len - 1 )
               {
                  var csName = '', clName = '' ;
                  var tmp = fullName.split( '.' ) ;
                  csName = tmp[0] ;
                  clName = tmp[1] ;
                  $.each( $scope.csList, function( index, csInfo ){
                     csInfo.hide = !( csInfo['Name'] == csName ) ;
                  } ) ;
                  $.each( $scope.clList, function( index, clInfo ){
                     clInfo.hide = !( clInfo['Name'] == clName && clInfo['csName'] == csName ) ;
                  } ) ;
               }
               //xxx.
               else
               {
                  var csName = '' ;
                  var tmp = fullName.split( '.' ) ;
                  csName = tmp[0] ;
                  $.each( $scope.csList, function( index, csInfo ){
                     csInfo.hide = !( csInfo['Name'] == csName ) ;
                  } ) ;
                  $.each( $scope.clList, function( index, clInfo ){
                     clInfo.hide = !( clInfo['csName'] == csName ) ;
                  } ) ;
               }
            }
            //.xxx
            else if( pointIndex == 0 )
            {
               //.
               if( len == 0 )
               {
                  $.each( $scope.clList, function( index, clInfo ){
                     clInfo.hide = false ;
                  } ) ;
                  $.each( $scope.csList, function( index, csInfo ){
                     csInfo.hide = false ;
                  } ) ;
               }
               //.xx
               else
               {
                  var clName = '' ;
                  var tmp = fullName.split( '.' ) ;
                  clName = tmp[1] ;
                  $.each( $scope.csList, function( index, csInfo ){
                     csInfo.hide = true ;
                  } ) ;
                  $.each( $scope.clList, function( index, clInfo ){
                     clInfo.hide = true
                     if( clInfo['Name'] == clName )
                     {
                        clInfo.hide = false ;
                        $.each( $scope.csList, function( index, csInfo ){
                           if( csInfo['Name'] == clInfo['csName'] )
                           {
                              csInfo.hide = false ;
                              return false ;
                           }
                        } ) ;
                     }
                  } ) ;
               }
            }
            //xxx
            else
            {
               $.each( $scope.csList, function( index, csInfo ){
                  csInfo.hide = true ;
               } ) ;
               $.each( $scope.clList, function( index, clInfo ){
                  clInfo.hide = true ;
                  if( clInfo['Name'].indexOf( fullName ) >= 0 )
                  {
                     clInfo.hide = false ;
                     $.each( $scope.csList, function( index, csInfo ){
                        if( csInfo['Name'] == clInfo['csName'] )
                        {
                           csInfo.hide = false ;
                           return false ;
                        }
                     } ) ;
                  }
               } ) ;
               $.each( $scope.csList, function( index, csInfo ){
                  if( csInfo['Name'].indexOf( fullName ) >= 0 )
                  {
                     csInfo.hide = false ;
                     $.each( $scope.clList, function( index, clInfo ){
                        if( clInfo['csName'] == csInfo['Name'] )
                        {
                           clInfo.hide = false ;
                        }
                     } ) ;
                  }
               } ) ;
               if( isNumber )
               {
                  var num = parseInt( fullName ) ;
                  $.each( $scope.clList, function( index, clInfo ){
                     if( clInfo['TotalLobs'] == num || clInfo['Record'] == num || clInfo['Index'] == num )
                     {
                        clInfo.hide = false ;
                        $.each( $scope.csList, function( index, csInfo ){
                           if( csInfo['Name'] == clInfo['csName'] )
                           {
                              csInfo.hide = false ;
                              return false ;
                           }
                        } ) ;
                     }
                  } ) ;
               }
            }
         }
         else
         {
            $.each( $scope.clList, function( index, clInfo ){
               clInfo.hide = false ;
            } ) ;
            $.each( $scope.csList, function( index, csInfo ){
               csInfo.hide = false ;
               $scope.clTableHide( csInfo['Name'] ) ;
            } ) ;
         }
         $.each( $scope.csList, function( index, csInfo ){
            if( csInfo.hide == false )
            {
               ++$scope.findNum ;
            }
         } ) ;
      }

      //展开cs下所有的cl
      $scope.clTableShow = function( csName ){
         $.each( $scope.clList, function( index, clInfo ){
            if( clInfo.csName == csName )
            {
               clInfo.hide = false ;
            }
         } ) ;
         $.each( $scope.csList, function( index, csInfo ){
            if( csInfo.Name == csName )
            {
               csInfo.show = true ;
               return false ;
            }
         } ) ;
      }

      //收起cs下所有的cl
      $scope.clTableHide = function( csName ){
         var showNum = 0 ;
         $.each( $scope.clList, function( index, clInfo ){
            if( clInfo.csName == csName )
            {
               ++showNum ;
               clInfo.hide = ( showNum > $scope.maxShowCLNum ) ;
               if( $scope.showType == 'cl' && $scope.clID == index )
               {
                  clInfo.hide = false ;
               }
            }
         } ) ;
         $.each( $scope.csList, function( index, csInfo ){
            if( csInfo.Name == csName )
            {
               csInfo.show = false ;
               return false ;
            }
         } ) ;
      }

      //展示CS属性
      $scope.showCSInfo = function( index ){
         $scope.showType = 'cs' ;
         $scope.csID = index ;
         $scope.selectGroup = null ;
         $scope.attr = $.extend( true, {}, $scope.csList[index] ) ;
      }

      //展示CL属性
      $scope.showCLInfo = function( csIndex, clIndex ){
         $scope.showType = 'cl' ;
         $scope.csID = csIndex ;
         $scope.clID = clIndex ;
         $scope.selectGroup = null ;
         $scope.attr = $.extend( true, {}, $scope.clList[clIndex] ) ;
      }
      
      $scope.fullName = SdbFunction.LocalData( 'SdbFullName' ) ;
      SdbFunction.LocalData( 'SdbFullName', null ) ;

      //打开 创建集合空间 的窗口
      $scope.showCreateCS = function(){
         if( _IndexPublic.checkEditionAndSupport( $scope, 'sequoiadb', 'Metadata' ) == false )
         {
            return ;
         }
         var domainInput = [ { 'key': '', 'value': '' } ] ;
         if( domainList.length > 0 )
         {
            $.each( domainList, function( index, domainInfo ){
               domainInput.push( { 'key': domainInfo['Name'], 'value': domainInfo['Name'] } ) ;
            } ) ;
         }
         $scope.Components.Modal.icon = 'fa-plus' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '创建集合空间' ) ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            inputList: [
               {
                  "name": "name",
                  "webName": $scope.autoLanguage( '集合空间名' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1,
                     "max": 127,
                     "ban": [ ".", "$" ]
                  }
               },
               {
                  "name": "PageSize",
                  "webName": $scope.autoLanguage( '数据页大小' ),
                  "type": "select",
                  "desc": $scope.autoLanguage( '数据页大小创建后不可更改。' ),
                  "value": "65536",
                  "valid": [
                     { "key": "4KB", "value": "4096" },
                     { "key": "8KB", "value": "8192" },
                     { "key": "16KB", "value": "16384" },
                     { "key": "32KB", "value": "32768" },
                     { "key": "64KB", "value": "65536" }
                  ]
               },
               {
                  "name": "Domain",
                  "webName": $scope.autoLanguage( '所属域' ),
                  "type": "select",
                  "desc": $scope.autoLanguage( '所属域必须已经存在。' ),
                  "value": domainInput[0]['value'],
                  "valid": domainInput
               },
               {
                  "name": "LobPageSize",
                  "webName": $scope.autoLanguage( 'Lob数据页大小' ),
                  "type": "select",
                  "desc": $scope.autoLanguage( 'Lob数据页大小创建后不可更改。' ),
                  "value": "262144",
                  "valid": [
                     { "key": "4KB", "value": "4096" },
                     { "key": "8KB", "value": "8192" },
                     { "key": "16KB", "value": "16384" },
                     { "key": "32KB", "value": "32768" },
                     { "key": "64KB", "value": "65536" },
                     { "key": "128KB", "value": "131072" },
                     { "key": "256KB", "value": "262144" },
                     { "key": "512KB", "value": "524288" }
                  ]
               }
            ]
         } ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
         $scope.Components.Modal.ok = function(){
            var isAllClear = $scope.Components.Modal.form.check() ;
            if( isAllClear )
            {
               var value = $scope.Components.Modal.form.getValue() ;
               var options = { 'PageSize': autoTypeConvert( value['PageSize'] ), 'LobPageSize': autoTypeConvert( value['LobPageSize'] ) } ;
               if( value['Domain'].length > 0 )
               {
                  options['Domain'] = value['Domain'] ;
               }
               var data = { 'cmd': 'create collectionspace', 'name': value['name'], 'options': JSON.stringify( options ) } ;
               var exec = function(){
                  SdbRest.DataOperation( data, {
                     'success': function( json ){
                        _DataDatabaseIndex.getCSInfo( $scope, SdbRest ) ;
                     },
                     'failed': function( errorInfo ){
                        _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                           exec() ;
                           return true ;
                        } ) ;
                     }
                  } ) ;
               } ;
               exec() ;
            }
            return isAllClear ;
         }
      }

      //打开 删除集合空间 的窗口
      $scope.showRemoveCS = function(){
         if( _IndexPublic.checkEditionAndSupport( $scope, 'sequoiadb', 'Metadata' ) == false )
         {
            return ;
         }
         var csValid = [] ;
         //获取集合空间名显示在列表中
         $.each( $scope.csList, function( index, csInfo ){
            csValid.push( { 'key': csInfo['Name'], 'value': index } ) ;
         } ) ;
         $scope.Components.Modal.icon = 'fa-trash-o' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '删除集合空间' ) ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            inputList: [
               {
                  "name": "name",
                  "webName": $scope.autoLanguage( '集合空间名' ),
                  "type": "select",
                  "value": csValid[ $scope.csID ]['value'],
                  "desc": $scope.autoLanguage( '选择集合空间' ),
                  "valid": csValid,
                  "onChange": function( name, key, value ){
                     $scope.Components.Modal.Table = $scope.csList[value]['Info'] ;
                  }
               }
            ]
         } ;
         $scope.Components.Modal.Table = $scope.csList[ $scope.csID ]['Info'] ;
         $scope.Components.Modal.Context = '\
      <div form-create para="data.form"></div>\
      <table class="table loosen border">\
      <tr>\
      <td style="width:40%;background-color:#F1F4F5;"><b>Key</b></td>\
      <td style="width:60%;background-color:#F1F4F5;"><b>Value</b></td>\
      </tr>\
      <tr ng-repeat="(key, value) in data.Table track by $index" ng-if="key != \'Name\'">\
      <td>{{key}}</td>\
      <td>{{value}}</td>\
      </tr>\
      </table>' ;
         $scope.Components.Modal.ok = function(){
            var isAllClear = $scope.Components.Modal.form.check() ;
            if( isAllClear )
            {
               var value = $scope.Components.Modal.form.getValue() ;
               var csName = $scope.csList[ value['name'] ]['Name'] ;
               var data = { 'cmd': 'drop collectionspace', 'name': csName } ;
               var exec = function(){
                  SdbRest.DataOperation( data, {
                     'success': function( json ){
                        if( $scope.csID == value['name'] )
                        {
                           $scope.showCSInfo( 0 ) ;
                        }
                        _DataDatabaseIndex.getCSInfo( $scope, SdbRest ) ;
                     },
                     'failed': function( errorInfo ){
                        _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                           exec() ;
                           return true ;
                        } ) ;
                     }
                  } ) ;
               } ;
               exec() ;
            }
            return isAllClear ;
         }
      }

      //打开 创建集合 的窗口
      $scope.showCreateCL = function(){
         if( _IndexPublic.checkEditionAndSupport( $scope, 'sequoiadb', 'Metadata' ) == false )
         {
            return ;
         }
         //把获取表单的值转换成请求参数
         var modalValue2Create = function( valueJson ){
            var fullName = $scope.csList[ valueJson['csName'] ]['Name'] + '.' + valueJson['clName'] ;
            var rv = { 'name': fullName, 'options': {} } ;
            if( $scope.moduleMode == 'distribution' )
            {
               if( valueJson['type'] == '' )
               {
                  rv['options']['Compressed'] = valueJson['Compressed'] == 0 ? false : true ;
                  if( rv['options']['Compressed'] == true )
                  {
                     if( valueJson['Compressed'] == 1 )
                     {
                        rv['options']['CompressionType'] = 'snappy' ;
                     }
                     else if( valueJson['Compressed'] == 2 )
                     {
                        rv['options']['CompressionType'] = 'lzw' ;
                     }
                  }
                  rv['options']['ReplSize'] = valueJson['ReplSize'] ;
                  if( valueJson['Group'] > 0 )
                  {
                     rv['options']['Group'] = $scope.GroupList[ valueJson['Group'] - 1 ]['GroupName'] ;
                  }
               }
               else if( valueJson['type'] == 'range' )
               {
                  rv['options']['ShardingType'] = 'range' ;
                  rv['options']['ShardingKey'] = {} ;
                  $.each( valueJson['ShardingKey'], function( index, field ){
                     rv['options']['ShardingKey'][ field['field'] ] = field['sort'] ;
                  } ) ;
                  rv['options']['ReplSize'] = valueJson['ReplSize'] ;
                  rv['options']['Compressed'] = valueJson['Compressed'] == 0 ? false : true ;
                  if( rv['options']['Compressed'] == true )
                  {
                     if( valueJson['Compressed'] == 1 )
                     {
                        rv['options']['CompressionType'] = 'snappy' ;
                     }
                     else if( valueJson['Compressed'] == 2 )
                     {
                        rv['options']['CompressionType'] = 'lzw' ;
                     }
                  }
                  if( valueJson['Group'] > 0 )
                  {
                     rv['options']['Group'] = $scope.GroupList[ valueJson['Group'] - 1 ]['GroupName'] ;
                  }
                  rv['options']['EnsureShardingIndex'] = valueJson['EnsureShardingIndex'] ;
               }
               else if( valueJson['type'] == 'hash' )
               {
                  rv['options']['ShardingType'] = 'hash' ;
                  rv['options']['ShardingKey'] = {} ;
                  $.each( valueJson['ShardingKey'], function( index, field ){
                     rv['options']['ShardingKey'][ field['field'] ] = field['sort'] ;
                  } ) ;
                  rv['options']['Partition'] = valueJson['Partition'] ;
                  rv['options']['ReplSize'] = valueJson['ReplSize'] ;
                  rv['options']['Compressed'] = valueJson['Compressed'] == 0 ? false : true ;
                  if( rv['options']['Compressed'] == true )
                  {
                     if( valueJson['Compressed'] == 1 )
                     {
                        rv['options']['CompressionType'] = 'snappy' ;
                     }
                     else if( valueJson['Compressed'] == 2 )
                     {
                        rv['options']['CompressionType'] = 'lzw' ;
                     }
                  }
                  rv['options']['AutoSplit'] = valueJson['AutoSplit'] ;
                  if( valueJson['Group'] > 0 )
                  {
                     rv['options']['Group'] = $scope.GroupList[ valueJson['Group'] - 1 ]['GroupName'] ;
                  }
                  rv['options']['EnsureShardingIndex'] = valueJson['EnsureShardingIndex'] ;
               }
               else if( valueJson['type'] == 'main' )
               {
                  rv['options']['IsMainCL'] = true ;
                  rv['options']['ShardingKey'] = {} ;
                  $.each( valueJson['ShardingKey'], function( index, field ){
                     rv['options']['ShardingKey'][ field['field'] ] = field['sort'] ;
                  } ) ;
                  rv['options']['ReplSize'] = valueJson['ReplSize'] ;
               }
            }
            else if( $scope.moduleMode == 'standalone' )
            {
               rv['options']['Compressed'] = valueJson['Compressed'] == 0 ? false : true ;
               if( rv['options']['Compressed'] == true )
                  {
                     if( valueJson['Compressed'] == 1 )
                     {
                        rv['options']['CompressionType'] = 'snappy' ;
                     }
                     else if( valueJson['Compressed'] == 2 )
                     {
                        rv['options']['CompressionType'] = 'lzw' ;
                     }
                  }
            }
            return rv ;
         }
         var createCLExec = function( data ){
            SdbRest.DataOperation( data, {
               'success': function( json ){
                  _DataDatabaseIndex.getCSInfo( $scope, SdbRest ) ;
               }, 
               'failed': function( errorInfo ){
                  _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                     createCLExec( data ) ;
                     return true ;
                  } ) ;
                  _DataDatabaseIndex.getCLInfo( $scope, SdbRest ) ;
               }
            } ) ;
         }
         var csValid = [] ;
         //获取集合空间名显示在列表中
         $.each( $scope.csList, function( index, csInfo ){
            csValid.push( { 'key': csInfo['Name'], 'value': index } ) ;
         } ) ;
         var groupValid = [ { "key": "Auto", "value": 0 } ] ;
         //获取分区组列表
         $.each( $scope.GroupList, function( index, groupInfo ){
            groupValid.push( { 'key': groupInfo['GroupName'], 'value': index + 1 } ) ;
         } ) ;
         $scope.Components.Modal.icon = 'fa-plus' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '创建集合' ) ;
         $scope.Components.Modal.isShow = true ;
         if( $scope.moduleMode == 'distribution' )
         {
            $scope.Components.Modal.formShow = 1 ;
            $scope.Components.Modal.form1 = {
               inputList: [
                  {
                 
                     "name": "csName",
                     "webName": $scope.autoLanguage( '集合空间' ),
                     "type": "select",
                     "value": $scope.csID,
                     'valid': csValid
                  },
                  {
                 
                     "name": "type",
                     "webName": $scope.autoLanguage( '集合类型' ),
                     "type": "select",
                     "value": "",
                     "valid": [
                        { "key": $scope.autoLanguage( '普通' ), "value": "" },
                        { "key": $scope.autoLanguage( '水平范围分区' ), "value": "range" },
                        { "key": $scope.autoLanguage( '水平散列分区' ), "value": "hash" },
                        { "key": $scope.autoLanguage( '垂直分区' ), "value": "main" }         
                     ],
                     "onChange": function( name, key, value ){
                        if( value == '' )
                        {
                           $scope.Components.Modal.formShow = 1 ;
                        }
                        else if( value == 'range' )
                        {
                           $scope.Components.Modal.formShow = 2 ;
                        }
                        else if( value == 'hash' )
                        {
                           $scope.Components.Modal.formShow = 3 ;
                        }
                        else if( value == 'main' )
                        {
                           $scope.Components.Modal.formShow = 4 ;
                        }
                        $scope.Components.Modal.form1.inputList[1]['value'] = '' ;
                        $scope.Components.Modal.form2.inputList[0]['value'] = $scope.Components.Modal.form1.inputList[0]['value'] ;
                        $scope.Components.Modal.form3.inputList[0]['value'] = $scope.Components.Modal.form1.inputList[0]['value'] ;
                        $scope.Components.Modal.form4.inputList[0]['value'] = $scope.Components.Modal.form1.inputList[0]['value'] ;
                        $scope.Components.Modal.form2.inputList[2]['value'] = $scope.Components.Modal.form1.inputList[2]['value'] ;
                        $scope.Components.Modal.form3.inputList[2]['value'] = $scope.Components.Modal.form1.inputList[2]['value'] ;
                        $scope.Components.Modal.form4.inputList[2]['value'] = $scope.Components.Modal.form1.inputList[2]['value'] ;
                     }
                  },
                  {
                     "name": "clName",
                     "webName": $scope.autoLanguage( '集合名' ),
                     "type": "string",
                     "required": true,
                     "value": "",
                     "valid": {
                        "min": 1,
                        "max": 127,
                        "ban": [ ".", "$" ]
                     }
                  },
                  {
                     "name": "ReplSize",
                     "webName":  $scope.autoLanguage( '一致性写副本数' ),
                     "type": "int",
                     "required": true,
                     "value": 1,
                     "valid": {
                        "min": -1,
                        "max": 7
                     }
                  },
                  {
                     "name": "Compressed",
                     "webName":  $scope.autoLanguage( '数据压缩' ),
                     "type": "select",
                     "required": false,
                     "value": 0,
                     "valid": [
                        { "key": $scope.autoLanguage( '关' ), "value": 0 },
                        { "key": 'Snappy', "value": 1 },
                        { "key": 'LZW', "value": 2 }
                     ]
                  },
                  {
                     "name": "Group",
                     "webName":  $scope.autoLanguage( '分区组' ),
                     "type": "select",
                     "value": 0,
                     "valid": groupValid
                  }
               ]
            } ;
            $scope.Components.Modal.form2 = {
               inputList: [
                  {
                     "name": "csName",
                     "webName": $scope.autoLanguage( '集合空间' ),
                     "type": "select",
                     "value": $scope.csID,
                     'valid': csValid
                  },
                  {
                     "name": "type",
                     "webName": $scope.autoLanguage( '集合类型' ),
                     "type": "select",
                     "value": "range",
                     "valid": [
                        { "key": $scope.autoLanguage( '普通' ), "value": "" },
                        { "key": $scope.autoLanguage( '水平范围分区' ), "value": "range" },
                        { "key": $scope.autoLanguage( '水平散列分区' ), "value": "hash" },
                        { "key": $scope.autoLanguage( '垂直分区' ), "value": "main" }         
                     ],
                     "onChange": function( name, key, value ){
                        if( value == '' )
                        {
                           $scope.Components.Modal.formShow = 1 ;
                        }
                        else if( value == 'range' )
                        {
                           $scope.Components.Modal.formShow = 2 ;
                        }
                        else if( value == 'hash' )
                        {
                           $scope.Components.Modal.formShow = 3 ;
                        }
                        else if( value == 'main' )
                        {
                           $scope.Components.Modal.formShow = 4 ;
                        }
                        $scope.Components.Modal.form2.inputList[1]['value'] = 'range' ;
                        $scope.Components.Modal.form1.inputList[0]['value'] = $scope.Components.Modal.form2.inputList[0]['value'] ;
                        $scope.Components.Modal.form3.inputList[0]['value'] = $scope.Components.Modal.form2.inputList[0]['value'] ;
                        $scope.Components.Modal.form4.inputList[0]['value'] = $scope.Components.Modal.form2.inputList[0]['value'] ;
                        $scope.Components.Modal.form1.inputList[2]['value'] = $scope.Components.Modal.form2.inputList[2]['value'] ;
                        $scope.Components.Modal.form3.inputList[2]['value'] = $scope.Components.Modal.form2.inputList[2]['value'] ;
                        $scope.Components.Modal.form4.inputList[2]['value'] = $scope.Components.Modal.form2.inputList[2]['value'] ;
                     }
                  },
                  {
                     "name": "clName",
                     "webName": $scope.autoLanguage( '集合名' ),
                     "type": "string",
                     "required": true,
                     "value": "",
                     "valid": {
                        "min": 1,
                        "max": 127,
                        "ban": [ ".", "$" ]
                     }
                  },
                  {
                     "name": "ShardingKey",
                     "webName":  $scope.autoLanguage( '分区键' ),
                     "required": true,
                     "desc": $scope.autoLanguage( '分区键是一个或多个集合列的有续集和，其中的值用来确定每个集合所述的数据分区。' ),
                     "type": "list",
                     "valid": {
                        "min": 1
                     },
                     "child":[
                        [
                           {
                              "name": "field",
                              "webName": $scope.autoLanguage( "字段名" ),
                              "type": "string",
                              "value": "",
                              "valid": {
                                 "min": 1,
                                 "regex": "^[^/$].*",
                                 "ban": "."
                              }
                           },
                           {
                              "name": "sort",
                              "type": "select",
                              "value": 1,
                              "valid": [
                                 { "key": $scope.autoLanguage( '升序' ), "value": 1 },
                                 { "key": $scope.autoLanguage( '降序' ), "value": -1 }
                              ]
                           }
                        ]
                     ]
                  },
                  {
                     "name": "ReplSize",
                     "webName":  $scope.autoLanguage( '一致性写副本数' ),
                     "type": "int",
                     "required": true,
                     "value": 1,
                     "valid": {
                        "min": -1,
                        "max": 7
                     }
                  },
                  {
                     "name": "Compressed",
                     "webName":  $scope.autoLanguage( '数据压缩' ),
                     "type": "select",
                     "required": false,
                     "value": 0,
                     "valid": [
                        { "key": $scope.autoLanguage( '关' ), "value": 0 },
                        { "key": 'Snappy', "value": 1 },
                        { "key": 'LZW', "value": 2 }
                     ]
                  },
                  {
                     "name": "Group",
                     "webName":  $scope.autoLanguage( '分区组' ),
                     "type": "select",
                     "value": 0,
                     "valid": groupValid
                  },
                  {
                     "name": "EnsureShardingIndex",
                     "webName":  $scope.autoLanguage( '分区索引' ),
                     "type": "select",
                     "required": false,
                     "desc": "",
                     "value": true,
                     "valid": [
                        { "key": $scope.autoLanguage( '开' ), "value": true },
                        { "key": $scope.autoLanguage( '关' ), "value": false }
                     ]
                  }
               ]
            } ;
            $scope.Components.Modal.form3 = {
               inputList: [
                  {
                     "name": "csName",
                     "webName": $scope.autoLanguage( '集合空间' ),
                     "type": "select",
                     "value": $scope.csID,
                     'valid': csValid
                  },
                  {
                     "name": "type",
                     "webName": $scope.autoLanguage( '集合类型' ),
                     "type": "select",
                     "value": "hash",
                     "valid": [
                        { "key": $scope.autoLanguage( '普通' ), "value": "" },
                        { "key": $scope.autoLanguage( '水平范围分区' ), "value": "range" },
                        { "key": $scope.autoLanguage( '水平散列分区' ), "value": "hash" },
                        { "key": $scope.autoLanguage( '垂直分区' ), "value": "main" }         
                     ],
                     "onChange": function( name, key, value ){
                        if( value == '' )
                        {
                           $scope.Components.Modal.formShow = 1 ;
                        }
                        else if( value == 'range' )
                        {
                           $scope.Components.Modal.formShow = 2 ;
                        }
                        else if( value == 'hash' )
                        {
                           $scope.Components.Modal.formShow = 3 ;
                        }
                        else if( value == 'main' )
                        {
                           $scope.Components.Modal.formShow = 4 ;
                        }
                        $scope.Components.Modal.form3.inputList[1]['value'] = 'hash' ;
                        $scope.Components.Modal.form1.inputList[0]['value'] = $scope.Components.Modal.form3.inputList[0]['value'] ;
                        $scope.Components.Modal.form2.inputList[0]['value'] = $scope.Components.Modal.form3.inputList[0]['value'] ;
                        $scope.Components.Modal.form4.inputList[0]['value'] = $scope.Components.Modal.form3.inputList[0]['value'] ;
                        $scope.Components.Modal.form1.inputList[2]['value'] = $scope.Components.Modal.form3.inputList[2]['value'] ;
                        $scope.Components.Modal.form2.inputList[2]['value'] = $scope.Components.Modal.form3.inputList[2]['value'] ;
                        $scope.Components.Modal.form4.inputList[2]['value'] = $scope.Components.Modal.form3.inputList[2]['value'] ;
                     }
                  },
                  {
                     "name": "clName",
                     "webName": $scope.autoLanguage( '集合名' ),
                     "type": "string",
                     "required": true,
                     "value": "",
                     "valid": {
                        "min": 1,
                        "max": 127,
                        "ban": [ ".", "$" ]
                     }
                  },
                  {
                     "name": "ShardingKey",
                     "webName":  $scope.autoLanguage( '分区键' ),
                     "required": true,
                     "desc": $scope.autoLanguage( '分区键是一个或多个集合列的有续集和，其中的值用来确定每个集合所述的数据分区。' ),
                     "type": "list",
                     "valid": {
                        "min": 1
                     },
                     "child":[
                        [
                           {
                              "name": "field",
                              "webName": $scope.autoLanguage( "字段名" ),
                              "type": "string",
                              "value": "",
                              "valid": {
                                 "min": 1,
                                 "regex": "^[^/$].*",
                                 "ban": "."
                              }
                           },
                           {
                              "name": "sort",
                              "type": "select",
                              "value": 1,
                              "valid": [
                                 { "key": $scope.autoLanguage( '升序' ), "value": 1 },
                                 { "key": $scope.autoLanguage( '降序' ), "value": -1 }
                              ]
                           }
                        ]
                     ]
                  },
                  {
                     "name": "Partition",
                     "webName":  $scope.autoLanguage( '分区数' ),
                     "type": "int",
                     "required": true,
                     "value": 4096,
                     "valid": {
                        "min": 8,
                        "max": 1048576,
                        "step": 2
                     }
                  },
                  {
                     "name": "ReplSize",
                     "webName":  $scope.autoLanguage( '一致性写副本数' ),
                     "type": "int",
                     "required": true,
                     "value": 1,
                     "valid": {
                        "min": -1,
                        "max": 7
                     }
                  },
                  {
                     "name": "Compressed",
                     "webName":  $scope.autoLanguage( '数据压缩' ),
                     "type": "select",
                     "required": false,
                     "value": 0,
                     "valid": [
                        { "key": $scope.autoLanguage( '关' ), "value": 0 },
                        { "key": 'Snappy', "value": 1 },
                        { "key": 'LZW', "value": 2 }
                     ]
                  },
                  {
                     "name": "AutoSplit",
                     "webName":  $scope.autoLanguage( '自动切分' ),
                     "type": "select",
                     "value": false,
                     "valid": [
                        { "key": $scope.autoLanguage( '开' ), "value": true },
                        { "key": $scope.autoLanguage( '关' ), "value": false }
                     ]
                  },
                  {
                     "name": "Group",
                     "webName":  $scope.autoLanguage( '分区组' ),
                     "type": "select",
                     "value": 0,
                     "valid": groupValid
                  },
                  {
                     "name": "EnsureShardingIndex",
                     "webName":  $scope.autoLanguage( '分区索引' ),
                     "type": "select",
                     "required": false,
                     "desc": "",
                     "value": true,
                     "valid": [
                        { "key": $scope.autoLanguage( '开' ), "value": true },
                        { "key": $scope.autoLanguage( '关' ), "value": false }
                     ]
                  }
               ]
            } ;
            $scope.Components.Modal.form4 = {
               inputList: [
                  {
                 
                     "name": "csName",
                     "webName": $scope.autoLanguage( '集合空间' ),
                     "type": "select",
                     "value": $scope.csID,
                     'valid': csValid
                  },
                  {
                 
                     "name": "type",
                     "webName": $scope.autoLanguage( '集合类型' ),
                     "type": "select",
                     "value": "main",
                     "valid": [
                        { "key": $scope.autoLanguage( '普通' ), "value": "" },
                        { "key": $scope.autoLanguage( '水平范围分区' ), "value": "range" },
                        { "key": $scope.autoLanguage( '水平散列分区' ), "value": "hash" },
                        { "key": $scope.autoLanguage( '垂直分区' ), "value": "main" }         
                     ],
                     "onChange": function( name, key, value ){
                        if( value == '' )
                        {
                           $scope.Components.Modal.formShow = 1 ;
                        }
                        else if( value == 'range' )
                        {
                           $scope.Components.Modal.formShow = 2 ;
                        }
                        else if( value == 'hash' )
                        {
                           $scope.Components.Modal.formShow = 3 ;
                        }
                        else if( value == 'main' )
                        {
                           $scope.Components.Modal.formShow = 4 ;
                        }
                        $scope.Components.Modal.form4.inputList[1]['value'] = 'main' ;
                        $scope.Components.Modal.form1.inputList[0]['value'] = $scope.Components.Modal.form4.inputList[0]['value'] ;
                        $scope.Components.Modal.form2.inputList[0]['value'] = $scope.Components.Modal.form4.inputList[0]['value'] ;
                        $scope.Components.Modal.form3.inputList[0]['value'] = $scope.Components.Modal.form4.inputList[0]['value'] ;
                        $scope.Components.Modal.form1.inputList[2]['value'] = $scope.Components.Modal.form4.inputList[2]['value'] ;
                        $scope.Components.Modal.form2.inputList[2]['value'] = $scope.Components.Modal.form4.inputList[2]['value'] ;
                        $scope.Components.Modal.form3.inputList[2]['value'] = $scope.Components.Modal.form4.inputList[2]['value'] ;
                     }
                  },
                  {
                     "name": "clName",
                     "webName": $scope.autoLanguage( '集合名' ),
                     "type": "string",
                     "required": true,
                     "value": "",
                     "valid": {
                        "min": 1,
                        "max": 127,
                        "ban": [ ".", "$" ]
                     }
                  },
                  {
                     "name": "ShardingKey",
                     "webName":  $scope.autoLanguage( '分区键' ),
                     "required": true,
                     "desc": $scope.autoLanguage( '分区键是一个或多个集合列的有续集和，其中的值用来确定每个集合所述的数据分区。' ),
                     "type": "list",
                     "valid": {
                        "min": 1
                     },
                     "child":[
                        [
                           {
                              "name": "field",
                              "webName": $scope.autoLanguage( "字段名" ),
                              "type": "string",
                              "value": "",
                              "valid": {
                                 "min": 1,
                                 "regex": "^[^/$].*",
                                 "ban": "."
                              }
                           },
                           {
                              "name": "sort",
                              "type": "select",
                              "value": 1,
                              "valid": [
                                 { "key": $scope.autoLanguage( '升序' ), "value": 1 },
                                 { "key": $scope.autoLanguage( '降序' ), "value": -1 }
                              ]
                           }
                        ]
                     ]
                  },
                  {
                     "name": "ReplSize",
                     "webName":  $scope.autoLanguage( '副本数' ),
                     "type": "int",
                     "required": true,
                     "value": 1,
                     "valid": {
                        "min": -1,
                        "max": 7
                     }
                  }
               ]
            } ;
            $scope.Components.Modal.Context = '\
      <div ng-if="data.formShow == 1" form-create para="data.form1"></div>\
      <div ng-if="data.formShow == 2" form-create para="data.form2"></div>\
      <div ng-if="data.formShow == 3" form-create para="data.form3"></div>\
      <div ng-if="data.formShow == 4" form-create para="data.form4"></div>' ;
            $scope.Components.Modal.ok = function(){
               if( $scope.Components.Modal.formShow == 1 )
               {
                  var isAllClear = $scope.Components.Modal.form1.check() ;
                  if( isAllClear )
                  {
                     var value = $scope.Components.Modal.form1.getValue() ;
                     var data = modalValue2Create( value ) ;
                     data['cmd'] = 'create collection' ;
                     data['options'] = JSON.stringify( data['options'] ) ;
                     createCLExec( data ) ;
                  }
               }
               else if( $scope.Components.Modal.formShow == 2 )
               {
                  var isAllClear = $scope.Components.Modal.form2.check() ;
                  if( isAllClear )
                  {
                     var value = $scope.Components.Modal.form2.getValue() ;
                     var data = modalValue2Create( value ) ;
                     data['cmd'] = 'create collection' ;
                     data['options'] = JSON.stringify( data['options'] ) ;
                     createCLExec( data ) ;
                  }
               }
               else if( $scope.Components.Modal.formShow == 3 )
               {
                  var isAllClear = $scope.Components.Modal.form3.check( function( valueJson ){
                     var rv = [] ;
                     if( valueJson['AutoSplit'] == true && valueJson['Group'] > 0 )
                     {
                        rv.push( { 'name': 'AutoSplit', 'error': sprintf( $scope.autoLanguage( '指定了?，不能开启?。' ), $scope.autoLanguage( '分区组' ), $scope.autoLanguage( '自动切分' ) ) } ) ;
                        rv.push( { 'name': 'Group', 'error': sprintf( $scope.autoLanguage( '开启了?，不能指定?。' ), $scope.autoLanguage( '自动切分' ), $scope.autoLanguage( '分区组' ) ) } ) ;
                     }
                     return rv ;
                  } ) ;
                  if( isAllClear )
                  {
                     var value = $scope.Components.Modal.form3.getValue() ;
                     var data = modalValue2Create( value ) ;
                     data['cmd'] = 'create collection' ;
                     data['options'] = JSON.stringify( data['options'] ) ;
                     createCLExec( data ) ;
                  }
               }
               else if( $scope.Components.Modal.formShow == 4 )
               {
                  var isAllClear = $scope.Components.Modal.form4.check() ;
                  if( isAllClear )
                  {
                     var value = $scope.Components.Modal.form4.getValue() ;
                     var data = modalValue2Create( value ) ;
                     data['cmd'] = 'create collection' ;
                     data['options'] = JSON.stringify( data['options'] ) ;
                     createCLExec( data ) ;
                  }
               }
               return isAllClear ;
            }
         }
         else if( $scope.moduleMode == 'standalone' )
         {
            $scope.Components.Modal.form = {
               inputList: [
                  {
                 
                     "name": "csName",
                     "webName": $scope.autoLanguage( '集合空间' ),
                     "type": "select",
                     "value": $scope.csID,
                     'valid': csValid
                  },
                  {
                     "name": "clName",
                     "webName": $scope.autoLanguage( '集合名' ),
                     "type": "string",
                     "required": true,
                     "value": "",
                     "valid": {
                        "min": 1,
                        "ban": "."
                     }
                  },
                  {
                     "name": "Compressed",
                     "webName":  $scope.autoLanguage( '数据压缩' ),
                     "type": "select",
                     "required": false,
                     "value": 0,
                     "valid": [
                        { "key": $scope.autoLanguage( '关' ), "value": 0 },
                        { "key": 'Snappy', "value": 1 },
                        { "key": 'LZW', "value": 2 }
                     ]
                  }
               ]
            } ;
            $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
            $scope.Components.Modal.ok = function(){
               var isAllClear = $scope.Components.Modal.form.check() ;
               if( isAllClear )
               {
                  var value = $scope.Components.Modal.form.getValue() ;
                  var data = modalValue2Create( value ) ;
                  data['cmd'] = 'create collection' ;
                  data['options'] = JSON.stringify( data['options'] ) ;
                  createCLExec( data ) ;
               }
               return isAllClear ;
            }
         }
      }

      //打开 删除集合 的窗口
      $scope.showRemoveCL = function(){
         if( _IndexPublic.checkEditionAndSupport( $scope, 'sequoiadb', 'Metadata' ) == false )
         {
            return ;
         }
         var clValid = [] ;
         var clIndex = -1 ;
         $.each( $scope.clList, function( index, clInfo ){
            if( $scope.showType == 'cs' )
            {
               if( $scope.csList[ $scope.csID ]['Name'] == clInfo['csName'] && clIndex < 0 )
               {
                  clIndex = index ;
               }
            }
            else
            {
               if( index == $scope.clID )
               {
                  clIndex = index ;
               }
            }
            clValid.push( { 'key' : clInfo['csName'] + '.' + clInfo['Name'] , 'value' : index } );
         } ) ;
         if( clIndex < 0 )
            clIndex = 0 ;
         $scope.Components.Modal.icon = 'fa-trash-o' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '删除集合' ) ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            inputList: [
               {
                  "name": "name",
                  "webName": $scope.autoLanguage( '集合名' ),
                  "type": "select",
                  "value": clIndex,
                  "valid": clValid,
                  "onChange": function( name, key, value ){
                     $scope.Components.Modal.Table = $scope.clList[value]['Info'] ;
                  }
               }
            ]
         } ;
         $scope.Components.Modal.Table = $scope.clList[ clIndex ]['Info'] ;
         var maskContext = '\
      <div form-create para="data.form"></div>\
      <table class="table loosen border">\
      <tr>\
      <td style="width:40%;background-color:#F1F4F5;"><b>Key</b></td>\
      <td style="width:60%;background-color:#F1F4F5;"><b>Value</b></td>\
      </tr>\
      <tr ng-repeat="(key, value) in data.Table track by $index" ng-if="key != \'Name\' && key != \'CataInfo\' && key != \'GroupName\' && value != undefined">\
      <td>{{key}}</td>\
      <td>{{value}}</td>\
      </tr>\
      </table>';
         $scope.Components.Modal.Context = maskContext ;
         $scope.Components.Modal.ok = function(){
            var isAllClear = $scope.Components.Modal.form.check() ;
            if( isAllClear )
            {
               var value = $scope.Components.Modal.form.getValue() ;
               var fullName = $scope.clList[ value['name'] ]['csName'] + '.' + $scope.clList[ value['name'] ]['Name'] ;
               var data = { 'cmd': 'drop collection', 'name': fullName } ;
               var exec = function(){
                  SdbRest.DataOperation( data, {
                     'success': function( json ){
                        if( $scope.clID == value['name'] )
                        {
                           $scope.clID = 0 ;
                           $scope.showCSInfo( $scope.csID ) ;
                        }
                        _DataDatabaseIndex.getCSInfo( $scope, SdbRest ) ;
                     },
                     'failed': function( errorInfo ){
                        _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                           exec() ;
                           return true ;
                        } ) ;
                     }
                  } ) ;
               } ;
               exec() ;
            }
            return isAllClear ;
         }
      }


      //打开 分离集合 的窗口
      $scope.showDetachCL = function(){
         if( _IndexPublic.checkEditionAndSupport( $scope, 'sequoiadb', 'Metadata' ) == false )
         {
            return ;
         }
         var findMainCL = function( mainList, name ){
            var index = -1 ;
            $.each( mainList, function( mainIndex, mainInfo ){
               if( mainInfo['key'] == name )
               {
                  index = mainIndex ;
                  return false ;
               }
            } ) ;
            return index ;
         }
         var mainCL = [] ;
         var childCL = [] ;
         $.each( $scope.clList, function( index, clInfo ){
            if( typeof( clInfo['MainCLName'] ) == 'string' && clInfo['MainCLName'].length > 0 )
            {
               if( findMainCL( mainCL, clInfo['MainCLName'] ) < 0 )
               {
                  mainCL.push( { 'key': clInfo['MainCLName'], 'value': clInfo['MainCLName'] } ) ;
               }
               childCL.push( { 'mainCL': clInfo['MainCLName'], 'childCL': clInfo['csName'] + '.' + clInfo['Name'] } ) ;
            }
         } ) ;
         //设置子表的列表
         var SetSubCLList = function( formInput, mainCL ){
            //构造子表列表
            var subClList = [] ;
            $.each( childCL, function( childIndex, childInfo ){
               if( childInfo['mainCL'] == mainCL )
               {
                  subClList.push( { 'key': childInfo['childCL'], 'value': childInfo['childCL'] } ) ;
               }
            } ) ;
            formInput['valid'] = subClList ;
            //设置子表下拉菜单默认值
            if( subClList.length > 0 )
            {
               formInput['value'] = subClList[0]['value'] ;
            }
         }
         var form = {
            inputList: [
               {
                  "name": "mainCL",
                  "webName": $scope.autoLanguage( '集合' ),
                  "type": "select",
                  "value": mainCL[0]['value'],
                  "valid": mainCL,
                  "onChange": function( name, key, value ){
                     SetSubCLList( form['inputList'][1], value ) ;
                  }
               },
               {
                  "name": "childCL",
                  "webName": $scope.autoLanguage( '分区' ),
                  "type": "select",
                  "value": '',
                  "valid": []
               }
            ]
         } ;
         SetSubCLList( form['inputList'][1], mainCL[0]['value'] ) ;
         $scope.DetachCL['config'] = form ;
         $scope.DetachCL['callback']['Open']() ;
         $scope.DetachCL['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = form.check() ;
            if( isAllClear )
            {
               var formVal = form.getValue() ;
               $scope.DetachCL['callback']['Close']() ;
               var data = {
                  'cmd': 'detach collection',
                  'collectionname': formVal['mainCL'],
                  'subclname': formVal['childCL']
               } ;
               var exec = function(){
                  SdbRest.DataOperation( data, {
                     'success': function( json ){
                        _DataDatabaseIndex.getCSInfo( $scope, SdbRest ) ;
                     },
                     'failed': function( errorInfo ){
                        _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                           exec() ;
                           return true ;
                        } ) ;
                     }
                  } ) ;
               } ;
               exec() ;
            }
         } ) ;
         $scope.DetachCL['callback']['SetTitle']( $scope.autoLanguage( '分离集合' ) ) ;
         $scope.DetachCL['callback']['SetIcon']() ;
      }

      //打开 挂载集合 的窗口
      $scope.showAttachCL = function(){
         if( _IndexPublic.checkEditionAndSupport( $scope, 'sequoiadb', 'Metadata' ) == false )
         {
            return ;
         }
         var mainCL = [] ;
         var childCL = [] ;
         //获取主分区集合
         $.each( $scope.clList, function( index, clInfo ){
            if( clInfo['IsMainCL'] == true )
            {
               var id = mainCL.length ;
               mainCL.push( { 'key' : clInfo['csName'] + '.' + clInfo['Name'] , 'value' : id } ) ;
            }
            else
            {
               var id = childCL.length ;
               if( typeof( clInfo['MainCLName'] ) != 'string' )
               {
                  childCL.push( { 'key' : clInfo['csName'] + '.' + clInfo['Name'] , 'value' : id } ) ;
               }
            }
         } ) ;
         $scope.Components.Modal.icon = 'fa-paperclip' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '挂载集合' ) ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            inputList: [
               {
                  "name": "name",
                  "webName": $scope.autoLanguage( '集合' ),
                  "type": "select",
                  "value": 0,
                  "valid": mainCL
               },
               {
                  "name": "attachName",
                  "webName": $scope.autoLanguage( '分区' ),
                  "type": "select",
                  "value": 0,
                  "desc": $scope.autoLanguage( '选择在主分区集合下挂载的分区。' ),
                  "valid": childCL
               },
               {
                  "name": "range",
                  "webName":  $scope.autoLanguage( '分区范围' ),
                  "required": true,
                  "desc": $scope.autoLanguage( '分区范围，包含两个字段“LowBound”（区间左值，填$minKey为负无穷）以及“UpBound”（区间右值，填$maxKey为正无穷），例如：{LowBound:{a:0},UpBound:{a:100}表示取字段“a”的范围区间：[0, 100)。' ),
                  "type": "list",
                  "valid": {
                     "min": 1
                  },
                  "child":[
                     [
                        {
                           "name": "field",
                           "webName": $scope.autoLanguage( "字段名" ),
                           "placeholder": $scope.autoLanguage( "字段名" ),
                           "type": "string",
                           "value": "",
                           "valid": {
                              "min": 1,
                              "regex": "^[^/$].*",
                              "ban": "."
                           }
                        },
                        {
                           "name": "type",
                           "webName": $scope.autoLanguage( "类型" ),
                           "placeholder": $scope.autoLanguage( "类型" ),
                           "type": "select",
                           "value": "Auto",
                           "valid": [
                              { "key": "Auto",      "value": "Auto" },
                              { "key": "Bool",      "value": "Bool" },
                              { "key": "Number",    "value": "Number" },
                              { "key": "Decimal",   "value": "Decimal" },
                              { "key": "String",    "value": "String" },
                              { "key": "ObjectId",  "value": "ObjectId" },
                              { "key": "Regex",     "value": "Regex" },
                              { "key": "Binary",    "value": "Binary" },
                              { "key": "Timestamp", "value": "Timestamp" },
                              { "key": "Date",      "value": "Date" }
                           ]
                        },
                        {
                           "name": "LowBound",
                           "webName": $scope.autoLanguage( "区间左值" ),
                           "placeholder": $scope.autoLanguage( "区间左值" ),
                           "type": "string",
                           "value": "",
                           "selectList": [ '$minKey' ]
                        },
                        {
                           "name": "UpBound",
                           "webName": $scope.autoLanguage( "区间右值" ),
                           "placeholder": $scope.autoLanguage( "区间右值" ),
                           "type": "string",
                           "value": "",
                           "selectList": [ '$maxKey' ]
                        }
                     ]
                  ]
               }
            ]
         } ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
         $scope.Components.Modal.ok = function(){
            var isAllClear = $scope.Components.Modal.form.check() ;
            if( isAllClear )
            {
               var value = $scope.Components.Modal.form.getValue() ;
               var lowbound = {} ;
               var upbound = {} ;
               $.each( value['range'], function( index, fieldInfo ){
                  var fieldName = fieldInfo['field'] ;

                  if( fieldInfo['type'] == 'Auto' )
                  {
                     lowbound[ fieldName ] = autoTypeConvert( fieldInfo['LowBound'], true ) ;
                     upbound[ fieldName ]  = autoTypeConvert( fieldInfo['UpBound'], true ) ;
                  }
                  else
                  {
                     if( fieldInfo['LowBound'].toLowerCase() == '$minkey' )
                     {
                        lowbound[ fieldName ] = { '$minKey': 1 } ;
                     }
                     else
                     {
                        lowbound[ fieldName ] = specifyTypeConvert( fieldInfo['LowBound'], fieldInfo['type'] ) ;
                     }
                     if( fieldInfo['UpBound'].toLowerCase() == '$maxkey' )
                     {
                        upbound[ fieldName ] = { '$maxKey': 1 } ;
                     }
                     else
                     {
                        upbound[ fieldName ] = specifyTypeConvert( fieldInfo['UpBound'], fieldInfo['type'] ) ;
                     }
                  }
               } ) ;
               var data = { 'cmd': 'attach collection',
                            'collectionname': mainCL[ value['name'] ]['key'],
                            'subclname': childCL[ value['attachName'] ]['key'],
                            'lowbound': JSON.stringify( lowbound ),
                            'upbound': JSON.stringify( upbound )
                          } ;
               var exec = function(){
                  SdbRest.DataOperation( data, {
                     'success': function( json ){
                        _DataDatabaseIndex.getCSInfo( $scope, SdbRest ) ;
                     },
                     'failed': function( errorInfo ){
                        _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                           exec() ;
                           return true ;
                        } ) ;
                     }
                  } ) ;
               } ;
               exec() ;
            }
            return isAllClear ;
         }
      }

      //打开 切分数据 的窗口
      $scope.showSplit = function(){
         if( _IndexPublic.checkEditionAndSupport( $scope, 'sequoiadb', 'Metadata' ) == false )
         {
            return ;
         }
         var clValid = [] ;
         var clIndex = -1 ;
         var sourceGroupValid = [] ;
         var groupValid = [] ;
         var type = '' ;
         $.each( $scope.GroupList, function( index, groupInfo ){
            groupValid.push( { 'key': groupInfo['GroupName'], 'value': index } ) ;
         } ) ;
         $.each( $scope.clList, function( index, clInfo ){
            if( $scope.showType == 'cs' )
            {
               if( $scope.csList[ $scope.csID ]['Name'] == clInfo['csName'] && clIndex < 0 )
               {
                  if( clInfo['IsMainCL'] != true && ( clInfo['ShardingType'] == 'hash' || clInfo['ShardingType'] == 'range' ) )
                  {
                     clIndex = index ;
                     $.each( $scope.clList[ clIndex ]['GroupName'], function( index2, groupInfo ){
                        sourceGroupValid.push( { 'key': groupInfo['key'], 'value': index2 } ) ;
                     } ) ;
                  }
               }
            }
            else
            {
               if( index == $scope.clID && clIndex < 0 && clInfo['IsMainCL'] != true && ( clInfo['ShardingType'] == 'hash' || clInfo['ShardingType'] == 'range' ) )
               {
                  clIndex = index ;
                  $.each( $scope.clList[ clIndex ]['GroupName'], function( index2, groupInfo ){
                     sourceGroupValid.push( { 'key': groupInfo['key'], 'value': index2 } ) ;
                  } ) ;
               }
            }
            if( clInfo['IsMainCL'] != true && ( clInfo['ShardingType'] == 'hash' || clInfo['ShardingType'] == 'range' ) )
            {
               clValid.push( { 'key' : clInfo['csName'] + '.' + clInfo['Name'] , 'value' : index, 'type': clInfo['ShardingType'] } ) ;
            }
         } ) ;
         if( clIndex < 0 && clValid.length > 0 )
         {
            clIndex = clValid[0]['value'] ;
            $.each( $scope.clList[ clIndex ]['GroupName'], function( index2, groupInfo ){
               sourceGroupValid.push( { 'key': groupInfo['key'], 'value': index2 } ) ;
            } ) ;
         }
         $scope.Components.Modal.icon = 'fa-scissors' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '切分数据' ) ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.formShow = 0 ;
         $scope.Components.Modal.form1 = {
            inputList: [
               {
                  "name": "type",
                  "webName": $scope.autoLanguage( '切分方式' ),
                  "type": "select",
                  "value": 0,
                  "valid": [
                     { 'key': $scope.autoLanguage( '百分比切分' ), 'value': 0 },
                     { 'key': $scope.autoLanguage( '条件切分' ), 'value': 1 }
                  ],
                  "onChange": function( name, key, value ){
                     if( value == 0 )
                     {
                        $scope.Components.Modal.formShow = 0 ;
                     }
                     else if( value == 1 )
                     {
                        $scope.Components.Modal.formShow = 1 ;
                        $.each( clValid, function( index, info ){
                           if( $scope.Components.Modal.form1['inputList'][1]['value'] == info['value'] )
                           {
                              type = info['type'] ;
                           }
                        } ) ;
                        if( type == 'hash' )
                        {
                           $scope.Components.Modal.form2.inputList[4]['type'] = 'inline' ;
                           $scope.Components.Modal.form2.inputList[4]['child'] = [
                              {
                                 "name": "field",
                                 "webName": $scope.autoLanguage( "字段名" ),
                                 "placeholder": $scope.autoLanguage( "字段名" ),
                                 "type": "string",
                                 "disabled": true,
                                 "value": "Partition",
                                 "valid": {
                                    "min": 1,
                                    "regex": "^[^/$].*",
                                    "ban": "."
                                 }
                              },
                              {
                                 "name": "start",
                                 "webName": $scope.autoLanguage( "起始范围" ),
                                 "placeholder": $scope.autoLanguage( "起始范围" ),
                                 "type": "string",
                                 "value": "",
                                 "selectList": [ '$minKey' ]
                              },
                              {
                                 "name": "end",
                                 "webName": $scope.autoLanguage( "结束范围" ),
                                 "placeholder": $scope.autoLanguage( "结束范围" ),
                                 "type": "string",
                                 "value": "",
                                 "selectList": [ '$maxKey' ]
                              }
                           ] ;
                        }
                        else
                        {
                           $scope.Components.Modal.form2.inputList[4]['type'] = 'list' ;
                           $scope.Components.Modal.form2.inputList[4]['child'] = [
                              [
                                 {
                                    "name": "field",
                                    "webName": $scope.autoLanguage( "字段名" ),
                                    "placeholder": $scope.autoLanguage( "字段名" ),
                                    "type": "string",
                                    "value": "",
                                    "valid": {
                                       "min": 1,
                                       "regex": "^[^/$].*",
                                       "ban": "."
                                    }
                                 },
                                 {
                                    "name": "type",
                                    "webName": $scope.autoLanguage( "类型" ),
                                    "placeholder": $scope.autoLanguage( "类型" ),
                                    "type": "select",
                                    "value": "Auto",
                                    "valid": [
                                       { "key": "Auto",      "value": "Auto" },
                                       { "key": "Bool",      "value": "Bool" },
                                       { "key": "Number",    "value": "Number" },
                                       { "key": "Decimal",   "value": "Decimal" },
                                       { "key": "String",    "value": "String" },
                                       { "key": "ObjectId",  "value": "ObjectId" },
                                       { "key": "Regex",     "value": "Regex" },
                                       { "key": "Binary",    "value": "Binary" },
                                       { "key": "Timestamp", "value": "Timestamp" },
                                       { "key": "Date",      "value": "Date" }
                                    ]
                                 },
                                 {
                                    "name": "start",
                                    "webName": $scope.autoLanguage( "起始范围" ),
                                    "placeholder": $scope.autoLanguage( "起始范围" ),
                                    "type": "string",
                                    "value": "",
                                    "selectList": [ '$minKey' ]
                                 },
                                 {
                                    "name": "end",
                                    "webName": $scope.autoLanguage( "结束范围" ),
                                    "placeholder": $scope.autoLanguage( "结束范围" ),
                                    "type": "string",
                                    "value": "",
                                    "selectList": [ '$maxKey' ]
                                 }
                              ]
                           ] ;
                        }
                     }
                     $scope.Components.Modal.form1.inputList[0]['value'] = 0 ;
                     $scope.Components.Modal.form2.inputList[1]['value'] = $scope.Components.Modal.form1.inputList[1]['value'] ;
                     $scope.Components.Modal.form2.inputList[2]['valid'] = $scope.Components.Modal.form1.inputList[2]['valid'] ;
                     $scope.Components.Modal.form2.inputList[2]['value'] = $scope.Components.Modal.form1.inputList[2]['value'] ;
                     $scope.Components.Modal.form2.inputList[3]['value'] = $scope.Components.Modal.form1.inputList[3]['value'] ;
                  }
               },
               {
                  "name": "name",
                  "webName": $scope.autoLanguage( '集合名' ),
                  "type": "select",
                  "value": clIndex,
                  "valid": clValid,
                  "onChange": function( name, key, value ){
                     $.each( $scope.clList, function( index, clInfo ){
                        if( key == ( clInfo['csName'] + '.' + clInfo['Name'] ) )
                        {
                           type = clInfo['ShardingType'] ;
                           sourceGroupValid = [] ;
                           var sourceIndex = -1 ;
                           $.each( $scope.clList[ index ]['GroupName'], function( index2, groupInfo ){
                              if( sourceIndex < 0 )
                              {
                                 sourceIndex = index2 ;
                              }
                              sourceGroupValid.push( { 'key': groupInfo['key'], 'value': index2 } ) ;
                           } ) ;
                           $scope.Components.Modal.form1['inputList'][2]['value'] = sourceIndex ;
                           $scope.Components.Modal.form1['inputList'][2]['valid'] = sourceGroupValid ;
                           return false;
                        }
                     } ) ;

                     if( type == 'hash' )
                     {
                        $scope.Components.Modal.form2.inputList[4]['type'] = 'inline' ;
                        $scope.Components.Modal.form2.inputList[4]['child'] = [
                           {
                              "name": "field",
                              "webName": $scope.autoLanguage( "字段名" ),
                              "placeholder": $scope.autoLanguage( "字段名" ),
                              "type": "string",
                              "disabled": true,
                              "value": "Partition",
                              "valid": {
                                 "min": 1,
                                 "regex": "^[^/$].*",
                                 "ban": "."
                              }
                           },
                           {
                              "name": "start",
                              "webName": $scope.autoLanguage( "起始范围" ),
                              "placeholder": $scope.autoLanguage( "起始范围" ),
                              "type": "string",
                              "value": "",
                              "selectList": [ '$minKey' ]
                           },
                           {
                              "name": "end",
                              "webName": $scope.autoLanguage( "结束范围" ),
                              "placeholder": $scope.autoLanguage( "结束范围" ),
                              "type": "string",
                              "value": "",
                              "selectList": [ '$maxKey' ]
                           }
                        ] ;
                     }
                     else
                     {

                        $scope.Components.Modal.form2.inputList[4]['type'] = 'list' ;
                        $scope.Components.Modal.form2.inputList[4]['child'] = [
                           [
                              {
                                 "name": "field",
                                 "webName": $scope.autoLanguage( "字段名" ),
                                 "placeholder": $scope.autoLanguage( "字段名" ),
                                 "type": "string",
                                 "value": "",
                                 "valid": {
                                    "min": 1,
                                    "regex": "^[^/$].*",
                                    "ban": "."
                                 }
                              },
                              {
                                 "name": "type",
                                 "webName": $scope.autoLanguage( "类型" ),
                                 "placeholder": $scope.autoLanguage( "类型" ),
                                 "type": "select",
                                 "value": "Auto",
                                 "valid": [
                                    { "key": "Auto",      "value": "Auto" },
                                    { "key": "Bool",      "value": "Bool" },
                                    { "key": "Number",    "value": "Number" },
                                    { "key": "Decimal",   "value": "Decimal" },
                                    { "key": "String",    "value": "String" },
                                    { "key": "ObjectId",  "value": "ObjectId" },
                                    { "key": "Regex",     "value": "Regex" },
                                    { "key": "Binary",    "value": "Binary" },
                                    { "key": "Timestamp", "value": "Timestamp" },
                                    { "key": "Date",      "value": "Date" }
                                 ]
                              },
                              {
                                 "name": "start",
                                 "webName": $scope.autoLanguage( "起始范围" ),
                                 "placeholder": $scope.autoLanguage( "起始范围" ),
                                 "type": "string",
                                 "value": "",
                                 "selectList": [ '$minKey' ]
                              },
                              {
                                 "name": "end",
                                 "webName": $scope.autoLanguage( "结束范围" ),
                                 "placeholder": $scope.autoLanguage( "结束范围" ),
                                 "type": "string",
                                 "value": "",
                                 "selectList": [ '$maxKey' ]
                              }
                           ]
                        ] ;
                     }
                  }
               },
               {
                  "name": "source",
                  "webName": $scope.autoLanguage( '源分区组' ),
                  "type": "select",
                  "value": 0,
                  "valid": sourceGroupValid
               },
               {
                  "name": "target",
                  "webName": $scope.autoLanguage( '目标分区组' ),
                  "type": "select",
                  "value": 0,
                  "valid": groupValid
               },
               {
                  "name": "percent",
                  "webName": $scope.autoLanguage( '百分比切分' ),
                  "type": "double",
                  "required": true,
                  "value": 50,
                  "valid": {
                     'min': 1,
                     'max': 100
                  }
               }
            ]
         } ;
         $scope.Components.Modal.form2 = {
            inputList: [
               {
                  "name": "type",
                  "webName": $scope.autoLanguage( '切分方式' ),
                  "type": "select",
                  "value": 1,
                  "valid": [
                     { 'key': $scope.autoLanguage( '百分比切分' ), 'value': 0 },
                     { 'key': $scope.autoLanguage( '条件切分' ), 'value': 1 }
                  ],
                  "onChange": function( name, key, value ){
                     if( value == 0 )
                     {
                        $scope.Components.Modal.formShow = 0 ;
                     }
                     else if( value == 1 )
                     {
                        $scope.Components.Modal.formShow = 1 ;
                        
                        $.each( clValid, function( index, info ){
                           if( $scope.Components.Modal.form2['inputList'][1]['value'] == info['value'] )
                           {
                              type = info['type'] ;
                           }
                        } ) ;

                        if( type == 'hash' )
                        {
                           $scope.Components.Modal.form2.inputList[4]['type'] = 'inline' ;
                           $scope.Components.Modal.form2.inputList[4]['child'] = [
                              {
                                 "name": "field",
                                 "webName": $scope.autoLanguage( "字段名" ),
                                 "placeholder": $scope.autoLanguage( "字段名" ),
                                 "type": "string",
                                 "disabled": true,
                                 "value": "Partition",
                                 "valid": {
                                    "min": 1,
                                    "regex": "^[^/$].*",
                                    "ban": "."
                                 }
                              },
                              {
                                 "name": "start",
                                 "webName": $scope.autoLanguage( "起始范围" ),
                                 "placeholder": $scope.autoLanguage( "起始范围" ),
                                 "type": "string",
                                 "value": "",
                                 "selectList": [ '$minKey' ]
                              },
                              {
                                 "name": "end",
                                 "webName": $scope.autoLanguage( "结束范围" ),
                                 "placeholder": $scope.autoLanguage( "结束范围" ),
                                 "type": "string",
                                 "value": "",
                                 "selectList": [ '$maxKey' ]
                              }
                           ] ;
                        }
                        else
                        {
                           $scope.Components.Modal.form2.inputList[4]['type'] = 'list' ;
                           $scope.Components.Modal.form2.inputList[4]['child'] = [
                              [
                                 {
                                    "name": "field",
                                    "webName": $scope.autoLanguage( "字段名" ),
                                    "placeholder": $scope.autoLanguage( "字段名" ),
                                    "type": "string",
                                    "value": "",
                                    "valid": {
                                       "min": 1,
                                       "regex": "^[^/$].*",
                                       "ban": "."
                                    }
                                 },
                                 {
                                    "name": "type",
                                    "webName": $scope.autoLanguage( "类型" ),
                                    "placeholder": $scope.autoLanguage( "类型" ),
                                    "type": "select",
                                    "value": "Auto",
                                    "valid": [
                                       { "key": "Auto",      "value": "Auto" },
                                       { "key": "Bool",      "value": "Bool" },
                                       { "key": "Number",    "value": "Number" },
                                       { "key": "Decimal",   "value": "Decimal" },
                                       { "key": "String",    "value": "String" },
                                       { "key": "ObjectId",  "value": "ObjectId" },
                                       { "key": "Regex",     "value": "Regex" },
                                       { "key": "Binary",    "value": "Binary" },
                                       { "key": "Timestamp", "value": "Timestamp" },
                                       { "key": "Date",      "value": "Date" }
                                    ]
                                 },
                                 {
                                    "name": "start",
                                    "webName": $scope.autoLanguage( "起始范围" ),
                                    "placeholder": $scope.autoLanguage( "起始范围" ),
                                    "type": "string",
                                    "value": "",
                                    "selectList": [ '$minKey' ]
                                 },
                                 {
                                    "name": "end",
                                    "webName": $scope.autoLanguage( "结束范围" ),
                                    "placeholder": $scope.autoLanguage( "结束范围" ),
                                    "type": "string",
                                    "value": "",
                                    "selectList": [ '$maxKey' ]
                                 }
                              ]
                           ] ;
                        }
                     }
                     $scope.Components.Modal.form2.inputList[0]['value'] = 1 ;
                     $scope.Components.Modal.form1.inputList[1]['value'] = $scope.Components.Modal.form2.inputList[1]['value'] ;
                     $scope.Components.Modal.form1.inputList[2]['valid'] = $scope.Components.Modal.form2.inputList[2]['valid'] ;
                     $scope.Components.Modal.form1.inputList[2]['value'] = $scope.Components.Modal.form2.inputList[2]['value'] ;
                     $scope.Components.Modal.form1.inputList[3]['value'] = $scope.Components.Modal.form2.inputList[3]['value'] ;
                  }
               },
               {
                  "name": "name",
                  "webName": $scope.autoLanguage( '集合名' ),
                  "type": "select",
                  "value": clIndex,
                  "valid": clValid,
                  "onChange": function( name, key, value ){
                     $.each( $scope.clList, function( index, clInfo ){
                        if( key == ( clInfo['csName'] + '.' + clInfo['Name'] ) )
                        {
                           type = clInfo['ShardingType'] ;
                           var sourceIndex = -1 ;
                           sourceGroupValid = [] ;
                           $.each( $scope.clList[ index ]['GroupName'], function( index2, groupInfo ){
                              if( sourceIndex < 0 )
                              {
                                 sourceIndex = index2 ;
                              }
                              sourceGroupValid.push( { 'key': groupInfo['key'], 'value': index2 } ) ;
                           } ) ;
                           $scope.Components.Modal.form2['inputList'][2]['value'] = sourceIndex ;
                           $scope.Components.Modal.form2['inputList'][2]['valid'] = sourceGroupValid ;
                           return false;
                        }
                     } ) ;

                     if( type == 'hash' )
                     {
                        $scope.Components.Modal.form2.inputList[4]['type'] = 'inline' ;
                        $scope.Components.Modal.form2.inputList[4]['child'] = [
                           {
                              "name": "field",
                              "webName": $scope.autoLanguage( "字段名" ),
                              "placeholder": $scope.autoLanguage( "字段名" ),
                              "type": "string",
                              "disabled": true,
                              "value": "Partition",
                              "valid": {
                                 "min": 1,
                                 "regex": "^[^/$].*",
                                 "ban": "."
                              }
                           },
                           {
                              "name": "start",
                              "webName": $scope.autoLanguage( "起始范围" ),
                              "placeholder": $scope.autoLanguage( "起始范围" ),
                              "type": "string",
                              "value": "",
                              "selectList": [ '$minKey' ]
                           },
                           {
                              "name": "end",
                              "webName": $scope.autoLanguage( "结束范围" ),
                              "placeholder": $scope.autoLanguage( "结束范围" ),
                              "type": "string",
                              "value": "",
                              "selectList": [ '$maxKey' ]
                           }
                        ] ;
                     }
                     else
                     {
                        $scope.Components.Modal.form2.inputList[4]['type'] = 'list' ;
                        $scope.Components.Modal.form2.inputList[4]['child'] = [
                           [
                              {
                                 "name": "field",
                                 "webName": $scope.autoLanguage( "字段名" ),
                                 "placeholder": $scope.autoLanguage( "字段名" ),
                                 "type": "string",
                                 "value": "",
                                 "valid": {
                                    "min": 1,
                                    "regex": "^[^/$].*",
                                    "ban": "."
                                 }
                              },
                              {
                                 "name": "type",
                                 "webName": $scope.autoLanguage( "类型" ),
                                 "placeholder": $scope.autoLanguage( "类型" ),
                                 "type": "select",
                                 "value": "Auto",
                                 "valid": [
                                    { "key": "Auto",      "value": "Auto" },
                                    { "key": "Bool",      "value": "Bool" },
                                    { "key": "Number",    "value": "Number" },
                                    { "key": "Decimal",   "value": "Decimal" },
                                    { "key": "String",    "value": "String" },
                                    { "key": "ObjectId",  "value": "ObjectId" },
                                    { "key": "Regex",     "value": "Regex" },
                                    { "key": "Binary",    "value": "Binary" },
                                    { "key": "Timestamp", "value": "Timestamp" },
                                    { "key": "Date",      "value": "Date" }
                                 ]
                              },
                              {
                                 "name": "start",
                                 "webName": $scope.autoLanguage( "起始范围" ),
                                 "placeholder": $scope.autoLanguage( "起始范围" ),
                                 "type": "string",
                                 "value": "",
                                 "selectList": [ '$minKey' ]
                              },
                              {
                                 "name": "end",
                                 "webName": $scope.autoLanguage( "结束范围" ),
                                 "placeholder": $scope.autoLanguage( "结束范围" ),
                                 "type": "string",
                                 "value": "",
                                 "selectList": [ '$maxKey' ]
                              }
                           ]
                        ] ;
                     }
                  }
               },
               {
                  "name": "source",
                  "webName": $scope.autoLanguage( '源分区组' ),
                  "type": "select",
                  "value": 0,
                  "valid": sourceGroupValid
               },
               {
                  "name": "target",
                  "webName": $scope.autoLanguage( '目标分区组' ),
                  "type": "select",
                  "value": 0,
                  "valid": groupValid
               },
               {
                  "name": "condition",
                  "webName":  $scope.autoLanguage( '切分条件' ),
                  "required": true,
                  "type": "list",
                  "desc": $scope.autoLanguage( '起始范围填$minKey为负无穷，结束范围填$maxKey为正无穷。') ,
                  "valid": {
                     "min": 1
                  },
                  "child":[
                     [
                        {
                           "name": "field",
                           "webName": $scope.autoLanguage( "字段名" ),
                           "placeholder": $scope.autoLanguage( "字段名" ),
                           "type": "string",
                           "value": "",
                           "valid": {
                              "min": 1,
                              "regex": "^[^/$].*",
                              "ban": "."
                           }
                        },
                        {
                           "name": "type",
                           "webName": $scope.autoLanguage( "类型" ),
                           "placeholder": $scope.autoLanguage( "类型" ),
                           "type": "select",
                           "value": "Auto",
                           "valid": [
                              { "key": "Auto",      "value": "Auto" },
                              { "key": "Bool",      "value": "Bool" },
                              { "key": "Number",    "value": "Number" },
                              { "key": "Decimal",   "value": "Decimal" },
                              { "key": "String",    "value": "String" },
                              { "key": "ObjectId",  "value": "ObjectId" },
                              { "key": "Regex",     "value": "Regex" },
                              { "key": "Binary",    "value": "Binary" },
                              { "key": "Timestamp", "value": "Timestamp" },
                              { "key": "Date",      "value": "Date" }
                           ]
                        },
                        {
                           "name": "start",
                           "webName": $scope.autoLanguage( "起始范围" ),
                           "placeholder": $scope.autoLanguage( "起始范围" ),
                           "type": "string",
                           "value": "",
                           "selectList": [ '$minKey' ]
                        },
                        {
                           "name": "end",
                           "webName": $scope.autoLanguage( "结束范围" ),
                           "placeholder": $scope.autoLanguage( "结束范围" ),
                           "type": "string",
                           "value": "",
                           "selectList": [ '$maxKey' ]
                        }
                     ]
                  ]
               }
            ]
         } ;
         $scope.Components.Modal.Context = '\
      <div ng-if="data.formShow == 0" form-create para="data.form1"></div>\
      <div ng-if="data.formShow == 1" form-create para="data.form2"></div>' ;
         $scope.Components.Modal.ok = function(){
            var isAllClear = true ;
            if( $scope.Components.Modal.formShow == 0 )
            {
               isAllClear = $scope.Components.Modal.form1.check() ;
            }
            else
            {
               isAllClear = $scope.Components.Modal.form2.check() ;
            }
            if( isAllClear )
            {
               var value = {} ;
               if( $scope.Components.Modal.formShow == 0 )
               {
                  value = $scope.Components.Modal.form1.getValue() ;
               }
               else
               {
                  value = $scope.Components.Modal.form2.getValue() ;
               }
               var fullname = '' ;
               $.each( clValid, function( index, clValidInfo ){
                  if( clValidInfo['value'] == value['name'] )
                  {
                     fullname = clValidInfo['key'] ;
                     return false ;
                  }
               } ) ;
               var data = {
                  'cmd': 'split',
                  'name': fullname,
                  'source': sourceGroupValid[ value['source'] ]['key'],
                  'target': groupValid[ value['target'] ]['key']
               } ;
               if( value['type'] == 0 )
               {
                  //百分比
                  data['splitpercent'] = value['percent'] ;
               }
               else
               {
                  //条件
                  var splitquery = {} ;
                  var splitendquery = {} ;
                  if( type == 'hash' )
                  {
                     var fieldName = value['condition']['field'] ;
                     splitquery[ fieldName ]    = autoTypeConvert( value['condition']['start'], true ) ;
                     splitendquery[ fieldName ] = autoTypeConvert( value['condition']['end'], true ) ;
                     data['splitquery'] = JSON.stringify( splitquery ) ;
                     data['splitendquery'] = JSON.stringify( splitendquery ) ;
                  }
                  else
                  {
                     $.each( value['condition'], function( index, conditionInfo ){
                        var fieldName = conditionInfo['field'] ;
                        if( conditionInfo['type'] == 'Auto' )
                        {
                           splitquery[ fieldName ]    = autoTypeConvert( conditionInfo['start'], true ) ;
                           splitendquery[ fieldName ] = autoTypeConvert( conditionInfo['end'], true ) ;
                        }
                        else
                        {
                           if( conditionInfo['start'].toLowerCase() == '$minkey' )
                           {
                              splitquery[ fieldName ] = { '$minKey': 1 } ;
                           }
                           else
                           {
                              splitquery[ fieldName ]    = specifyTypeConvert( conditionInfo['start'], conditionInfo['type'] ) ;
                           }
                           if( conditionInfo['end'].toLowerCase() == '$maxkey' )
                           {
                              splitendquery[ fieldName ] = { '$maxKey': 1 } ;
                           }
                           else
                           {
                              splitendquery[ fieldName ] = specifyTypeConvert( conditionInfo['end'], conditionInfo['type'] ) ;
                           }
                        }
                     } ) ;
                     data['splitquery'] = JSON.stringify( splitquery ) ;
                     data['splitendquery'] = JSON.stringify( splitendquery ) ;
                  }
               }
               var exec = function(){
                  SdbRest.DataOperation( data, {
                     'success': function( json ){
                        _DataDatabaseIndex.getCSInfo( $scope, SdbRest ) ;
                     },
                     'failed': function( errorInfo ){
                        _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                           exec() ;
                           return true ;
                        } ) ;
                     }
                  } ) ;
               } ;
               exec() ;
            }
            return isAllClear ;
         }
      }

      //打开 创建索引 的窗口
      $scope.showCreateIndex = function(){
         if( _IndexPublic.checkEditionAndSupport( $scope, 'sequoiadb', 'Metadata' ) == false )
         {
            return ;
         }
         var clValid = [] ;
         $.each( $scope.clList, function( index, clInfo ){
            clValid.push( { 'key': clInfo['csName'] + '.' + clInfo['Name'], 'value': index } ) ;
         } ) ;
         $scope.Components.Modal.icon = 'fa-plus' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '创建索引' ) ;
         $scope.Components.Modal.isShow = true ;
         $scope.Components.Modal.form = {
            inputList: [
               _operate.selectCL( $scope, clValid ),
               _operate.indexName( $scope ),
               _operate.indexDef( $scope ),
               _operate.unique( $scope ),
               _operate.enforced( $scope ),
               _operate.sortbuffersize( $scope )
            ]
         } ;
         $scope.Components.Modal.Context = '<div form-create para="data.form"></div>' ;
         $scope.Components.Modal.ok = function(){
            var isAllClear = $scope.Components.Modal.form.check() ;
            if( isAllClear )
            {
               function modalValue2CreateIndex( valueJson )
               {
                  var indexdef = parseIndexDefValue( valueJson['indexKey'] ) ;
                  //组装
                  var returnJson = {} ;
                  returnJson['collectionname'] = clValid[ valueJson['clName'] ]['key'] ;
                  returnJson['indexname'] = valueJson['indexName'] ;
                  returnJson['indexdef'] = JSON.stringify( indexdef ) ;
                  returnJson['unique'] = valueJson['isUnique'] ;
                  returnJson['enforced'] = valueJson['enforced'] ;
                  returnJson['sortbuffersize'] = valueJson['sortbuffersize'] ;
                  return returnJson ;
               }
               var value = $scope.Components.Modal.form.getValue() ;
               var data = modalValue2CreateIndex( value ) ;
               data['cmd'] = 'create index' ;
               var exec = function(){
                  SdbRest.DataOperation( data, {
                     'success': function( json ){
                        _DataDatabaseIndex.getCLInfo( $scope, SdbRest ) ;
                     },
                     'failed': function( errorInfo ){
                        _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                           exec() ;
                           return true ;
                        } ) ;
                     }
                  } ) ;
               } ;
               exec() ;
            }
            return isAllClear ;
         }
      }

      //打开 删除索引 的窗口
      $scope.showRemoveIndex = function(){
         if( _IndexPublic.checkEditionAndSupport( $scope, 'sequoiadb', 'Metadata' ) == false )
         {
            return ;
         }
         var clValid = [] ;
         var indexValid = [] ; 
         var fullName = '' ;
         var indexesInfoList = [] ;
         var clDefault = -1 ;
         $.each( $scope.clList, function( index, clInfo ){
            if( clInfo['IsMainCL'] === true && isEmpty( clInfo['Info']['CataInfo'] ) )
            {
               return true ;
            }

            clValid.push( { 'key': clInfo['csName'] + '.' + clInfo['Name'], 'value': index } ) ;
            if( isEmpty( fullName ) )
            {
               fullName = clInfo['csName'] + '.' + clInfo['Name'] ;
            }

            if( clDefault < 0 )
            {
               clDefault = index ;
            }

            if( $scope.clID == index )
            {
               clDefault = index ;
               fullName = clInfo['csName'] + '.' + clInfo['Name'] ;
            }
         } ) ;

         $scope.Components.Modal.indexList = {} ;
         $scope.Components.Modal.icon = 'fa-trash-o' ;
         $scope.Components.Modal.title =  $scope.autoLanguage( '删除索引' ) ;
         $scope.Components.Modal.ok = function(){
            return false ;
         }    
         var data = { 'cmd': 'list indexes', 'collectionname': fullName } ;
         SdbRest.DataOperation( data, {
            'success': function( indexList ){
               indexesInfoList = indexList ;
               $.each( indexList, function( index, indexInfo ){
                  indexValid.push( { 'key': indexInfo['IndexDef']['name'], 'value': index } ) ;
               } ) ;
               $scope.Components.Modal.form = {
                  inputList: [
                     {
                        "name": "clName",
                        "webName": $scope.autoLanguage( '集合' ),
                        "type": "select",
                        "value": clDefault,
                        "valid": clValid,
                        "onChange": function( name, key, value ){
                           var data = { 'cmd': 'list indexes', 'collectionname': key } ;
                           SdbRest.DataOperation( data, {
                              'success': function( indexList ){
                                 indexesInfoList = indexList ;
                                 $scope.Components.Modal.indexList = indexesInfoList[0] ;
                                 indexValid = [] ;
                                 $.each( indexList, function( index, indexInfo ){
                                    indexValid.push( { 'key': indexInfo['IndexDef']['name'], 'value': index } ) ;
                                 } ) ;
                                 $scope.Components.Modal.form['inputList'][1]['value'] = 0 ;
                                 $scope.Components.Modal.form['inputList'][1]['valid'] = indexValid ;
                              },
                              'failed': function( errorInfo ){
                                 _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                                    exec() ;
                                    return true ;
                                 } ) ;
                              }
                           } ) ;
                        }
                     },
                     {
                        "name": "indexName",
                        "webName":  $scope.autoLanguage( '索引名' ),
                        "type": "select",
                        "value": 0,
                        "valid": indexValid,
                        "onChange": function( name, key, value ){
                           $scope.Components.Modal.indexList = indexesInfoList[ value ] ;
                        }
                     }
                  ]
               } ;
               $scope.Components.Modal.indexList = indexesInfoList[0] ;
               $scope.Components.Modal.Context = '\
         <div form-create para="data.form"></div>\
         <table class="table loosen border" ng-if="data.indexList">\
         <tr>\
         <td style="width:40%;background-color:#F1F4F5;"><b>Key</b></td>\
         <td style="width:60%;background-color:#F1F4F5;"><b>Value</b></td>\
         </tr>\
         <tr>\
         <td>Name</td>\
         <td>{{data.indexList.IndexDef.name}}</td>\
         </tr>\
         <tr ng-repeat="(key, value) in data.indexList.IndexDef track by $index" ng-if="key != \'name\'&&key != \'_id\'">\
         <td>{{key}}</td>\
         <td>{{value}}</td>\
         </tr>\
         <tr>\
         <td>IndexFlag</td>\
         <td>{{data.indexList.IndexFlag}}</td>\
         </tr>\
         </table>' ;
               $scope.Components.Modal.isShow = true ;
               $scope.Components.Modal.ok = function(){
                  var isAllClear = $scope.Components.Modal.form.check() ;
                  if( isAllClear )
                  {
                     var value = $scope.Components.Modal.form.getValue() ;
                     var data = { 'cmd': 'drop index' } ;
                     data['collectionname'] = clValid[ value['clName'] ]['key'] ;
                     data['indexname'] = indexValid[ value['indexName'] ]['key'] ;
                     var exec = function(){
                        SdbRest.DataOperation( data, {
                           'success': function( json ){
                              _DataDatabaseIndex.getCLInfo( $scope, SdbRest ) ;
                           },
                           'failed': function( errorInfo ){
                              _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                                 exec() ;
                                 return true ;
                              } ) ;
                           }
                        } ) ;
                     } ;
                     exec() ;
                  }
                  return isAllClear ;
               }   
               $scope.$apply() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  exec() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //打开 索引详细 的窗口
      $scope.showIndex = function( csIndex, clIndex ){
         var fullName = $scope.clList[clIndex]['csName'] + '.' + $scope.clList[clIndex]['Name'] ;
         var data = { 'cmd': 'list indexes', 'collectionname': fullName } ;
         SdbRest.DataOperation( data, {
            'success': function( indexList ){
               var indexName = [] ; 
               var indexContent = [] ;
               $.each( indexList, function( index, indexInfo ){
                  indexName.push( { 'key': indexInfo['IndexDef']['name'] , 'value': index } ) ;
                  indexContent.push( indexInfo ) ;
               } ) ;
               $scope.Components.Modal.icon = '' ;
               $scope.Components.Modal.title = $scope.autoLanguage( '索引信息' ) ;
               $scope.Components.Modal.noOK = true ;
               $scope.Components.Modal.isShow = true ;
               $scope.Components.Modal.CloseText = $scope.autoLanguage( "关闭" ) ;
               $scope.Components.Modal.form = {
                  inputList: [
                     {
                        "name": "index",
                        "webName": $scope.autoLanguage( "索引名" ),
                        "type": "select",
                        "value":indexName[0]['value'] ,
                        "valid": indexName,
                        "onChange": function( name, key, value ){
                           $scope.Components.Modal.indexList = indexContent[ value ] ;
                        }
                     }
                  ]
               } ;
               $scope.Components.Modal.indexList = indexContent[0] ;
               $scope.Components.Modal.Context = '\
         <div form-create para="data.form"></div>\
         <table class="table loosen border">\
         <tr>\
         <td style="width:40%;background-color:#F1F4F5;"><b>Key</b></td>\
         <td style="width:60%;background-color:#F1F4F5;"><b>Value</b></td>\
         </tr>\
         <tr>\
         <td>Name</td>\
         <td>{{data.indexList.IndexDef.name}}</td>\
         </tr>\
         <tr ng-repeat="(key, value) in data.indexList.IndexDef track by $index" ng-if="key != \'name\'&&key != \'_id\'">\
         <td>{{key}}</td>\
         <td>{{value}}</td>\
         </tr>\
         <tr>\
         <td>IndexFlag</td>\
         <td>{{data.indexList.IndexFlag}}</td>\
         </tr>\
         </table>' ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  exec() ;
                  return true ;
               }, $scope.autoLanguage( '获取索引信息失败' ) ) ;
            }
         } ) ;
      }

      //添加自增字段 弹窗
      $scope.CreateAutoIncrementWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 添加自增字段 窗口
      $scope.ShowCreateAutoIncrement = function(){
         if( _IndexPublic.checkEditionAndSupport( $scope, 'sequoiadb', 'Metadata' ) == false )
         {
            return ;
         }
         var clValid = [] ;
         var clIndex = -1 ;
         $.each( $scope.clList, function( index, clInfo ){
            if( $scope.showType == 'cs' )
            {
               if( $scope.csList[ $scope.csID ]['Name'] == clInfo['csName'] && clIndex < 0 )
               {
                  clIndex = index ;
               }
            }
            else
            {
               if( index == $scope.clID )
               {
                  clIndex = index ;
               }
            }
            clValid.push( { 'key' : clInfo['csName'] + '.' + clInfo['Name'] , 'value' : index } );
         } ) ;
         if( clIndex < 0 )
            clIndex = 0 ;
         $scope.CreateAutoIncrementWindow['callback']['SetTitle']( $scope.autoLanguage( '创建自增字段' ) ) ;
         $scope.CreateAutoIncrementWindow['callback']['SetIcon']( 'fa-plus' ) ;
         $scope.CreateAutoIncrementWindow['config'] = {
            'inputList': [
               {
                  "name": "Name",
                  "webName": "Collection",
                  "type": "select",
                  "required": true,
                  "value": clIndex,
                  "valid": clValid
               },
               {
                  "name": "Field",
                  "webName": 'Field',
                  "desc": $scope.autoLanguage( '自增字段名' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1,
                     "regex": "^[^/$].*"
                  }
               },
               {
                  "name": "StartValue",
                  "webName": 'StartValue',
                  "desc": $scope.autoLanguage( '起始值' ),
                  "type": "int",
                  "value": 1,
                  "valid": {
                     "empty": true,
                     "min": 1
                  }
               },
               {
                  "name": "Increment",
                  "webName": 'Increment',
                  "desc": $scope.autoLanguage( '自增间隔' ),
                  "type": "int",
                  "value": 1,
                  "valid": {
                     "empty": true,
                     "min": 1
                  }
               },
               {
                  "name": "MinValue",
                  "webName": 'MinValue',
                  "desc": $scope.autoLanguage( '最小值' ),
                  "type": "int",
                  "value": 1,
                  "valid": {
                     "empty": true,
                     "min": 0
                  }
               },
               {
                  "name": "MaxValue",
                  "webName": 'MaxValue',
                  "desc": $scope.autoLanguage( '最大值' ),
                  "type": "string",
                  "value": '9223372036854775807',
                  "valid": {
                     "empty": true
                  }
               },
               {
                  "name": "CacheSize",
                  "webName": 'CacheSize',
                  'desc': $scope.autoLanguage( '编目节点每次缓存的序列值的数量，取值须大于0' ),
                  "type": "int",
                  "value": 1000,
                  "valid": {
                     "empty": true,
                     "min": 0
                  }
               },
               {
                  "name": "AcquireSize",
                  "webName": 'AcquireSize',
                  'desc': $scope.autoLanguage( '协调节点每次获取的序列值的数量，取值须大于0，小于等于CacheSize' ),
                  "type": "int",
                  "value": 1000,
                  "valid": {
                     "empty": true,
                     "min": 0
                  }
               },
               {
                  "name": "Cycled",
                  "webName": 'Cycled',
                  'desc': $scope.autoLanguage( '序列值达到最大值或最小值时是否允许循环' ),
                  "type": "select",
                  "value": false,
                  "valid": [
                     { 'key': false, 'value': false },
                     { 'key': true, 'value': true }
                  ]
               },
               {
                  "name": "Generated",
                  "webName": 'Generated',
                  'desc': $scope.autoLanguage( '自增字段生成方式' ),
                  "type": "select",
                  "value": 'default',
                  "valid": [
                     { 'key': 'default', 'value': 'default' },
                     { 'key': 'always', 'value': 'always' },
                     { 'key': 'strict', 'value': 'strict' }
                  ]
               }
            ]
         } ;
         
         $scope.CreateAutoIncrementWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isClear = $scope.CreateAutoIncrementWindow['config'].check( function( valueJson ){
               var rv = [] ;
               if( valueJson['StartValue'] < valueJson['MinValue'] || valueJson['StartValue'] > valueJson['MaxValue'] )
               {
                  rv.push( { 'name': 'StartValue', 'error': $scope.autoLanguage( 'StartValue 的值必须大于等于 MinValue，小于等于 MaxValue') } ) ;
               }
               return rv ;
            } ) ;
            if( isClear )
            {
               var formVal = $scope.CreateAutoIncrementWindow['config'].getValue() ;
               var data = { 'cmd': 'create autoincrement' } ;
               data['Name'] = clValid[ formVal['Name'] ]['key'] ;
               data['options'] = { 'AutoIncrement': {} } ;
               $.each( formVal, function( key, value ){
                  if( key != "Name" )
                  {
                     data['options']['AutoIncrement'][key] = value ;
                  }
               } ) ;
               data['options']['AutoIncrement']['MaxValue'] = { '$numberLong': data['options']['AutoIncrement']['MaxValue'] }  ;
               data['options'] = JSON.stringify(data['options']) ;
               var exec = function(){
                  SdbRest.DataOperation( data, {
                     'success': function( json ){
                        _DataDatabaseIndex.getCLInfo( $scope, SdbRest ) ;
                     },
                     'failed': function( errorInfo ){
                        _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                           exec() ;
                           return true ;
                        } ) ;
                     }
                  } ) ;
               } ;
               exec() ;
            }
            return isClear ;
         } ) ;
         $scope.CreateAutoIncrementWindow['callback']['Open']() ;
      }

      //添加自增字段 弹窗
      $scope.RemoveAutoIncrementWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 删除自增字段 窗口
      $scope.ShowRemoveAutoIncrement = function(){
         if( _IndexPublic.checkEditionAndSupport( $scope, 'sequoiadb', 'Metadata' ) == false )
         {
            return ;
         }
         if( !$scope.HasAutoIncrement )
         {
            return ;
         }
         var clValid = [] ;
         var inInfo = [] ;
         var incrementValid = {} ; 
         var fullName = '' ;
         var incrementInfoList = [] ;
         var clDefault = -1 ;
         $.each( $scope.clList, function( index, clInfo ){
            if( isUndefined( clInfo['AutoIncrement'] ) || isEmpty( clInfo['AutoIncrement'] ) )
            {
               return true ;
            }
            clValid.push( { 'key': clInfo['csName'] + '.' + clInfo['Name'], 'value': index } ) ;
            if( isEmpty( fullName ) )
               fullName = clInfo['csName'] + '.' + clInfo['Name'] ;
            if( clDefault < 0 )
               clDefault = index ;
            if( $scope.clID == index )
            {
               clDefault = index ;
               fullName = clInfo['csName'] + '.' + clInfo['Name'] ;
            }
            inInfo = [] ;
            $.each( clInfo['AutoIncrement'], function( index, info ){
               inInfo.push( { 'key': info['Field'], 'value': index, 'info': info } ) ;
            } ) ;
            incrementValid[clInfo['csName'] + '.' + clInfo['Name']] = inInfo ;
         } ) ;

         $scope.RemoveAutoIncrementWindow.sequenceContent = {} ;
         $scope.RemoveAutoIncrementWindow['callback']['SetTitle']( $scope.autoLanguage( '删除自增字段' ) ) ;
         $scope.RemoveAutoIncrementWindow['callback']['SetIcon']( 'fa-trash-o' ) ;
         $scope.RemoveAutoIncrementWindow['config'] = {
            inputList: [
               {
                  "name": "clName",
                  "webName": $scope.autoLanguage( '集合' ),
                  "type": "select",
                  "value": clDefault,
                  "valid": clValid,
                  "onChange": function( name, key, value ){
                     $scope.RemoveAutoIncrementWindow['config']['inputList'][1]['value'] = 0 ;
                     $scope.RemoveAutoIncrementWindow['config']['inputList'][1]['valid'] = incrementValid[key] ;
                     $scope.RemoveAutoIncrementWindow.sequenceContent = incrementValid[key][0]['info'] ;
                     fullName = key ;
                  }
               },
               {
                  "name": "autoIncrementName",
                  "webName":  $scope.autoLanguage( '自增字段名' ),
                  "type": "select",
                  "value": incrementValid[fullName][0]['value'],
                  "valid": incrementValid[fullName],
                  "onChange": function( name, key, value ){
                     $scope.RemoveAutoIncrementWindow.sequenceContent = incrementValid[fullName][value]['info'] ;
                  }
               }
            ]
         } ;
         $scope.RemoveAutoIncrementWindow.sequenceContent = incrementValid[fullName][0]['info'] ;
         $scope.RemoveAutoIncrementWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isClear = $scope.RemoveAutoIncrementWindow['config'].check() ;
            if( isClear )
            {
               var formVal = $scope.RemoveAutoIncrementWindow['config'].getValue() ;
               var autoIncrementName = incrementValid[fullName][formVal['autoIncrementName']]['info']['Field'] ;
               var data = { 'cmd': 'drop autoincrement' } ;
               data['Name'] = fullName ;
               data['options'] = { 'Field': autoIncrementName } ;
               data['options'] = JSON.stringify( data['options'] ) ;
               var exec = function(){
                  SdbRest.DataOperation( data, {
                     'success': function( json ){
                        $scope.HasAutoIncrement = false ;
                        _DataDatabaseIndex.getCLInfo( $scope, SdbRest ) ;
                     },
                     'failed': function( errorInfo ){
                        _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                           exec() ;
                           return true ;
                        } ) ;
                     }
                  } ) ;
               } ;
               exec() ;
            }
            return isClear ;
         } ) ;
         $scope.RemoveAutoIncrementWindow['callback']['Open']() ;
      }

      //自增字段信息 弹窗
      $scope.AutoIncrementWindow = {
         'config': {},
         'callback': {}
      }

      //打开自增字段信息弹窗
      $scope.ShowAutoIncrement = function( list ){
         $scope.AutoIncrementWindow.sequenceContent = {} ;
         $scope.AutoIncrementWindow['callback']['SetTitle']( $scope.autoLanguage( '自增字段信息' ) ) ;
         var sql = 'SELECT * FROM $SNAPSHOT_SEQUENCES'  ;
         SdbRest.Exec( sql, {
            'success': function( result ){
               var fieldName = [] ; 
               var sequenceContent = [] ;
               $.each( list, function( index, sequenceInfo ){
                  fieldName.push( { 'key': sequenceInfo['Field'], 'value': index } ) ;
               } ) ;

               $.each( list, function( index, info ){
                  $.each( result, function( index2, info2 ){
                     if( info['SequenceID'] == info2['ID'] )
                     {
                        info2['Field'] = info['Field'] ;
                        info2['MaxValue'] = JSON.stringify( info2['MaxValue'] ) ;
                        sequenceContent.push( info2 ) ;
                     }
                  } ) ;
               } ) ;
               $scope.AutoIncrementWindow['config'] = {
                  inputList: [
                     {
                        "name": "sequence",
                        "webName": $scope.autoLanguage( "自增字段名" ),
                        "type": "select",
                        "value":fieldName[0]['value'] ,
                        "valid": fieldName,
                        "onChange": function( name, key, value ){
                           $scope.AutoIncrementWindow.sequenceContent = sequenceContent[value] ;
                        }
                     }
                  ]
               } ;
               $timeout( function(){
                  $scope.bindResize() ;
               } ) ;
               $scope.AutoIncrementWindow.sequenceContent = sequenceContent[0] ;
               $scope.AutoIncrementWindow['callback']['Open']() ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  exec() ;
                  return true ;
               }, $scope.autoLanguage( '获取自增字段失败' ) ) ;
            }
         }, { 'showLoading': true } ) ;
      }

      //打开 切分范围 的窗口
      $scope.showPartitions = function(){
         $scope.Components.Modal.icon = '' ;
         $scope.Components.Modal.title = $scope.autoLanguage( '分区信息' ) ;
         $scope.Components.Modal.noOK = true ;
         $scope.Components.Modal.isShow = true ;
         var partitionInfo = [] ;
         if( $scope.clList[ $scope.clID ]['IsMainCL'] == true )
         {
            $.each( $scope.clList[ $scope.clID ]['Info']['CataInfo'], function( index, info ){
               partitionInfo.push( {
                  'name'     : info['SubCLName'],
                  'LowBound' : info['LowBound'],
                  'UpBound'  : info['UpBound']
               } ) ;
            } ) ;
            $scope.Components.Modal.partitionInfo = partitionInfo ;
            $scope.Components.Modal.Context = '\
      <table class="table loosen border">\
         <tr>\
            <td style="width:30%;background-color:#F1F4F5;"><b>Partition</b></td>\
            <td style="width:35%;background-color:#F1F4F5;"><b>LowBound</b></td>\
            <td style="width:35%;background-color:#F1F4F5;"><b>UpBound</b></td>\
         </tr>\
         <tr ng-repeat="(key, value) in data.partitionInfo track by $index">\
            <td>{{value["name"]}}</td>\
            <td>{{value["LowBound"]}}</td>\
            <td>{{value["UpBound"]}}</td>\
         </tr>\
      </table>' ;
         }
         else
         {
            if( $scope.selectGroup == null )
            {
               $.each( $scope.clList[ $scope.clID ]['GroupName'], function( index, groupInfo ){
                  $.each( $scope.clInfo[ groupInfo['value'] ]['LowBound'], function( index2 ){
                     partitionInfo.push( {
                        'GroupName': $scope.clInfo[ groupInfo['value'] ]['GroupName'],
                        'LowBound' : $scope.clInfo[ groupInfo['value'] ]['LowBound'][index2],
                        'UpBound'  : $scope.clInfo[ groupInfo['value'] ]['UpBound'][index2]
                     } ) ;
                  } ) ;
               } ) ;
            }
            else
            {
               $.each( $scope.clInfo[ $scope.selectGroup ]['LowBound'], function( index2 ){
                  partitionInfo.push( {
                     'GroupName': $scope.clInfo[ $scope.selectGroup ]['GroupName'],
                     'LowBound' : $scope.clInfo[ $scope.selectGroup ]['LowBound'][index2],
                     'UpBound'  : $scope.clInfo[ $scope.selectGroup ]['UpBound'][index2]
                  } ) ;
               } ) ;
            }
            $scope.Components.Modal.partitionInfo = partitionInfo ;
            $scope.Components.Modal.Context = '\
      <table class="table loosen border">\
         <tr>\
            <td style="width:30%;background-color:#F1F4F5;"><b>Group</b></td>\
            <td style="width:35%;background-color:#F1F4F5;"><b>LowBound</b></td>\
            <td style="width:35%;background-color:#F1F4F5;"><b>UpBound</b></td>\
         </tr>\
         <tr ng-repeat="(key, value) in data.partitionInfo track by $index">\
            <td>{{value["GroupName"]}}</td>\
            <td>{{value["LowBound"]}}</td>\
            <td>{{value["UpBound"]}}</td>\
         </tr>\
      </table>' ;
         }
      }

      //显示不同分区组的信息
      $scope.showGroupInfo = function( index ){
         $scope.selectGroup = index ;
         if( $scope.showType == 'cs' )
         {
            if( index == null )
            {
               $scope.attr['Info'] = $scope.csList[ $scope.csID ]['Info'] ;
            }
            else
            {
               $scope.attr['Info'] = $scope.csInfo[ index ] ;
            }
         }
         else
         {
            if( index == null )
            {
               $scope.attr['Info'] = $scope.clList[ $scope.clID ]['Info'] ;
            }
            else
            {
               $scope.attr['Info'] = $scope.clInfo[ index ] ;
            }
         }
      }

      //是否显示子集合
      $scope.switchShowSubCl = function( event ){
         var clList = $.extend( true, [], $scope.sourceClList ) ;
         var newClList = [] ;
         $scope.isHideSubCl = $( event.target ).is(':checked') ;
         if( $scope.isHideSubCl == true )
         {
            $.each( clList, function( index, clInfo ){
               if( typeof( clInfo['MainCLName'] ) != 'string' )
               {
                  newClList.push( clInfo ) ;
               }
            } ) ;
            SdbFunction.LocalData( 'SdbHidePartition', 1 ) ;
         }
         else
         {
            newClList = clList ;
            SdbFunction.LocalData( 'SdbHidePartition', null ) ;
         }
         $scope.showCSInfo( 0 ) ;
         _DataDatabaseIndex.buildClList( $scope, newClList ) ;
         $.each( $scope.csList, function( index, csInfo ){
            csInfo.show = false ;
         } ) ;
         setClTableTitle() ;
      }
      //获取cs信息
      _DataDatabaseIndex.getCSInfo( $scope, SdbRest ) ;

   } ) ;
}());