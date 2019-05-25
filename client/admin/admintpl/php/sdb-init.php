<?php

include_once('./smarty/Smarty.class.php');
$smarty = new Smarty(); 
$db = new Sequoiadb() ;
$cs = NULL ;
$cl = NULL ;
$smarty -> template_dir = "./templates"; //模板存放目录 
$smarty -> compile_dir = "./templates_c"; //编译目录 
$smarty -> caching = false ;  //是否缓存
$isConnect = false ;
if ( isset($_SESSION['sdb_monitor_address']) )
{
	$arr = array() ;
	$sdb_monitor_address  = $_SESSION['sdb_monitor_address'] ;
	if ( isset($_SESSION['sdb_monitor_user']) )
	{
		$sdb_monitor_user     = $_SESSION['sdb_monitor_user'] ;
		$sdb_monitor_password = isset($_SESSION['sdb_monitor_password']) ? $_SESSION['sdb_monitor_password'] : "" ;
		$arr = $db->connect ( $sdb_monitor_address, $sdb_monitor_user, $sdb_monitor_password ) ;
	}
	else
	{
		$arr = $db->connect ( $sdb_monitor_address ) ;
	}
	if ( $arr['errno'] != 0 )
	{
		$isConnect = false ;
		//$smarty -> cache_lifetime = -1 ;  //缓存时间,永久
	}
	else
	{
		$isConnect = true ;
		//$smarty -> cache_lifetime = 24 * 60 * 60 ;  //缓存时间
	}
}
$smarty -> left_delimiter = "{{"; //左定界符 
$smarty -> right_delimiter = "}}"; //右定界符 

function convertStr2array( $str, $find, $replace, $type )
{
	$temp1 = "" ;
	$temp2 = "" ;
	$strposnum = 0 ;
	$num = 0 ;
	$len = strlen($find) ;
	$tempnum = 0 ;
	$array_len = count ( $replace ) ;
	if ( $type )
	{
		while( $strposnum = strpos( $str, $find, $tempnum ) )
		{
			$temp1 =  substr ( $str, 0, $strposnum  ) ;
			$temp2 =  substr ( $str, $strposnum + $len ) ;
			$str = $temp1.'"'.$replace[$num].'"'.$temp2 ;
			++$num ;
			if ( $num == $array_len )
			{
				$num = 0 ;
			}
			$tempnum = $strposnum + $len ;
		}
	}
	else
	{
		while( $strposnum = strpos( $str, $find, $tempnum ) )
		{
			$temp1 =  substr ( $str, 0, $strposnum  ) ;
			$temp2 =  substr ( $str, $strposnum + $len ) ;
			$str = $temp1.$replace.$temp2 ;
			$tempnum = $strposnum + $len ;
		}
	}
	return $str ;
}

?>