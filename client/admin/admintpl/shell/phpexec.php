<?php
set_time_limit(0);

//初始化常量
require_once ( "../php/html_conf.php" ) ;

$common = empty( $_POST['common'] ) ? '' : $_POST['common'] ;
$order  = empty( $_POST['order']  ) ? '' : $_POST['order']  ;

$output = "" ;
$return = 0 ;
$return_arr = array() ;

if ( $common == "checkfile" )
{
	//判断安装文件是否存在
	$common = ' "1"' ;
	$order  = ' '.$order  ;
	exec( './sequoiadbphp.sh'.$common.$order, $output, $return ) ;

	$str = "" ;
	$arr_len = count($output) ;
	if ( $arr_len > 0 )
	{
		$str = $output[$arr_len-1] ;
	}
	
	$diary = "" ;
	foreach ( $output as $child )
	{
		$diary .= $child.'<br />' ;
	}
	
	$return_arr = array( 'output' => $str, "errno" => $return, "diary" => $diary ) ;

}
else if ( $common == "checkhost" )
{
	//判断host能否连接
	$common = ' 2' ;
	$order  = ' '.$order  ;
	$str_host = "" ;
	$str_ip = "" ;
	exec( './sequoiadbphp.sh'.$common.$order, $output, $return ) ;

	$str = "" ;
	$arr_len = count($output) ;
	if ( $arr_len > 0 )
	{
		$str = $output[$arr_len-1] ;
	}
	
	$diary = "" ;
	foreach ( $output as $child )
	{
		$diary .= $child.'<br />' ;
	}

	$ips = explode( ' ', $order ) ;
	$reg = '/^((2[0-4]\d|25[0-5]|[01]?\d\d?)\.){3}(2[0-4]\d|25[0-5]|[01]?\d\d?)$/' ;
	if ( preg_match_all( $reg, $ips[1] ) )
	{
		$str_ip = $ips[1] ;
	}
	else
	{
		$str_host = $ips[1] ;
	}

	if ( $return == 0 )
	{
		if ( $str_ip != "" )
		{
			exec( './sequoiadbphp.sh 3 '.$str_ip, $output ) ;
			$arr_len = count($output) ;
			if ( $arr_len > 0 )
			{
				$str_host = $output[$arr_len-1] ;
			}
		}
		else
		{
			exec( './sequoiadbphp.sh 4 '.$str_host, $output ) ;
			$arr_len = count($output) ;
			if ( $arr_len > 0 )
			{
				$str_ip = $output[$arr_len-1] ;
				$str_ip = explode( '|', $str_ip ) ;
				$str_ip = $str_ip [0] ;
			}
		}
	}
	
	$return_arr = array( 'ip' => $str_ip, 'host' => $str_host, 'output' => $str, "errno" => $return, "diary" => $diary ) ;
}
else if ( $common == "postallnodeconf" )
{
	if ( file_exists ( './sequoiadbconfig.sh' ) )
	{
		copy( './sequoiadbconfig.sh', './sequoiadbconfig.sh.bak' ) ;
	}
	
	$file = fopen( './sequoiadbconfig.sh', 'w' ) ;
	
	$setup_conf = json_decode( $order, true ) ;
	
	$shell_str = '' ;
	$temp_1 = '' ;
	$temp_2 = '' ;
	$temp_3 = '' ;
	$temp_4 = '' ;

	$temp_1 = '#!/bin/bash'.PHP_EOL.PHP_EOL.'#部署错误是否需要回滚[1:是,0:否]'.PHP_EOL.'IS_ROLLBACK=0'.PHP_EOL.PHP_EOL.'#开机自动启动[1:是,0:否]'.PHP_EOL.'IS_AUTOSTART='.$setup_conf['autostart'].''.PHP_EOL.PHP_EOL.'#需要回滚的路径(不需要填写)'.PHP_EOL.'DELETE_PATH_ARR=()'.PHP_EOL.PHP_EOL.'#需要回滚的任务(不需要填写)'.PHP_EOL.'REVOKE_TASK_ARR=()'.PHP_EOL.PHP_EOL.'#是否输出调试信息[1:输出调试信息,2:输出普通信息,3:不输出]'.PHP_EOL.'IS_PRINGT_DEBUG='.$setup_conf['debug'].PHP_EOL.PHP_EOL.'#安装文件的路径'.PHP_EOL.'INSTALL_PATH="'.dirname( $setup_conf['install'] ).'"'.PHP_EOL.PHP_EOL.'#安装文件的文件名'.PHP_EOL.'INSTALL_NAME="'.basename( $setup_conf['install'] ).'"'.PHP_EOL.PHP_EOL.'#sdbcm端口'.PHP_EOL.'SDBCM_PORT="'.$setup_conf['sdbcm'].'"'.PHP_EOL.PHP_EOL.'#分区组列表'.PHP_EOL.'LIST_GROUP=('.$setup_conf['group'].')'.PHP_EOL.PHP_EOL ;

	$shell_str .= $temp_1 ;

	$temp_1 = '#主机列表'.PHP_EOL.'LIST_HOST=(\\'.PHP_EOL ;

	$temp_2 = '' ;
	$host_num = 1 ;
   foreach( $setup_conf['host'] as $child )
	{
		$temp_1 .= '"ARRAY_HOST_'.$host_num.'" \\'.PHP_EOL ;
		$temp_2 .= 'ARRAY_HOST_'.$host_num.'=("'.$child[0].'" "'.$child[1].'" "'.$child[2].'" "'.$child[3].'" "'.$child[4].'")'.PHP_EOL ;
		++$host_num ;
	}
	$temp_1 .= ')'.PHP_EOL.PHP_EOL ;
	$temp_2 .= PHP_EOL ;

	$shell_str .= $temp_1 ;
	$shell_str .= $temp_2 ;

	$isfirst = true ;
	$temp_4 = '#配置文件参数表'.PHP_EOL.'SDB_CONFIG=(' ;
	foreach ( $globalvar_setup_conf as $child )
	{
		if ( $isfirst )
		{
			$isfirst = false ;
		}
		else
		{
			$temp_4 .= ' ' ;
		}
		$temp_4 .= $child[0] ;
	}
	$temp_4 .= ')'.PHP_EOL ;

	$node_num  = 1 ;
	$node_conf = $setup_conf['node'] ;
	$temp_1 = '#节点列表'.PHP_EOL.'LIST_NODE=(\\'.PHP_EOL ;
	$temp_2 = '' ;
	$temp_3 = '' ;
	$temp_svcname = 0 ;
	foreach( $node_conf['coord'] as $child )
	{
		$conf = $child["conf"] ;
		$temp_1 .= '"ARRAY_NODE_'.$node_num.'" \\'.PHP_EOL ;
		$host_num = 1 ;
		foreach( $setup_conf['host'] as $host_child )
		{
			if ( $host_child[0] == $child['hostname'] )
			{
				break ;
			}
			++$host_num ;
		}
		$temp_2 .= 'ARRAY_NODE_'.$node_num.'=("coord" "" "ARRAY_HOST_'.$host_num.'" "SDBCONF_'.$node_num.'")'.PHP_EOL ;
		$temp_3 .= 'SDBCONF_'.$node_num.'=(' ;
		$conf_num = count( $globalvar_setup_conf ) ;
		$isfirst = true ;
		for ( $i = 0; $i < $conf_num; ++$i )
		{
			if ( $isfirst )
			{
				$isfirst = false ;
			}
			else
			{
				$temp_3 .= ' ' ;
			}
			if ( $conf[$i] == '' )
			{
			   $temp_name = $temp_svcname ;
			   if ( $globalvar_setup_conf[$i][0] == 'replname' )
			   {
			      $temp_name = $temp_svcname + 1 ;
			      $temp_3 .= '"'.$temp_name.'"' ;
			   }
			   else if ( $globalvar_setup_conf[$i][0] == 'shardname' )
			   {
			      $temp_name = $temp_svcname + 2 ;
			      $temp_3 .= '"'.$temp_name.'"' ;
			   }
			   else if ( $globalvar_setup_conf[$i][0] == 'catalogname' )
			   {
			      $temp_name = $temp_svcname + 3 ;
			      $temp_3 .= '"'.$temp_name.'"' ;
			   }
			   else if ( $globalvar_setup_conf[$i][0] == 'httpname' )
			   {
			      $temp_name = $temp_svcname + 4 ;
			      $temp_3 .= '"'.$temp_name.'"' ;
			   }
			   else
			   {
            $temp_3 .= '"'.$conf[$i].'"' ;
			   }
			}
			else
			{
			   $temp_3 .= '"'.$conf[$i].'"' ;
			}
			if ( $globalvar_setup_conf[$i][0] == 'svcname' )
			{
			   $temp_svcname = (int)$conf[$i] ;
			}
		}
		$temp_3 .= ')'.PHP_EOL ;
		++$node_num ;
	}
	
	foreach( $node_conf["cata"] as $child )
	{
		$conf = $child["conf"] ;
		$temp_1 .= '"ARRAY_NODE_'.$node_num.'" \\'.PHP_EOL ;
		$host_num = 1 ;
		foreach( $setup_conf['host'] as $host_child )
		{
			if ( $host_child[0] == $child['hostname'] )
			{
				break ;
			}
			++$host_num ;
		}
		$temp_2 .= 'ARRAY_NODE_'.$node_num.'=("cata" "" "ARRAY_HOST_'.$host_num.'" "SDBCONF_'.$node_num.'")'.PHP_EOL ;
		$temp_3 .= 'SDBCONF_'.$node_num.'=(' ;
		$conf_num = count( $globalvar_setup_conf ) ;
		$isfirst = true ;
		for ( $i = 0; $i < $conf_num; ++$i )
		{
			if ( $isfirst )
			{
				$isfirst = false ;
			}
			else
			{
				$temp_3 .= ' ' ;
			}
			if ( $conf[$i] == '' )
			{
			   $temp_name = $temp_svcname ;
			   if ( $globalvar_setup_conf[$i][0] == 'replname' )
			   {
			      $temp_name = $temp_svcname + 1 ;
			      $temp_3 .= '"'.$temp_name.'"' ;
			   }
			   else if ( $globalvar_setup_conf[$i][0] == 'shardname' )
			   {
			      $temp_name = $temp_svcname + 2 ;
			      $temp_3 .= '"'.$temp_name.'"' ;
			   }
			   else if ( $globalvar_setup_conf[$i][0] == 'catalogname' )
			   {
			      $temp_name = $temp_svcname + 3 ;
			      $temp_3 .= '"'.$temp_name.'"' ;
			   }
			   else if ( $globalvar_setup_conf[$i][0] == 'httpname' )
			   {
			      $temp_name = $temp_svcname + 4 ;
			      $temp_3 .= '"'.$temp_name.'"' ;
			   }
			   else
			   {
            $temp_3 .= '"'.$conf[$i].'"' ;
			   }
			}
			else
			{
			   $temp_3 .= '"'.$conf[$i].'"' ;
			}
			if ( $globalvar_setup_conf[$i][0] == 'svcname' )
			{
			   $temp_svcname = (int)$conf[$i] ;
			}
		}
		$temp_3 .= ')'.PHP_EOL ;
		++$node_num ;
	}
	
	foreach( $node_conf["data"] as $child )
	{
		$conf = $child["conf"] ;
		$temp_1 .= '"ARRAY_NODE_'.$node_num.'" \\'.PHP_EOL ;
		$host_num = 1 ;
		foreach( $setup_conf['host'] as $host_child )
		{
			if ( $host_child[0] == $child['hostname'] )
			{
				break ;
			}
			++$host_num ;
		}
		$temp_2 .= 'ARRAY_NODE_'.$node_num.'=("data" "'.$child['groupname'].'" "ARRAY_HOST_'.$host_num.'" "SDBCONF_'.$node_num.'")'.PHP_EOL ;
		$temp_3 .= 'SDBCONF_'.$node_num.'=(' ;
		$conf_num = count( $globalvar_setup_conf ) ;
		$isfirst = true ;
		for ( $i = 0; $i < $conf_num; ++$i )
		{
			if ( $isfirst )
			{
				$isfirst = false ;
			}
			else
			{
				$temp_3 .= ' ' ;
			}
			if ( $conf[$i] == '' )
			{
			   $temp_name = $temp_svcname ;
			   if ( $globalvar_setup_conf[$i][0] == 'replname' )
			   {
			      $temp_name = $temp_svcname + 1 ;
			      $temp_3 .= '"'.$temp_name.'"' ;
			   }
			   else if ( $globalvar_setup_conf[$i][0] == 'shardname' )
			   {
			      $temp_name = $temp_svcname + 2 ;
			      $temp_3 .= '"'.$temp_name.'"' ;
			   }
			   else if ( $globalvar_setup_conf[$i][0] == 'catalogname' )
			   {
			      $temp_name = $temp_svcname + 3 ;
			      $temp_3 .= '"'.$temp_name.'"' ;
			   }
			   else if ( $globalvar_setup_conf[$i][0] == 'httpname' )
			   {
			      $temp_name = $temp_svcname + 4 ;
			      $temp_3 .= '"'.$temp_name.'"' ;
			   }
			   else
			   {
            $temp_3 .= '"'.$conf[$i].'"' ;
			   }
			}
			else
			{
			   $temp_3 .= '"'.$conf[$i].'"' ;
			}
			if ( $globalvar_setup_conf[$i][0] == 'svcname' )
			{
			   $temp_svcname = (int)$conf[$i] ;
			}
		}
		$temp_3 .= ')'.PHP_EOL ;
		++$node_num ;
	}
	
	$temp_1 .= ')'.PHP_EOL.PHP_EOL ;
	$temp_2 .= PHP_EOL ;
	
	$shell_str .= $temp_1 ;
	$shell_str .= $temp_2 ;
	$shell_str .= $temp_4 ;
	$shell_str .= $temp_3 ;

	fwrite( $file, $shell_str ) ;
	fclose( $file ) ;
	$return_arr = array( 'output' => "", "errno" => "0", "diary" => "" ) ;
}
else if ( $common == "checkenvhost" )
{
	//检查指定机器系统环境
	//$order = 'ubuntu-test-01 50000' ;
	$line = empty( $_POST['line'] ) ? 0 : $_POST['line'] ;
	$common = ' 5' ;
	$order  = ' '.$order  ;
	exec( './sequoiadbphp.sh'.$common.$order, $output, $return ) ;

	$str = "" ;
	$arr_len = count($output) ;
	if ( $arr_len > 0 )
	{
		$str = $output[$arr_len-1] ;
	}
	
	$diary = "" ;
	foreach ( $output as $child )
	{
		$diary .= $child.'<br />' ;
	}
	
	$return_arr = array( 'output' => $str, "errno" => $return, "diary" => $diary, "line" => $line ) ;
}
else if ( $common == "sendinstallfile" )
{
	//分派安装文件
	//$order = 'ubuntu-test-01' ;
	$line = empty( $_POST['line'] ) ? 0 : $_POST['line'] ;
	$common = ' 6' ;
	$order  = ' '.$order  ;
	exec( './sequoiadbphp.sh'.$common.$order, $output, $return ) ;

	$str = "" ;
	$arr_len = count($output) ;
	if ( $arr_len > 0 )
	{
		$str = $output[$arr_len-1] ;
	}
	
	$diary = "" ;
	foreach ( $output as $child )
	{
		$diary .= $child.'<br />' ;
	}
	
	$return_arr = array( 'output' => $str, "errno" => $return, "diary" => $diary, "line" => $line ) ;
}
else if ( $common == "installthefile" )
{
	//安装文件
	//$order = 'ubuntu-test-01 50000' ;
	$line = empty( $_POST['line'] ) ? 0 : $_POST['line'] ;
	$common = ' 7' ;
	$order  = ' '.$order  ;
	exec( './sequoiadbphp.sh'.$common.$order, $output, $return ) ;

	$str = "" ;
	$arr_len = count($output) ;
	if ( $arr_len > 0 )
	{
		$str = $output[$arr_len-1] ;
	}
	
	$diary = "" ;
	foreach ( $output as $child )
	{
		$diary .= $child.'<br />' ;
	}
	
	$return_arr = array( 'output' => $str, "errno" => $return, "diary" => $diary, "line" => $line ) ;
}
else if ( $common == "createnode" )
{
	//创建节点
	//$order = 'ubuntu-test-01 0 ubuntu-test-01 50000 50000' ;
	$line = empty( $_POST['line'] ) ? 0 : $_POST['line'] ;
	$common = ' 8' ;
	$order  = ' '.$order  ;
	exec( './sequoiadbphp.sh'.$common.$order, $output, $return ) ;

	$str = "" ;
	$arr_len = count($output) ;
	if ( $arr_len > 0 )
	{
		$str = $output[$arr_len-1] ;
	}
	
	$diary = "" ;
	foreach ( $output as $child )
	{
		$diary .= $child.'<br />' ;
	}
	
	$return_arr = array( 'output' => $str, "errno" => $return, "diary" => $diary, "line" => $line ) ;
}
else if ( $common == "activegroup" )
{
	//激活分区组
	//$order = '/opt/sequoiadb/bin/sdb ubuntu-test-01 50000 g1' ;
	$line = empty( $_POST['line'] ) ? 0 : $_POST['line'] ;
	$common = ' 9' ;
	$order  = ' '.$order ;
	exec( './sequoiadbphp.sh'.$common.$order, $output, $return ) ;

	$str = "" ;
	$arr_len = count($output) ;
	if ( $arr_len > 0 )
	{
		$str = $output[$arr_len-1] ;
	}
	
	$diary = "" ;
	foreach ( $output as $child )
	{
		$diary .= $child.'<br />' ;
	}
	
	$return_arr = array( 'output' => $str, "errno" => $return, "diary" => $diary, "line" => $line ) ;
}
$return_json = json_encode($return_arr) ;
	
echo $return_json ;




?>