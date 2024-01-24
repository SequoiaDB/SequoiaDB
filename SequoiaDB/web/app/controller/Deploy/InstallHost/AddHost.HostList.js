//@ sourceURL=Deploy.AddHost.HostList.Ctrl.js
//"use strict" ;
(function(){
   var sacApp = window.SdbSacManagerModule ;
   //控制器
   sacApp.controllerProvider.register( 'Deploy.AddHost.HostList.Ctrl', function( $scope, SdbSwap, SdbSignal ){

      $scope.CurrentHost = 0 ;
      $scope.Search = { 'text': '' } ;
      $scope.HostInfoList = [] ;

      SdbSwap.hostList.then( function( result ){
         $scope.HostInfoList = result['HostList'] ;
      } ) ;

      SdbSignal.on( 'HostInfo', function( result ){
         $scope.HostInfoList = result ;
         $scope.SwitchHost( $scope.CurrentHost ) ;
         SdbSignal.commit( 'CheckedHostNum', null ) ;
      } ) ;

      //左侧栏的过滤
      $scope.Filter = function(){
         if( $scope.Search.text.length > 0 )
         {
            $.each( $scope.HostInfoList, function( index, hostInfo ){
               if( hostInfo['HostName'].indexOf( $scope.Search.text ) >= 0 )
               {
                  hostInfo['IsHostName'] = true ;
               }
               else
               {
                  hostInfo['IsHostName'] = false ;
               }
               if( hostInfo['IP'].indexOf( $scope.Search.text ) >= 0 )
               {
                  hostInfo['IsIP'] = true ;
               }
               else
               {
                  hostInfo['IsIP'] = false ;
               }
            } ) ;
         }
         else
         {
            $.each( $scope.HostInfoList, function( index, hostInfo ){
               hostInfo['IsHostName'] = false ;
               hostInfo['IsIP'] = false ;
            } ) ;
         }
      } ;

      //切换主机
      $scope.SwitchHost = function( index ){
         $scope.CurrentHost = index ;
         SdbSignal.commit( 'UpdateHostConfig', $scope.HostInfoList[index] ) ;
      }

      //检查主机
      $scope.CheckedHost = function( index ){
         if( typeof( $scope.HostInfoList[index]['errno'] ) == 'undefined' || $scope.HostInfoList[index]['errno'] == 0 )
         {
            if( $scope.HostInfoList[index]['CanUse'] == true )
            {
               $scope.HostInfoList[index]['IsUse'] = !$scope.HostInfoList[index]['IsUse'] ;
               SdbSignal.commit( 'CheckedHostNum', null ) ;
            }
         }
      }

   } ) ;

}());
