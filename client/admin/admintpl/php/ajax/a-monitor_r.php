<?php

$model      = empty ( $_POST['model'] ) ? "" : $_POST['model'] ;
$groupname  = empty ( $_POST['group'] ) ? "" : $_POST['group'] ;
$host       = empty ( $_POST['host']  ) ? "" : $_POST['host'] ;
$svc        = empty ( $_POST['svc']   ) ? "" : $_POST['svc'] ;

//$db -> install ( "{ install : false }" ) ;
$cursor = NULL ;
if ( $model == "all" )
{
	$cursor = $db -> getSnapshot ( SDB_SNAP_DATABASE, NULL, '{TotalNumConnects:1,TotalDataRead:1,TotalIndexRead:1,TotalDataWrite:1,TotalIndexWrite:1,TotalUpdate:1,TotalDelete:1,TotalInsert:1,ReplUpdate:1,ReplDelete:1,ReplInsert:1,TotalSelect:1,TotalRead:1}' ) ;
}
else if ( $model == "group" )
{
	$cursor = $db -> getSnapshot ( SDB_SNAP_DATABASE, '{ GroupName: "'.$groupname.'" }' ) ;
}
else if ( $model == "node" )
{
	$cursor = $db -> getSnapshot ( SDB_SNAP_DATABASE, '{ HostName: "'.$host.'", svcname: "'.$svc.'" }' ) ;
}
if ( !empty ( $cursor ) )
{
	while ( $arr = $cursor -> getNext() )
	{
	   $arr['TotalInsert'] = $arr['TotalInsert'] - $arr['ReplInsert'] ;
	   $arr['TotalDelete'] = $arr['TotalDelete'] - $arr['ReplDelete'] ;
	   $arr['TotalUpdate'] = $arr['TotalUpdate'] - $arr['ReplUpdate'] ;
		echo json_encode( $arr ) ;
	}
}
//$db -> install ( "{ install : true }" ) ;

?>