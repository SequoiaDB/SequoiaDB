<?php

$type          = empty ( $_POST['type']          ) ? ""    :  $_POST['type']           ;
$csname        = empty ( $_POST['csname']        ) ? ""    :  $_POST['csname']         ;
$clname        = empty ( $_POST['clname']        ) ? ""    :  $_POST['clname']         ;
$record        = empty ( $_POST['record']        ) ? ""    :  $_POST['record']         ;
$rule          = empty ( $_POST['rule']          ) ? NULL  :  $_POST['rule']           ;
$condition     = empty ( $_POST['condition']     ) ? NULL  :  $_POST['condition']      ;
$endcondition  = empty ( $_POST['endcondition']  ) ? NULL  :  $_POST['endcondition']   ;
$selecter      = empty ( $_POST['selecter']      ) ? NULL  :  $_POST['selecter']       ;
$orderby       = empty ( $_POST['orderby']       ) ? NULL  :  $_POST['orderby']        ;
$hint          = empty ( $_POST['hint']          ) ? NULL  :  $_POST['hint']           ; 
$numtoskip     = empty ( $_POST['numtoskip']     ) ? 0     :  $_POST['numtoskip']      ;
$numtoreturn   = empty ( $_POST['numtoreturn']   ) ? -1    :  $_POST['numtoreturn']    ;
$pagesize      = empty ( $_POST['pagesize']      ) ? '{PageSize:4096}'  :  $_POST['pagesize']       ;
$options       = empty ( $_POST['options']       ) ? NULL  :  $_POST['options']        ;
$sourcename    = empty ( $_POST['sourcename']    ) ? ""    :  $_POST['sourcename']     ;
$destname      = empty ( $_POST['destname']      ) ? ""    :  $_POST['destname']       ;
$indexdef      = empty ( $_POST['indexdef']      ) ? NULL  :  $_POST['indexdef']       ;
$indexname     = empty ( $_POST['indexname']     ) ? ""    :  $_POST['indexname']      ;
$indexunique   = empty ( $_POST['indexunique']   ) ? false :  $_POST['indexunique']    ;
$indexenforced = empty ( $_POST['indexenforced'] ) ? false :  $_POST['indexenforced']  ;


