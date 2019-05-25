//@ sourceURL=Network.js
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Performance.Network.Index.Ctrl', function( $scope, $compile, $location, SdbRest, SdbFunction ){
      
      _IndexPublic.checkMonitorEdition( $location ) ; //检测是不是企业版

      var clusterName = SdbFunction.LocalData( 'SdbClusterName' ) ;
      var moduleType = SdbFunction.LocalData( 'SdbModuleType' ) ;
      var moduleName = SdbFunction.LocalData( 'SdbModuleName' ) ;
      var moduleMode = SdbFunction.LocalData( 'SdbModuleMode' ) ;
      var hostName = SdbFunction.LocalData( 'SdbHostName' ) ;
      if( clusterName == null || moduleType != 'sequoiadb' || moduleMode == null || moduleName == null )
      {
         $location.path( '/Transfer' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      if( hostName == null )
      {
         $location.path( '/Monitor/SDB-Host/List/Index' ).search( { 'r': new Date().getTime() } ) ;
         return ;
      }

      //初始化
      //主机状态
      $scope.HostStatus = null ;
      //主机名
      $scope.hostName = hostName ;
      //网卡列表
      $scope.NetList = [] ;

      //获取网卡信息
      var getNetworkInfo = function(){
         var isFirstBuild = true ;
         var lastNetInfo = [] ;
         var data = {
            'cmd': 'query host status',
            'HostInfo': JSON.stringify( { 'HostInfo': [ { 'HostName': hostName } ] } )
         } ;
         SdbRest.OmOperation( data, {
            'success': function( hostList ){
               if( hostList.length > 0 )
               {
                  var netList = [] ;
                  $.each( hostList[0]['HostInfo'], function( Index, hostInfo ){
                     if( isNaN( hostInfo['errno'] ) == false && hostInfo['errno'] != 0 )
                     {
                        $scope.HostStatus = false ;
                        return true ;
                     }
                     $scope.HostStatus = true ;

                        $.each( hostInfo['Net']['Net'], function( index2, networkInfo ){
                           var rxPackets = networkInfo['RXPackets']['Megabit'] * 1024 * 1024 + networkInfo['RXPackets']['Unit'] ;
                           var txPackets = networkInfo['TXPackets']['Megabit'] * 1024 * 1024 + networkInfo['TXPackets']['Unit'] ;
                           var rxBytes   = fixedNumber( networkInfo['RXBytes']['Megabit']   + ( networkInfo['RXBytes']['Unit'] / 1024 / 1024 ), 2 ) ;
                           var txBytes   = fixedNumber( networkInfo['TXBytes']['Megabit']   + ( networkInfo['TXBytes']['Unit'] / 1024 / 1024 ), 2 ) ;
                           var rxBytes2  = networkInfo['RXBytes']['Megabit'] * 1024 + ( networkInfo['RXBytes']['Unit'] ) / 1024 ;
                           var txBytes2  = networkInfo['TXBytes']['Megabit'] * 1024 + ( networkInfo['TXBytes']['Unit'] ) / 1024 ;
                           if( isFirstBuild == true )
                           {
                              netList.push( {
                                 'Name': networkInfo['Name'],
                                 'Wirespeed': '-',
                                 'RXBytes':   sizeConvert( rxBytes ),
                                 'RXPackets': rxPackets,
                                 'TXBytes':   sizeConvert( txBytes ),
                                 'TXPackets': txPackets,
                                 'RXChart': { 'options': window.SdbSacManagerConf.NetwordInEchart },
                                 'TXChart': { 'options': window.SdbSacManagerConf.NetwordOutEchart }
                              } ) ;
                              netList[index2]['TXChart']['value'] = [ [ 0, 0, true, false ] ] ;
                              netList[index2]['RXChart']['value'] = [ [ 0, 0, true, false ] ] ;
                              lastNetInfo.push( { 'TXBytes': txBytes2, 'RXBytes': rxBytes2 } ) ;
                           }
                           else
                           {
                              netList = $scope.NetList ;
                              netList[index2]['RXBytes'] = sizeConvert( rxBytes ) ;
                              netList[index2]['TXBytes'] = sizeConvert( txBytes ) ;
                              netList[index2]['RXPackets'] = rxPackets ;
                              netList[index2]['TXPackets'] = txPackets ;
                              var avgTX = fixedNumber( ( txBytes2 - lastNetInfo[index2]['TXBytes'] ) / 5, 2 ) ;
                              var avgRX = fixedNumber( ( rxBytes2 - lastNetInfo[index2]['RXBytes'] ) / 5, 2 ) ;
                              avgTX = avgTX < 0 ? 0 : avgTX ;
                              avgRX = avgRX < 0 ? 0 : avgRX ;
                              netList[index2]['TXChart']['value'] = [ [ 0, avgTX, true, false ] ] ;
                              netList[index2]['RXChart']['value'] = [ [ 0, avgRX, true, false ] ] ;
                              lastNetInfo[index2]['TXBytes'] = txBytes2 ;
                              lastNetInfo[index2]['RXBytes'] = rxBytes2 ;
                           }
                        } ) ;

                  } ) ;
                  isFirstBuild = false ;
                  $scope.NetList = netList ;
               }
            }
         }, {
            'showLoading': false,
            'delay': 5000,
            'loop': true
         } ) ;
      }

      getNetworkInfo() ;

      //跳转至资源
      $scope.GotoResources = function(){
         $location.path( '/Monitor/SDB-Resources/Session' ).search( { 'r': new Date().getTime() } ) ;
      } ;

      //跳转至主机列表
      $scope.GotoHosts = function(){
         $location.path( '/Monitor/SDB-Host/List/Index' ).search( { 'r': new Date().getTime() } ) ;
      } ;
      
      
      //跳转至节点列表
      $scope.GotoNodes = function(){
         if( moduleMode == 'distribution' )
         {
            $location.path( '/Monitor/SDB-Nodes/Nodes' ).search( { 'r': new Date().getTime() } ) ;
         }
         else
         {
            $location.path( '/Monitor/SDB-Nodes/Node/Index' ).search( { 'r': new Date().getTime() } ) ;
         }
      } ;
      
   } ) ;

}());