<?php

$sql = empty ( $_POST['sql'] ) ? "" : $_POST['sql'] ;

$sql = ltrim( $sql ) ;
$str = "" ;
$model =  "noselect" ;
$rc = 0 ;
$record_array = array() ;
if ( strtolower(substr ( $sql, 0, 6 )) == "select" )
{
	$cursor = $db -> execSQL( $sql ) ;
	if ( !empty ( $cursor ) )
	{
		$model =  "select" ;
		$db -> install ( "{ install : false }" ) ;
		while ( $arr = $cursor -> getNext() )
		{
			$arr = str_replace( '\\', '\\\\', $arr ) ;
			$arr = str_replace( '"', '\"', $arr ) ;
			array_push( $record_array, $arr ) ;
		}
		$db -> install ( "{ install : true }" ) ;
		$str = date("Y-m-d h:i:s")." SQL执行成功" ;
	}
	else
	{
		$arr = $db -> getError();
		$rc = $arr["errno"] ;
		$str = date("Y-m-d h:i:s")." SQL执行错误: ".(array_key_exists( $rc, $errno_cn ) ? $errno_cn[$rc] : $rc) ;
	}
}
else
{
	$arr = $db -> execUpdateSQL( $sql ) ;
	$rc = $arr["errno"] ;
	$str = date("Y-m-d h:i:s")." SQL执行结果: ".(array_key_exists( $rc, $errno_cn ) ? $errno_cn[$rc] : $rc) ;
}

$smarty -> assign( "sql_model", $model ) ;
$smarty -> assign( "sql_rc", $rc ) ;
$smarty -> assign( "sql_return", $str ) ;
$smarty -> assign( "sql_record_array", $record_array ) ;
?>