if ( $numtoreturn > 20 || $numtoreturn < 0 )
{
	$numtoreturn = 20 ;
}
$rc = 0 ;
$find_arr = array() ;
$count_num = 0 ;
$common_msg = "" ;
if ( $type == "createcs" )
{
	$common_msg = "创建集合空间" ;
	$db -> selectCS ( $csname, $pagesize ) ;
	$arr = $db -> getError() ;
	$rc = $arr["errno"] ;
}
else if ( $type == "dropcs" )
{
	$common_msg = "删除集合空间" ;
	$cs = $db -> selectCS ( $csname ) ;
	if ( !empty($cs) )
	{
		$arr = $cs -> drop() ;
		$rc = $arr["errno"] ;
	}
	else
	{
		$arr = $db -> getError() ;
		$rc = $arr["errno"] ;
	}
}
else if ( $type == "createcl" )
{
	$common_msg = "创建集合" ;
	$cs = $db -> selectCS ( $csname ) ;
	if ( !empty($cs) )
	{
		$cl = $cs -> selectCollection ( $clname, $options ) ;
		if ( !empty( $cl ) )
		{
			$rc = 0 ;
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
else if ( $type == "dropcl" )
{
	$common_msg = "删除集合" ;
	$cs = $db -> selectCS ( $csname ) ;
	if ( !empty($cs) )
	{
		$cl = $cs -> selectCollection ( $clname ) ;
		if ( !empty( $cl ) )
		{
			$arr = $cl -> drop() ;
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
else if ( $type == "insert" )
{
	$common_msg = "插入记录" ;
	$cs = $db -> selectCS ( $csname ) ;
	if ( !empty($cs) )
	{
		$cl = $cs -> selectCollection ( $clname ) ;
		if ( !empty( $cl ) )
		{
			$arr = $cl -> insert ( $record ) ;
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
else if ( $type == "delete" )
{
	$common_msg = "删除记录" ;
	$cs = $db -> selectCS ( $csname ) ;
	if ( !empty($cs) )
	{
		$cl = $cs -> selectCollection ( $clname ) ;
		if ( !empty( $cl ) )
		{
			$arr = $cl -> remove ( $condition, $hint ) ;
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
else if ( $type == "update" )
{
	$common_msg = "更新记录" ;
	$cs = $db -> selectCS ( $csname ) ;
	if ( !empty($cs) )
	{
		$cl = $cs -> selectCollection ( $clname ) ;
		if ( !empty( $cl ) )
		{
			$arr = $cl -> update ( $rule, $condition, $hint ) ;
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
else if ( $type == "split" )
{
	$common_msg = "切分" ;
	$cs = $db -> selectCS ( $csname ) ;
	if ( !empty($cs) )
	{
		$cl = $cs -> selectCollection ( $clname ) ;
		if ( !empty( $cl ) )
		{
			if ( is_numeric( $condition ) )
			{
				$condition = (float)$condition ;
			}
			$arr = $cl -> split ( $sourcename, $destname, $condition, $endcondition ) ;
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
else if ( $type == "count" )
{
	$common_msg = "获取记录总数" ;
	$cs = $db -> selectCS ( $csname ) ;
	if ( !empty($cs) )
	{
		$cl = $cs -> selectCollection ( $clname ) ;
		if ( !empty( $cl ) )
		{
			$count_num = $cl -> count ( $condition ) ;
			$count_num = "记录总数： ".$count_num ;
			$arr = $db -> getError() ;
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
else if ( $type == "createindex" )
{
	$common_msg = "创建索引" ;
	$cs = $db -> selectCS ( $csname ) ;
	if ( !empty($cs) )
	{
		$cl = $cs -> selectCollection ( $clname ) ;
		if ( !empty( $cl ) )
		{
			if ( $indexunique == "true" )
			{
				$indexunique = true ;
			}
			else
			{
				$indexunique = false ;
			}
			if ( $indexenforced == "true" )
			{
				$indexenforced = true ;
			}
			else
			{
				$indexenforced = false ;
			}
			$arr = $cl -> createIndex ( $indexdef, $indexname, $indexunique, $indexenforced ) ;
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
else if ( $type == "deleteindex" )
{
	$common_msg = "删除索引" ;
	$cs = $db -> selectCS ( $csname ) ;
	if ( !empty($cs) )
	{
		$cl = $cs -> selectCollection ( $clname ) ;
		if ( !empty( $cl ) )
		{
			$arr = $cl -> deleteIndex ( $indexname ) ;
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
else if ( $type == "find" )
{
	$common_msg = "查询" ;
	$cs = $db -> selectCS ( $csname ) ;
	if ( !empty($cs) )
	{
		$cl = $cs -> selectCollection ( $clname ) ;
		if ( !empty( $cl ) )
		{
			$cursor = $cl -> find ( $condition, $selecter, $orderby, $hint, $numtoskip, $numtoreturn ) ;
			if ( !empty( $cursor ) )
			{
				$db -> install ( "{ install : false }" ) ;
				while ( $str = $cursor -> getNext() )
				{
                    $str = str_replace( '\\', '\\\\', $str ) ;
					$str = str_replace( '"', '\"', $str ) ;
					array_push ( $find_arr, $str ) ;
				}
				$db -> install ( "{ install : true }" ) ;
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
	else
	{
		$arr = $db -> getError() ;
		$rc = $arr["errno"] ;
	}
}
$error_msg = array_key_exists( $rc, $errno_cn ) ? $errno_cn[$rc] : $rc ;
if ( $rc == 0 )
{
	$error_msg = date("Y-m-d h:i:s")." ".$common_msg." 执行成功" ;
}
else
{
	$error_msg = date("Y-m-d h:i:s")." ".$common_msg." 执行失败：".$error_msg ;
}
$error_msg = str_replace( '\\', '\\\\', $error_msg ) ;
$error_msg = str_replace( '"', '\"', $error_msg ) ;
$smarty -> assign( "count_num", $count_num ) ;
$smarty -> assign( "find_arr", $find_arr ) ;
$smarty -> assign( "data_type", $type ) ;
$smarty -> assign( "data_rc", $rc ) ;
$smarty -> assign( "data_respond", $error_msg ) ;
?>