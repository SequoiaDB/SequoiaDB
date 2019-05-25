<?php

$type    = empty ( $_POST['ty'] ) ? "" : $_POST['ty'] ;
$csname  = empty ( $_POST['cs'] ) ? "" : $_POST['cs'] ;
$clname  = empty ( $_POST['cl'] ) ? "" : $_POST['cl'] ;
$skipnum = empty ( $_POST['sk'] ) ? 0  : $_POST['sk'] ;

if ( $skipnum > 0 )
{
	$skipnum -= 20 ;
}

$cs_info = array() ;
$cl_info = array() ;
$cl_nature = array() ;
$cl_split = array() ;
$style = 0 ;
$recordnum = 0 ;
$count_num = 0 ;
$pagesum = 0 ;
$index_info = array() ;

if ( $type == "csnature" )
{
	$style = 1 ;
	$cursor = $db -> getSnapshot ( SDB_SNAP_COLLECTIONSPACE, '{Name:"'.$csname.'"}' ) ;
	if ( !empty ( $cursor ) )
	{
		while ( $arr = $cursor -> getNext() )
		{
			$cs_info = $arr ;
			break ;
		}
	}
}
else if ( $type == "cldata" )
{
	$style = 2 ;
	$cs = $db -> selectCS ( $csname ) ;
	if ( !empty ( $cs ) )
	{
		$cl = $cs -> selectCollection ( $clname ) ;
		if ( !empty ( $cl ) )
		{
			$db -> install ( "{ install : false }" ) ;
			$cursor = $cl -> find( NULL, NULL, NULL, NULL, $skipnum, 20 ) ;
			if ( !empty ( $cursor ) )
			{
				while ( $str = $cursor -> getNext() )
				{
					array_push ( $cl_info, $str ) ;
					++$recordnum ;
				}
			}
			$recordsum = $cl -> count() ;
			$pagesum =  (int)($recordsum / 20) ;
			if ( $recordsum % 20 > 0 )
			{
				++$pagesum ;
			}
			$db -> install ( "{ install : true }" ) ;
		}
	}
}
else if ( $type == "clindex" )
{
	$style = 3 ;
	$cs = $db -> selectCS ( $csname ) ;
	if ( !empty ( $cs ) )
	{
		$cl = $cs -> selectCollection ( $clname ) ;
		if ( !empty ( $cl ) )
		{
			$cursor = $cl -> getIndex() ;
			if ( !empty ( $cursor ) )
			{
				while ( $str = $cursor -> getNext() )
				{
					array_push ( $index_info, $str ) ;
				}
			}
		}
	}
}
else if ( $type == "clnature" )
{
	$style = 4 ;

	$cs = $db -> selectCS ( $csname ) ;
	if ( !empty($cs) )
	{
		$cl = $cs -> selectCollection ( $clname ) ;
		if ( !empty( $cl ) )
		{
			$count_num = $cl -> count() ;
		}
	}

	$cursor = $db -> getSnapshot ( SDB_SNAP_COLLECTION, '{Name:"'.$csname.'.'.$clname.'"}' ) ;//
	if ( !empty ( $cursor ) )
	{
		while ( $arr = $cursor -> getNext() )
		{
			$cl_nature = $arr ;
			if ( $cl_nature['Name'] == $csname.'.'.$clname )
			{
				break ;
			}
		}
	}

	$db -> install ( "{ install : false }" ) ;
	$cursor = $db -> getSnapshot ( SDB_SNAP_CATALOG, '{Name:"'.$csname.'.'.$clname.'"}' ) ;
	if ( !empty ( $cursor ) )
	{
		while ( $str = $cursor -> getNext() )
		{
			$str = convertStr2array( $str, 'MinKey', '"MinKey"',  false ) ;
			$str = convertStr2array( $str, 'MaxKey', '"MaxKey"',  false ) ;
			$sourcestr = $str ;
			$arr = json_decode ( $str, true ) ;
			if ( !$arr )
			{
				$arr = array() ;
			}
			else
			{
				if ( array_key_exists( "ShardingKey", $arr ) )
				{
					$temp = $arr["ShardingKey"] ;
					$keys = array() ;
					foreach ( $temp as $key => $value )
					{
						array_push( $keys, $key ) ;
					}
					$str = convertStr2array( $sourcestr, '""', $keys, true  ) ;
					$arr = json_decode ( $str, true ) ;
					if ( !$arr )
					{
						$arr = array() ;
					}
				}
			}
			$cl_split = $arr ;
			break ;
		}
	}
	else
	{
	   $cl_split = array( 'Name' => $csname.'.'.$clname ) ;
	}
	$db -> install ( "{ install : true }" ) ;
}

$pageturn = 0 ;
if ( $recordnum < 20 && $skipnum == 0 )
{
	$pageturn = 3 ; //都没有
}
else if ( ( $recordnum < 20 && $recordnum != 0 ) || ( $recordnum == 0 && $skipnum > 0 ) )
{
	$pageturn = 2 ; //没有向下
}
else if ( $skipnum == 0 )
{
	$pageturn = 1 ; //没有向上
}
//echo $pageturn ;
$smarty -> assign( "count_num" , $count_num ) ;
$smarty -> assign( "pagesum" , $pagesum ) ;
$smarty -> assign( "pagenumber" , $skipnum/20 ) ;
$smarty -> assign( "pageturn" , $pageturn ) ;
$smarty -> assign( "iscscl" , $style ) ;
$smarty -> assign( "cs_info", $cs_info ) ;
$smarty -> assign( "cl_info", $cl_info ) ;
$smarty -> assign( "cl_nature", $cl_nature ) ;
$smarty -> assign( "cl_split", $cl_split ) ;
$smarty -> assign( "index_info", $index_info ) ;
?>