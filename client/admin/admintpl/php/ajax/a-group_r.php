<?php

$groupname  = empty ( $_POST['group'] ) ? "" : $_POST['group'] ;
$hostname  = empty ( $_POST['hostname'] ) ? "" : $_POST['hostname'] ;
$nodename   = empty ( $_POST['node'] )  ? "" : $_POST['node'] ;

$isgroupnode = 0 ;
$group_list = array() ;
$node_list = array() ;

if ( $groupname != "" && $nodename == "" )
{
	$isgroupnode = 1 ;
	$cursor = $db -> getList ( SDB_LIST_GROUPS, '{GroupName:"'.$groupname.'"}' ) ;
	if ( !empty ( $cursor ) )
	{
		while ( $arr = $cursor -> getNext() )
		{
			$group_list = $arr ;
		}
	}
}
else if ( $groupname != "" && $nodename != "" )
{
	$isgroupnode = 2 ;
	$cursor = $db -> getList ( SDB_LIST_GROUPS, '{GroupName:"'.$groupname.'"}', '{Group:1}' ) ;
	if ( !empty ( $cursor ) )
	{
		while ( $arr = $cursor -> getNext() )
		{
			foreach ( $arr["Group"] as $child )
			{
				if ( $child["HostName"] == $hostname )
				{
					foreach ( $child["Service"] as $child_child )
					{
						if ( $nodename == $child_child["Name"] && $child_child["Type"] == 0 )
						{
							$node_list = $child ;
						}
					}
				}
			}
		}
	}
}

$smarty -> assign( "isgroupnode" , $isgroupnode ) ;
$smarty -> assign( "group_list" , $group_list ) ;
$smarty -> assign( "node_list" , $node_list ) ;

?>