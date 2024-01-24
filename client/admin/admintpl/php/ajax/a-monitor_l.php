<?php

$isfirst = true ;
$group_list = '' ;
$db -> install ( "{ install : false }" ) ;
$cursor = $db -> getList ( SDB_LIST_GROUPS ) ;
if ( !empty ( $cursor ) )
{
	$group_list = '{"name":"数据库","child":{"name1":"数据统计","name2":"性能监控","name3":"分区组","child":[' ;
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
		$group_list .= $arr ;
	}
	$group_list .= ']}}' ;
}
else
{
	$group_list = '{"name":"数据库","child":{"name1":"数据统计","name2":"性能监控"}}' ;
}
$db -> install ( "{ install : true }" ) ;

$smarty -> assign( "group_list", $group_list ) ;

?>