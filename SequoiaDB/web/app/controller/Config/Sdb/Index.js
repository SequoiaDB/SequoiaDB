//@ sourceURL=Index.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Config.SDB.Index.Ctrl', function( $scope, $location, SdbFunction, SdbRest, SdbSignal, SdbSwap, SdbPromise ){
      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;
      if( clusterName == null || moduleType != 'sequoiadb' || moduleName == null || moduleMode == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      SdbSwap.clusterName = clusterName ;
      SdbSwap.moduleName = moduleName ;
      SdbSwap.moduleMode = moduleMode ;

      $scope.ModuleName = moduleName ;
      $scope.ModuleMode = moduleMode ;
      $scope.ConfigName = '' ;
      $scope.BrowseMode = 'Node' ;
      SdbSwap.Expand = false ;
      SdbSwap.TemplateIndex = [] ;
      SdbSwap.TemplateDesc = {} ;
      SdbSwap.Template = [] ;
      SdbSwap.HiddenTemplate = [] ;
      SdbSwap.RunConfigs = [] ;
      SdbSwap.LocalConfigs = [] ;
      SdbSwap.GroupList = [] ;

      if ( moduleMode == 'standalone' )
      {
         $scope.BrowseMode = 'Config' ;
      }

      $scope.SwitchTab = function( name ) {
         $scope.BrowseMode = name ;

         if ( name == 'Node' )
         {
            SdbSignal.commit( 'ResizeNodeTable' ) ;
         }
         else
         {
            SdbSignal.commit( 'ResizeConfigTable' ) ;
         }
      }

      function getConfigTemplate()
      {
         var data = { 'cmd': 'get config template', 'BusinessType': 'sequoiadb' } ;
         SdbRest.OmOperation( data, {
            'success': function( result ){
               if ( result.length > 0 )
               {
                  SdbSwap.Template = result[0]['Property'] ;
                  SdbSwap.TemplateDesc = {} ;

                  $.each( SdbSwap.Template, function( index, configInfo ){
                     var key = configInfo['Name'] ;
                     if ( configInfo['hidden'] != 'true' )
                     {
                        SdbSwap.TemplateIndex.push( configInfo['Name'] ) ;
                     }
                     else
                     {
                        SdbSwap.HiddenTemplate.push( configInfo ) ;
                     }
                     SdbSwap.TemplateDesc[key] = configInfo ;
                  } ) ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getConfigTemplate() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      SdbSwap.sortNodeName = function( a, b )
      {
         if( a['NodeName'] == b['NodeName'] )
         {
            return 0 ;
         }
         else if( a['NodeName'] > b['NodeName'] )
         {
            return 1 ;
         }
         else
         {
            return -1 ;
         }
      }

      SdbSwap.getGroupList = function( func )
      {
         if ( moduleMode == 'standalone' )
         {
            var result = [] ;

            SdbSwap.GroupList = result ;
            SdbSignal.commit( 'SetGroupList', result ) ;
            SdbSignal.commit( 'SetNodeList', result ) ;
            if ( isFunction( func ) )
            {
               func() ;
            }
         }
         else
         {
            var data = { 'cmd': 'list groups' } ;
            SdbRest.DataOperation( data, {
               'success': function( result ){
                  SdbSwap.GroupList = result ;
                  SdbSignal.commit( 'SetGroupList', result ) ;
                  SdbSignal.commit( 'SetNodeList', result ) ;
                  if ( isFunction( func ) )
                  {
                     func() ;
                  }
               },
               'failed': function( errorInfo ){
                  _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                     SdbSwap.getGroupList() ;
                     return true ;
                  } ) ;
               }
            } ) ;
         }
      }

      SdbSwap.initDefer = SdbPromise.init( 3 ) ;

      SdbSwap.getRunConfigs = function( expand, func )
      {
         var data = { 'cmd': 'snapshot configs', 'hint': JSON.stringify( { '$options': { 'mode': 'run', 'expand': expand } } ) } ;
         SdbRest.DataOperation( data, {
            'success': function( result ){

               var newResult = [] ;
               $.each( result, function( index, info ){
                  if( hasKey( info, 'ErrNodes' ) && getObjectSize( info ) == 1 )
                  {
                     for( var i in info['ErrNodes'] )
                     {
                        newResult.push( { 'NodeName': info['ErrNodes'][i]['NodeName'] } ) ;
                     }
                  }
                  else
                  {
                     newResult.push( info ) ;
                  }
               } ) ;

               newResult = newResult.sort( SdbSwap.sortNodeName ) ;

               SdbSwap.RunConfigs = newResult ;
               SdbSwap.initDefer.resolve( 'run', newResult ) ;
               if ( isFunction( func ) )
               {
                  func() ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  SdbSwap.getRunConfigs( expand, func ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      SdbSwap.getLocalConfigs = function( expand, func )
      {
         var data = { 'cmd': 'snapshot configs', 'hint': JSON.stringify( { '$options': { 'mode': 'local', 'expand': expand } } ) } ;
         SdbRest.DataOperation( data, {
            'success': function( result ){

               var newResult = [] ;
               $.each( result, function( index, info ){
                  if( hasKey( info, 'ErrNodes' ) && getObjectSize( info ) == 1 )
                  {
                     for( var i in info['ErrNodes'] )
                     {
                        newResult.push( { 'NodeName': info['ErrNodes'][i]['NodeName'] } ) ;
                     }
                  }
                  else
                  {
                     newResult.push( info ) ;
                  }
               } ) ;

               newResult = newResult.sort( SdbSwap.sortNodeName ) ;

               SdbSwap.LocalConfigs = newResult ;
               SdbSwap.initDefer.resolve( 'local', newResult ) ;
               if ( isFunction( func ) )
               {
                  func() ;
               }
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  SdbSwap.getLocalConfigs( expand, func ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //节点列表
      $scope.SwitchNodeList = function() {
         var nodeList = [] ;
         $.each( SdbSwap.GroupList, function( index, groupInfo ){
            $.each( groupInfo['Group'], function( index2, nodeInfo ){
               nodeList.push( nodeInfo['HostName'] + ':' + nodeInfo['Service'][0]['Name'] ) ;
            } ) ;
         } ) ;
         SdbSignal.commit( 'ShowConfig', nodeList ) ;
         $scope.SwitchTab( 'Config' ) ;
      }

      SdbSignal.on( 'SetTitle', function( name ){
         if ( isArray( name ) && name.length > 1 )
         {
            $scope.ConfigName = $scope.autoLanguage( '批量节点配置' ) ;
         }
         else
         {
            if ( name.length > 0 )
            {
               $scope.ConfigName = $scope.autoLanguage( '节点配置' ) ;
            }
            else
            {
               $scope.ConfigName = '' ;
            }
         }
      } ) ;

      getConfigTemplate() ;
      SdbSwap.getGroupList() ;

      if ( moduleMode != 'standalone' )
      {
         SdbSwap.getRunConfigs( SdbSwap.Expand ) ;
         SdbSwap.getLocalConfigs( SdbSwap.Expand ) ;
      }
      else
      {
         var defer = SdbPromise.init( 3 ) ;

         SdbSwap.getGroupList( function(){
            defer.resolve( 'group', 1 ) ;
         } ) ;

         SdbSwap.getRunConfigs( SdbSwap.Expand, function(){
            defer.resolve( 'run', 1 ) ;
         } ) ;

         SdbSwap.getLocalConfigs( SdbSwap.Expand, function(){
            defer.resolve( 'local', 1 ) ;
         } ) ;

         defer.then( function(){
            SdbSignal.commit( 'ShowConfig', [ SdbSwap.RunConfigs[0]['NodeName'] ] ) ;
         } ) ;
      }

   } ) ;

   sacApp.controllerProvider.register( 'Config.SDB.Group.Ctrl', function( $scope, SdbSignal ){

      var groupList = [] ;
      $scope.SearchGroupName = '' ;
      $scope.GroupList = [] ;

      //分区组列表
      SdbSignal.on( 'SetGroupList', function( data ){
         var groupFilter = [] ;
         groupList = [] ;
         $.each( data, function( index, groupInfo ){
            var newGroup = {} ;

            newGroup['groupName'] = groupInfo['GroupName'] ;
            newGroup['role'] = groupInfo['Role'] == 0 ? 'data' : '' ;
            newGroup['nodeNum'] = groupInfo['Group'].length ;
            newGroup['checked'] = false ;

            groupFilter.push( { 'key': groupInfo['GroupName'], 'value': groupInfo['GroupName'] } ) ;

            groupList.push( newGroup ) ;
         } ) ;
         $scope.GroupList = groupList ;

         SdbSignal.commit( 'SetNodeTableGroupSelect', groupFilter ) ;
      } ) ;

      $scope.Find = function(){
         if ( $scope.SearchGroupName.length == 0 )
         {
            $scope.GroupList = groupList ;
         }
         else
         {
            $scope.GroupList = [] ;
            $.each( groupList, function( index, groupInfo ){
               if ( groupInfo['groupName'].indexOf( $scope.SearchGroupName ) >= 0 ||
                    groupInfo['nodeNum'] == $scope.SearchGroupName )
               {
                  $scope.GroupList.push( groupInfo ) ;
               }
            } ) ;
         }
      }

      //选择分区组
      $scope.SwitchGroup = function( index ){
         //关闭分区组列表其他组的选中状态
         $.each( $scope.GroupList, function( index2 ){
            if( index != index2 )
            {
               $scope.GroupList[index2]['checked'] = false ;
            }
         } ) ;

         //切换分区组状态
         $scope.GroupList[index]['checked'] = !$scope.GroupList[index]['checked'] ;

         SdbSignal.commit( 'SetNodeTableFilter', { 'key': 'status', 'value': '' } ) ;
         SdbSignal.commit( 'SetNodeTableFilter', { 'key': 'NodeName', 'value': '' } ) ;
         SdbSignal.commit( 'SetNodeTableFilter', { 'key': 'dbpath', 'value': '' } ) ;
         SdbSignal.commit( 'SetNodeTableFilter', { 'key': 'role', 'value': '' } ) ;
         if( $scope.GroupList[index]['checked'] == true )
         {
            //选中状态
            SdbSignal.commit( 'SetNodeTableFilter', { 'key': 'datagroupname', 'value': $scope.GroupList[index]['groupName'] } ) ;
         }
         else
         {
            //取消选中状态
            SdbSignal.commit( 'SetNodeTableFilter', { 'key': 'datagroupname', 'value': '' } ) ;
         }
      }

   } ) ;

   sacApp.controllerProvider.register( 'Config.SDB.Node.Ctrl', function( $scope, SdbSignal, SdbSwap, SdbFunction, SdbPromise, SdbRest, Loading ){

      $scope.NodeTable = {
         'title': {
            'checked':        '',
            'status':         $scope.autoLanguage( '状态' ),
            'NodeName':       $scope.autoLanguage( '节点名' ),
            'dbpath':         $scope.autoLanguage( '数据路径' ),
            'role':           $scope.autoLanguage( '角色' ),
            'datagroupname':  $scope.autoLanguage( '分区组' )
         },
         'body': [],
         'options': {
            'width': {
               'checked':        '26px',
               'status':         '60px',
               'NodeName':       '30%',
               'dbpath':         '45%',
               'role':           '70px',
               'datagroupname':  '25%'
            },
            'sort': {
               'checked':        false,
               'status':         true,
               'NodeName':       true,
               'dbpath':         true,
               'role':           true,
               'datagroupname':  true
            },
            'filter': {
               'checked': null,
               'status': [
                  { 'key': $scope.autoLanguage( '全部' ), 'value': '' },
                  { 'key': $scope.autoLanguage( '变化' ), 'value': 'change' },
                  { 'key': $scope.autoLanguage( '无变化' ), 'value': 'unchanged' }
               ],
               'NodeName': 'indexof',
               'dbpath':   'indexof',
               'role': [
                  { 'key': $scope.autoLanguage( '全部' ), 'value': '' },
                  { 'key': 'coord', 'value': 'coord' },
                  { 'key': 'catalog', 'value': 'catalog' },
                  { 'key': 'data', 'value': 'data' }
               ],
               'datagroupname': []
            }
         },
         'callback': {}
      } ;

      $scope.SwitchNode = function( nodeName ){
         SdbSignal.commit( 'SetTitle', nodeName ) ;
         $scope.SwitchTab( 'Config' ) ;
         SdbSignal.commit( 'ShowConfig', nodeName ) ;
      }

      //节点表格 Resize
      SdbSignal.on( 'ResizeNodeTable', function(){
         $scope.NodeTable['callback']['Resize']() ;
         $scope.NodeTable['callback']['ResizeTableHeader']() ;
      } ) ;

      //编辑配置
      $scope.ConfigWindows = {
         'config': {
            'type': 'csv',
            'text': ''
         },
         'callback': {}
      } ;


      //解析导入的配置
      function parseSaveConfig()
      {
         var json, data ;
         var nodeList = [] ;

         if( $scope.ConfigWindows['config']['type'] == 'csv' )
         {
            try {
               nodeList = CSV.parse( $scope.ConfigWindows['config']['text'] ) ;
            } catch( e ) {
               alert( $scope.autoLanguage("无效的CSV格式") ) ;
               return false ;
            }
         }
         else
         {
            //转成json字符串
            if( $scope.ConfigWindows['config']['type'] == 'json' )
            {
               data = $scope.ConfigWindows['config']['text'] ;
            }
            else if( $scope.ConfigWindows['config']['type'] == 'xml' )
            {
               var xotree = new XML.ObjTree();
				   var dumper = new JKL.Dumper(); 
				   var tree = xotree.parseXML( $scope.ConfigWindows['config']['text'] ) ;
				   data = dumper.dump( tree ) ;
            }
            //解析成对象
            try{
               data = JSON.parse( data ) ;
            }catch( e ){
               alert( e.message ) ;
               return false ;
            }
            //简单校验
            if( isObject( data['Deploy'] ) == false )
            {
               alert( sprintf( $scope.autoLanguage( '导入失败, ?解析失败。' ), 'Deploy' ) ) ;
               return false ;
            }

            //转换coord
            if( isObject( data['Deploy']['Coord'] ) )
            {
               if( isArray( data['Deploy']['Coord']['Node'] ) )
               {
                  $.each( data['Deploy']['Coord']['Node'], function( index, nodeInfo ){
                     nodeList.push( nodeInfo ) ;
                  } ) ;
               }
               else if( isObject( data['Deploy']['Coord']['Node'] ) )
               {
                  var nodeInfo = data['Deploy']['Coord']['Node'] ;
                  nodeList.push( nodeInfo ) ;
               }
            }
            //转换catalog
            if( isObject( data['Deploy']['Catalog'] ) )
            {
               if( isArray( data['Deploy']['Catalog']['Node'] ) )
               {
                  $.each( data['Deploy']['Catalog']['Node'], function( index, nodeInfo ){
                     nodeList.push( nodeInfo ) ;
                  } ) ;
               }
               else if( isObject( data['Deploy']['Catalog']['Node'] ) )
               {
                  var nodeInfo = data['Deploy']['Catalog']['Node'] ;
                  nodeList.push( nodeInfo ) ;
               }
            }
            //转换data
            if( isObject( data['Deploy']['Data'] ) )
            {
               if( isArray( data['Deploy']['Data']['Group'] ) )
               {
                  $.each( data['Deploy']['Data']['Group'], function( index, groupInfo ){
                     if( isArray( groupInfo['Node'] ) )
                     {
                        $.each( groupInfo['Node'], function( index, nodeInfo ){
                           nodeList.push( nodeInfo ) ;
                        } ) ;
                     }
                     else if( isObject( groupInfo['Node'] ) )
                     {
                        nodeList.push( nodeInfo ) ;
                     }
                  } ) ;
               }
               else if( isObject( data['Deploy']['Data']['Group'] ) )
               {
                  var groupInfo = data['Deploy']['Data']['Group'] ;
                  if( isArray( groupInfo['Node'] ) )
                  {
                     $.each( groupInfo['Node'], function( index, nodeInfo ){
                        nodeList.push( nodeInfo ) ;
                     } ) ;
                  }
                  else if( isObject( groupInfo['Node'] ) )
                  {
                     var nodeInfo = groupInfo['Node'] ;
                     nodeList.push( nodeInfo ) ;
                  }
               }
            }
         }
         return nodeList ;
      }

      //导入配置
      function importConfig( config, func )
      {
         var configInfo = {
            'ClusterName':  SdbSwap.clusterName,
            'BusinessName': SdbSwap.moduleName,
            'Config': config
         } ;

         var data = {
            'cmd': 'update business config',
            'ConfigInfo': JSON.stringify( configInfo )
         } ;

         SdbRest.OmOperation( data, {
            'success': function(){
               if ( isFunction( func ) )
               {
                  func() ;
               }
            }, 
            'complete': function(){
               Loading.close() ;
            },
            'failed': function( errorInfo ){
               //部分节点未成功
               if ( errorInfo['errno'] == -264 )
               {
                  var hasError = false ;
                  $.each( errorInfo['ErrNodes'], function( index, node ){
                     //错误码不是 部分配置修改未生效
                     if ( node['Flag'] != -322 )
                     {
                        var tmpError = {
                           'cmd': 'update business config',
                           'errno': node['Flag'],
                           'detail': sprintf( $scope.autoLanguage( '节点错误 ? : ?' ), node['NodeName'], node['ErrInfo']['detail'] )
                        } ;
                        _IndexPublic.createRetryModel( $scope, tmpError, function(){
                           importConfig( config ) ;
                           return true ;
                        } ) ;
                        hasError = true ;
                        return false ;
                     }
                  } ) ;

                  if ( hasError == false )
                  {
                     if ( isFunction( func ) )
                     {
                        func() ;
                     }
                  }
               }
               else if ( errorInfo['errno'] == -322 )
               {
                  if ( isFunction( func ) )
                  {
                     func() ;
                  }
               }
               else
               {
                  _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                     importConfig( config ) ;
                     return true ;
                  } ) ;
               }
            }
         } ) ;
      }

      //导入配置
      function saveConfig()
      {
         var nodeList = parseSaveConfig() ;
         if ( nodeList === false )
         {
            return false ;
         }

         var contrastList = [] ;
         $.each( SdbSwap.Template, function( index, item ){
            if ( item['reloadable'].length > 0 )
            {
               contrastList.push( item['Name'] ) ;
            }
         } ) ;

         var nodeNum = nodeList.length ;

         if ( nodeNum == 0 )
         {
            return true ;
         }

         var task = [] ;

         for( var k = 0; k < nodeNum; ++k )
         {
            var tmp = nodeList[k] ;
            var config = {
               "property": null,
               "options": { "NodeName": [] }
            } ;

            for( var i = k; i < nodeNum; ++i )
            {
               if ( objectEqual( tmp, nodeList[i], contrastList, false ) )
               {
                  var nodeInfo = filterObject( nodeList[i], contrastList, false, false ) ;

                  if ( hasKey( nodeList[i], 'HostName' ) == false ||  hasKey( nodeList[i], 'svcname' ) == false )
                  {
                     alert( $scope.autoLanguage( '配置项必须有HostName和svcname' ) ) ;
                     return false ;
                  }

                  for( var key in nodeInfo )
                  {
                     var tmpValue = nodeInfo[key] ;

                     if( SdbSwap.TemplateDesc[key]['Type'] == 'int' )
                     {
                        tmpValue = parseInt( tmpValue ) ;
                        if ( isNaN( tmpValue ) )
                        {
                           continue ;
                        }
                     }
                     else if( SdbSwap.TemplateDesc[key]['Type'] == 'double' )
                     {
                        tmpValue = parseFloat( tmpValue ) ;
                        if ( isNaN( tmpValue ) )
                        {
                           continue ;
                        }
                     }
                     else if( SdbSwap.TemplateDesc[key]['Type'] == 'bool' )
                     {
                        tmpValue = tmpValue.toUpperCase() ;
                     }

                     nodeInfo[key] = tmpValue ;
                  }

                  config['property'] = nodeInfo ;
                  config['options']['NodeName'].push( nodeList[i]['HostName'] + ':' + nodeList[i]['svcname'] ) ;
                  k = i ;
               }
               else
               {
                  k = i - 1 ;
                  break ;
               }
            }

            task.push( config ) ;
         }

         Loading.create() ;

         var defer = SdbPromise.init( task.length ) ;

         for( var i in task )
         {
            importConfig( task[i], function(){
               defer.resolve( 'count', 1 ) ;
            } ) ;
         }

         defer.then( function(){
            var defer = SdbPromise.init( 3 ) ;

            SdbSwap.getGroupList( function(){
               defer.resolve( 'group', 1 ) ;
            } ) ;

            SdbSwap.getRunConfigs( SdbSwap.Expand, function(){
               defer.resolve( 'run', 1 ) ;
            } ) ;

            SdbSwap.getLocalConfigs( SdbSwap.Expand, function(){
               defer.resolve( 'local', 1 ) ;
            } ) ;

            Loading.close() ;
         } ) ;
         
         return true ;
      }

      //创建 导出配置 弹窗
      $scope.CreateExportConfigModel = function(){
         $scope.BuildConfig() ;
         //设置确定按钮
         $scope.ConfigWindows['callback']['SetOkButton']( $scope.autoLanguage( '保存' ), function(){
            return saveConfig() ;
         } ) ;
         //关闭窗口滚动条
         $scope.ConfigWindows['callback']['DisableBodyScroll']() ;
         //设置标题
         $scope.ConfigWindows['callback']['SetTitle']( $scope.autoLanguage( '编辑配置' ) ) ;
         //设置图标
         $scope.ConfigWindows['callback']['SetIcon']( 'fa-edit' ) ;
         //打开窗口
         $scope.ConfigWindows['callback']['Open']() ;
      }

      //生成对应格式的配置
      $scope.BuildConfig = function(){

         function getGroupName( hostName, svcname )
         {
            var groupName = '' ;
            $.each( SdbSwap.GroupList, function( index1, groupInfo ){
               if ( groupInfo['Role'] == 0 )
               {
                  $.each( groupInfo['Group'], function( index2, nodeInfo ){
                     if ( nodeInfo['HostName'] == hostName &&
                           nodeInfo['Service'][0]['Name'] == svcname )
                     {
                        groupName = groupInfo['GroupName'] ;
                        return false ;
                     }
                  } ) ;
                  if ( groupName.length > 0 )
                  {
                     return false ;
                  }
               }
            } ) ;
            return groupName ;
         }

         $scope.ConfigWindows['config']['text'] = '' ;

         if( $scope.ConfigWindows['config']['type'] == 'csv' )
         {
            var keyList = [] ;
            var filterField = [ "NodeName", "clustername", "businessname", "omaddr", "catalogaddr", "usertag" ] ;

            $.each( SdbSwap.RunConfigs, function( index, config ){
               keyList = SdbFunction.getJsonKeys( config, 0, keyList ) ;
            } ) ;

            var tmpKeyList = [ 'HostName' ] ;
            $.each( keyList, function( index, key ){
               if ( filterField.indexOf( key ) < 0 )
               {
                  tmpKeyList.push( key ) ;
               }
            } ) ;

            $scope.ConfigWindows['config']['text'] += tmpKeyList.toString() + '\r\n' ;

            $.each( SdbSwap.RunConfigs, function( index, nodeInfo ){
               var hostName = nodeInfo['NodeName'].split( ':' )[0] ;
               var newNode = $.extend( { 'HostName': hostName }, nodeInfo ) ;
               newNode['datagroupname'] = getGroupName( hostName, nodeInfo['svcname'] ) ;

               $scope.ConfigWindows['config']['text'] += object2csv( newNode, tmpKeyList ) + '\r\n' ;
            } ) ;
         }
         else
         {
            var newConfig = { 'Deploy': { 'Coord': { 'Node': [] }, 'Catalog': { 'Node': [] }, 'Data': { 'Group': [] } } } ;
            var filterField = [ "NodeName", "role", "datagroupname", "clustername", "businessname", "omaddr", "catalogaddr", "usertag" ] ;
            var tmpGroupList = [] ;
            $.each( SdbSwap.RunConfigs, function( index, nodeInfo ){
               var hostName = nodeInfo['NodeName'].split( ':' )[0] ;
               var role = nodeInfo['role'] ;
               var groupName = getGroupName( hostName, nodeInfo['svcname'] ) ;
               var newNode = $.extend( { 'HostName': hostName }, filterObject( nodeInfo, filterField ) ) ;

               if ( role == 'coord' )
               {
                  newConfig['Deploy']['Coord']['Node'].push( newNode ) ;
               }
               else if ( role == 'catalog' )
               {
                  newConfig['Deploy']['Catalog']['Node'].push( newNode ) ;
               }
               else if ( role == 'data' )
               {
                  var index3 = tmpGroupList.indexOf( groupName ) ;
                  if ( index3 >= 0 )
                  {
                     newConfig['Deploy']['Data']['Group'][index3]['Node'].push( newNode ) ;
                  }
                  else
                  {
                     var newGroup = {
                        'GroupName': groupName,
                        'Node': [ newNode ]
                     } ;
                     newConfig['Deploy']['Data']['Group'].push( newGroup ) ;
                     tmpGroupList.push( groupName ) ;
                  }
               }
            } ) ;

            if( $scope.ConfigWindows['config']['type'] == 'json' )
            {
               $scope.ConfigWindows['config']['text'] = JSON.stringify( newConfig, null, 3 ) ;
            }
            else if( $scope.ConfigWindows['config']['type'] == 'xml' )
            {
               var xotree = new XML.ObjTree();
               $scope.ConfigWindows['config']['text'] = formatXml( xotree.writeXML( newConfig ) ) ;
            }
         }
      }

      //下载配置
      $scope.DownloadConfig = function(){
         var blob = new Blob( [ $scope.ConfigWindows['config']['text'] ], { type: "text/plain;charset=utf-8" } ) ;
         if( $scope.ConfigWindows['config']['type'] == 'json' )
         {
            saveAs( blob, $scope.ModuleName + '.json' ) ;
         }
         else if( $scope.ConfigWindows['config']['type'] == 'xml' )
         {
            saveAs( blob, $scope.ModuleName + '.xml' ) ;
         }
         else if( $scope.ConfigWindows['config']['type'] == 'csv' )
         {
            saveAs( blob, $scope.ModuleName + '.csv' ) ;
         }
      }

      //修改节点表格的过滤
      SdbSignal.on( 'SetNodeTableFilter', function( filter ){
         $scope.NodeTable['callback']['SetFilter']( filter['key'], filter['value'] ) ;
      } ) ;

      SdbSignal.on( 'SetNodeTableGroupSelect', function( groupFilter ){
         groupFilter.unshift( { 'key': $scope.autoLanguage( '全部' ), 'value': '' } ) ;
         $scope.NodeTable['options']['filter']['datagroupname'] = groupFilter ;
      } ) ;

      //检查配置有变化的节点
      SdbSwap.initDefer.then( function( data ){
         if ( SdbSwap.moduleMode != 'standalone' )
         {
            var run = data['run'] ;
            var local = data['local'] ;
            var table = data['table'] ;

            for( var index in run )
            {
               table[index]['status'] = objectEqual( run[index], local[index] ) ? 'unchanged' : 'change' ;
            }
         }
      } ) ;

      //节点列表
      SdbSignal.on( 'SetNodeList', function( data ){
         var tableBody = [] ;

         $.each( data, function( index, groupInfo ){
            var role = 'data' ;

            if ( groupInfo['Role'] == 1 )
            {
               role = 'coord'
            }
            else if ( groupInfo['Role'] == 2 )
            {
               role = 'catalog'
            }

            $.each( groupInfo['Group'], function( index2, nodeInfo ){
               var newNode = {} ;

               newNode['checked'] = false ;
               newNode['status'] = '' ;
               newNode['NodeName'] = nodeInfo['HostName'] + ':' + nodeInfo['Service'][0]['Name'] ;
               newNode['dbpath'] = nodeInfo['dbpath'] ;
               newNode['role'] = role ;
               newNode['datagroupname'] = groupInfo['GroupName'] ;

               tableBody.push( newNode ) ;
            } ) ;
         } ) ;

         $scope.NodeTable['body'] = tableBody.sort( SdbSwap.sortNodeName ) ;
         SdbSwap.initDefer.resolve( 'table', tableBody ) ;

      } ) ;

      //全选
      $scope.SelectAll = function(){
         var dataList ;
         var isFilter = $scope.NodeTable['callback']['GetFilterStatus']() ;
         if( isFilter )
         {
            //如果开了过滤，那么只修改过滤的
            dataList = $scope.NodeTable['callback']['GetFilterAllData']() ;
         }
         else
         {
            dataList = $scope.NodeTable['callback']['GetAllData']() ;
         }
         $.each( dataList, function( index ){
            dataList[index]['checked'] = true ;
         } ) ;
      }

      //反选
      $scope.Unselect = function(){
         var dataList ;
         var isFilter = $scope.NodeTable['callback']['GetFilterStatus']() ;
         if( isFilter )
         {
            //如果开了过滤，那么只修改过滤的
            dataList = $scope.NodeTable['callback']['GetFilterAllData']() ;
         }
         else
         {
            dataList = $scope.NodeTable['callback']['GetAllData']() ;
         }
         $.each( dataList, function( index ){
            dataList[index]['checked'] = !dataList[index]['checked'] ;
         } ) ;
      }

      //批量修改节点
      $scope.BatchNode = function(){
         var tmpList = $scope.NodeTable['callback']['GetFilterAllData']() ;
         var nodeNameList = [] ;
         $.each( tmpList, function( index, nodeInfo ){
            if ( nodeInfo['checked'] == true )
            {
               nodeNameList.push( nodeInfo['NodeName'] ) ;
            }
         } ) ;
         if ( nodeNameList.length == 0 )
         {
            $.each( tmpList, function( index, nodeInfo ){
               nodeNameList.push( nodeInfo['NodeName'] ) ;
            } ) ;
         }
         $scope.SwitchNode( nodeNameList ) ;
      }

   } ) ;

   sacApp.controllerProvider.register( 'Config.SDB.Config.Ctrl', function( $location, $rootScope, $scope, SdbSignal, SdbSwap, SdbFunction, SdbPromise, SdbRest ){

      function DisableKeyClass()
      {
         this.isInit = false ;
         this.list = [ 'nodename', 'hostname', 'confpath' ] ;

         this.init = function( hiddenKey, templateKey )
         {
            if ( this.isInit )
            {
               return ;
            }

            for( var i = 0; i < hiddenKey.length; ++i )
            {
               this.list.push( hiddenKey[i].toLowerCase() ) ;
            }

            for( var i = 0; i < templateKey.length; ++i )
            {
               this.list.push( templateKey[i].toLowerCase() ) ;
            }

            this.isInit = true ;
         }

         this.isDisableKey = function( key )
         {
            return ( this.list.indexOf( key.toLowerCase() ) >= 0 ) ;
         }
      }

      function HidderKeyClass()
      {
         this.list = [ 'businessname', 'clustername', 'usertag' ] ;
         this.get = function()
         {
            return this.list ;
         }
         this.isHiddenKey = function( key )
         {
            return ( this.list.indexOf( key.toLowerCase() ) >= 0 ) ;
         }
      }

      //生成弹窗表单的格式
      function ModifyConfigForm()
      {
         this.useKeyList = [] ;
         this.normal = [] ;
         this.advance = [] ;
         this.custom = [] ;

         this.isSame = function( configs, key )
         {
            var result = true ;
            var tmp = configs[0][key] ;
            $.each( configs, function( index, item ){
               if ( item[key] != tmp )
               {
                  result = false ;
                  return false ;
               }
            } ) ;
            return result ;
         }

         this.init = function( template )
         {
            this.normal = configConvertTemplate( template, 0 ) ;
            this.advance = configConvertTemplate( template, 1 ) ;
            var template = [
               {
                  "name": "name",
                  "webName": $scope.autoLanguage( "参数名" ), 
                  "placeholder": $scope.autoLanguage( "参数名" ),
                  "type": "string",
                  "valid": {
                     "min": 1
                  },
                  "default": "",
                  "value": ""
               },
               {
                  "name": "value",
                  "webName": $scope.autoLanguage( "值" ), 
                  "placeholder": $scope.autoLanguage( "值" ),
                  "type": "string",
                  "valid": {},
                  "default": "",
                  "value": ""
               }
            ] ;
            this.custom = [
               {
                  "name": "other",
                  "webName": $scope.autoLanguage( "自定义配置" ),
                  "type": "list",
                  "listType": 2,
                  "valid": {
                     "min": 0
                  },
                  "template": $.extend( true, [], template ),
                  "child": [ template ]
               }
            ] ;
         }

         this.loadNormal = function( nodeNum, configs )
         {
            if ( nodeNum == 1 )
            {
               for( var index = 0; index < this.normal.length; ++index )
               {
                  var item = this.normal[index] ;
                  var key = item['name'] ;

                  this.useKeyList.push( key ) ;

                  var tmpVal = configs[key] ;

                  if ( tmpVal == 'TRUE' || tmpVal == 'FALSE' )
                  {
                     tmpVal = tmpVal.toLowerCase() ;
                  }

                  this.normal[index]['value'] = convertValueString( tmpVal ) ;
               }
            }
            else
            {
               for( var index1 = 0; index1 < this.normal.length; ++index1 )
               {
                  var item = this.normal[index1] ;
                  var key = item['name'] ;

                  this.useKeyList.push( key ) ;

                  var tmpVal = '' ;
                  if ( this.isSame( configs, key ) )
                  {
                     tmpVal = configs[0][key] ;
                  }

                  if ( tmpVal == 'TRUE' || tmpVal == 'FALSE' )
                  {
                     tmpVal = tmpVal.toLowerCase() ;
                  }

                  this.normal[index1]['value'] = convertValueString( tmpVal ) ;
               }
            }
         }

         this.loadAdvance = function( nodeNum, configs )
         {
            if ( nodeNum == 1 )
            {
               for( var index = 0; index < this.advance.length; ++index )
               {
                  var item = this.advance[index] ;
                  var key = item['name'] ;

                  this.useKeyList.push( key ) ;

                  var tmpVal = configs[key] ;

                  if ( tmpVal == 'TRUE' || tmpVal == 'FALSE' )
                  {
                     tmpVal = tmpVal.toLowerCase() ;
                  }

                  this.advance[index]['value'] = convertValueString( tmpVal ) ;
               }
            }
            else
            {
               for( var index1 = 0; index1 < this.advance.length; ++index1 )
               {
                  var item = this.advance[index1] ;
                  var key = item['name'] ;

                  this.useKeyList.push( key ) ;

                  var tmpVal = '' ;
                  if ( this.isSame( configs, key ) )
                  {
                     tmpVal = configs[0][key] ;
                  }

                  if ( tmpVal == 'TRUE' || tmpVal == 'FALSE' )
                  {
                     tmpVal = tmpVal.toLowerCase() ;
                  }

                  this.advance[index1]['value'] = convertValueString( tmpVal ) ;
               }
            }
         }

         this.getHiddenTemplate = function( key )
         {
            for( var i in SdbSwap.HiddenTemplate )
            {
               var template = SdbSwap.HiddenTemplate[i] ;
               if ( template['Name'] == key )
               {
                  return configConvertTemplateByOne( template ) ;
               }
            }
            return { 'type': 'string', 'valid': '' } ;
         }

         this.loadCustom = function( nodeNum, configs, disableKeyObj )
         {
            var isFirst = true ;

            if ( nodeNum == 1 )
            {
               for( var key in configs )
               {
                  var value = configs[key] ;

                  if ( this.useKeyList.indexOf( key ) >= 0 || disableKeyObj.isDisableKey( key ) )
                  {
                     continue ;
                  }

                  if ( value == 'TRUE' || value == 'FALSE' )
                  {
                     value = value.toLowerCase() ;
                  }

                  this.useKeyList.push( key ) ;

                  var hiddenTemplate = this.getHiddenTemplate( key ) ;

                  if( isFirst )
                  {
                     this.custom[0]['child'][0][0]['value']    = key ;
                     this.custom[0]['child'][0][0]['disabled'] = true ;
                     this.custom[0]['child'][0][1]['value']    = value ;
                     this.custom[0]['child'][0][1]['type']     = hiddenTemplate['type'] ;
                     this.custom[0]['child'][0][1]['valid']    = hiddenTemplate['valid'] ;

                     isFirst = false ;
                  }
                  else
                  {
                     var newInput = $.extend( true, [], this.custom[0]['child'][0] ) ;

                     newInput[0]['value']    = key ;
                     newInput[0]['disabled'] = true ;
                     newInput[1]['value']    = value ;
                     newInput[1]['type']     = hiddenTemplate['type'] ;
                     newInput[1]['valid']    = hiddenTemplate['valid'] ;

                     this.custom[0]['child'].push( newInput ) ;
                  }
               }
            }
            else
            {
               for ( var index in configs )
               {
                  var config = configs[index] ;

                  for( var key in config )
                  {
                     if ( this.useKeyList.indexOf( key ) >= 0 || disableKeyObj.isDisableKey( key ) )
                     {
                        continue ;
                     }

                     if ( !this.isSame( configs, key ) )
                     {
                        continue ;
                     }

                     this.useKeyList.push( key ) ;

                     var value = configs[0][key] ;
                     if ( value == 'TRUE' || value == 'FALSE' )
                     {
                        value = value.toLowerCase() ;
                     }

                     var hiddenTemplate = this.getHiddenTemplate( key ) ;

                     if( isFirst )
                     {
                        this.custom[0]['child'][0][0]['value']    = key ;
                        this.custom[0]['child'][0][0]['disabled'] = true ;
                        this.custom[0]['child'][0][1]['value']    = value ;
                        this.custom[0]['child'][0][1]['type']     = hiddenTemplate['type'] ;
                        this.custom[0]['child'][0][1]['valid']    = hiddenTemplate['valid'] ;

                        isFirst = false ;
                     }
                     else
                     {
                        var newInput = $.extend( true, [], this.custom[0]['child'][0] ) ;

                        newInput[0]['value']    = key ;
                        newInput[0]['disabled'] = true ;
                        newInput[1]['value']    = value ;
                        newInput[1]['type']     = hiddenTemplate['type'] ;
                        newInput[1]['valid']    = hiddenTemplate['valid'] ;

                        this.custom[0]['child'].push( newInput ) ;
                     }
                  }
               }
            }
         }

         this.loadConfig = function( nodeNum, configs, disableKeyObj )
         {
            this.useKeyList = [ 'catalogaddr', 'omaddr' ] ;

            this.loadNormal( nodeNum, configs ) ;

            this.loadAdvance( nodeNum, configs ) ;

            this.loadCustom( nodeNum, configs, disableKeyObj ) ;
         }

         this.getNormal = function()
         {
            return this.normal ;
         }

         this.getAdvance = function()
         {
            return this.advance ;
         }

         this.getCustom = function()
         {
            return this.custom ;
         }
      }

      //把弹窗的表单生成rest请求格式
      function ModifyConfigRequest()
      {
         //要修改的配置项
         this.updateConfig = {
            'property': {},
            'options': { 'NodeName': [] }
         } ;

         //要删除的配置项
         this.deleteConfig = {
            'property': {},
            'options': { 'NodeName': [] }
         } ;

         this.loadNodes = function( nodeNum, nodeConfigs )
         {
            if ( nodeNum == 1 )
            {
               this.updateConfig['options']['NodeName'].push( nodeConfigs['NodeName'] ) ;
               this.deleteConfig['options']['NodeName'].push( nodeConfigs['NodeName'] ) ;
            }
            else
            {
               for( var index in nodeConfigs )
               {
                  this.updateConfig['options']['NodeName'].push( nodeConfigs[index]['NodeName'] ) ;
                  this.deleteConfig['options']['NodeName'].push( nodeConfigs[index]['NodeName'] ) ;
               }
            }
         }

         this.convertCustom = function( configs )
         {
            var result = {} ;

            for( var index in configs['other'] )
            {
               var key = configs['other'][index]['name'] ;
               var val = configs['other'][index]['value'] ;

               if ( key.length == 0 )
               {
                  continue ;
               }

               result[key] = val ;
            }

            return result ;
         }

         this.setUpdateItem = function( nodeNum, nodeConfigs, key, value )
         {
            if ( nodeNum == 1 )
            {
               //只要有一个值不一样，就修改
               if ( !isSameValueByStrBool( nodeConfigs[key], value ) )
               {
                  this.updateConfig['property'][key] = value ;
               }
            }
            else
            {
               for( var index in nodeConfigs )
               {
                  if ( !isSameValueByStrBool( nodeConfigs[index][key], value ) )
                  {
                     //只要有一个值不一样，就修改
                     this.updateConfig['property'][key] = value ;
                     break ;
                  }
               }
            }
         }

         this.loadUpdateConfig = function( nodeNum, nodeConfigs, template, config )
         {
            for( var key in config )
            {
               var tmpValue = config[key] ;
               var tmpType = typeof( tmpValue ) ;

               if ( ( tmpType == 'string' && tmpValue.length == 0 ) || 
                    ( tmpType == 'number' && isNaN( tmpValue ) ) )
               {
                  //没有填值
                  continue ;
               }

               if ( template === null )
               {
                  tmpValue = autoTypeConvert( tmpValue ) ;
               }
               else
               {
                  //根据模板做类型转换
                  if ( template[key] )
                  {
                     if( template[key]['Type'] == 'int' )
                     {
                        tmpValue = parseInt( tmpValue ) ;
                        if ( isNaN( tmpValue ) )
                        {
                           continue ;
                        }
                     }
                     else if( template[key]['Type'] == 'double' )
                     {
                        tmpValue = parseFloat( tmpValue ) ;
                        if ( isNaN( tmpValue ) )
                        {
                           continue ;
                        }
                     }
                     else if( template[key]['Type'] == 'bool' )
                     {
                        tmpValue = tmpValue.toUpperCase() ;
                     }
                  }
               }

               this.setUpdateItem( nodeNum, nodeConfigs, key, tmpValue ) ;
            }
         }

         this.setDeleteItem = function( nodeNum, nodeConfigs, key )
         {
            if ( nodeNum == 1 )
            {
               if ( nodeConfigs.hasOwnProperty( key ) )
               {
                  //看看节点原来有没有这个值
                  this.deleteConfig['property'][key] = 1 ;
               }
            }
            else
            {
               if ( nodeConfigs[0].hasOwnProperty( key ) == false )
               {
                  //节点没有这配置项, 说明值肯定不一致
                  return ;
               }

               var isSame = true ;
               var value = nodeConfigs[0][key] ;

               for( var index in nodeConfigs )
               {
                  if ( nodeConfigs[index][key] !== value )
                  {
                     //值不一致
                     isSame = false ;
                     break ;
                  }
               }

               if ( isSame == true )
               {
                  //值一致，并且输入值是空，说明要删除
                  this.deleteConfig['property'][key] = 1 ;
               }
            }
         }

         this.loadDeleteConfigs = function( nodeNum, nodeConfigs, config )
         {
            for( var key in config )
            {
               var value = config[key] ;
               var type = typeof( value ) ;

               if ( ( type == 'string' && value.length == 0 ) ||
                    ( type == 'number' && isNaN( value ) ) )
               {
                  //值是空的
                  this.setDeleteItem( nodeNum, nodeConfigs, key ) ;
               }
            }
         }

         this.parse = function( nodeNum, nodeConfigs, template, config1, config2, config3 )
         {
            this.updateConfig = {
               'property': {},
               'options': { 'NodeName': [] }
            } ; ;
            this.deleteConfig = {
               'property': {},
               'options': { 'NodeName': [] }
            } ;

            this.loadNodes( nodeNum, nodeConfigs ) ;

            this.loadUpdateConfig( nodeNum, nodeConfigs, template, config1 ) ;
            this.loadUpdateConfig( nodeNum, nodeConfigs, template, config2 ) ;
            this.loadUpdateConfig( nodeNum, nodeConfigs, template, this.convertCustom( config3 ) ) ;

            this.loadDeleteConfigs( nodeNum, nodeConfigs, config1 ) ;
            this.loadDeleteConfigs( nodeNum, nodeConfigs, config2 ) ;
            this.loadDeleteConfigs( nodeNum, nodeConfigs, this.convertCustom( config3 ) ) ;
         }

         this.getUpdateConfig = function()
         {
            if( getObjectSize( this.updateConfig['property'] ) > 0 )
            {
               return this.updateConfig ;
            }
            else
            {
               return null ;
            }
         }

         this.getDeleteConfig = function()
         {
            if( getObjectSize( this.deleteConfig['property'] ) > 0 )
            {
               return this.deleteConfig ;
            }
            else
            {
               return null ;
            }
         }
      }

      //支持修改的配置项列表
      var dynamicConfig = [] ;

      //查看的节点数量
      $scope.NodeNum = 0 ;

      //是否完整配置
      $scope.Expand = false ;

      //当前查看的节点
      $scope.NodeName = [] ;

      //配置模板信息
      $scope.TemplateDesc = null ;

      //运行的配置
      var runConfigs = {} ;

      //本地的配置
      var localConfigs = {} ;

      function requestModifyConfig( cmd, config, func )
      {
         if ( config == null )
         {
            if ( isFunction( func ) )
            {
               func( false ) ;
            }
            return ;
         }

         var configInfo = {
            'ClusterName': SdbSwap.clusterName,
            'BusinessName': $scope.ModuleName,
            'Config': config
         } ;

         var data = {
            'cmd': cmd,
            'ConfigInfo': JSON.stringify( configInfo )
         } ;

         SdbRest.OmOperation( data, {
            'success': function(){
               if ( isFunction( func ) )
               {
                  func( false ) ;
               }
            }, 
            'failed': function( errorInfo ){
               //部分节点未成功
               if ( errorInfo['errno'] == -264 )
               {
                  var hasError = false ;
                  $.each( errorInfo['ErrNodes'], function( index, node ){
                     //错误码不是 部分配置修改未生效
                     if ( node['Flag'] != -322 )
                     {
                        var tmpError = {
                           'cmd': cmd,
                           'errno': node['Flag'],
                           'detail': ''
                        } ;

                        if ( hasKey( node['ErrInfo'], 'detail' ) )
                        {
                           tmpError['detail'] = sprintf( $scope.autoLanguage( '节点错误 ? : ?' ), node['NodeName'], node['ErrInfo']['detail'] ) ;
                        }
                        else
                        {
                           tmpError['detail'] = sprintf( $scope.autoLanguage( '节点错误 ? : ?' ), node['NodeName'], node['Flag'] ) ;
                        }

                        _IndexPublic.createRetryModel( $scope, tmpError, function(){
                           requestModifyConfig( cmd, config, func ) ;
                           return true ;
                        } ) ;
                        hasError = true ;
                        return false ;
                     }
                  } ) ;

                  if ( hasError == false )
                  {
                     if ( isFunction( func ) )
                     {
                        func( true ) ;
                     }
                  }
               }
               else if ( errorInfo['errno'] == -322 )
               {
                  if ( isFunction( func ) )
                  {
                     func( true ) ;
                  }
               }
               else
               {
                  _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                     requestModifyConfig( cmd, config, func ) ;
                     return true ;
                  } ) ;
               }
            }
         } ) ;
      }

      //修改配置弹窗
      $scope.ModifyConfigWindow = {
         'config': {
            'ShowType': 1,
            'inputErrNum1': 0,
            'inputErrNum2': 0,
            'inputErrNum3': 0,
            'form1': { 'keyWidth': '160px', 'inputList': [] },
            'form2': { 'keyWidth': '160px', 'inputList': [] },
            'form3': { 'keyWidth': '160px', 'inputList': [] }
         },
         'callback': {}
      } ;

      //打开修改配置弹窗
      $scope.OpenModifyConfigWindow = function(){
         //不显示的字段
         var hiddenKeyObj = new HidderKeyClass() ;
         //禁用的自定义字段
         var disableKeyObj = new DisableKeyClass() ;

         disableKeyObj.init( hiddenKeyObj.get(), SdbSwap.TemplateIndex ) ;

         $scope.ModifyConfigWindow['config']['ShowType'] = 1 ;

         if ( dynamicConfig.length == 0 )
         {
            $.each( SdbSwap.Template, function( index, item ){
               if ( item['reloadable'].length > 0 )
               {
                  dynamicConfig.push( item ) ;
               }
            } ) ;
         }

         //加载值
         var modifyFormObj = new ModifyConfigForm() ;
         modifyFormObj.init( dynamicConfig ) ;
         modifyFormObj.loadConfig( $scope.NodeNum, localConfigs, disableKeyObj ) ;

         $scope.ModifyConfigWindow['config']['form1']['inputList'] = modifyFormObj.getNormal() ;
         $scope.ModifyConfigWindow['config']['form2']['inputList'] = modifyFormObj.getAdvance() ;
         $scope.ModifyConfigWindow['config']['form3']['inputList'] = modifyFormObj.getCustom() ;

         $scope.ModifyConfigWindow['callback']['SetTitle']( $scope.autoLanguage( '修改配置项' ) ) ;
         $scope.ModifyConfigWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var inputErrNum1 = $scope.ModifyConfigWindow['config']['form1'].getErrNum() ;
            var inputErrNum2 = $scope.ModifyConfigWindow['config']['form2'].getErrNum() ;
            var inputErrNum3 = $scope.ModifyConfigWindow['config']['form3'].getErrNum( function( valueList ){
               var error = [] ;
               //检查禁用字段
               $.each( valueList['other'], function( index, configInfo ){
                  if( disableKeyObj.isDisableKey( configInfo['name'] ) )
                  {
                     error.push( { 'name': 'other', 'error': sprintf( $scope.autoLanguage( '自定义配置不能设置?。' ), configInfo['name'] ) } ) ;
                  }
               } ) ;
               return error ;
            } ) ;
            $scope.ModifyConfigWindow['config']['inputErrNum1'] = inputErrNum1 ;
            $scope.ModifyConfigWindow['config']['inputErrNum2'] = inputErrNum2 ;
            $scope.ModifyConfigWindow['config']['inputErrNum3'] = inputErrNum3 ;
            if( inputErrNum1 == 0 && inputErrNum2 == 0 && inputErrNum3 == 0 )
            {
               var configs1 = $scope.ModifyConfigWindow['config']['form1'].getValue() ;
               var configs2 = $scope.ModifyConfigWindow['config']['form2'].getValue() ;
               var configs3 = $scope.ModifyConfigWindow['config']['form3'].getValue() ;

               var modifyConfReq = new ModifyConfigRequest() ;

               modifyConfReq.parse( $scope.NodeNum, localConfigs, $scope.TemplateDesc, configs1, configs2, configs3 ) ;

               requestModifyConfig( 'update business config', modifyConfReq.getUpdateConfig(), function( needTips1 ){
                  requestModifyConfig( 'delete business config', modifyConfReq.getDeleteConfig(), function( needTips2 ){
                     $scope.ReloadConfig() ;
                     if ( $scope.NodeNum > 1 && ( needTips1 || needTips2 ) )
                     {
                        $scope.Components.Confirm.type = 2 ;
                        $scope.Components.Confirm.context = $scope.autoLanguage( '红色标注的节点配置需要重启生效。' ) ;
                        $scope.Components.Confirm.isShow = true ;
                        $scope.Components.Confirm.noClose = true ;
                        $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
                        $scope.Components.Confirm.ok = function(){
                           $scope.Components.Confirm.isShow = false ;
                           $scope.Components.Confirm.noClose = false ;
                        }
                     }
                  } ) ;
               } ) ;
            }
            else
            {
               if ( inputErrNum1 > 0 )
               {
                  $scope.ModifyConfigWindow['config']['ShowType'] = 1 ;
                  $scope.ModifyConfigWindow['config']['form1'].scrollToError( null ) ;
               }
               else if ( inputErrNum2 > 0 )
               {
                  $scope.ModifyConfigWindow['config']['ShowType'] = 2 ;
                  $scope.ModifyConfigWindow['config']['form2'].scrollToError( null ) ;
               }
               else if ( inputErrNum3 > 0 )
               {
                  $scope.ModifyConfigWindow['config']['ShowType'] = 3 ;
                  $scope.ModifyConfigWindow['config']['form3'].scrollToError( null ) ;
               }
            }
            return inputErrNum1 == 0 && inputErrNum2 == 0 && inputErrNum3 == 0 ;
         } ) ;
         $scope.ModifyConfigWindow['callback']['Open']() ;
      }

      //单节点配置表
      $scope.OneNodeTable = {
         'table': {
            'width': {
               'name':     '200px',
               'webName':  '200px'
            },
            'tools': false,
            'max': 10000
         },
         'title': {},
         'title1': {
            'name':     $scope.autoLanguage( '配置项' ),
            'webName':  $scope.autoLanguage( '项名' ),
            'value':    $scope.autoLanguage( '值' ),
            'desc':     $scope.autoLanguage( '描述' )
         },
         'title2': {
            'name':        $scope.autoLanguage( '配置项' ),
            'webName':     $scope.autoLanguage( '项名' ),
            'value1':      $scope.autoLanguage( '值' ) + '(Run)',
            'value2':      $scope.autoLanguage( '值' ) + '(Local)',
            'reloadable':  $scope.autoLanguage( '生效类型' ),
            'desc':        $scope.autoLanguage( '描述' )
         },
         'content': [],
         'callback': {}
      } ;

      //批量节点配置表
      $scope.MultiNodeTable = {
         'table': {
            'tools': false,
            'max': 10000
         },
         'title': {},
         'content': [],
         'callback': {}
      } ;

      //显示列 下拉菜单
      $scope.FieldDropdown = {
         'config': [],
         'callback': {}
      } ;

      //节点表格 Resize
      SdbSignal.on( 'ResizeConfigTable', function(){
         var nodeNum = 1 ;
         if ( isArray( $scope.NodeName ) )
         {
            nodeNum = $scope.NodeName.length ;
         }

         if( nodeNum == 1 && $scope.OneNodeTable['callback']['Resize'] )
         {
            $scope.OneNodeTable['callback']['Resize']() ;
            $scope.OneNodeTable['callback']['ResizeTableHeader']() ;
         }
         else if( nodeNum > 1 && $scope.MultiNodeTable['callback']['Resize'] )
         {
            $scope.MultiNodeTable['callback']['Resize']() ;
            $scope.MultiNodeTable['callback']['ResizeTableHeader']() ;
         }
      } ) ;

      //打开 显示列 的下拉菜单
      $scope.OpenShowFieldDropdown = function( event ){
         $.each( $scope.FieldDropdown['config'], function( index, fieldInfo ){
            $scope.FieldDropdown['config'][index]['isShow'] = typeof( $scope.MultiNodeTable['title'][fieldInfo['key']] ) == 'string' ;
         } ) ;
         $scope.FieldDropdown['callback']['Open']( event.currentTarget ) ;
      }

      //保存 显示列
      $scope.SaveField = function(){
         $.each( $scope.FieldDropdown['config'], function( index, fieldInfo ){
            $scope.MultiNodeTable['title'][fieldInfo['key']] = fieldInfo['isShow'] ? fieldInfo['value'] : false ;
         } ) ;
         $scope.MultiNodeTable['callback']['ShowCurrentPage']() ;
      }

      SdbSignal.on( 'ShowConfig', function( nodeName ){
         var matchNum = 0 ;
         var nodeNum = 1 ;

         if ( $scope.TemplateDesc === null )
         {
            $scope.TemplateDesc = {
               'InstallPath': { 'WebName': $scope.autoLanguage( '安装路径' ) },
               'NodeName':    { 'WebName': $scope.autoLanguage( '节点名' ) }
            } ;

            $scope.TemplateDesc = $.extend( $scope.TemplateDesc, SdbSwap.TemplateDesc ) ;
         }

         if ( isArray( nodeName ) )
         {
            nodeNum = nodeName.length ;
         }

         $scope.NodeName = nodeName ;
         
         if( nodeNum == 1 )
         {
            runConfigs = null ;
            localConfigs = null ;
            $scope.MultiNodeTable['callback'] = {} ;
            clearArray( $scope.OneNodeTable['content'] ) ;

            $.each( SdbSwap.RunConfigs, function( index, config ){
               if( nodeName.indexOf( config['NodeName'] ) >= 0 )
               {
                  runConfigs = config ;
                  localConfigs = SdbSwap.LocalConfigs[index] ;
                  return false ;
               }
            } ) ;

            //节点没了
            if ( runConfigs == null )
            {
               SdbSignal.commit( 'SetTitle', '' ) ;
               $scope.SwitchTab( 'Node' ) ;
               return ;
            }

            var diffs = diffObject( runConfigs, localConfigs ) ;

            if ( diffs.length == 0 )
            {
               $scope.OneNodeTable['title'] = $scope.OneNodeTable['title1'] ;

               $.each( runConfigs, function( key, value ) {
                  $scope.OneNodeTable['content'].push( { 'name': key, 'webName': '', 'value': value, 'desc': '', 'diff': false } ) ;
               } ) ;

               $scope.IsConfigsSame = true ;
            }
            else
            {
               $scope.OneNodeTable['title'] = $scope.OneNodeTable['title2'] ;

               $.each( runConfigs, function( key ) {
                  var isSame = ( runConfigs[key] === localConfigs[key] ) ;
                  $scope.OneNodeTable['content'].push( {
                     'name': key,
                     'webName': '',
                     'value1': runConfigs[key],
                     'value2': isSame ? '' : localConfigs[key],
                     'reloadable': '',
                     'desc': '',
                     'diff': !isSame
                  } ) ;
               } ) ;

               $scope.IsConfigsSame = false ;
            }

         }
         else
         {
            runConfigs = [] ;
            localConfigs = [] ;
            var keyList = [] ;
            var priorityList = [ 'NodeName', 'role', 'dbpath', 'weight', 'transactionon', 'transactiontimeout', 'transisolation', 'auditmask',
               'logfilesz', 'logfilenum', 'preferedinstance', 'instanceid', 'planbuckets', 'plancachelevel', 'maxpool', 'maxcachesize', 'directioinlob',
               'syncdeep', 'syncinterval', 'syncrecordnum' ] ;

            clearObject( $scope.MultiNodeTable['title'] ) ;
            clearArray( $scope.MultiNodeTable['content'] ) ;
            clearArray( $scope.FieldDropdown['config'] ) ;

            $.each( SdbSwap.RunConfigs, function( index, config ){
               if( nodeName.indexOf( config['NodeName'] ) >= 0 )
               {
                  keyList = SdbFunction.getJsonKeys( config, 0, keyList ) ;
                  runConfigs.push( config ) ;
               }
            } ) ;

            $.each( SdbSwap.LocalConfigs, function( index, config ){
               if( nodeName.indexOf( config['NodeName'] ) >= 0 )
               {
                  keyList = SdbFunction.getJsonKeys( config, 0, keyList ) ;
                  localConfigs.push( config ) ;
                  $scope.MultiNodeTable['content'].push( config ) ;
               }
            } ) ;

            $scope.DiffNodesConfigs = [] ;
            var hasDiff = false ;
            for( var i = 0; i < localConfigs.length; ++i )
            {
               if ( runConfigs[i]['NodeName'] == localConfigs[i]['NodeName'] )
               {
                  var diff = diffObject( runConfigs[i], localConfigs[i] ) ;
                  $scope.DiffNodesConfigs.push( diff ) ;
                  if( diff.length > 0 )
                  {
                     hasDiff = true ;
                  }
               }
               else
               {
                  $scope.DiffNodesConfigs.push( [] ) ;
               }
            }

            keyList.sort() ;

            var hiddenKeyObj = new HidderKeyClass() ;

            var maxShow = 8 ;
            var showNum = 0 ;
            $.each( keyList, function( index, key ){

               if ( hiddenKeyObj.isHiddenKey( key ) )
               {
                  return true ;
               }

               //优先显示指定字段
               var isShow = ( priorityList.indexOf( key ) >= 0 ) ;

               if ( isShow && showNum < maxShow )
               {
                  ++showNum ;
               }
               else
               {
                  isShow = false ;
               }

               $scope.MultiNodeTable['title'][key] = isShow ? key : isShow ;

               $scope.FieldDropdown['config'].push( { 'key': key, 'value': key, 'isShow': isShow } ) ;

            } ) ;

            //显示字段不足maxShow个，补充
            for( var i = 0; i < $scope.FieldDropdown['config'].length; ++i )
            {
               if ( showNum >= maxShow )
               {
                  break ;
               }
               if ( $scope.FieldDropdown['config'][i]['isShow'] == false )
               {
                  var key = $scope.FieldDropdown['config'][i]['key'] ;
                  $scope.FieldDropdown['config'][i]['isShow'] = true ;
                  $scope.MultiNodeTable['title'][key] = $scope.FieldDropdown['config'][i]['value'] ;
                  ++showNum ;
               }
            }

            if ( $scope.MultiNodeTable['callback']['ShowCurrentPage'] )
            {
               $scope.MultiNodeTable['callback']['ShowCurrentPage']() ;
            }
         }

         $scope.NodeNum = nodeNum ;

      } ) ;

      //刷新
      $scope.ReloadConfig = function() {
         var defer = SdbPromise.init( 3 ) ;

         SdbSwap.getGroupList( function(){
            defer.resolve( 'group', 1 ) ;
         } ) ;

         SdbSwap.getRunConfigs( SdbSwap.Expand, function(){
            defer.resolve( 'run', 1 ) ;
         } ) ;

         SdbSwap.getLocalConfigs( SdbSwap.Expand, function(){
            defer.resolve( 'local', 1 ) ;
         } ) ;

         defer.then( function(){
            SdbSignal.commit( 'ShowConfig', $scope.NodeName ) ;
         } ) ;
      }

      //查看详细配置
      $scope.SwitchExpandConfigs = function() {
         SdbSwap.Expand = !SdbSwap.Expand ;
         $scope.Expand = SdbSwap.Expand ;

         $scope.ReloadConfig() ;
      }
      
      function restartNode( nodes )
      {
         var data = {
            'cmd': 'restart business',
            'ClusterName': SdbSwap.clusterName,
            'BusinessName': SdbSwap.moduleName,
            'Options': JSON.stringify( { 'Nodes': nodes } )
         } ;

         SdbRest.OmOperation( data, {
            'success': function( taskInfo ){
               $rootScope.tempData( 'Deploy', 'Model', 'Update Config' ) ;
               $rootScope.tempData( 'Deploy', 'Module', 'sequoiadb' ) ;
               $rootScope.tempData( 'Deploy', 'ModuleTaskID', taskInfo[0]['TaskID'] ) ;
               $location.path( '/Deploy/Task/Restart' ).search( { 'r': new Date().getTime() } ) ;
            }, 
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  restartNode( nodes ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //重启节点
      $scope.RestartNode = function(){
         var nodes = [] ;
         if ( isArray( $scope.NodeName ) )
         {
            $.each( $scope.NodeName, function( index, nodename ){
               var tmp = nodename.split( ':' ) ;
               nodes.push( { 'HostName': tmp[0], 'svcname': tmp[1] } ) ;
            } ) ;
         }
         else
         {
            var tmp = $scope.NodeName.split( ':' ) ;
            nodes.push( { 'HostName': tmp[0], 'svcname': tmp[1] } ) ;
         }

         $scope.Components.Confirm.type = 2 ;
         $scope.Components.Confirm.context = $scope.autoLanguage( '该操作将重启节点，是否确定继续？' ) ;
         $scope.Components.Confirm.isShow = true ;
         $scope.Components.Confirm.okText = $scope.autoLanguage( '确定' ) ;
         $scope.Components.Confirm.ok = function(){
            restartNode( nodes ) ;
            $scope.Components.Confirm.isShow = false ;
         }
      }

   } ) ;

}());