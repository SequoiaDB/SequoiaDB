<?php

$type        = empty ( $_POST['type']        ) ? ""   :  $_POST['type']        ;
$groupname   = empty ( $_POST['groupname']   ) ? ""   :  $_POST['groupname']   ;
$nodename    = empty ( $_POST['nodename']    ) ? ""   :  $_POST['nodename']    ;
$hostname    = empty ( $_POST['hostname']    ) ? ""   :  $_POST['hostname']    ;
$serivcename = empty ( $_POST['serivcename'] ) ? ""   :  $_POST['serivcename'] ;
$databasepath= empty ( $_POST['databasepath']) ? ""   :  $_POST['databasepath'];
$config      = empty ( $_POST['config']      ) ? NULL :  $_POST['config']      ;

$rc = 0 ;
if ( $type == "creategroup" )
{
	$group = $db -> createGroup ( $groupname ) ;
	if ( empty ( $group ) )
	{
		$arr = $db -> getError() ;
		$rc = $arr["errno"] ;
	}
}
else if ( $type == "createcatalog" )
{
	$arr = $db -> createCataGroup ( $hostname, $serivcename, $databasepath, $config ) ;
	$rc = $arr["errno"] ;
}
else if ( $type == "createnode" )
{
	$group = $db -> selectGroup ( $groupname ) ;
	if ( !empty ( $group ) )
	{
		$arr = $group -> createNode ( $hostname, $serivcename, $databasepath, $config ) ;
		$rc = $arr["errno"] ;
	}
	else
	{
		$arr = $db -> getError() ;
		$rc = $arr["errno"] ;
	}
}
else if ( $type == "startgroup" )
{
	$group = $db -> selectGroup ( $groupname ) ;
	if ( !empty ( $group ) )
	{
		$arr = $group -> start() ;
		$rc = $arr["errno"] ;
	}
	else
	{
		$arr = $db -> getError() ;
		$rc = $arr["errno"] ;
	}
}
else if ( $type == "stopgroup" )
{
	$group = $db -> selectGroup ( $groupname ) ;
	if ( !empty ( $group ) )
	{
		$arr = $group -> stop() ;
		$rc = $arr["errno"] ;
	}
	else
	{
		$arr = $db -> getError() ;
		$rc = $arr["errno"] ;
	}
}
else if ( $type == "startnode" )
{
	$group = $db -> selectGroup ( $groupname ) ;
	if ( !empty ( $group ) )
	{
		$node = $group -> getNode( $nodename ) ;
		if ( !empty ( $node ) )
		{
			$arr = $node -> start() ;
			$rc = $arr["errno"] ;
		}
		else
		{
			$arr = $db -> getError() ;
			$rc = $arr["errno"] ;
		}
	}
	else
	{
		$arr = $db -> getError() ;
		$rc = $arr["errno"] ;
	}
}
else if ( $type == "stopnode" )
{
	$group = $db -> selectGroup ( $groupname ) ;
	if ( !empty ( $group ) )
	{
		$node = $group -> getNode( $nodename ) ;
		if ( !empty ( $node ) )
		{
			$arr = $node -> stop() ;
			$rc = $arr["errno"] ;
		}
		else
		{
			$arr = $db -> getError() ;
			$rc = $arr["errno"] ;
		}
	}
	else
	{
		$arr = $db -> getError() ;
		$rc = $arr["errno"] ;
	}
}

$smarty -> assign( "group_type", $type ) ;
$smarty -> assign( "group_rc", $rc ) ;
$smarty -> assign( "group_respond", array_key_exists( $rc, $errno_cn ) ? $errno_cn[$rc] : $rc ) ;
?>