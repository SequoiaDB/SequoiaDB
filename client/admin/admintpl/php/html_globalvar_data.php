<?php
//工具栏的按钮
$globalvar_tool_button_cn = array(
//		显示或隐藏用的id				鼠标点击的事件									按钮显示的文字				图片文件名					类型				属于谁的按钮
array( 'showid' => '', 				'common' => 'toggle_tb_data("createcs")', 		'name' => '创建集合空间',		'pic' => 'addcs.png',		'type' => 1,	'dependence' => '' 		),
array( 'showid' => 'tool_button_', 	'common' => 'toggle_tb_data("dropcs")', 		'name' => '删除集合空间',		'pic' => 'delcs.png',		'type' => 1,	'dependence' => 'cs' 	),
array( 'showid' => 'tool_button_', 	'common' => 'toggle_tb_data("createcl")', 	 	'name' => '创建集合', 		'pic' => 'addcl.png',		'type' => 1,	'dependence' => 'cs' 	),
array( 'showid' => 'tool_button_', 	'common' => 'toggle_tb_data("dropcl")', 		'name' => '删除集合', 		'pic' => 'delcl.png',		'type' => 1,	'dependence' => 'cl' 	),
array( 'showid' => 'tool_button_', 	'common' => 'toggle_tb_data("split")', 	 		'name' => '集合切分', 		'pic' => 'splitcl.png',		'type' => 1,	'dependence' => 'cl' 	),
array( 'showid' => 'tool_button_', 	'common' => 'toggle_tb_data("createindex")', 	'name' => '创建索引', 		'pic' => 'addindex.png',	'type' => 1,	'dependence' => 'index' ),
array( 'showid' => 'tool_button_', 	'common' => 'toggle_tb_data("deleteindex")', 	'name' => '删除索引', 		'pic' => 'delindex.png',	'type' => 1,	'dependence' => 'index' ),
array( 'showid' => 'tool_button_', 	'common' => 'toggle_tb_data("insert")', 	 	'name' => 'Insert', 		'pic' => 'adddata.png',		'type' => 1,	'dependence' => 'data' 	),
array( 'showid' => 'tool_button_', 	'common' => 'toggle_tb_data("delete")', 	 	'name' => 'Delete',		 	'pic' => 'deldata.png',		'type' => 1,	'dependence' => 'data' 	),
array( 'showid' => 'tool_button_', 	'common' => 'toggle_tb_data("update")', 	 	'name' => 'Update',		 	'pic' => 'updatedata.png',	'type' => 1,	'dependence' => 'data' 	),
array( 'showid' => 'tool_button_', 	'common' => 'toggle_tb_data("find")', 	 		'name' => 'Find',		 	'pic' => 'finddata.png',	'type' => 1,	'dependence' => 'data' 	),
array( 'showid' => 'tool_button_', 	'common' => 'convert_record_show()', 	 		'name' => '切换记录展示',		'pic' => 'transformation.png',	'type' => 2,	'dependence' => 'data' 	)

) ;

//提示框的参数列表
$globalvar_tool_parameter_cn = array(
//		显示或隐藏用的id			//输入框的id									输入框的类型				标题										输入框文字提示								类型
array( 'showid' => 'tb-data-', 	'inputid' => 'tool_data_record',			'input' => 'text', 		'name_title' => 'Record',				'placeholder' => '插入记录',		 			'type' => 1 ),
array( 'showid' => 'tb-data-', 	'inputid' => 'tool_data_rule',				'input' => 'input', 	'name_title' => 'Rule',					'placeholder' => '更新规则',		 			'type' => 1 ),
array( 'showid' => 'tb-data-', 	'inputid' => 'tool_data_sourcename',		'input' => 'input', 	'name_title' => 'Source Group Name',	'placeholder' => '源分区组名',	 			'type' => 1 ),
array( 'showid' => 'tb-data-', 	'inputid' => 'tool_data_destname',			'input' => 'input', 	'name_title' => 'Dest Group Name',		'placeholder' => '目标分区组名',	 			'type' => 1 ),
array( 'showid' => 'tb-data-', 	'inputid' => 'tool_data_condition',			'input' => 'input', 	'name_title' => 'Condition',			'placeholder' => '匹配条件',					'type' => 1 ),
array( 'showid' => 'tb-data-', 	'inputid' => 'tool_data_selecter',			'input' => 'input', 	'name_title' => 'Selecter',				'placeholder' => '查询字段',					'type' => 1 ),
array( 'showid' => 'tb-data-', 	'inputid' => 'tool_data_orderby',			'input' => 'input', 	'name_title' => 'OrderBy',				'placeholder' => '排序条件',					'type' => 1 ),
array( 'showid' => 'tb-data-', 	'inputid' => 'tool_data_hint',				'input' => 'input', 	'name_title' => 'Hint',					'placeholder' => '提示',						'type' => 1 ),
array( 'showid' => 'tb-data-', 	'inputid' => 'tool_data_numtoskip',			'input' => 'input', 	'name_title' => 'Num To Skip',			'placeholder' => '跳过记录数，default：0',	'type' => 1 ),
array( 'showid' => 'tb-data-', 	'inputid' => 'tool_data_numbertoreturn',	'input' => 'input', 	'name_title' => 'Num To Return',		'placeholder' => '返回记录数，default：-1',	'type' => 1 ),
array( 'showid' => 'tb-data-', 	'inputid' => 'tool_data_csname',			'input' => 'input', 	'name_title' => 'Collection Space Name','placeholder' => '集合空间名',		 		'type' => 1 ),
array( 'showid' => 'tb-data-', 	'inputid' => 'tool_data_pagesize',			'input' => 'input', 	'name_title' => 'Options',			'placeholder' => '',		 				'type' => 1 ),
array( 'showid' => 'tb-data-', 	'inputid' => 'tool_data_clname',			'input' => 'input', 	'name_title' => 'Collection Name',		'placeholder' => '集合名',		 			'type' => 1 ),
array( 'showid' => 'tb-data-', 	'inputid' => 'tool_data_options',			'input' => 'text', 		'name_title' => 'Options',				'placeholder' => '参数，default：NULL',		'type' => 1 ),
array( 'showid' => 'tb-data-', 	'inputid' => 'tool_data_indexdef',			'input' => 'input', 	'name_title' => 'Index Definition',		'placeholder' => '索引定义',		 			'type' => 1 ),
array( 'showid' => 'tb-data-', 	'inputid' => 'tool_data_indexname',			'input' => 'input', 	'name_title' => 'Index Name',			'placeholder' => '索引名',		 			'type' => 1 ),
array( 'showid' => 'tb-data-', 	'inputid' => 'tool_data_indexunique',		'input' => 'checkbox', 	'name_title' => 'Unique',				'placeholder' => '唯一索引，default：false',	'type' => 3 ),
array( 'showid' => 'tb-data-', 	'inputid' => 'tool_data_indexenforced',		'input' => 'checkbox', 	'name_title' => 'Enforced',				'placeholder' => '强制唯一，default：false',	'type' => 3 ),
array( 'showid' => 'tb-data-', 	'inputid' => '',							'input' => 'div', 		'name_title' => '注意！',   'placeholder' => '由于网页显示空间有限，最大查询返回20条记录',	'type' => 4 ),
array( 'showid' => 'tb-data-', 	'inputid' => 'tool_data_endcondition',		'input' => 'input', 	'name_title' => 'End Condition',		'placeholder' => '结束匹配条件',				'type' => 1 )

) ;





















?>