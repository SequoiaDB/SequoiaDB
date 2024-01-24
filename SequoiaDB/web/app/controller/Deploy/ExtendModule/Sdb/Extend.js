//@ sourceURL=Extend.js
//"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Deploy.Sdb.Extend.Ctrl', function( $scope, $location, $rootScope, SdbRest, SdbFunction, SdbPromise, SdbSwap ){
      SdbSwap.groupDefer    = SdbPromise.init( 3 ) ;
      SdbSwap.nodeDefer     = SdbPromise.init( 3 ) ;
      SdbSwap.templateDefer = SdbPromise.init( 1 ) ;

      var clusterName     = $rootScope.tempData( 'Deploy', 'ClusterName' ) ;
      var deployMod       = $rootScope.tempData( 'Deploy', 'DeployMod' ) ;
      var extendConfigure = $rootScope.tempData( 'Deploy', 'ModuleConfig' ) ;
      $scope.ModuleName   = $rootScope.tempData( 'Deploy', 'ModuleName' ) ;
      if( clusterName == null || $scope.ModuleName == null || deployMod !== 'distribution' || extendConfigure == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return;
      }

      SdbSwap.groupDefer.resolve( 'HostList', extendConfigure['HostInfo'] ) ;

      //初始化
      var extendNodeList = [] ;
      var template = [] ;
      //帮助信息 弹窗
      $scope.HelperWindow = {
         'config': {},
         'callback': {}
      } ;

      $scope.stepList = _Deploy.BuildSdbExtStep( $scope, $location, $scope['Url']['Action'], 'sequoiadb' ) ;
      if( $scope.stepList['info'].length == 0 )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }
      
      //获取扩容配置
      var getExtendConfig = function(){
         var data = { 'cmd': 'get business config', 'TemplateInfo': JSON.stringify( extendConfigure ), 'OperationType': 'extend' } ;
         SdbRest.OmOperation( data, {
            'success': function( result ){
               extendNodeList = result[0]['Config'] ;
               SdbSwap.groupDefer.resolve( 'ExtendNodeList', extendNodeList ) ;
               SdbSwap.nodeDefer.resolve( 'ExtendNodeList', extendNodeList ) ;
               
               template = [] ;
               $.each( result[0]['Property'], function( index, info ){
                  if ( info['hidden'] == 'true' )
                  {
                     return true ;
                  }
                  template.push( info ) ;
               } ) ;

               SdbSwap.templateDefer.resolve( 'Template', template ) ;
               
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getExtendConfig() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //获取业务配置
      var getModuleConfig = function(){
         var data = { 'cmd': 'query node configure', 'filter': JSON.stringify( { 'ClusterName': clusterName, 'BusinessName': $scope.ModuleName } ) } ;
         SdbRest.OmOperation( data, {
            'success': function( result ){
               var moduleConfig = [] ;
               $.each( result, function( index, hostInfo ){
                  $.each( hostInfo['Config'], function( index2, nodeInfo ){
                     nodeInfo['HostName'] = hostInfo['HostName'] ;
                     moduleConfig.push( nodeInfo ) ;
                  } ) ;
               } ) ;
               SdbSwap.groupDefer.resolve( 'NodeList', moduleConfig ) ;
               SdbSwap.nodeDefer.resolve( 'NodeList', moduleConfig ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getExtendConfig() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      getExtendConfig() ;
      getModuleConfig() ;

      //跳转到上一步
      $scope.GotoPrev = function(){
         $location.path( '/Deploy/SDB-ExtendConf' ).search( { 'r': new Date().getTime() } ) ;
      }

      var extendSdb = function( config ){
         var data = { 'cmd': 'extend business', 'Force': true, 'ConfigInfo': JSON.stringify( config ) } ;
         SdbRest.OmOperation( data, {
            'success': function( taskInfo ){
               $rootScope.tempData( 'Deploy', 'ModuleTaskID', taskInfo[0]['TaskID'] ) ;
               $rootScope.tempData( 'Deploy', 'Module', extendConfigure['BusinessType'] ) ;
               $location.path( '/Deploy/SDB-ExtendInstall' ).search( { 'r': new Date().getTime() } ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  extendSdb( config ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      var isFilterDefaultItem = function( key, value ) {
         var result = false ;
         var unSkipList = [ 'HostName', 'dbpath', 'role', 'svcname', 'datagroupname' ] ;
         if ( unSkipList.indexOf( key ) >= 0 )
         {
            return false ;
         }
         $.each( template, function( index, item ){
            if ( item['Name'] == key )
            {
               result = ( item['Default'] == value ) ;
               return false ;
            }
         } ) ;
         return result ;
      }

      //跳转到下一步
      $scope.GotoExtend = function(){
         var oldExtendNodeList = extendNodeList ;
         if( oldExtendNodeList.length == 0 )
         {
            $scope.Components.Confirm.type = 3 ;
            $scope.Components.Confirm.context = $scope.autoLanguage( '没有扩容节点，请修改扩容配置。' ) ;
            $scope.Components.Confirm.isShow = true ;
            $scope.Components.Confirm.okText = $scope.autoLanguage( '上一步' ) ;
            $scope.Components.Confirm.ok = function(){
               $scope.GotoPrev() ;
            }
            return ;
         }
         var newExtendNodeList = [] ;
         $.each( oldExtendNodeList, function( index, nodeInfo ){
            var nodeConf = {} ;
            $.each( nodeInfo, function( key, value ){
               if( ( ( value.length > 0 && isFilterDefaultItem( key, value ) == false ) || key == 'datagroupname' ) && key != '_other' )
               {
                  nodeConf[key] = value ;
               }
            } ) ;
            nodeConf = convertJsonValueString( nodeConf ) ;
            newExtendNodeList.push( nodeConf ) ;
         } ) ;
         var config = {
            'ClusterName':  extendConfigure['ClusterName'],
            'BusinessType': extendConfigure['BusinessType'],
            'BusinessName': extendConfigure['BusinessName'],
            'DeployMod':    extendConfigure['DeployMod'],
            'Config': newExtendNodeList
         } ;
         extendSdb( config ) ;
      }

   } ) ;

   sacApp.controllerProvider.register( 'Deploy.Sdb.Extend.Group.Ctrl', function( $scope, $location, $rootScope, SdbRest, SdbFunction, SdbSwap, SdbSignal ){

      var hostList = [] ;
      var dropDownGroupName = '' ;
      var dataGroupList = [ 'SYSCoord', 'SYSCatalogGroup' ] ;
      var dataGroupSelect = [ { 'key': $scope.autoLanguage( '全部' ), 'value': '' } ] ;
      $scope.GroupList = [
         { 'role': 'coord',   'groupName': 'SYSCoord',        'checked': false, 'exist': true, 'isShow': true, 'nodeNum': 0, 'extendNodeNum': 0 },
         { 'role': 'catalog', 'groupName': 'SYSCatalogGroup', 'checked': false, 'exist': true, 'isShow': true, 'nodeNum': 0, 'extendNodeNum': 0 }
      ] ;
      $scope.FilterGroup = {
         'firstIndex': 0,
         'value': 0,
         'select': [
            { 'key': $scope.autoLanguage( '全部' ),   'value': 0 },
            { 'key': $scope.autoLanguage( '新增加' ), 'value': 1 },
            { 'key': $scope.autoLanguage( '已存在' ), 'value': 2 }
         ],
         'onChange': function(){
            if( $scope.FilterGroup['value'] == 0 )
            {
               $scope.FilterGroup['firstIndex'] = 0 ;
               $.each( $scope.GroupList, function( index, groupInfo ){
                  groupInfo['isShow'] = true ;
               } ) ;
            }
            else if( $scope.FilterGroup['value'] == 1 )
            {
               $scope.FilterGroup['firstIndex'] = -1 ;
               $.each( $scope.GroupList, function( index, groupInfo ){
                  groupInfo['isShow'] = !groupInfo['exist'] ;
                  if( groupInfo['isShow'] == true && $scope.FilterGroup['firstIndex'] == -1 )
                  {
                     $scope.FilterGroup['firstIndex'] = index ;
                  }
               } ) ;
            }
            else if( $scope.FilterGroup['value'] == 2 )
            {
               $scope.FilterGroup['firstIndex'] = -1 ;
               $.each( $scope.GroupList, function( index, groupInfo ){
                  groupInfo['isShow'] = groupInfo['exist'] ;
                  if( groupInfo['isShow'] == true && $scope.FilterGroup['firstIndex'] == -1 )
                  {
                     $scope.FilterGroup['firstIndex'] = index ;
                  }
               } ) ;
            }
         }
      } ;
      //创建分区组 弹窗
      $scope.CreateGroupWindow = {
         'config': {
            'inputList': [
               {
                  "name": "groupName",
                  "webName": $scope.autoLanguage( '分区组名' ),
                  "type": "string",
                  "required": true,
                  "value": "",
                  "valid": {
                     "min": 1,
                     "max": 127,
                     "ban": [ ".", "$", 'SYSCatalogGroup', 'SYSCoord' ]
                  }
               }
            ]
         },
         'callback': {}
      }
      //添加节点 弹窗
      $scope.AddNodeWindow = {
         'config': {
            'inputList': [
               {
                  "name": "createModel",
                  "webName": $scope.autoLanguage( '创建节点模式' ),
                  "type": "select",
                  "value": 0,
                  "valid": [
                     { 'key': $scope.autoLanguage( '默认配置' ), 'value': 0 },
                     { 'key': $scope.autoLanguage( '新建配置' ), 'value': 1 },
                     { 'key': $scope.autoLanguage( '复制节点配置' ), 'value': 2 }
                  ]
               },
               {
                  "name": "hostname",
                  "webName": $scope.autoLanguage( '添加节点的主机' ),
                  "type": "select",
                  "value": '',
                  "valid": []
               }
            ]
         },
         'callback': {}
      }
      //删除节点 弹窗
      $scope.RemoveNodeWindow = {
         'config': {
            'inputList': [
               {
                  "name": "nodename",
                  "webName": $scope.autoLanguage( '节点名' ),
                  "type": "select",
                  "value": '',
                  "valid": []
               }
            ]
         },
         'callback': {}
      } ;
      //修改分区组名 弹窗
      $scope.RenameGroupWindow = {
         'config': {
            'inputList': [
               {
                  "name": "groupName",
                  "webName": $scope.autoLanguage( '新的分区组名' ),
                  "type": "string",
                  "required": true,
                  "value": '',
                  "valid": {
                     "min": 1,
                     "max": 127,
                     "ban": [ ".", "$", 'SYSCatalogGroup', 'SYSCoord' ]
                  }
               }
            ]
         },
         'callback': {}
      } ;
      //显示列 下拉菜单
      $scope.GroupDropdown = {
         'config': [],
         'callback': {}
      } ;

      //获取分区组索引
      var getGroupIndex = function( groupName ){
         return dataGroupList.indexOf( groupName ) ;
      }

      //聚合分区组
      var aggreGroup = function( nodeInfo, isExtend, defaultNum ){
         if( nodeInfo['role'] == 'coord' )
         {
            if( isExtend )
            {
               ++$scope.GroupList[0]['extendNodeNum'] ;
            }
            else
            {
               ++$scope.GroupList[0]['nodeNum'] ;
            }
         }
         else if( nodeInfo['role'] == 'catalog' )
         {
            if( isExtend )
            {
               ++$scope.GroupList[1]['extendNodeNum'] ;
            }
            else
            {
               ++$scope.GroupList[1]['nodeNum'] ;
            }
         }
         else if( nodeInfo['role'] == 'data' )
         {
            var groupName = nodeInfo['datagroupname'] ;
            var index = getGroupIndex( groupName ) ;
            if( index == -1 )
            {
               //新组
               dataGroupList.push( groupName ) ;
               var newGroupInfo = { 'role': nodeInfo['role'], 'groupName': groupName, 'checked': false, 'exist': !isExtend, 'isShow': true,
                                    'nodeNum': 0, 'extendNodeNum': 0 } ;
               if( isExtend )
               {
                  newGroupInfo['extendNodeNum'] = defaultNum ;
               }
               else
               {
                  newGroupInfo['nodeNum'] = defaultNum ;
               }
               $scope.GroupList.push( newGroupInfo ) ;
            }
            else
            {
               if( isExtend )
               {
                  ++$scope.GroupList[index]['extendNodeNum'] ;
               }
               else
               {
                  ++$scope.GroupList[index]['nodeNum'] ;
               }
            }
         }
      }

      SdbSwap.groupDefer.then( function( result ){

         hostList = result['HostList'] ;

         $.each( result['NodeList'], function( index, nodeInfo ){
            aggreGroup( nodeInfo, false, 1 ) ;
         } ) ;
         $.each( result['ExtendNodeList'], function( index, nodeInfo ){
            aggreGroup( nodeInfo, true, 1 ) ;
         } ) ;

         $.each( dataGroupList, function( index, groupName ){
            if( index > 1 )
            {
               dataGroupSelect.push( { 'key': groupName, 'value': groupName } ) ;
            }
         } ) ;
         SdbSwap.nodeDefer.resolve( 'GroupList', dataGroupSelect ) ;
      } ) ;
    
      //选中分区组
      $scope.SwitchGroup = function( index ){
         $.each( $scope.GroupList, function( i, groupInfo ){
            if( i == index )
            {
               return true ;
            }
            groupInfo['checked'] = false ;
         } ) ;
         $scope.GroupList[index]['checked'] = !$scope.GroupList[index]['checked'] ;
         var role      = $scope.GroupList[index]['role'] ;
         var groupName = $scope.GroupList[index]['groupName'] ;
         var checked   = $scope.GroupList[index]['checked'] ;
         SdbSignal.commit( 'FilterNodeByGroup', { 'role': role, 'groupName': groupName, 'checked': checked } ) ;
      }

      //打开 帮助信息 弹窗
      $scope.ShowHelper = function(){
         $scope.HelperWindow['callback']['SetCloseButton']( $scope.autoLanguage( '关闭' ), function(){
            return true ;
         } ) ;
         $scope.HelperWindow['callback']['SetTitle']( $scope.autoLanguage( '帮助' ) ) ;
         $scope.HelperWindow['callback']['SetIcon']( '' ) ;
         $scope.HelperWindow['callback']['Open']() ;
      }

      //打开 创建分区组 弹窗
      $scope.ShowCreateGroup = function(){
         $scope.CreateGroupWindow['config']['inputList'][0]['value'] = '' ;
         $scope.CreateGroupWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = $scope.CreateGroupWindow['config'].check( function( thisValue ){
               if( getGroupIndex( thisValue['groupName']  ) >= 0 )
               {
                  return [ { 'name': 'groupName', 'error': $scope.autoLanguage( '分区组已经存在' ) } ] ;
               }
               return [] ;
            } ) ;
            if( isAllClear )
            {
               var formVal = $scope.CreateGroupWindow['config'].getValue() ;
               aggreGroup( { 'role': 'data', 'datagroupname': formVal['groupName'] }, true, 0 ) ;
               //表格的分区组列表
               dataGroupSelect.push( { 'key': formVal['groupName'], 'value': formVal['groupName'] } ) ;
            }
            return isAllClear ;
         } ) ;
         $scope.CreateGroupWindow['callback']['SetTitle']( $scope.autoLanguage( '创建分区组' ) ) ;
         $scope.CreateGroupWindow['callback']['Open']() ;
      }

      //打开 添加节点 弹窗
      var openAddNode = function( groupInfo ){
         if( groupInfo['nodeNum'] + groupInfo['extendNodeNum'] >= 7 && groupInfo['role'] != 'coord' )
         {
            return ;
         }
         var inputList     = $scope.AddNodeWindow['config']['inputList'] ;
         var modelInput    = inputList[0] ;
         var hostNameInput = inputList[1] ;

         if( inputList.length > 2 )
         {
            inputList.splice( 2, inputList.length - 2 ) ;
         }

         modelInput['onChange'] = function( name, key, value ){
            if( value == 2 )
            {
               var results = SdbSignal.commit( 'GetAllNode', null ) ;
               inputList.push( {
                  "name": "copyNode",
                  "webName": $scope.autoLanguage( '复制的节点' ),
                  "type": "select",
                  "value": results[0][0]['value'],
                  "valid": results[0]
               } ) ;
            }
            else
            {
               inputList.splice( 2, 1 ) ;
            }
         }

         modelInput['value'] = 0 ;
         hostNameInput['valid'] = [] ;
         $.each( hostList, function( index, hostInfo ){
            if( index == 0 )
            {
               hostNameInput['value'] = hostInfo['HostName'] ;
            }
            hostNameInput['valid'].push( { 'key': hostInfo['HostName'], 'value': hostInfo['HostName'] } ) ;
         } ) ;

         $scope.AddNodeWindow['callback']['SetTitle']( $scope.autoLanguage( '添加节点' ) ) ;
         $scope.AddNodeWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = $scope.AddNodeWindow['config'].check() ;
            if( isAllClear )
            {
               var formVal    = $scope.AddNodeWindow['config'].getValue() ;
               var groupName  = groupInfo['groupName'] ;
               var role       = groupInfo['role'] ;
               var copyNode   = null ;

               if( formVal['createModel'] == 2 )
               {
                  copyNode = formVal['copyNode'] ;
               }

               $scope.AddNodeWindow['callback']['Close']() ;
               SdbSignal.commit( 'CreateNode', { 'type': formVal['createModel'], 'hostName': formVal['hostname'], 'role': role, 'groupName': groupName, 'copyNode': copyNode, 'callback': function(){
                  ++groupInfo['extendNodeNum'] ;
               } } ) ;
            }
            return isAllClear ;
         } ) ;
         $scope.AddNodeWindow['callback']['Open']() ;
      }

      //打开 删除节点 弹窗
      var openRmNode = function( groupInfo ){
         if( groupInfo['extendNodeNum'] <= 0 )
         {
            return ;
         }
         var nodeInput = $scope.RemoveNodeWindow['config']['inputList'][0] ;
         var results   = SdbSignal.commit( 'GetNodeName', groupInfo ) ;
         var nodeList  = results[0] ;

         nodeInput['value'] = nodeList.length > 0 ? nodeList[0]['value'] : '' ;
         nodeInput['valid'] = nodeList ;
         $scope.RemoveNodeWindow['callback']['SetTitle']( $scope.autoLanguage( '删除节点' ) ) ;
         $scope.RemoveNodeWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = $scope.RemoveNodeWindow['config'].check() ;
            if( isAllClear )
            {
               var formVal = $scope.RemoveNodeWindow['config'].getValue() ;
               SdbSignal.commit( 'DropNode', formVal['nodename'] ) ;
               --groupInfo['extendNodeNum'] ;
            }
            return isAllClear ;
         } ) ;
         $scope.RemoveNodeWindow['callback']['Open']() ;
      }

      //打开 修改分区组名 弹窗
      var openRenameGroup = function( groupInfo ){
         if( groupInfo['exist'] == true )
         {
            return ;
         }
         $scope.RenameGroupWindow['config']['inputList'][0]['value'] = groupInfo['groupName'] ;
         $scope.RenameGroupWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = $scope.RenameGroupWindow['config'].check( function( thisValue ){
               if( getGroupIndex( thisValue['groupName'] ) >= 0 )
               {
                  return [ { 'name': 'groupName', 'error': $scope.autoLanguage( '分区组已经存在' ) } ] ;
               }
               return [] ;
            } ) ;
            if( isAllClear )
            {
               var formVal = $scope.RenameGroupWindow['config'].getValue() ;
               var role    = groupInfo['role'] ;
               var oldGroupName = groupInfo['groupName'] ;
               var newGroupName = formVal['groupName'] ;
               var groupIndex   = getGroupIndex( oldGroupName ) ;

               dataGroupList[groupIndex] = newGroupName ;
               $scope.GroupList[groupIndex]['groupName'] = newGroupName ;
               $.each( dataGroupSelect, function( index, groupItem ){
                  if( index > 0 && groupItem['value'] === oldGroupName )
                  {
                     groupItem['key'] = newGroupName ;
                     groupItem['value'] = newGroupName ;
                     return false ;
                  }
               } ) ;
               SdbSignal.commit( 'RenameGroup', { 'oldGroupName': oldGroupName, 'newGroupName': newGroupName } ) ;
            }
            return isAllClear ;
         } ) ;
         $scope.RenameGroupWindow['callback']['SetTitle']( $scope.autoLanguage( '修改分区组名' ) ) ;
         $scope.RenameGroupWindow['callback']['Open']() ;
      }

      //打开 删除分区组 弹窗
      var openDropGroup = function( index, groupInfo ){
         if( groupInfo['exist'] == true )
         {
            return ;
         }
         if( groupInfo['extendNodeNum'] > 0 && groupInfo['exist'] == false )
         {
            _IndexPublic.createInfoModel( $scope, sprintf( $scope.autoLanguage( '确定要把分区组?和分区组下的?个节点都删除吗？' ), groupInfo['groupName'], groupInfo['extendNodeNum'] ), $scope.autoLanguage( '是的' ), function(){
               
               dataGroupList.splice( index, 1 ) ;
               $scope.GroupList.splice( index, 1 ) ;
               if( groupInfo['role'] == 'data' )
               {
                  $.each( dataGroupSelect, function( index2, groupItem ){
                     if( index > 0 && groupItem['value'] === groupInfo['groupName'] )
                     {
                        dataGroupSelect.splice( index2, 1 ) ;
                        return false ;
                     }
                  } ) ;
               }

               SdbSignal.commit( 'DropGroup', { 'groupName': groupInfo['groupName'] } ) ;
            } ) ;
         }
         else
         {
            dataGroupList.splice( index, 1 ) ;
            $scope.GroupList.splice( index, 1 ) ;
            if( groupInfo['role'] == 'data' )
            {
               $.each( dataGroupSelect, function( index2, groupItem ){
                  if( index > 0 && groupItem['value'] === groupInfo['groupName'] )
                  {
                     dataGroupSelect.splice( index2, 1 ) ;
                     return false ;
                  }
               } ) ;
            }
         }
      }

      //打开分区组的下拉菜单
      $scope.OpenGroupDropdown = function( event, groupName ){
         var index     = getGroupIndex( groupName ) ;
         var role      = $scope.GroupList[index]['role'] ;
         var nodeNum   = $scope.GroupList[index]['nodeNum'] ;
         var extendNum = $scope.GroupList[index]['extendNodeNum'] ;
         var exist     = $scope.GroupList[index]['exist'] ;

         $scope.GroupDropdown['config'] = [
            { 'key': $scope.autoLanguage( '添加节点' ),    'action': 'addNode',     'show': true },
            { 'key': $scope.autoLanguage( '删除节点' ),    'action': 'rmNode',      'show': true },
            { 'divider': true },
            { 'key': $scope.autoLanguage( '修改分区组名' ), 'action': 'resetGroup', 'show': true },
            { 'key': $scope.autoLanguage( '删除分区组' ),   'action': 'rmGroup',    'show': true }
         ] ;

         if( role == 'coord' || role == 'catalog' )
         {
            if( role == 'catalog' && nodeNum + extendNum == 7 )
            {
               $scope.GroupDropdown['config'][0]['disabled'] = true ;
            }
            if( extendNum == 0 || nodeNum == 0 )
            {
               $scope.GroupDropdown['config'][1]['disabled'] = true ;
            }
            //不能修改或删除分区组
            $scope.GroupDropdown['config'][2]['divider'] = false ;
            $scope.GroupDropdown['config'][3]['show'] = false ;
            $scope.GroupDropdown['config'][4]['show'] = false ;
         }
         else
         {
            //data
            if( nodeNum + extendNum == 7 )
            {
               $scope.GroupDropdown['config'][0]['disabled'] = true ;
            }
            if( extendNum == 0 )
            {
               $scope.GroupDropdown['config'][1]['disabled'] = true ;
            }
         }
         
         if( exist )
         {
            $scope.GroupDropdown['config'][3]['disabled'] = true ;
            $scope.GroupDropdown['config'][4]['disabled'] = true ;
         }

         dropDownGroupName = groupName ;
         $scope.GroupDropdown['callback']['Open']( event.currentTarget ) ;
      }

      //分区组下拉菜单的选项点击事件
      $scope.ClickGroupDropdown = function( action ){
         var index     = getGroupIndex( dropDownGroupName ) ;
         var groupInfo = $scope.GroupList[index] ;
         $scope.GroupDropdown['callback']['Close']() ;
         if( action == 'addNode' )
         {
            openAddNode( groupInfo ) ;
         }
         else if( action == 'rmNode' )
         {
            openRmNode( groupInfo ) ;
         }
         else if( action == 'resetGroup' )
         {
            openRenameGroup( groupInfo ) ;
         }
         else if( action == 'rmGroup' )
         {
            openDropGroup( index, groupInfo ) ;
         }
      }

   } ) ;

   sacApp.controllerProvider.register( 'Deploy.Sdb.Extend.Table.Ctrl', function( $scope, $location, $rootScope, SdbRest, SdbFunction, SdbSwap, SdbSignal ){

      var extendNodeList = [] ;
      var existNodeList = [] ;
      var template = [] ;
      $scope.NodeList = [] ;

      //编辑节点 弹窗
      $scope.SetNodeConfWindow = {
         'config': {
            'form1': {
               'keyWidth': '160px',
               'inputList': []
            },
            'form2': {
               'keyWidth': '160px',
               'inputList': []
            },
            'form3': {
               'keyWidth': '160px',
               'inputList': []
            }
         },
         'callback': {}
      } ;
      //查看节点 弹窗
      $scope.CheckNodeConfWindow = {
         'config': [],
         'callback': {}
      } ;

      //节点列表
      $scope.NodeTable = {
         'title': {
            '_other.checked': '',
            'HostName':       $scope.autoLanguage( '主机名' ),
            'svcname':        $scope.autoLanguage( '服务名' ),
            'dbpath':         $scope.autoLanguage( '数据路径' ),
            'role':           $scope.autoLanguage( '角色' ),
            'datagroupname':  $scope.autoLanguage( '分区组' ),
            '_other.type':    $scope.autoLanguage( '类型' )
         },
         'body': [],
         'options': {
            'width': {
               '_other.checked': '26px',
               'HostName': '23%',
               'svcname': '12%',
               'dbpath': '31%',
               'role': '10%',
               'datagroupname': '12%',
               '_other.type': '12%'
            },
            'sort': {
               '_other.checked': false,
               'HostName': true,
               'svcname': true,
               'dbpath': true,
               'role': true,
               'datagroupname': true,
               '_other.type': true
            },
            'max': 50,
            'filter': {
               '_other.checked': null,
               'HostName': 'indexof',
               'svcname': 'indexof',
               'dbpath': 'indexof',
               'role': [
                  { 'key': $scope.autoLanguage( '全部' ), 'value': '' },
                  { 'key': 'coord', 'value': 'coord' },
                  { 'key': 'catalog', 'value': 'catalog' },
                  { 'key': 'data', 'value': 'data' }
               ],
               'datagroupname': [ { 'key': $scope.autoLanguage( '全部' ), 'value': '' } ],
               '_other.type': [
                  { 'key': $scope.autoLanguage( '全部' ),     'value': '' },
                  { 'key': $scope.autoLanguage( '新增' ), 'value': 2 },
                  { 'key': $scope.autoLanguage( '已存在' ), 'value': 1 },
               ]
            }
         },
         'callback': {}
      } ;

      var initEditNodeInput = function(){
         $scope.SetNodeConfWindow['config']['HostName'] = '' ;
         $scope.SetNodeConfWindow['config']['form1']['inputList'] = _Deploy.ConvertTemplate( template, 0, false, false, true ) ;
         $scope.SetNodeConfWindow['config']['form1']['inputList'][0]['valid'] = {} ;
         $scope.SetNodeConfWindow['config']['form2']['inputList'] = _Deploy.ConvertTemplate( template, 1, false, false, true ) ;
         $scope.SetNodeConfWindow['config']['form3']['inputList'] = [
            {
               "name": "other",
               "webName": $scope.autoLanguage( "自定义配置" ),
               "type": "list",
               "valid": {
                  "min": 0
               },
               "child": [
                  [
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
                        "valid": {
                           "min": 1
                        },
                        "default": "",
                        "value": ""
                     }
                  ]
               ]
            }
         ] ;
      }

      SdbSwap.nodeDefer.then( function( result ){
         extendNodeList = result['ExtendNodeList'] ;
         $.each( extendNodeList, function( index, nodeInfo ){
            nodeInfo['_other'] = {} ;
            nodeInfo['_other']['checked'] = false ;
            nodeInfo['_other']['canChecked'] = true ;
            nodeInfo['_other']['type'] = 2 ;
            nodeInfo['_other']['i'] = index ;
            $scope.NodeList.push( nodeInfo ) ;
         } ) ;

         existNodeList = result['NodeList'] ;
         $.each( existNodeList, function( index, nodeInfo ){
            nodeInfo['_other'] = {} ;
            nodeInfo['_other']['checked'] = false ;
            nodeInfo['_other']['canChecked'] = false ;
            nodeInfo['_other']['type'] = 1 ;
            nodeInfo['_other']['i'] = index ;
            $scope.NodeList.push( nodeInfo ) ;
         } ) ;
         $scope.NodeTable['body'] = $scope.NodeList ;

         $scope.NodeTable['options']['filter']['datagroupname'] = result['GroupList'] ;
      } ) ;

      SdbSwap.templateDefer.then( function( result ){
         template = result['Template'] ;
         initEditNodeInput() ;
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
            if( dataList[index]['_other']['canChecked'] == true )
            {
               dataList[index]['_other']['checked'] = true ;
            }
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
            if( dataList[index]['_other']['canChecked'] == true )
            {
               dataList[index]['_other']['checked'] = !dataList[index]['_other']['checked'] ;
            }
         } ) ;
      }

      //切换编辑节点窗口分类
      $scope.SwitchNodeConfClass = function( index ){
         $scope.SetNodeConfWindow['ShowType'] = index ;
      }

      //校验编辑节点
      var checkNodeConf = function( type ){
         var isAllClear1 = $scope.SetNodeConfWindow['config']['form1'].check( function( valueList ){
            var error = [] ;
            if( type != 4 && valueList['dbpath'].length == 0 )
            {
               error.push( { 'name': 'dbpath', 'error': $scope.autoLanguage( '数据路径不能为空。' ) } ) ;
            }
            if( type != 4 && valueList['svcname'].length == 0 )
            {
               error.push( { 'name': 'svcname', 'error': $scope.autoLanguage( '服务名不能为空。' ) } ) ;
            }
            else if( type != 4 && portEscape( valueList['svcname'], 0 ) == null )
            {
               error.push( { 'name': 'svcname', 'error': $scope.autoLanguage( '服务名格式错误。' ) } ) ;
            }
            return error ;
         } ) ;
         var isAllClear2 = $scope.SetNodeConfWindow['config']['form2'].check() ;
         var isAllClear3 = $scope.SetNodeConfWindow['config']['form3'].check( function( valueList ){
            var error = [] ;
            $.each( valueList['other'], function( index, configInfo ){
               var lowercaseName = configInfo['name'].toLowerCase() ;
               if( lowercaseName == 'hostname' ||
                   lowercaseName == 'datagroupname' ||
                   lowercaseName == 'role' ||
                   lowercaseName == '_other' )
               {
                  error.push( { 'name': 'other', 'error': $scope.autoLanguage( '自定义配置不能设置HostName、datagroupname、role。' ) } ) ;
                  return false ;
               }
            } )
            return error ;
         } ) ;
         if( isAllClear1 && isAllClear2 && isAllClear3 )
         {
            var formVal1 = $scope.SetNodeConfWindow['config']['form1'].getValue() ;
            var formVal2 = $scope.SetNodeConfWindow['config']['form2'].getValue() ;
            var formVal3 = $scope.SetNodeConfWindow['config']['form3'].getValue() ;
            var formVal = $.extend( true, formVal1, formVal2 ) ;
            $.each( formVal3['other'], function( index, configInfo ){
               if( configInfo['name'] != '' )
               {
                  formVal[ configInfo['name'] ] = configInfo['value'] ;
               }
            } ) ;
            return formVal ;
         }
         else
         {
            if( !isAllClear1 )
            {
               $scope.SetNodeConfWindow['ShowType'] = 1 ;
            }
            else if( !isAllClear2 )
            {
               $scope.SetNodeConfWindow['ShowType'] = 2 ;
            }
            else if( !isAllClear3 )
            {
               $scope.SetNodeConfWindow['ShowType'] = 3 ;
            }
         }
         return isAllClear1 && isAllClear2 && isAllClear3 ;
      }

      //创建节点
      var createNode = function( type, hostName, role, groupName ){
         var result = checkNodeConf( type ) ;
         if( result === false )
         {
            return result ;
         }
         var newNodeConf = result ;
         var index = extendNodeList.length ;
         var nodeInfo = {} ;
         nodeInfo['HostName']      = hostName ;
         nodeInfo['role']          = role ;
         nodeInfo['datagroupname'] = role == 'data' ? groupName : '' ;
         nodeInfo['_other'] = {} ;
         nodeInfo['_other']['checked'] = false ;
         nodeInfo['_other']['canChecked'] = true ;
         nodeInfo['_other']['type'] = 2 ;
         nodeInfo['_other']['i'] = index ;
         $.each( newNodeConf, function( key, value ){
            if( key == '' )
            {
               return true ;
            }
            nodeInfo[key] = value ;
         } ) ;
         $scope.NodeList.unshift( nodeInfo ) ;
         extendNodeList.push( nodeInfo ) ;
         return true ;
      }

      //删除节点
      var removeNode = function( condition ){
         //删除节点
         for( var i = 0; i < extendNodeList.length; )
         {
            var isFind = false ;
            $.each( extendNodeList, function( index, nodeInfo ){
               if( index >= i && condition( nodeInfo ) )
               {
                  extendNodeList.splice( index, 1 ) ;
                  i = index ;
                  isFind = true ;
                  return false ;
               }
            } ) ;
            if( isFind == false )
            {
               break ;
            }
         }
         //重置节点的序列
         $.each( extendNodeList, function( index, nodeInfo ){
            nodeInfo['_other']['i'] = index ;
         } ) ;
         //删除表格节点
         for( var i = 0; i < $scope.NodeList.length; )
         {
            var isFind = false ;
            $.each( $scope.NodeList, function( index, nodeInfo ){
               if( index >= i && nodeInfo['_other']['canChecked'] === true && condition( nodeInfo ) )
               {
                  $scope.NodeList.splice( index, 1 ) ;
                  i = index ;
                  isFind = true ;
                  return false ;
               }
            } ) ;
            if( isFind == false )
            {
               break ;
            }
         }
      }

      //保存单个节点
      var saveOneNodeConf = function( nodeConfig, newNodeConf, isBatchSave, nodeNum ){
         var nodeFields = {} ;
         newNodeConf['svcname'] = portEscape( newNodeConf['svcname'], nodeNum ) ;
         newNodeConf['dbpath']  = dbpathEscape( newNodeConf['dbpath'], nodeConfig['HostName'],
                                                newNodeConf['svcname'].length == 0 ? nodeConfig['svcname'] : newNodeConf['svcname'],
                                                nodeConfig['role'], nodeConfig['datagroupname'] ) ;
         //记录原节点拥有的字段
         $.each( nodeConfig, function( key, val ){
            nodeFields[key] = false ;
            if( key == 'HostName' || key == 'datagroupname' || key == 'role' || key == '_other' )
            {
               nodeFields[key] = true ;
            }
         } ) ;
         $.each( newNodeConf, function( key, value ){
            value = value.toString() ;
            if( isBatchSave )
            {
               if( ( key == 'dbpath' || key == 'svcname' ) && value.length == 0 )
               {
                  nodeFields[key] = true ;
                  return true ;
               }
               if( value.length == 0 )
               {
                  nodeFields[key] = true ;
                  return true ;
               }
            }
            if( key.length == 0 )
            {
               return true ;
            }
            nodeConfig[key] = value.toString() ;
            nodeFields[key] = true ;
         } ) ;
         //把原节点配置有的，newNodeConf没有的字段删除
         $.each( nodeFields, function( key, isExist ){
            if( isExist == false )
            {
               delete nodeConfig[key] ;
            }
         } ) ;
      }

      //保存节点
      var saveNodeConf = function( type, nodeIndex ){
         var result = checkNodeConf( type ) ;
         if( result === false )
         {
            return result ;
         }
         var newNodeConf = result ;
         if( type == 3 )
         {
            //保存单个节点配置
            saveOneNodeConf( extendNodeList[nodeIndex], newNodeConf, false, 0 ) ;
         }
         else if( type == 4 )
         {
            //保存批量节点配置
            var num = 0 ;
            $.each( extendNodeList, function( index, nodeInfo ){
               if( nodeInfo['_other']['checked'] == true )
               {
                  //把配置复制出来，作为模板
                  var newFormVal = $.extend( true, {}, newNodeConf ) ;
                  saveOneNodeConf( nodeInfo, newFormVal, true, num ) ;
                  ++num ;
               }
            } ) ;
         }
         return true ;
      }

      //加载单个节点配置到编辑窗口
      var loadOneNodeConfig = function( nodeIndex ){
         var loadName = [] ;
         //加载指定某个节点的配置
         $scope.SetNodeConfWindow['config']['HostName']      = extendNodeList[nodeIndex]['HostName'] ;
         $scope.SetNodeConfWindow['config']['role']          = extendNodeList[nodeIndex]['role'] ;
         $scope.SetNodeConfWindow['config']['datagroupname'] = extendNodeList[nodeIndex]['datagroupname'] ;

         $.each( $scope.SetNodeConfWindow['config']['form1']['inputList'], function( index, inputInfo ){
            var name = inputInfo['name'] ;
            loadName.push( name.toLowerCase() ) ;
            inputInfo['value'] = extendNodeList[nodeIndex][name] ;
         } ) ;
         $.each( $scope.SetNodeConfWindow['config']['form2']['inputList'], function( index, inputInfo ){
            var name = inputInfo['name'] ;
            loadName.push( name.toLowerCase() ) ;
            inputInfo['value'] = extendNodeList[nodeIndex][name] ;
         } ) ;
         //加载自定义配置项
         var isFirst = true ;
         $.each( extendNodeList[nodeIndex], function( key, value ){
            var lowercaseKey = key.toLowerCase() ;
            if( lowercaseKey != 'hostname' &&
                lowercaseKey != 'datagroupname' &&
                lowercaseKey != 'role' &&
                lowercaseKey != '_other' &&
                loadName.indexOf( lowercaseKey ) == -1 )
            {
               if( isFirst )
               {
                  $scope.SetNodeConfWindow['config']['form3']['inputList'][0]['child'][0][0]['value'] = key ;
                  $scope.SetNodeConfWindow['config']['form3']['inputList'][0]['child'][0][1]['value'] = value ;
                  isFirst = false ;
               }
               else
               {
                  var newInput = $.extend( true, [], $scope.SetNodeConfWindow['config']['form3']['inputList'][0]['child'][0] ) ;
                  newInput[0]['value'] = key ;
                  newInput[1]['value'] = value ;
                  $scope.SetNodeConfWindow['config']['form3']['inputList'][0]['child'].push( newInput ) ;
               }
            }
         } ) ;
      }

      //加载单个节点配置到查看窗口
      var loadOneNodeConfig2Check = function( nodeIndex ){
         var loadName = [] ;
         //加载指定某个节点的配置
         $scope.CheckNodeConfWindow['config'] = [] ;
         $scope.CheckNodeConfWindow['config'].push( { 'key': $scope.autoLanguage( '主机名' ), 'value': existNodeList[nodeIndex]['HostName'] } ) ;
         $scope.CheckNodeConfWindow['config'].push( { 'key': $scope.autoLanguage( '角色' ),   'value': existNodeList[nodeIndex]['role'] } ) ;
         $scope.CheckNodeConfWindow['config'].push( { 'key': $scope.autoLanguage( '分区组' ), 'value': existNodeList[nodeIndex]['datagroupname'] } ) ;
         $.each( template, function( index, item ){
            var name = item['WebName']
            var key  = item['Name'] ;
            if( key === 'role' )
            {
               return true ;
            }
            $scope.CheckNodeConfWindow['config'].push( { 'key': name, 'value': existNodeList[nodeIndex][key] } ) ;
         } ) ;

         $scope.CheckNodeConfWindow['callback']['SetIcon']( '' ) ;
         $scope.CheckNodeConfWindow['callback']['SetTitle']( $scope.autoLanguage( '节点配置' ) ) ;
         $scope.CheckNodeConfWindow['callback']['Open']() ;
      }

      //加载选择的所有节点配置
      var loadCheckedNodeConfig = function(){
         var loadName = [] ;
         var sum = 0 ;
         $.each( extendNodeList, function( index ){
            if( extendNodeList[index]['_other']['checked'] == true )
            {
               ++sum ;
            }
         } ) ;
         if( sum == 0 )
         {
            _IndexPublic.createInfoModel( $scope, $scope.autoLanguage( '修改配置至少需要选择一个节点。' ), $scope.autoLanguage( '好的' ) ) ;
            return false ;
         }
         $.each( $scope.SetNodeConfWindow['config']['form1']['inputList'], function( index, inputInfo ){
            var isFirst = true ;
            var name = inputInfo['name'] ;
            var value = '' ;
            var offset = null ;
            loadName.push( name.toLowerCase() ) ;
            var prevNodeInfo = null ;
            $.each( extendNodeList, function( index2, nodeInfo ){
               if( nodeInfo['_other']['checked'] == true )
               {
                  if( name == 'dbpath' )
                  {
                     if( isFirst == true )
                     {
                        value = selectDBPath( nodeInfo['dbpath'], nodeInfo['role'], nodeInfo['svcname'], nodeInfo['datagroupname'], nodeInfo['HostName'] ) ;
                        isFirst = false ;
                     }
                     else
                     {
                        if( value != selectDBPath( nodeInfo['dbpath'], nodeInfo['role'], nodeInfo['svcname'], nodeInfo['datagroupname'], nodeInfo['HostName'] ) )
                        {
                           value = '' ;
                           return false ;
                        }
                     }
                  }
                  else if( name == 'svcname' )
                  {
                     if( isFirst == true )
                     {
                        value = nodeInfo['svcname'] ;
                        prevNodeInfo = nodeInfo ;
                        isFirst = false ;
                     }
                     else
                     {
                        if( offset == null )
                        {
                           offset = parseInt( nodeInfo['svcname'] ) - parseInt( prevNodeInfo['svcname'] ) ;
                           if( offset != 0 )
                           {
                              value = value + '[' + ( offset > 0 ? '+' : '' ) + offset + ']' ;
                           }
                        }
                        else
                        {
                           if( offset != parseInt( nodeInfo['svcname'] ) - parseInt( prevNodeInfo['svcname'] ) )
                           {
                              value = '' ;
                              return false ;
                           }
                        }
                        prevNodeInfo = nodeInfo ;
                     }
                  }
                  else
                  {
                     if( isFirst == true )
                     {
                        value = nodeInfo[name] ;
                        isFirst = false ;
                     }
                     else
                     {
                        if( value != nodeInfo[name] )
                        {
                           value = '' ;
                           return false ;
                        }
                     }
                  }
               }
            } ) ;
            inputInfo['value'] = value ;
         } ) ;
         $.each( $scope.SetNodeConfWindow['config']['form2']['inputList'], function( index, inputInfo ){
            var isFirst = true ;
            var name = inputInfo['name'] ;
            loadName.push( name.toLowerCase() ) ;
            var value = '' ;
            $.each( extendNodeList, function( index2, nodeInfo ){
               if( nodeInfo['_other']['checked'] == true )
               {
                  if( isFirst == true )
                  {
                     value = nodeInfo[name] ;
                     isFirst = false ;
                  }
                  else
                  {
                     if( value != nodeInfo[name] )
                     {
                        value = '' ;
                        return false ;
                     }
                  }
               }
            } ) ;
            inputInfo['value'] = value ;
         } ) ;
         //加载自定义配置项
         var customConfig = [] ;
         $.each( extendNodeList, function( index, nodeInfo ){
            if( nodeInfo['_other']['checked'] == true )
            {
               $.each( nodeInfo, function( key, value ){
                  var lowercaseKey = key.toLowerCase() ;
                  if( lowercaseKey != 'hostname' &&
                        lowercaseKey != 'datagroupname' &&
                        lowercaseKey != 'role' &&
                        lowercaseKey != '_other' &&
                        loadName.indexOf( lowercaseKey ) == -1 &&
                        customConfig.indexOf( lowercaseKey ) == -1 )
                  {
                     customConfig.push( key ) ;
                  }
               } ) ;
            }
         } ) ;
         var isFirst = true ;
         $.each( customConfig, function( customIndex, config ){
            var value = '' ;
            var isFirst2 = true ;
            $.each( extendNodeList, function( index, nodeInfo ){
               if( nodeInfo['_other']['checked'] == true )
               {
                  if( isFirst2 == true )
                  {
                     value = nodeInfo[config] ;
                     isFirst2 = false ;
                  }
                  else
                  {
                     if( value != nodeInfo[config] )
                     {
                        value = '' ;
                        return false ;
                     }
                  }
               }
            } ) ;
            if( isFirst )
            {
               $scope.SetNodeConfWindow['config']['form3']['inputList'][0]['child'][0][0]['value'] = config ;
               $scope.SetNodeConfWindow['config']['form3']['inputList'][0]['child'][0][1]['value'] = value ;
               isFirst = false ;
            }
            else
            {
               var newInput = $.extend( true, [], $scope.SetNodeConfWindow['config']['form3']['inputList'][0]['child'][0] ) ;
               newInput[0]['value'] = config ;
               newInput[1]['value'] = value ;
               $scope.SetNodeConfWindow['config']['form3']['inputList'][0]['child'].push( newInput ) ;
            }
         } ) ;
         return true ;
      }

      /*
      打开编辑节点窗口
            type    3    加载指定节点配置, 修改节点
                    4    加载批量节点配置, 修改节点
            groupIndex   分区组的索引id
            hostIndex    主机的索引id
            nodeIndex    节点的索引id
            isExtendNode 是否扩容的节点
      */
      $scope.OpenEditNode = function( type, groupIndex, nodeIndex, isExtendNode ){
         initEditNodeInput() ;
         $scope.SetNodeConfWindow['WindowsType'] = type ;
         $scope.SetNodeConfWindow['ShowType'] = 1 ;
         if( type == 3 )
         {
            if( isExtendNode == true )
            {
               //扩容节点
               loadOneNodeConfig( nodeIndex ) ;
            }
            else
            {
               //已存在节点
               loadOneNodeConfig2Check( nodeIndex ) ;
               return ;
            }
         }
         else if( type == 4 )
         {
            //批量加载配置
            if( loadCheckedNodeConfig() == false )
            {
               return ;
            }
         }

         $scope.SetNodeConfWindow['callback']['SetIcon']( '' ) ;
         $scope.SetNodeConfWindow['callback']['SetTitle']( $scope.autoLanguage( '编辑节点配置' ) ) ;
         $scope.SetNodeConfWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            return saveNodeConf( type, nodeIndex ) ;
         } ) ;
         $scope.SetNodeConfWindow['callback']['Open']() ;
      }

      /*
      创建节点的弹窗
            type    0    默认配置, 创建新节点
                    1    空配置, 创建新节点
                    2    加载指定节点配置, 创建新节点
      */
      var openCreateNode = function( type, hostName, role, groupName, copyNode, successCallback ){
         initEditNodeInput() ;
         $scope.SetNodeConfWindow['WindowsType'] = type ;
         $scope.SetNodeConfWindow['ShowType']    = 1 ;
         $scope.SetNodeConfWindow['config']['HostName'] = hostName ;
         $scope.SetNodeConfWindow['config']['role']     = role ;
         $scope.SetNodeConfWindow['config']['datagroupname'] = role == 'data' ? groupName : '' ;
         if( type == 0 )
         {
         }
         else if( type == 1 )
         {
            $.each( $scope.SetNodeConfWindow['config']['form1']['inputList'], function( index, inputInfo ){
               if( inputInfo['type'] != 'select' )
               {
                  inputInfo['value'] = '' ;
               }
            } ) ;
            $.each( $scope.SetNodeConfWindow['config']['form2']['inputList'], function( index, inputInfo ){
               if( inputInfo['type'] != 'select' )
               {
                  inputInfo['value'] = '' ;
               }
            } ) ;
         }
         else if( type == 2 )
         {
            $.each( $scope.SetNodeConfWindow['config']['form1']['inputList'], function( index, inputInfo ){
               var key = inputInfo['name'] ;
               if( hasKey( copyNode, key ) )
               {
                  inputInfo['value'] = copyNode[key] ;
               }
            } ) ;
            $.each( $scope.SetNodeConfWindow['config']['form2']['inputList'], function( index, inputInfo ){
               var key = inputInfo['name'] ;
               if( hasKey( copyNode, key ) )
               {
                  inputInfo['value'] = copyNode[key] ;
               }
            } ) ;
         }
         $scope.SetNodeConfWindow['callback']['SetIcon']( '' ) ;
         $scope.SetNodeConfWindow['callback']['SetTitle']( $scope.autoLanguage( '编辑节点配置' ) ) ;
         $scope.SetNodeConfWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            var isAllClear = createNode( type, hostName, role, groupName ) ;
            if( isAllClear )
            {
               successCallback() ;
            }
            return isAllClear ;
         } ) ;
         $scope.SetNodeConfWindow['callback']['Open']() ;
      }

      SdbSignal.on( 'CreateNode', function( signalInfo ){
         openCreateNode( signalInfo['type'], signalInfo['hostName'], signalInfo['role'], signalInfo['groupName'], signalInfo['copyNode'], signalInfo['callback'] ) ;
      } ) ;

      SdbSignal.on( 'DropNode', function( signalInfo ){
         removeNode( function( nodeInfo ){
            return ( nodeInfo === signalInfo ) ;
         } ) ;
      } )

      SdbSignal.on( 'DropGroup', function( signalInfo ){
         var groupName = signalInfo['groupName'] ;
         removeNode( function( nodeInfo ){
            return ( nodeInfo['role'] == 'data' && nodeInfo['datagroupname'] == groupName ) ;
         } ) ;
      } ) ;

      SdbSignal.on( 'RenameGroup', function( signalInfo ){
         $.each( $scope.NodeList, function( index, nodeInfo ){
            if( nodeInfo['role'] == 'data' && nodeInfo['datagroupname'] === signalInfo['oldGroupName'] )
            {
               nodeInfo['datagroupname'] = signalInfo['newGroupName'] ;
            }
         } ) ;
      } ) ;

      SdbSignal.on( 'FilterNodeByGroup', function( signalInfo ){
         var role      = signalInfo['role'] ;
         var groupName = signalInfo['groupName'] ;
         var isChecked = signalInfo['checked'] ;
         if( isChecked == true )
         {
            //选中状态
            $scope.NodeTable['callback']['SetFilter']( 'HostName', '' ) ;
            $scope.NodeTable['callback']['SetFilter']( 'svcname', '' ) ;
            $scope.NodeTable['callback']['SetFilter']( 'dbpath', '' ) ;
            $scope.NodeTable['callback']['SetFilter']( '_other.type', '' ) ;
            if( role == 'data' )
            {
               $scope.NodeTable['callback']['SetFilter']( 'role', role ) ;
               $scope.NodeTable['callback']['SetFilter']( 'datagroupname', groupName ) ;
            }
            else
            {
               $scope.NodeTable['callback']['SetFilter']( 'role', role ) ;
               $scope.NodeTable['callback']['SetFilter']( 'datagroupname', '' ) ;
            }
         }
         else
         {
            //取消选中状态
            $scope.NodeTable['callback']['SetFilter']( 'role', '' ) ;
            $scope.NodeTable['callback']['SetFilter']( 'datagroupname', '' ) ;
         }
      } ) ;

      SdbSignal.on( 'GetNodeName', function( groupInfo ){
         var role      = groupInfo['role'] ;
         var groupName = groupInfo['groupName'] ;
         var nodeNameList = [] ;
         $.each( extendNodeList, function( index, nodeInfo ){
            if( nodeInfo['role'] == role && role != 'data' )
            {
               var nodeName = nodeInfo['HostName'] + ':' + nodeInfo['svcname'] ;
               nodeNameList.push( { 'key': nodeName, 'value': nodeInfo } ) ;
            }
            else if( role == 'data' && nodeInfo['datagroupname'] == groupName )
            {
               var nodeName = nodeInfo['HostName'] + ':' + nodeInfo['svcname'] ;
               nodeNameList.push( { 'key': nodeName, 'value': nodeInfo } ) ;
            }
         } ) ;
         return nodeNameList ;
      } ) ;

      SdbSignal.on( 'GetAllNode', function(){
         var nodeNameList = [] ;
         $.each( $scope.NodeList, function( index, nodeInfo ){
            var nodeName = nodeInfo['HostName'] + ':' + nodeInfo['svcname'] ;
            nodeNameList.push( { 'key': nodeName + ' ' + ( nodeInfo['role'] == 'data' ? nodeInfo['datagroupname'] : nodeInfo['role'] ), 'value': nodeInfo } ) ;
         } ) ;
         return nodeNameList ;
      } ) ;

   } ) ;

}());