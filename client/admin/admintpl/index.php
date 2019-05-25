<?php
session_start() ;
date_default_timezone_set ( "UTC" ) ;
set_time_limit( 0 ) ;

//初始化常量
require_once ( "./php/html_conf.php" ) ;

//初始化php模板
require_once ( "./php/sdb-init.php" ) ;

//取得错误映射表
require_once ( "./php/error_cn.php" ) ;

//进入网页显示
require_once ( './php/html-show.php' ) ;

?>