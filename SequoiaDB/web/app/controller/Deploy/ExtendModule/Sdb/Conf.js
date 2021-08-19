//@ sourceURL=Conf.js
//"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Deploy.Sdb.ExtendConf.Ctrl', function( $scope, $location, $rootScope, SdbRest, SdbFunction, SdbPromise, SdbSwap ){
      SdbSwap.templateDefer = SdbPromise.init( 1 ) ;
      SdbSwap.hostListDefer = SdbPromise.init( 1 ) ;
      SdbSwap.beforeExtendDefer = SdbPromise.init( 3 ) ;
      SdbSwap.afterExtendDefer = SdbPromise.init( 4 ) ;
      SdbSwap.replicaNumArr = SdbPromise.init( 2 ) ;
      SdbSwap.isLockExtendMode = false ;
      SdbSwap.extendMode = '' ;
      SdbSwap.templateInfo = {} ;

      SdbSwap.clusterName = $rootScope.tempData( 'Deploy', 'ClusterName' ) ;
      var extendMode      = $rootScope.tempData( 'Deploy', 'ExtendMode' ) ;
      var deployMod       = $rootScope.tempData( 'Deploy', 'DeployMod' ) ;
      $scope.ModuleName   = $rootScope.tempData( 'Deploy', 'ModuleName' ) ;
      if( SdbSwap.clusterName == null || $scope.ModuleName == null || deployMod != 'distribution' )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }
      if( extendMode == 'horizontal' )
      {
         SdbSwap.isLockExtendMode = true ;
      }

      //步骤条
      $scope.stepList = _Deploy.BuildSdbExtStep( $scope, $location, $scope['Url']['Action'], 'sequoiadb' ) ;
      if( $scope.stepList['info'].length == 0 )
      {
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      //初始化
      $scope.IsAllClear = false ;
      var hostList = [] ;

      SdbSwap.updatePreview = function(){}

      SdbSwap.setClear = function( isAllClear ){
         $scope.IsAllClear = isAllClear ;
      }

      //获取冗余等信息
      SdbSwap.predictCapacity = function( replica, dataGroupNum, numCatalog, numCoord, hostInfo, successCallback ){
         //当没有数据节点时做标记
         if( replica == 0 && dataGroupNum == 0 )
         {
            replica = '1' ;
            dataGroupNum = '1' ;
         }
         var predictInfo = {
            'ClusterName':  SdbSwap.clusterName,
            'BusinessName': $scope.ModuleName,
            'DeployMod':    deployMod,
            'BusinessType': 'sequoiadb',
            'Property': [
               { 'Name': 'coordnum',     'Value': numCoord + '' },
               { 'Name': 'catalognum',   'Value': numCatalog + '' },
               { 'Name': 'replicanum',   'Value': replica + '' },
               { 'Name': 'datagroupnum', 'Value': dataGroupNum + '' }
            ],
            'HostInfo': hostInfo
         } ;
         var data = { 'cmd': 'predict capacity', 'TemplateInfo': JSON.stringify( predictInfo ) } ;
         SdbRest.OmOperation( data, {
            'success': function( result ){
               var newResult = {} ;
               newResult['TotalSize']      = result[0]['TotalSize'] ;
               //当没有数据节点时，可用容量为总容量
               newResult['ValidSize']      = typeof(replica) == 'string' ? result[0]['TotalSize'] : result[0]['ValidSize'] ;
               newResult['RedundancySize'] = result[0]['TotalSize'] - result[0]['ValidSize'] ;
               newResult['Redundancy']     = result[0]['RedundancyRate'] * 100 ;
               successCallback( newResult ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  SdbSwap.predictCapacity( replica, dataGroupNum, numCatalog, numCoord, hostInfo, successCallback ) ;
                  return true ;
               } ) ;
            }
         } ) ;
      }

      //获取节点配置
      var getModuleConfig = function(){
         var data = { 'cmd': 'query node configure', 'filter': JSON.stringify( { 'ClusterName': SdbSwap.clusterName, 'BusinessName': $scope.ModuleName } ) } ;
         SdbRest.OmOperation( data, {
            'success': function( result ){
               SdbSwap.beforeExtendDefer.resolve( 'GroupList', result ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getModuleConfig() ;
                  return true ;
               } ) ;
            }
         } ) ;
      }
      
      //获取业务信息
      var getBusiness = function(){
         var data = { 
            'cmd': 'query business',
            'filter' : JSON.stringify( { 'BusinessName': $scope.ModuleName } )
         } ;
         SdbRest.OmOperation( data, {
            'success': function( result ){
               var hostNameList = [] ;
               $.each( result[0]['Location'], function( index, hostInfo ){
                  hostNameList.push( hostInfo['HostName'] ) ;
               } ) ;
               SdbSwap.beforeExtendDefer.resolve( 'BusinessHostList', { 'obj': result[0]['Location'], 'arr': hostNameList } ) ;
               SdbSwap.afterExtendDefer.resolve( 'BusinessHostList', hostNameList ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getBusiness() ;
                  return true ;
               } ) ;
            }
         }, {
            'showLoading': true
         } ) ;
      }
      
      //获取主机列表
      var getHostList = function(){
         var filter = { 'ClusterName': SdbSwap.clusterName } ;
         var select = { 'HostName': '', 'IP': '', 'Disk': [] } ;
         var data = { 'cmd': 'query host', 'filter': JSON.stringify( filter ), 'selector': JSON.stringify( select ) } ;
         SdbRest.OmOperation( data, {
            'success': function( result ){
               SdbSwap.hostListDefer.resolve( 'ClusterHostList', result ) ;
               SdbSwap.beforeExtendDefer.resolve( 'ClusterHostList', result ) ;
               SdbSwap.afterExtendDefer.resolve( 'ClusterHostList', result ) ;
               hostList = result ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getHostList() ;
                  return true ;
               } ) ;
            }
         }, {
            'showLoading': true
         } ) ;
      }

      //获取扩容配置模板
      var getBusinessTemplate = function(){
         var data = { 'cmd': 'get business template', 'BusinessType': 'sequoiadb', 'OperationType': 'extend' } ;
         SdbRest.OmOperation( data, {
            'success': function( result ){
               $.each( result, function( index ){
                  result[index]['Property'] = _Deploy.ConvertTemplate( result[index]['Property'] ) ;
               } ) ;
               SdbSwap.templateDefer.resolve( 'template', result ) ;
            },
            'failed': function( errorInfo ){
               _IndexPublic.createRetryModel( $scope, errorInfo, function(){
                  getBusinessTemplate() ;
                  return true ;
               } ) ;
            }
         }, {
            'showLoading': true
         } ) ;
      }

      getModuleConfig() ;
      getBusiness() ;
      getHostList() ;
      getBusinessTemplate() ;

      $scope.SizeConvert = function( size ){
         if( isNaN( size ) || size === '' )
         {
            return '' ;
         }
         return sizeConvert( size ) ;
      } ;

      //返回
      $scope.GotoDeploy = function(){
         $location.path( '/Deploy/Index' ).search( { 'r': new Date().getTime() } ) ;
      }

      //下一步
      $scope.GotoExtend = function(){
         if( $scope.IsAllClear == true )
         {
            //选择安装节点的主机
            SdbSwap.templateInfo['HostInfo'] = [] ;
            $.each( hostList, function( index, hostInfo ){
               if( hostInfo['checked'] == true )
               {
                  SdbSwap.templateInfo['HostInfo'].push( { 'HostName': hostInfo['HostName'] } ) ;
               }
            } ) ;
            $rootScope.tempData( 'Deploy', 'ModuleConfig', SdbSwap.templateInfo ) ;
            $location.path( '/Deploy/SDB-Extend' ).search( { 'r': new Date().getTime() } ) ;
         }
      }

   } ) ;

   sacApp.controllerProvider.register( 'Deploy.Sdb.ExtendConf.Windows.Ctrl', function( $scope, SdbRest, SdbSwap ){
      $scope.HostList = [] ;

      SdbSwap.hostListDefer.then( function( result ){
         $scope.HostList = result['ClusterHostList'] ;
         $.each( $scope.HostList, function( index, hostInfo ){
            $scope.HostList[index]['checked'] = true ;
         } )  ;
      } ) ;

      //选择安装业务的主机 弹窗
      $scope.SwitchHostWindow = {
         'config': {},
         'callback': {}
      } ;

      //打开 选择安装业务的主机 弹窗
      SdbSwap.ShowSwitchHost = function(){
         var tempHostList = $.extend( true, [], $scope.HostList ) ;
         $scope.SwitchHostWindow['callback']['SetOkButton']( $scope.autoLanguage( '确定' ), function(){
            SdbSwap.updatePreview() ;
            return true ;
         } ) ;
         $scope.SwitchHostWindow['callback']['SetCloseButton']( $scope.autoLanguage( '取消' ), function(){
            $.each( tempHostList, function( index ){
               $scope.HostList[index]['check'] = tempHostList[index]['check'] ;
            } ) ;
            return true ;
         } ) ;
         $scope.SwitchHostWindow['callback']['SetTitle']( $scope.autoLanguage( '主机列表' ) ) ;
         $scope.SwitchHostWindow['callback']['Open']() ;
      }

      //主机全选
      $scope.SelectAll = function(){
         $.each( $scope.HostList, function( index ){
            $scope.HostList[index]['checked'] = true ;
         } ) ;
      }

      //主机反选
      $scope.Unselect = function(){
         $.each( $scope.HostList, function( index ){
            $scope.HostList[index]['checked'] = !$scope.HostList[index]['checked'] ;
         } ) ;
      }

   } ) ;

   sacApp.controllerProvider.register( 'Deploy.Sdb.ExtendConf.Template.Ctrl', function( $scope, $timeout, SdbSwap ){
      $scope.ReplicaTips = false ;
      $scope.MaxReplicaNum = 0 ;
      $scope.MinReplicaNum = 0 ;
      var deployMode = '' ;
      SdbSwap.replicaNumArr.then( function( replicaNumResult ){
         if( replicaNumResult['max'] > replicaNumResult['min'])
         {
            $scope.ReplicaTips = true ;
            $scope.MaxReplicaNum = replicaNumResult['max'] ;
            $scope.MinReplicaNum = replicaNumResult['min'] ;
         }
         
      } ) ;
      var templateList = [] ;

      $scope.ShowSwitchHost = function(){
         SdbSwap.ShowSwitchHost() ;
      } ;

      //左侧表单
      $scope.ConfForm2 = { 'inputList': [] } ;
      $scope.ConfForm1 = {
         'inputList': [
            {
               "name": "type",
               "webName": $scope.autoLanguage( '扩容模式' ),
               "type": "select",
               "disabled": false,
               "value": "",
               "valid": [],
               "onChange": function( name, key, value ){   

                  var deployMode = value ;
                  SdbSwap.extendMode = value ;

                  $.each( templateList, function( index, template ){
                     if( template['DeployMod'] == deployMode )
                     {
                        $.each( template['Property'], function( index2 ){
                           template['Property'][index2]['onChange'] = function(){
                              SdbSwap.updatePreview() ;
                           }
                        } ) ;
                        $scope.ConfForm2 = { 'inputList': template['Property'] } ;
                        if( template['DeployMod'] == 'vertical' && $scope.MaxReplicaNum > $scope.MinReplicaNum )
                        {
                           $scope.ConfForm2['inputList'][2]['value'] = $scope.MaxReplicaNum - $scope.MinReplicaNum + '' ;
                        }
                        return false ;
                     }
                  } ) ;

                  $timeout( SdbSwap.updatePreview, 0, false ) ;
               }
            }
         ]
      } ;

      SdbSwap.templateDefer.then( function( result ){

         templateList = result['template'] ;
         $scope.ConfForm1['inputList'][0]['valid'] = [] ;
         $.each( templateList, function( index, parameter ){
            if( index == 0 )
            {
               deployMode = parameter['DeployMod'] ;
               $scope.ConfForm1['inputList'][0]['value'] = parameter['DeployMod'] ;
               SdbSwap.extendMode = parameter['DeployMod'] ;

               $.each( parameter['Property'], function( index2, property ){
                  if( property['name'] == 'replicanum' )
                  {
                     SdbSwap.replicaNumArr.then( function( replicaNumResult ){
                        //当没有数据节点时，将默认副本数设置为3
                        parameter['Property'][index2]['value'] = ( replicaNumResult['max'] > 0 ? replicaNumResult['max'] : 3 ) + '' ;
                     } ) ;
                  }
                  parameter['Property'][index2]['onChange'] = function(){
                     SdbSwap.updatePreview() ;
                  }
               } ) ;
               $scope.ConfForm2['inputList'] = parameter['Property'] ;
            }
            $scope.ConfForm1['inputList'][0]['valid'].push( { 'key': parameter['WebName'], 'value': parameter['DeployMod'] } ) ;   
         } ) ;

         //设置如果从分区组列表进入扩容的话，只能添加分区组
         if( SdbSwap.isLockExtendMode )
         {
            $scope.ConfForm1['inputList'][0]['disabled'] = true ;
         }
        
         $timeout( function(){
            SdbSwap.afterExtendDefer.resolve( 'form', [ $scope.ConfForm1, $scope.ConfForm2 ] ) ;
         }, 0, false ) ;
         
      } ) ;

   } ) ;

   sacApp.controllerProvider.register( 'Deploy.Sdb.ExtendConf.Preview.Ctrl', function( $scope, $timeout, SdbRest, SdbPromise, SdbSwap ){

      //扇形图
      $scope.RedundancyChart1 = {} ;
      $scope.RedundancyChart1['options'] = $.extend( true, {}, window.SdbSacManagerConf.ExtendRedundancyChart ) ;
      $scope.RedundancyChart1['options']['title']['text']     = $scope.autoLanguage( '容量信息' ) ;
      $scope.RedundancyChart1['options']['legend']['data'][0] = $scope.autoLanguage( '可用容量' ) ;
      $scope.RedundancyChart1['options']['legend']['data'][1] = $scope.autoLanguage( '冗余容量' ) ;
      $scope.RedundancyChart1['options']['series'][0]['name'] = $scope.autoLanguage( '总容量' ) ;
      $scope.RedundancyChart1['options']['series'][0]['data'][0]['name'] = $scope.autoLanguage( '可用容量' ) ;
      $scope.RedundancyChart1['options']['series'][0]['data'][1]['name'] = $scope.autoLanguage( '冗余容量' ) ;

      $scope.RedundancyChart2 = {} ;
      $scope.RedundancyChart2['options'] = $.extend( true, {}, window.SdbSacManagerConf.ExtendRedundancyChart ) ;
      $scope.RedundancyChart2['options']['title']['text']     = $scope.autoLanguage( '容量信息' ) ;
      $scope.RedundancyChart2['options']['legend']['data'][0] = $scope.autoLanguage( '可用容量' ) ;
      $scope.RedundancyChart2['options']['legend']['data'][1] = $scope.autoLanguage( '冗余容量' ) ;
      $scope.RedundancyChart2['options']['series'][0]['name'] = $scope.autoLanguage( '总容量' ) ;
      $scope.RedundancyChart2['options']['series'][0]['data'][0]['name'] = $scope.autoLanguage( '可用容量' ) ;
      $scope.RedundancyChart2['options']['series'][0]['data'][1]['name'] = $scope.autoLanguage( '冗余容量' ) ;

      $scope.IsNaN = isNaN ;

      //扩容前配置
      $scope.BeforeExtend = {} ;

      //扩容后配置
      $scope.AfterExtend  = {} ;

      var form1, form2 ;
      var clusterHostList = [] ;
      var businessHostList = [] ;

      var updatePreview = function(){
         var isAllClear = form2.check() ;
         if( isAllClear )
         {
            var extendMod = form1.getValue() ;
            var formVal = form2.getValue() ;

            if( $.isEmptyObject( formVal ) )
            {
               $timeout( updatePreview, 100, false ) ;
               return ;
            }

            var diskTotalSize = $scope.AfterExtend['DiskTotalSize'] ;
            var diskValidSize = $scope.AfterExtend['DiskValidSize'] ;
            var diskRedundancySize = $scope.AfterExtend['DiskRedundancySize'] ;
            var diskRedundancy = $scope.AfterExtend['DiskRedundancy'] ;
            $scope.AfterExtend = $.extend( true, {}, $scope.BeforeExtend ) ;
            $scope.AfterExtend['DiskTotalSize'] = diskTotalSize ;
            $scope.AfterExtend['DiskValidSize'] = diskValidSize ;
            $scope.AfterExtend['DiskRedundancySize'] = diskRedundancySize ;
            $scope.AfterExtend['DiskRedundancy'] = diskRedundancy ;

            if( extendMod['type'] == 'horizontal' )
            {
               $scope.AfterExtend['NumDataGroups'] += formVal['datagroupnum'] ;
               $scope.AfterExtend['NumData']       += ( formVal['datagroupnum'] * parseInt( formVal['replicanum'] ) ) ;
               $scope.AfterExtend['TotalNodes']    = $scope.AfterExtend['NumCoord'] + $scope.AfterExtend['NumCatalog'] + $scope.AfterExtend['NumData'] ;
               $scope.AfterExtend['AverageReplica'] = Math.round( $scope.AfterExtend['NumData'] / $scope.AfterExtend['NumDataGroups'] ) ;

               if( parseInt( formVal['replicanum'] ) > $scope.AfterExtend['NumReplicaMax'] )
               {
                  $scope.AfterExtend['NumReplicaMax'] = formVal['replicanum'] ;
               }
               else if( parseInt( formVal['replicanum'] ) < $scope.AfterExtend['NumReplicaMin']  )
               {
                  $scope.AfterExtend['NumReplicaMin'] = formVal['replicanum'] ;
               }
            }
            else if( extendMod['type'] == 'vertical' )
            {
               $scope.AfterExtend['NumCoord']      += formVal['coordnum'] ;
               $scope.AfterExtend['NumCatalog']    += parseInt( formVal['catalognum'] ) ;
               $scope.AfterExtend['NumReplicaMin'] += parseInt( formVal['replicanum'] ) ;

               $scope.AfterExtend['NumCatalog']    = $scope.AfterExtend['NumCatalog'] > 7 ? 7 : $scope.AfterExtend['NumCatalog'] ;
               $scope.AfterExtend['NumReplicaMin'] = $scope.AfterExtend['NumReplicaMin'] > 7 ? 7 : $scope.AfterExtend['NumReplicaMin'] ;
               $scope.AfterExtend['NumReplicaMax'] = $scope.AfterExtend['NumReplicaMin'] >= $scope.AfterExtend['NumReplica'] ? $scope.AfterExtend['NumReplicaMin'] : $scope.AfterExtend['NumReplica'] ;
               $scope.AfterExtend['NumReplica']    = $scope.AfterExtend['NumReplicaMin'] ;

               $scope.AfterExtend['NumData'] = 0 ;
               $.each( $scope.AfterExtend['replicaNumArr'], function( index, replicaNum ){
                  if( replicaNum < $scope.AfterExtend['NumReplica'] )
                  {
                     $scope.AfterExtend['replicaNumArr'][index] = $scope.AfterExtend['NumReplica'] ;
                  }
                  $scope.AfterExtend['NumData'] += $scope.AfterExtend['replicaNumArr'][index] ;
               } ) ;
               $scope.AfterExtend['AverageReplica'] = Math.round( $scope.AfterExtend['NumData'] / $scope.AfterExtend['NumDataGroups'] ) ;
               $scope.AfterExtend['TotalNodes'] = $scope.AfterExtend['NumCoord'] + $scope.AfterExtend['NumCatalog'] + $scope.AfterExtend['NumData'] ;

            }
            var hostList = [] ;
            var diskNum = 0 ;
            $.each( clusterHostList, function( index, hostInfo ){
               if( hostInfo['checked'] == true || businessHostList.indexOf( hostInfo['HostName'] ) >= 0 )
               {
                  hostList.push( { 'HostName': hostInfo['HostName'] } ) ;
                  $.each( hostInfo['Disk'], function( index2, diskInfo ){
                     if( diskInfo['IsLocal'] == true )
                     {
                        ++diskNum ;
                     }
                  } ) ;
               }
            } ) ;

            $scope.AfterExtend['NumHosts'] = hostList.length ;
            $scope.AfterExtend['TotalDisks'] = diskNum ;

            //计算节点分布率
            $scope.AfterExtend['NodeSpread'] = twoDecimalPlaces( $scope.AfterExtend['TotalNodes'] / $scope.AfterExtend['TotalDisks'] ) ;
	         $scope.AfterExtend['NodeSpread'] = $scope.AfterExtend['NodeSpread'] < 1 ? 1 : $scope.AfterExtend['NodeSpread'] ;
            SdbSwap.predictCapacity( $scope.AfterExtend['AverageReplica'], $scope.AfterExtend['NumDataGroups'],
                                     $scope.AfterExtend['NumCatalog'], $scope.AfterExtend['NumCoord'],
                                     hostList,
                                     function( result ){  
                                        $scope.AfterExtend['DiskTotalSize']      = result['TotalSize'] ;
                                        $scope.AfterExtend['DiskValidSize']      = result['ValidSize'] ;
                                        $scope.AfterExtend['DiskRedundancySize'] = result['RedundancySize'] ;
                                        $scope.AfterExtend['DiskRedundancy']     = result['Redundancy'] ;

                                        $scope.RedundancyChart2 = {} ;
                                        $scope.RedundancyChart2['options'] = $.extend( true, {}, window.SdbSacManagerConf.ExtendRedundancyChart ) ;
                                        $scope.RedundancyChart2['options']['title']['text']     = $scope.autoLanguage( '容量信息' ) ;
                                        $scope.RedundancyChart2['options']['legend']['data'][0] = $scope.autoLanguage( '可用容量' ) ;
                                        $scope.RedundancyChart2['options']['legend']['data'][1] = $scope.autoLanguage( '冗余容量' ) ;
                                        $scope.RedundancyChart2['options']['series'][0]['name'] = $scope.autoLanguage( '总容量' ) + result['TotalSize'] ;
                                        $scope.RedundancyChart2['options']['series'][0]['data'][0]['name'] = $scope.autoLanguage( '可用容量' ) ;
                                        $scope.RedundancyChart2['options']['series'][0]['data'][1]['name'] = $scope.autoLanguage( '冗余容量' ) ;
                                        $scope.RedundancyChart2['options']['series'][0]['data'][0]['value'] = parseInt( result['ValidSize'] ) ;
                                        $scope.RedundancyChart2['options']['series'][0]['data'][1]['value'] = parseInt( result['RedundancySize'] ) ;

                                        if( SdbSwap.extendMode == 'horizontal' )
                                        {
                                           SdbSwap.templateInfo = {
                                              'ClusterName':  SdbSwap.clusterName,
                                              'BusinessName': $scope.ModuleName,
                                              'DeployMod':    SdbSwap.extendMode,
                                              'BusinessType': 'sequoiadb',
                                              'Property': [
                                                 { 'Name': 'replicanum',   'Value': formVal['replicanum'] },
                                                 { 'Name': 'datagroupnum', 'Value': formVal['datagroupnum'] + '' }
                                              ]
                                           } ;
                                        }
                                        else if( SdbSwap.extendMode == 'vertical' )
                                        {
                                           SdbSwap.templateInfo = {
                                              'ClusterName':  SdbSwap.clusterName,
                                              'BusinessName': $scope.ModuleName,
                                              'DeployMod':    SdbSwap.extendMode,
                                              'BusinessType': 'sequoiadb',
                                              'Property': [
                                                 { 'Name': 'coordnum',     'Value': $scope.AfterExtend['NumCoord'] + '' },
                                                 { 'Name': 'catalognum',   'Value': $scope.AfterExtend['NumCatalog'] + '' },
                                                 { 'Name': 'replicanum',   'Value': $scope.AfterExtend['NumReplica'] + '' }
                                              ]
                                           } ;
                                        }

                                        SdbSwap.setClear( true ) ;
                                     } ) ;
         }
         else
         {
            SdbSwap.setClear( isAllClear ) ;
         }
      }

      //统计总磁盘数
      var getDiskCount = function( hostList, filterHostList ){
         var diskNum = 0 ;
         $.each( hostList, function( index, hostInfo ){
            if( filterHostList.indexOf( hostInfo['HostName'] ) == -1 )
            {
               return true ;
            }
            $.each( hostInfo['Disk'], function( index2, diskInfo ){
               if( diskInfo['IsLocal'] == true )
               {
                  ++diskNum ;
               }
            } ) ;
         } ) ;
         return diskNum ;
      }

      SdbSwap.beforeExtendDefer.then( function( result ){

         //统计扩容前集群信息
         var coordNum = 0 ;
         var cataNum  = 0 ;
         var dataNum  = 0 ;
         var dataGroupNum = 0 ;
         var minReplicaNum   = 0 ;
         var maxReplicaNum   = 0 ;

         var tempReplicaNumArr = [] ;
         var groups = {} ;
         $.each( result['GroupList'], function( index, hostInfo  ){
            $.each( hostInfo['Config'], function( index2, nodeInfo ){
               if( nodeInfo['role'] == 'data' )
               {
                  if( typeof( groups[nodeInfo['datagroupname']] ) == 'undefined' )
                  {
                     groups[nodeInfo['datagroupname']] = 1 ;
                  }
                  else
                  {
                     ++ groups[nodeInfo['datagroupname']] ;
                  }
                  ++ dataNum ;
               }
               else if( nodeInfo['role'] == 'coord' )
               {
                  ++ coordNum ;
               }
               else
               {
                  ++ cataNum ;
               }
               
            } ) ;
         } ) ;
         $.each( groups, function( index, num ){
            tempReplicaNumArr.push( num ) ;
            ++ dataGroupNum ;
         } ) ;

         //当没有数据节点时，最大最小副本数皆为0
         minReplicaNum = dataGroupNum == 0 ? 0 : Math.min.apply( null, tempReplicaNumArr ) ;
         maxReplicaNum = dataGroupNum == 0 ? 0 : Math.max.apply( null, tempReplicaNumArr ) ;
         SdbSwap.replicaNumArr.resolve( 'max', maxReplicaNum ) ;
         SdbSwap.replicaNumArr.resolve( 'min', minReplicaNum ) ;

         $scope.BeforeExtend = {
            'NumHosts':       result['BusinessHostList']['arr'].length,
            'TotalNodes':     coordNum + cataNum + dataNum,
            'TotalDisks':     getDiskCount( result['ClusterHostList'], result['BusinessHostList']['arr'] ),
            'NodeSpread':     '',
            'NumDataGroups':  dataGroupNum,
            'NumReplica':     maxReplicaNum,
            'NumReplicaMin':  minReplicaNum,
            'NumReplicaMax':  maxReplicaNum,
            'NumCoord':       coordNum,
            'NumCatalog':     cataNum,
            'NumData':        dataNum,
            'replicaNumArr': tempReplicaNumArr
         } ;
         //当没有数据节点时，平均副本数设置为0，设置只能添加分区组
         if( dataNum == 0 && dataGroupNum == 0 )
         {
            $scope.BeforeExtend['AverageReplica'] = 0 ;
            SdbSwap.isLockExtendMode = true ;
         }
         else
         {
            $scope.BeforeExtend['AverageReplica'] = Math.round( $scope.BeforeExtend['NumData'] / $scope.BeforeExtend['NumDataGroups'] ) ;
         }
         //计算节点分布率
         $scope.BeforeExtend['NodeSpread'] = twoDecimalPlaces( $scope.BeforeExtend['TotalNodes'] / $scope.BeforeExtend['TotalDisks'] ) ;
	      $scope.BeforeExtend['NodeSpread'] = $scope.BeforeExtend['NodeSpread'] < 1 ? 1 : $scope.BeforeExtend['NodeSpread'] ;
         //获取磁盘冗余
         $timeout( function(){
            SdbSwap.predictCapacity( $scope.BeforeExtend['AverageReplica'], $scope.BeforeExtend['NumDataGroups'],
                                     $scope.BeforeExtend['NumCatalog'], $scope.BeforeExtend['NumCoord'],
                                     result['BusinessHostList']['obj'],
                                     function( result ){  
                                        $scope.BeforeExtend['DiskTotalSize']      = result['TotalSize'] ;
                                        $scope.BeforeExtend['DiskValidSize']      = result['ValidSize'] ;
                                        $scope.BeforeExtend['DiskRedundancySize'] = result['RedundancySize'] ;
                                        $scope.BeforeExtend['DiskRedundancy']     = result['Redundancy'] ;
                                     
                                        $scope.RedundancyChart1 = {} ;
                                        $scope.RedundancyChart1['options'] = $.extend( true, {}, window.SdbSacManagerConf.ExtendRedundancyChart ) ;
                                        $scope.RedundancyChart1['options']['title']['text']     = $scope.autoLanguage( '容量信息' ) ;
                                        $scope.RedundancyChart1['options']['legend']['data'][0] = $scope.autoLanguage( '可用容量' ) ;
                                        $scope.RedundancyChart1['options']['legend']['data'][1] = $scope.autoLanguage( '冗余容量' ) ;
                                        $scope.RedundancyChart1['options']['series'][0]['name'] = $scope.autoLanguage( '总容量' ) + result['TotalSize'] ;
                                        $scope.RedundancyChart1['options']['series'][0]['data'][0]['name'] = $scope.autoLanguage( '可用容量' ) ;
                                        $scope.RedundancyChart1['options']['series'][0]['data'][1]['name'] = $scope.autoLanguage( '冗余容量' ) ;
                                        $scope.RedundancyChart1['options']['series'][0]['data'][0]['value'] = parseInt( result['ValidSize'] ) ;
                                        $scope.RedundancyChart1['options']['series'][0]['data'][1]['value'] = parseInt( result['RedundancySize'] ) ;
                                     } ) ;
         }, 500, false ) ;

         SdbSwap.afterExtendDefer.resolve( 'config', $scope.BeforeExtend ) ;
      } ) ;

      SdbSwap.afterExtendDefer.then( function( result ){
         form1 = result['form'][0] ;
         form2 = result['form'][1] ;
         clusterHostList = result['ClusterHostList'] ;
         businessHostList = result['BusinessHostList'] ;
         SdbSwap.updatePreview = updatePreview ;
         $timeout( SdbSwap.updatePreview, 0, false ) ;
      } ) ;

   } ) ;

}());