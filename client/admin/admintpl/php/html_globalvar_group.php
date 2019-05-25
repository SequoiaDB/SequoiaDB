<?php

//工具栏的按钮
$globalvar_tool_button_cn = array(
//		显示或隐藏用的id				鼠标点击的事件									按钮显示的文字				图片文件名						类型				属于谁的按钮
array( 'showid' => '', 				'common' => 'toggle_tb_group("creategroup")', 	'name' => '创建分区组', 		'pic' => 'addgroup.png',		'type' => 1,	'dependence' => '' ),
array( 'showid' => 'tool_button_', 	'common' => 'toggle_tb_group("createnode")',  	'name' => '创建节点', 		'pic' => 'addnode.png',			'type' => 1,	'dependence' => 'group' ),
array( 'showid' => 'tool_button_', 	'common' => 'toggle_tb_group("startgroup")', 	'name' => '启动分区组', 		'pic' => 'groupplay.png',		'type' => 1,	'dependence' => 'group' ),
array( 'showid' => 'tool_button_', 	'common' => 'toggle_tb_group("stopgroup")', 	'name' => '停止分区组', 		'pic' => 'groupstop.png',		'type' => 1,	'dependence' => 'group' ),
array( 'showid' => 'tool_button_', 	'common' => 'toggle_tb_group("startnode")', 	'name' => '启动节点', 		'pic' => 'nodeplay.png',		'type' => 1,	'dependence' => 'node' ),
array( 'showid' => 'tool_button_', 	'common' => 'toggle_tb_group("stopnode")', 	 	'name' => '停止节点', 		'pic' => 'nodestop.png',		'type' => 1,	'dependence' => 'node' )
//array( 'showid' => '', 				'common' => 'toggle_tb_group("createcatalog")', 'name' => '创建编目组', 		'pic' => 'addcatalog.png',		'type' => 1,	'dependence' => '' ),
) ;

//提示框的参数列表
$globalvar_tool_parameter_cn = array(
//		显示或隐藏用的id				//输入框的id									输入框的类型				标题										输入框文字提示								类型
array( 'showid' => 'tb-group-', 	'inputid' => 'tool_group_groupname',		'input' => 'input', 	'name_title' => 'Group Name',			'placeholder' => '分区组名',		 			'type' => 1 ),
array( 'showid' => 'tb-group-', 	'inputid' => 'tool_group_hostname',			'input' => 'input', 	'name_title' => 'Host Name',			'placeholder' => '计算机名',		 			'type' => 1 ),
array( 'showid' => 'tb-group-', 	'inputid' => 'tool_group_servicename',		'input' => 'input', 	'name_title' => 'Service Name',			'placeholder' => '端口',	 					'type' => 1 ),
array( 'showid' => 'tb-group-', 	'inputid' => 'tool_group_databasepath',		'input' => 'input', 	'name_title' => 'Database Path',		'placeholder' => '存储路径',	 				'type' => 1 ),
array( 'showid' => 'tb-group-', 	'inputid' => 'tool_group_config',			'input' => 'input', 	'name_title' => 'Config',				'placeholder' => '配置',						'type' => 1 )

) ;





















?>