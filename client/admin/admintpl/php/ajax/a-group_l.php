<?php

$isfirst = true ;
$isfirst2 = true ;
$group_list = '' ;
$errnode = array() ;

$cursor = $db -> getSnapshot ( 7, '{GroupID:{$gt:0}}', '{ErrNodes:1}' ) ;
if ( !empty ( $cursor ) )
{
	if ( $arr = $cursor -> getNext() )
	{
		$errnode = $arr['ErrNodes'] ;
	}
}

/*$cursor = $db -> getSnapshot ( SDB_SNAP_DATABASE, '{GroupName:"SYSCatalogGroup"}', '{ErrNodes:1}' ) ;
if ( !empty ( $cursor ) )
{
	if ( $arr = $cursor -> getNext() )
	{
		$errnode = $arr['ErrNodes'] ;
	}
}*/

$db -> install ( "{ install : false }" ) ;

$cursor = $db -> getList ( SDB_LIST_GROUPS ) ;
if ( !empty ( $cursor ) )
{
	$group_list = '{"name":"数据库","name1":"数据统计","name2":"性能监控","child":{"name":"分区组","child":[' ;
	while ( $arr = $cursor -> getNext() )
	{
		if ( !$isfirst )
		{
			$group_list .= "," ;
		}
		else
		{
			$isfirst = false ;
		}
		//$arr = str_replace( '\\', '\\\\', $arr ) ;
		$group_list .= $arr ;
	}
	$group_list .= ']}' ;
	if ( count( $errnode ) > 0 )
	{
		$group_list .= ',"ErrNodes":[' ;
		foreach ( $errnode as $child )
		{
			if ( !$isfirst2 )
			{
				$group_list .= "," ;
			}
			else
			{
				$isfirst2 = false ;
			}
			$group_list .= '"'.$child['NodeName'].'"' ;
		}
		$group_list .= ']' ;
	}
	$group_list .= '}' ;
}
$db -> install ( "{ install : true }" ) ;

$smarty -> assign( "group_list", $group_list ) ;

?>