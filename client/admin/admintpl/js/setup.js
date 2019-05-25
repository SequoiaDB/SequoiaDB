//页面加载后自动调整右窗口的宽度和高度
$(document).ready(function()
{
	var widthnum = document.documentElement.clientWidth ;
	var heightnum = document.documentElement.clientHeight ;
	document.getElementById("h_context").style.width = (widthnum - 240) + "px" ;
	heightnum = (heightnum - 200) ;
	if ( heightnum < 500 )
	{
		heightnum = 500 ;
	}
	document.getElementById("h_context").style.height = heightnum + "px" ;
	
	//窗口改变时自动调整右窗口的宽度和高度
	window.onresize = function ()
	{
		var widthnum = document.documentElement.clientWidth ;
		var heightnum = document.documentElement.clientHeight ;
		document.getElementById("h_context").style.width = (widthnum - 240) + "px" ;
		heightnum = (heightnum - 200) ;
		if ( heightnum < 500 )
		{
			heightnum = 500 ;
		}
		document.getElementById("h_context").style.height = heightnum + "px" ;
	}
})

//跳到第几步骤
function switchshow( num )
{
	if ( num > 0 )
	{
		switch_left_show( num - 1 ) ;
		switch_left_image( num - 1 ) ;
	}
	switch_next_start_button(1);
	var obj = document.getElementById("step_"+num) ;
	if ( obj )
	{
		hiddendiv() ;
		if ( num == 6 )
		{
			//触发获取
			get_all_node_to_confsum() ;
		}
		else if ( num == 7 )
		{
			//触发获取
			switch_next_start_button(2) ;
			add_node_list_to_check_list() ;
		}
		else if ( num == 8 )
		{
			//触发获取
			switch_step_8_tab(0);
			switch_next_start_button(2) ;
			add_host_list_to_send_install() ;
			add_node_list_to_i_c_c_d() ;
			add_group_to_active_list();
		}
		obj.style.display = "" ;
	}
}

//显示当前步骤
function switch_left_show( num )
{
	var config_title_div_obj = document.getElementById("config_title").getElementsByClassName("config_title_div") ;
	
	var len = config_title_div_obj.length ;
	
	for ( var i = 0; i < len; ++i )
	{
		var temp = config_title_div_obj.item(i) ;
		if ( temp )
		{
			temp.style.display = 'none' ;
		}
	}
	
	var div_obj = config_title_div_obj.item(num) ;
	if ( div_obj )
	{
		div_obj.style.display = '' ;
	}
}

//控制左列表的图
function switch_left_image( num )
{
	var img_list = document.getElementById("config_title_img").getElementsByTagName("img") ;
	var len = img_list.length ;
	for ( var i = 0; i < len; ++i )
	{
		var img_obj = img_list.item(i) ;
		if ( i < num )
		{
			img_obj.src = "./images/status_1.png" ;
		}
		else if ( i == num )
		{
			img_obj.src = "./images/status_2.png" ;
		}
		else
		{
			img_obj.src = "./images/status_3.png" ;
		}
	}
}

//开始按钮的事件
function switchstart()
{
	var rc = true ;
	var setup_num = getCurrStep() ;
	if ( setup_num == 7 )
	{
		sys_check_async() ;
	}
	else if ( setup_num == 8 )
	{
		send_install_file() ;
	}
}

//继续按钮
function goon_switch_next_setp()
{
	switch_step_8_tab_next() ;
	var num = get_switch_step_8() ;
	if ( num == 1 )
	{
		install_the_file() ;
	}
	else if ( num == 2 )
	{
		//setTimeout( "create_node( 'step_8_tab_table_3' )", 500 ) ;
		create_node( 'step_8_tab_table_3' ) ;
	}
	else if ( num == 3 )
	{
		create_node( 'step_8_tab_table_4' ) ;
		//setTimeout( "create_node( 'step_8_tab_table_4' )", 500 ) ;
	}
	else if ( num == 4 )
	{
		create_node( 'step_8_tab_table_5' ) ;
		//setTimeout( "create_node( 'step_8_tab_table_5' )", 500 ) ;
	}
	else if ( num == 5 )
	{
		active_group() ;
	}
}

//切换部署集群分页到下一页
function switch_step_8_tab_next()
{
	var num = get_switch_step_8() ;
	if ( num == -1 )
	{
		num = 0 ;
	}
	else
	{
		++num ;
	}
	switch_step_8_tab( num ) ;
}

//切换部署集群分页
function switch_step_8_tab( num )
{
	$('#step_8_tab_ul li:eq(' + num + ') a').tab('show') ;
}

//判断部署集群当前是第几个分页
function get_switch_step_8()
{
	var step_ul_obj = document.getElementById("step_8_tab_ul") ;
	var step_li_obj = step_ul_obj.getElementsByTagName( 'li' ) ;
	
	var li_len = step_li_obj.length ;
	for ( var i = 0; i < li_len; ++i )
	{
		if ( step_li_obj.item(i).className == "active" )
		{
			return i ;
		}
	}
	return -1 ;
}

//把节点加载到协调、编目、数据列表中
function add_node_list_to_i_c_c_d()
{
	var source_tab_obj = document.getElementById( "step_6_tab_table_3" ) ;
	var dest_tab_obj_2 = document.getElementById( "step_8_tab_table_3" ) ;
	var dest_tab_obj_3 = document.getElementById( "step_8_tab_table_4" ) ;
	var dest_tab_obj_4 = document.getElementById( "step_8_tab_table_5" ) ;
	var dest_tab_obj = null ;
	var trs_obj = null ;
	
	var table_row_len = source_tab_obj.rows.length ;
	
	dest_tab_obj = dest_tab_obj_2 ;
	trs_obj = dest_tab_obj.getElementsByTagName("tr");
	for(var i = trs_obj.length - 1; i > 0; i--)
	{
		dest_tab_obj.deleteRow( i ) ;
	}
	
	dest_tab_obj = dest_tab_obj_3 ;
	trs_obj = dest_tab_obj.getElementsByTagName("tr");
	for(var i = trs_obj.length - 1; i > 0; i--)
	{
		dest_tab_obj.deleteRow( i ) ;
	}
	
	dest_tab_obj = dest_tab_obj_4 ;
	trs_obj = dest_tab_obj.getElementsByTagName("tr");
	for(var i = trs_obj.length - 1; i > 0; i--)
	{
		dest_tab_obj.deleteRow( i ) ;
	}
	
	for ( var i = 1; i < table_row_len; ++i )
	{
		var table_td_arr = new Array( '<img src="./images/loading.gif" width="15" height="15" style="display:none" /><img src="./images/tick.png" width="15" height="15" style="display:none" /><img src="./images/delete.png" width="15" height="15" style="display:none" />',
												source_tab_obj.rows[i].cells[0].innerHTML,
												source_tab_obj.rows[i].cells[1].innerHTML,
												source_tab_obj.rows[i].cells[2].innerHTML,
												'' ) ;
		if ( source_tab_obj.rows[i].cells[3].innerHTML == 'coord' )
		{
			table_insert_row_2( dest_tab_obj_2, table_td_arr ) ;
		}
		else if ( source_tab_obj.rows[i].cells[3].innerHTML == 'catalog' )
		{
			table_insert_row_2( dest_tab_obj_3, table_td_arr ) ;
		}
		else if ( source_tab_obj.rows[i].cells[3].innerHTML == 'data' )
		{
			table_insert_row_2( dest_tab_obj_4, table_td_arr ) ;
		}
	}
}

//激活分区组
function active_group()
{
	switch_next_start_button(3);
	var table_obj = document.getElementById( "step_8_tab_table_6" ) ;
	var table_row_len = table_obj.rows.length ;
	var success = 0 ;
	var failed  = 0 ;
	var order = '' ;
	var image = null ;
	
	var sdbpath = "" ;
	var coordadd = "" ;
	var coordport = "" ;
	
	var host_tab_obj  = document.getElementById("step_2_tab_table") ;
	var coord_tab_obj = document.getElementById("step_3_tab_table") ;
	if ( coord_tab_obj.rows[1] )
	{
		sdbpath = '/bin/sdb' ;
		coordadd = coord_tab_obj.rows[1].cells[2].innerHTML ;
		coordport = coord_tab_obj.rows[1].cells[3].innerHTML ;
		
		var host_tab_len = host_tab_obj.rows.length ;
		for ( var i = 1; i < host_tab_len; ++i )
		{
			if ( host_tab_obj.rows[i].cells[2].innerHTML == coordadd )
			{
				sdbpath = host_tab_obj.rows[i].cells[3].innerHTML + sdbpath ;
				break ;
			}
		}
	}
	
	function callback_active_group( str )
	{
		//alert ( str ) ;
		try
		{
			var json_obj = eval('(' + str + ')');
			var line = json_obj['line'] ;
			show_diary_win( json_obj['diary'] ) ;
			if ( json_obj['errno'] != 0 )
			{
				if ( line > 0 )
				{
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(0) ;
					image.style.display = "none" ;
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(1) ;
					image.style.display = "none" ;
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(2) ;
					image.style.display = "" ;
					//table_obj.rows[line].cells[0].innerHTML = '<img src="./images/delete.png" width="15" height="15" />' ;
					table_obj.rows[line].cells[2].innerHTML = json_obj['output'] ;
				}
				++failed ;
			}
			else
			{
				if ( line > 0 )
				{
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(0) ;
					image.style.display = "none" ;
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(1) ;
					image.style.display = "" ;
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(2) ;
					image.style.display = "none" ;
					//table_obj.rows[line].cells[0].innerHTML = '<img src="./images/tick.png" width="15" height="15" />' ;
					table_obj.rows[line].cells[2].innerHTML = json_obj['output'] ;
				}
				++success ;
			}
		}
		catch(e)
		{
			show_error_win( "系统错误,请重试" ) ;
			++failed ;
		}
	}
	function callback_active_group_err()
	{
		show_error_win( "服务端网络错误,请重试" ) ;
		++failed ;
	}
	for ( var i = 1; i < table_row_len; ++i )
	{
		image = table_obj.rows[i].cells[0].getElementsByTagName( 'img' ).item(0) ;
		image.style.display = "" ;
		image = table_obj.rows[i].cells[0].getElementsByTagName( 'img' ).item(1) ;
		image.style.display = "none" ;
		image = table_obj.rows[i].cells[0].getElementsByTagName( 'img' ).item(2) ;
		image.style.display = "none" ;
		order = sdbpath + ' ' + coordadd + ' ' + coordport + ' ' + table_obj.rows[i].cells[1].innerHTML ;
		var post_context = 'common=' + convert2post( 'activegroup' ) + '&order=' + convert2post( order ) + '&line=' + convert2post( i ) ;
		ajax2sendNew( 'POST', './shell/phpexec.php', post_context, true, callback_active_group, callback_active_group_err, callback_before, callback_complete ) ;
	}

	window.pd_check_end_a_g = function()
	{
		if ( ( success + failed ) == ( table_row_len - 1 ) )
		{
			if ( failed == 0 )
			{
				//window.location.href = './index.php?p=group' ;
			}
			else
			{
				//switchshow(7) ;
				switch_next_start_button(2) ;
			}
		}
		else
		{
			window.setTimeout( "pd_check_end_a_g()", 500 ) ;
		}
	}
	window.setTimeout( "pd_check_end_a_g()", 500 ) ;
}

//创建节点 包括 协调 编目 数据
function create_node( table_name )
{
	switch_next_start_button(3);
	var table_obj = document.getElementById( table_name ) ;
	var table_row_len = table_obj.rows.length ;
	var success = 0 ;
	var failed = 0 ;
	var order = '' ;
	var image = null ;
	var coord_add = "" ;
	var coord_port = "" ;
	var second_start = false ;

	var coord_tab = document.getElementById( "step_8_tab_table_3" ) ;
	if ( coord_tab.rows[1] )
	{
		coord_add = coord_tab.rows[1].cells[2].innerHTML ;
		coord_port = coord_tab.rows[1].cells[3].innerHTML ;
	}


	image = table_obj.rows[1].cells[0].getElementsByTagName( 'img' ).item(0) ;
	image.style.display = "" ;
	image = table_obj.rows[1].cells[0].getElementsByTagName( 'img' ).item(1) ;
	image.style.display = "none" ;
	image = table_obj.rows[1].cells[0].getElementsByTagName( 'img' ).item(2) ;
	image.style.display = "none" ;

	function callback_install_the_file( str )
	{
		try
		{
			var json_obj = eval('(' + str + ')');
			var line = json_obj['line'] ;
			show_diary_win( json_obj['diary'] ) ;
			if ( json_obj['errno'] != 0 )
			{
				if ( line > 0 )
				{
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(0) ;
					image.style.display = "none" ;
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(1) ;
					image.style.display = "none" ;
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(2) ;
					image.style.display = "" ;
					//table_obj.rows[line].cells[0].innerHTML = '<img src="./images/delete.png" width="15" height="15" />' ;
					table_obj.rows[line].cells[4].innerHTML = json_obj['output'] ;
				}
				++failed ;
			}
			else
			{
				if ( line > 0 )
				{
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(0) ;
					image.style.display = "none" ;
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(1) ;
					image.style.display = "" ;
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(2) ;
					image.style.display = "none" ;
					//table_obj.rows[line].cells[0].innerHTML = '<img src="./images/tick.png" width="15" height="15" />' ;
					table_obj.rows[line].cells[4].innerHTML = json_obj['output'] ;
				}
				++success ;
			}
		}
		catch(e)
		{
			show_error_win( "系统错误,请重试" ) ;
			++failed ;
		}
	}
	function callback_install_the_file_err()
	{
		show_error_win( "服务端网络错误,请重试" ) ;
		++failed ;
	}
	function callback_create_node_complete()
	{
		for ( var i = 2; i < table_row_len; ++i )
		{
			image = table_obj.rows[i].cells[0].getElementsByTagName( 'img' ).item(0) ;
			image.style.display = "" ;
			image = table_obj.rows[i].cells[0].getElementsByTagName( 'img' ).item(1) ;
			image.style.display = "none" ;
			image = table_obj.rows[i].cells[0].getElementsByTagName( 'img' ).item(2) ;
			image.style.display = "none" ;
			order = table_obj.rows[i].cells[2].innerHTML + ' 1 ' + coord_add + ' ' + coord_port + ' ' + table_obj.rows[i].cells[3].innerHTML ;
			var post_context = 'common=' + convert2post( 'createnode' ) + '&order=' + convert2post( order ) + '&line=' + convert2post( i ) ;
			ajax2sendNew( 'POST', './shell/phpexec.php', post_context, true, callback_install_the_file, callback_install_the_file_err, callback_before, callback_complete ) ;
		}
	}
	order = table_obj.rows[1].cells[2].innerHTML + ' 0 ' + coord_add + ' ' + coord_port + ' ' + table_obj.rows[1].cells[3].innerHTML ;
	var post_context = 'common=' + convert2post( 'createnode' ) + '&order=' + convert2post( order ) + '&line=' + convert2post( 1 ) ;
	ajax2sendNew( 'POST', './shell/phpexec.php', post_context, true, callback_install_the_file, callback_install_the_file_err, callback_before, callback_create_node_complete ) ;

	window.pd_check_end_c_n = function()
	{
		if ( ( success + failed ) == ( table_row_len - 1 ) )
		{
			if ( failed == 0 )
			{
				switch_next_start_button(4) ;
			}
			else
			{
				//switchshow(7) ;
				switch_next_start_button(2) ;
			}
		}
		else
		{
			window.setTimeout( "pd_check_end_c_n()", 500 ) ;
		}
	}
	window.setTimeout( "pd_check_end_c_n()", 500 ) ;
}

//安装sequoiadb
function install_the_file()
{
	switch_next_start_button(3);
	var table_obj = document.getElementById( "step_8_tab_table_2" ) ;
	var table_row_len = table_obj.rows.length ;
	var success = 0 ;
	var failed  = 0 ;
	var order = '' ;
	var image = null ;
	
	function callback_install_the_file( str )
	{
		//alert ( str ) ;
		try
		{
			var json_obj = eval('(' + str + ')');
			var line = json_obj['line'] ;
			show_diary_win( json_obj['diary'] ) ;
			if ( json_obj['errno'] != 0 )
			{
				if ( line > 0 )
				{
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(0) ;
					image.style.display = "none" ;
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(1) ;
					image.style.display = "none" ;
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(2) ;
					image.style.display = "" ;
					//table_obj.rows[line].cells[0].innerHTML = '<img src="./images/delete.png" width="15" height="15" />' ;
					table_obj.rows[line].cells[3].innerHTML = json_obj['output'] ;
				}
				++failed ;
			}
			else
			{
				if ( line > 0 )
				{
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(0) ;
					image.style.display = "none" ;
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(1) ;
					image.style.display = "" ;
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(2) ;
					image.style.display = "none" ;
					//table_obj.rows[line].cells[0].innerHTML = '<img src="./images/tick.png" width="15" height="15" />' ;
					table_obj.rows[line].cells[3].innerHTML = json_obj['output'] ;
				}
				++success ;
			}
		}
		catch(e)
		{
			show_error_win( "系统错误,请重试" ) ;
			++failed ;
		}
	}
	function callback_install_the_file_err()
	{
		show_error_win( "服务端网络错误,请重试" ) ;
		++failed ;
	}
	for ( var i = 1; i < table_row_len; ++i )
	{
		image = table_obj.rows[i].cells[0].getElementsByTagName( 'img' ).item(0) ;
		image.style.display = "" ;
		image = table_obj.rows[i].cells[0].getElementsByTagName( 'img' ).item(1) ;
		image.style.display = "none" ;
		image = table_obj.rows[i].cells[0].getElementsByTagName( 'img' ).item(2) ;
		image.style.display = "none" ;
		order = table_obj.rows[i].cells[2].innerHTML ;
		var post_context = 'common=' + convert2post( 'installthefile' ) + '&order=' + convert2post( order ) + '&line=' + convert2post( i ) ;
		ajax2sendNew( 'POST', './shell/phpexec.php', post_context, true, callback_install_the_file, callback_install_the_file_err, callback_before, callback_complete ) ;
	}

	window.pd_check_end_i_t_f = function()
	{
		if ( ( success + failed ) == ( table_row_len - 1 ) )
		{
			if ( failed == 0 )
			{
				switch_next_start_button(4) ;
			}
			else
			{
				//switchshow(7) ;
				switch_next_start_button(2) ;
			}
		}
		else
		{
			window.setTimeout( "pd_check_end_i_t_f()", 500 ) ;
		}
	}
	window.setTimeout( "pd_check_end_i_t_f()", 500 ) ;
}

//分发安装包
function send_install_file()
{
	switch_next_start_button(3);
	var table_obj = document.getElementById( "step_8_tab_table_1" ) ;
	var table_row_len = table_obj.rows.length ;
	var success = 0 ;
	var failed  = 0 ;
	var order = '' ;
	var image = null ;
	function callback_send_install_file( str )
	{
		//alert ( str ) ;
		try
		{
			var json_obj = eval('(' + str + ')');
			var line = json_obj['line'] ;
			show_diary_win( json_obj['diary'] ) ;
			if ( json_obj['errno'] != 0 )
			{
				if ( line > 0 )
				{
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(0) ;
					image.style.display = "none" ;
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(1) ;
					image.style.display = "none" ;
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(2) ;
					image.style.display = "" ;
					//table_obj.rows[line].cells[0].innerHTML = '<img src="./images/delete.png" width="15" height="15" />' ;
					table_obj.rows[line].cells[3].innerHTML = json_obj['output'] ;
				}
				++failed ;
			}
			else
			{
				if ( line > 0 )
				{
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(0) ;
					image.style.display = "none" ;
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(1) ;
					image.style.display = "" ;
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(2) ;
					image.style.display = "none" ;
					//table_obj.rows[line].cells[0].innerHTML = '<img src="./images/tick.png" width="15" height="15" />' ;
					table_obj.rows[line].cells[3].innerHTML = json_obj['output'] ;
				}
				++success ;
			}
		}
		catch(e)
		{
			show_error_win( "系统错误,请重试" ) ;
			++failed ;
		}
	}
	function callback_send_install_file_err()
	{
		show_error_win( "服务端网络错误,请重试" ) ;
		++failed ;
	}
	for ( var i = 1; i < table_row_len; ++i )
	{
		image = table_obj.rows[i].cells[0].getElementsByTagName( 'img' ).item(0) ;
		image.style.display = "" ;
		image = table_obj.rows[i].cells[0].getElementsByTagName( 'img' ).item(1) ;
		image.style.display = "none" ;
		image = table_obj.rows[i].cells[0].getElementsByTagName( 'img' ).item(2) ;
		image.style.display = "none" ;
		order = table_obj.rows[i].cells[2].innerHTML ;
		var post_context = 'common=' + convert2post( 'sendinstallfile' ) + '&order=' + convert2post( order ) + '&line=' + convert2post( i ) ;
		ajax2sendNew( 'POST', './shell/phpexec.php', post_context, true, callback_send_install_file, callback_send_install_file_err, callback_before, callback_complete ) ;
	}

	window.pd_check_end_s_i_f = function()
	{
		if ( ( success + failed ) == ( table_row_len - 1 ) )
		{
			if ( failed == 0 )
			{
				switch_next_start_button(4) ;
			}
			else
			{
				//switchshow(7) ;
				switch_next_start_button(2) ;
			}
		}
		else
		{
			window.setTimeout( "pd_check_end_s_i_f()", 500 ) ;
		}
	}
	window.setTimeout( "pd_check_end_s_i_f()", 500 ) ;
}

//系统环境检查同步执行
function sys_check_async()
{
	switch_next_start_button(3);
	var table_obj = document.getElementById( "step_7_tab_table" ) ;
	var table_row_len = table_obj.rows.length ;
	var success = 0 ;
	var failed  = 0 ;
	var order = '' ;
	var image = null ;
	function callback_sys_check_async( str )
	{
		//alert ( str ) ;
		try
		{
			var json_obj = eval('(' + str + ')');
			var line = json_obj['line'] ;
			show_diary_win( json_obj['diary'] ) ;
			if ( json_obj['errno'] != 0 )
			{
				if ( line > 0 )
				{
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(0) ;
					image.style.display = "none" ;
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(1) ;
					image.style.display = "none" ;
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(2) ;
					image.style.display = "" ;
					//table_obj.rows[line].cells[0].innerHTML = '<img src="./images/delete.png" width="15" height="15" />' ;
					table_obj.rows[line].cells[3].innerHTML = json_obj['output'] ;
				}
				++failed ;
			}
			else
			{
				if ( line > 0 )
				{
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(0) ;
					image.style.display = "none" ;
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(1) ;
					image.style.display = "" ;
					image = table_obj.rows[line].cells[0].getElementsByTagName( 'img' ).item(2) ;
					image.style.display = "none" ;
					//table_obj.rows[line].cells[0].innerHTML = '<img src="./images/tick.png" width="15" height="15" />' ;
					table_obj.rows[line].cells[3].innerHTML = json_obj['output'] ;
				}
				++success ;
			}
		}
		catch(e)
		{
			show_error_win( "系统错误,请重试" ) ;
			++failed ;
		}
	}
	function callback_sys_check_async_err()
	{
		show_error_win( "服务端网络错误,请重试" ) ;
		++failed ;
	}
	for ( var i = 1; i < table_row_len; ++i )
	{
		image = table_obj.rows[i].cells[0].getElementsByTagName( 'img' ).item(0) ;
		image.style.display = "" ;
		image = table_obj.rows[i].cells[0].getElementsByTagName( 'img' ).item(1) ;
		image.style.display = "none" ;
		image = table_obj.rows[i].cells[0].getElementsByTagName( 'img' ).item(2) ;
		image.style.display = "none" ;
		order = table_obj.rows[i].cells[2].innerHTML ;
		var post_context = 'common=' + convert2post( 'checkenvhost' ) + '&order=' + convert2post( order ) + '&line=' + convert2post( i ) ;
		ajax2sendNew( 'POST', './shell/phpexec.php', post_context, true, callback_sys_check_async, callback_sys_check_async_err, callback_before, callback_complete ) ;
	}

	window.pd_check_end_s_c_a = function()
	{
		if ( ( success + failed ) == ( table_row_len - 1 ) )
		{
			if ( failed == 0 )
			{
				switch_next_start_button(1) ;
			}
			else
			{
				switch_next_start_button(2) ;
			}
		}
		else
		{
			window.setTimeout( "pd_check_end_s_c_a()", 500 ) ;
		}
	}
	window.setTimeout( "pd_check_end_s_c_a()", 500 ) ;
}

//切换下一步跟开始按钮显示
//type 1: 显示下一步 2：显示开始 3：全隐藏 4：显示继续
function switch_next_start_button( type )
{
	var p_button_obj = document.getElementById("setup_button_m") ;	
	var p_button_child_obj = p_button_obj.getElementsByTagName("div") ;

	var child_button_obj = null ;
	
	if ( type == 1 )
	{
		child_button_obj = p_button_child_obj.item(0) ;
		child_button_obj.style.display = '' ;
		child_button_obj = p_button_child_obj.item(1) ;
		child_button_obj.style.display = 'none' ;
		child_button_obj = p_button_child_obj.item(2) ;
		child_button_obj.style.display = 'none' ;
	}
	else if ( type == 2 )
	{
		child_button_obj = p_button_child_obj.item(0) ;
		child_button_obj.style.display = 'none' ;
		child_button_obj = p_button_child_obj.item(1) ;
		child_button_obj.style.display = '' ;
		child_button_obj = p_button_child_obj.item(2) ;
		child_button_obj.style.display = 'none' ;
	}
	else if ( type == 3 )
	{
		child_button_obj = p_button_child_obj.item(0) ;
		child_button_obj.style.display = 'none' ;
		child_button_obj = p_button_child_obj.item(1) ;
		child_button_obj.style.display = 'none' ;
		child_button_obj = p_button_child_obj.item(2) ;
		child_button_obj.style.display = 'none' ;
	}
	else if ( type == 4 )
	{
		child_button_obj = p_button_child_obj.item(0) ;
		child_button_obj.style.display = 'none' ;
		child_button_obj = p_button_child_obj.item(1) ;
		child_button_obj.style.display = 'none' ;
		child_button_obj = p_button_child_obj.item(2) ;
		child_button_obj.style.display = '' ;
	}
}

//把分区组加载到激活列表中
function add_group_to_active_list()
{
	var group_arr = get_group_list() ;
	var dest_tab_obj = document.getElementById( "step_8_tab_table_6" ) ;
	var trs_obj = null ;
	
	trs_obj = dest_tab_obj.getElementsByTagName("tr");
	for(var i = trs_obj.length - 1; i > 0; i--)
	{
		dest_tab_obj.deleteRow( i ) ;
	}
	
	for( key in group_arr )
	{
		var table_td_arr = new Array( '<img src="./images/loading.gif" width="15" height="15" style="display:none" /><img src="./images/tick.png" width="15" height="15" style="display:none" /><img src="./images/delete.png" width="15" height="15" style="display:none" />',
												group_arr[key],
												'' ) ;
		table_insert_row_2( dest_tab_obj, table_td_arr ) ;
	}
}

//把主机加载到分发列表中
function add_host_list_to_send_install()
{
	var source_tab_obj = document.getElementById( "step_2_tab_table" ) ;
	var dest_tab_obj_1 = document.getElementById( "step_8_tab_table_1" ) ;
	var dest_tab_obj_2 = document.getElementById( "step_8_tab_table_2" ) ;
	var trs_obj = null ;
	
	var table_row_len = source_tab_obj.rows.length ;
	
	var dest_tab_obj = dest_tab_obj_1 ;
	trs_obj = dest_tab_obj.getElementsByTagName("tr");
	for(var i = trs_obj.length - 1; i > 0; i--)
	{
		dest_tab_obj.deleteRow( i ) ;
	}
	
	var dest_tab_obj = dest_tab_obj_2 ;
	trs_obj = dest_tab_obj.getElementsByTagName("tr");
	for(var i = trs_obj.length - 1; i > 0; i--)
	{
		dest_tab_obj.deleteRow( i ) ;
	}
	
	for ( var i = 1; i < table_row_len; ++i )
	{
		var table_td_arr = new Array( '<img src="./images/loading.gif" width="15" height="15" style="display:none" /><img src="./images/tick.png" width="15" height="15" style="display:none" /><img src="./images/delete.png" width="15" height="15" style="display:none" />',
												source_tab_obj.rows[i].cells[1].innerHTML,
												source_tab_obj.rows[i].cells[2].innerHTML,
												'' ) ;
		table_insert_row_2( dest_tab_obj_1, table_td_arr ) ;
		table_insert_row_2( dest_tab_obj_2, table_td_arr ) ;
	}
}

//把节点加载到系统环境检查列表
function add_node_list_to_check_list()
{
	var source_tab_obj = document.getElementById( "step_2_tab_table" ) ;
	var dest_tab_obj = document.getElementById( "step_7_tab_table" ) ;
	
	var table_row_len = source_tab_obj.rows.length ;
	
	var trs_obj = dest_tab_obj.getElementsByTagName("tr");
	for(var i = trs_obj.length - 1; i > 0; i--)
	{
		dest_tab_obj.deleteRow( i ) ;
	}
	
	for ( var i = 1; i < table_row_len; ++i )
	{
		var table_td_arr = new Array( '<img src="./images/loading.gif" width="15" height="15" style="display:none" /><img src="./images/tick.png" width="15" height="15" style="display:none" /><img src="./images/delete.png" width="15" height="15" style="display:none" />',
												source_tab_obj.rows[i].cells[1].innerHTML,
												source_tab_obj.rows[i].cells[2].innerHTML,
												'' ) ;
		table_insert_row_2( dest_tab_obj, table_td_arr ) ;
	}
}

//集群配置总结获取信息
function get_all_node_to_confsum()
{
	var confsum_list_obj_1 = document.getElementById("step_6_tab_table_1") ;
	var confsum_list_obj_2 = document.getElementById("step_6_tab_table_2") ;
	var confsum_list_obj_3 = document.getElementById("step_6_tab_table_3") ;
	//清空主机
	delete_table_all_rows( "step_6_tab_table_1" ) ;
	//清空分区组
	delete_table_all_rows( "step_6_tab_table_2" ) ;
	//清空节点
	delete_table_all_rows( "step_6_tab_table_3" ) ;
	
	var host_list_obj   = document.getElementById("step_2_tab_table") ;
	var coord_list_obj  = document.getElementById("step_3_tab_table") ;
	var cata_list_obj   = document.getElementById("step_4_tab_table") ;
	var group_arr		  = get_group_list() ;
	group_arr.push( "协调组&nbsp;(由系统自动生成)" ) ;
	group_arr.push( "编目组&nbsp;(由系统自动生成)" ) ;
	var data_list_obj   = document.getElementById("step_5_tab_table") ;
	//获取主机列表
	set_host_conf_list_table( confsum_list_obj_1, host_list_obj, coord_list_obj, cata_list_obj, data_list_obj ) ;
	//获取分区组列表
	set_group_conf_list_table( confsum_list_obj_2, group_arr, coord_list_obj, cata_list_obj, data_list_obj ) ;
	//获取节点列表
	set_node_conf_list_table( confsum_list_obj_3, coord_list_obj, cata_list_obj, data_list_obj ) ;
}

//配置总结，分区组列表配置
function set_group_conf_list_table( dest_obj,
										 	   group_arr,
											   coord_list_obj,
											   cata_list_obj,
											   data_list_obj )
{
	var table_td_arr = null ;
	for ( key in group_arr )
	{
		var node_sum = 0 ;
		//创建第一行
		table_td_arr = new Array( '<a href="#" onclick="fold_table_in_td(this)"><span class="glyphicon glyphicon-chevron-right"></span></a>',
		                           group_arr[key],
											'0' ) ;
		table_insert_row_2( dest_obj, table_td_arr ) ;
		
		//创建第二行
		var tab_row = dest_obj.insertRow ( dest_obj.rows.length ) ;
		tab_row.style.display = "none" ;
		var c0 = tab_row.insertCell( 0 ) ;
		c0.innerHTML = '' ;
		var c1 = tab_row.insertCell( 1 ) ;
		c1.colSpan = "3" ;
		//创建第二行的表格
		//设置div属性
		var temp_div = document.createElement("div") ;
		temp_div.style.border = "#D2D2D2 solid 1px" ;
		var temp_table = document.createElement("table") ;
		//设置表格属性
		temp_table.className = "table table-hover" ;
		temp_table.style.margin = "0px" ;
		table_td_arr = new Array( "IP", "主机名", "端口", "数据存储路径" ) ;
		table_insert_row_1( temp_table, table_td_arr ) ;

		if ( group_arr[key] == "协调组&nbsp;(由系统自动生成)" )
		{
			var coord_table_row_len = coord_list_obj.rows.length ;
			for ( var k = 1; k < coord_table_row_len; ++k )
			{
				table_td_arr = new Array( coord_list_obj.rows[k].cells[1].innerHTML,
												  coord_list_obj.rows[k].cells[2].innerHTML,
												  coord_list_obj.rows[k].cells[3].innerHTML,
												  coord_list_obj.rows[k].cells[4].innerHTML ) ;
				table_insert_row_2( temp_table, table_td_arr ) ;
				++node_sum ;
			}
		}
		else if ( group_arr[key] == "编目组&nbsp;(由系统自动生成)" )
		{
			var cata_table_row_len = cata_list_obj.rows.length ;
			for ( var k = 1; k < cata_table_row_len; ++k )
			{
				table_td_arr = new Array( cata_list_obj.rows[k].cells[1].innerHTML,
												  cata_list_obj.rows[k].cells[2].innerHTML,
												  cata_list_obj.rows[k].cells[3].innerHTML,
												  cata_list_obj.rows[k].cells[4].innerHTML ) ;
				table_insert_row_2( temp_table, table_td_arr ) ;
				++node_sum ;
			}
		}
		else
		{
			var data_table_row_len = data_list_obj.rows.length ;
			for ( var k = 2; k < data_table_row_len; ++k )
			{
				if ( group_arr[key] == data_list_obj.rows[k].cells[2].innerHTML )
				{
					++k ;
					var child_table_obj = data_list_obj.rows[k].cells[1].getElementsByTagName("table").item(0) ;
					var child_table_len = child_table_obj.rows.length ;
					for ( var l = 1; l < child_table_len; ++l )
					{
						table_td_arr = new Array( child_table_obj.rows[l].cells[1].innerHTML,
														  child_table_obj.rows[l].cells[2].innerHTML,
														  child_table_obj.rows[l].cells[3].innerHTML,
														  child_table_obj.rows[l].cells[4].innerHTML ) ;
						table_insert_row_2( temp_table, table_td_arr ) ;
						++node_sum ;
					}
					break ;
				}
				else
				{
					++k ;
				}
			}
		}
		temp_div.appendChild( temp_table ) ;
		
		dest_obj.rows[dest_obj.rows.length-2].cells[2].innerHTML = node_sum ;

		//最后把表格放入第二行第二格中
		c1.appendChild( temp_div ) ;
	}
}

//配置总结，节点列表配置
function set_node_conf_list_table( dest_obj,
											  coord_list_obj,
											  cata_list_obj,
											  data_list_obj )
{
	var table_td_arr = null ;
		
	var coord_table_row_len = coord_list_obj.rows.length ;
	for ( var k = 1; k < coord_table_row_len; ++k )
	{
		table_td_arr = new Array( coord_list_obj.rows[k].cells[1].innerHTML,
										  coord_list_obj.rows[k].cells[2].innerHTML,
										  coord_list_obj.rows[k].cells[3].innerHTML,
										  "coord","",
										  coord_list_obj.rows[k].cells[4].innerHTML ) ;
		table_insert_row_2( dest_obj, table_td_arr ) ;
	}
	
	var cata_table_row_len = cata_list_obj.rows.length ;
	for ( var k = 1; k < cata_table_row_len; ++k )
	{
		table_td_arr = new Array( cata_list_obj.rows[k].cells[1].innerHTML,
										  cata_list_obj.rows[k].cells[2].innerHTML,
										  cata_list_obj.rows[k].cells[3].innerHTML,
										  "catalog","",
										  cata_list_obj.rows[k].cells[4].innerHTML ) ;
		table_insert_row_2( dest_obj, table_td_arr ) ;
	}
	
	var data_table_row_len = data_list_obj.rows.length ;
	for ( var k = 2; k < data_table_row_len; ++k )
	{
		var group_name = data_list_obj.rows[k].cells[2].innerHTML ;
		++k ;
		var child_table_obj = data_list_obj.rows[k].cells[1].getElementsByTagName("table").item(0) ;
		var child_table_len = child_table_obj.rows.length ;
		for ( var l = 1; l < child_table_len; ++l )
		{
			table_td_arr = new Array( child_table_obj.rows[l].cells[1].innerHTML,
											  child_table_obj.rows[l].cells[2].innerHTML,
											  child_table_obj.rows[l].cells[3].innerHTML,
											  'data',
											  group_name,
											  child_table_obj.rows[l].cells[4].innerHTML ) ;
			table_insert_row_2( dest_obj, table_td_arr ) ;
		}
	}
}

//配置总结，主机列表配置
function set_host_conf_list_table( dest_obj,
											  host_list_obj,
											  coord_list_obj,
											  cata_list_obj,
											  data_list_obj )
{
	var table_td_arr = null ;
	var table_row_len = host_list_obj.rows.length ;
	for ( var i = 1; i < table_row_len; ++i )
	{
		var node_sum = 0 ;
		//创建第一行
		table_td_arr = new Array( '<a href="#" onclick="fold_table_in_td(this)"><span class="glyphicon glyphicon-chevron-right"></span></a>',
		                           host_list_obj.rows[i].cells[1].innerHTML,
											host_list_obj.rows[i].cells[2].innerHTML,
											'0' ) ;
		table_insert_row_2( dest_obj, table_td_arr ) ;
		
		//创建第二行
		var tab_row = dest_obj.insertRow ( dest_obj.rows.length ) ;
		tab_row.style.display = "none" ;
		var c0 = tab_row.insertCell( 0 ) ;
		c0.innerHTML = '' ;
		var c1 = tab_row.insertCell( 1 ) ;
		c1.colSpan = "3" ;
		//创建第二行的表格
		//设置div属性
		var temp_div = document.createElement("div") ;
		temp_div.style.border = "#D2D2D2 solid 1px" ;
		var temp_table = document.createElement("table") ;
		//设置表格属性
		temp_table.className = "table table-hover" ;
		temp_table.style.margin = "0px" ;
		table_td_arr = new Array( "端口", "角色", "所属分区组", "数据存储路径" ) ;
		table_insert_row_1( temp_table, table_td_arr ) ;
		
		var coord_table_row_len = coord_list_obj.rows.length ;
		for ( var k = 1; k < coord_table_row_len; ++k )
		{
			if ( host_list_obj.rows[i].cells[2].innerHTML == coord_list_obj.rows[k].cells[2].innerHTML )
			{
				table_td_arr = new Array( coord_list_obj.rows[k].cells[3].innerHTML,
												  "coord","",
												  coord_list_obj.rows[k].cells[4].innerHTML ) ;
				table_insert_row_2( temp_table, table_td_arr ) ;
				++node_sum ;
			}
		}
		
		var cata_table_row_len = cata_list_obj.rows.length ;
		for ( var k = 1; k < cata_table_row_len; ++k )
		{
			if ( host_list_obj.rows[i].cells[2].innerHTML == cata_list_obj.rows[k].cells[2].innerHTML )
			{
				table_td_arr = new Array( cata_list_obj.rows[k].cells[3].innerHTML,
												  "catalog","",
												  cata_list_obj.rows[k].cells[4].innerHTML ) ;
				table_insert_row_2( temp_table, table_td_arr ) ;
				++node_sum ;
			}
		}
		
		var data_table_row_len = data_list_obj.rows.length ;
		for ( var k = 2; k < data_table_row_len; ++k )
		{
			var group_name = data_list_obj.rows[k].cells[2].innerHTML ;
			++k ;
			var child_table_obj = data_list_obj.rows[k].cells[1].getElementsByTagName("table").item(0) ;
			var child_table_len = child_table_obj.rows.length ;
			for ( var l = 1; l < child_table_len; ++l )
			{
				if ( host_list_obj.rows[i].cells[2].innerHTML == child_table_obj.rows[l].cells[2].innerHTML )
				{
					table_td_arr = new Array( child_table_obj.rows[l].cells[3].innerHTML,
													  'data',
													  group_name,
													  child_table_obj.rows[l].cells[4].innerHTML ) ;
					table_insert_row_2( temp_table, table_td_arr ) ;
					++node_sum ;
				}
			}
		}
		
		temp_div.appendChild( temp_table ) ;
		
		dest_obj.rows[dest_obj.rows.length-2].cells[3].innerHTML = node_sum ;
		
		//最后把表格放入第二行第二格中
		c1.appendChild( temp_div ) ;
	}
}

//获取分区组，添加到列表中
function add_groups_to_list()
{
	var group_list = document.getElementById("group_list_input").value ;
	var group_arr = group_list.split(',') ;
	var group_tab_obj = document.getElementById("step_5_tab_table") ;
	var table_td_arr = null ;
	
	var temp_div_str = group_tab_obj.rows[1].cells[0].innerHTML ;
	

	for ( key in group_arr )
	{
		var str_group_name = group_arr[key].replace(/(^\s*)|(\s*$)/g, "");
		str_group_name = html_decode( str_group_name ) ;
		if ( str_group_name == "" )
		{
			continue ;
		}
		table_td_arr = new Array( '<input type="radio" name="group_radios">',
										  '<a href="#" onclick="fold_table_in_td(this)"><span class="glyphicon glyphicon-chevron-right"></span></a>',
										  str_group_name,
										  '0' ) ;
		table_insert_row_2( group_tab_obj, table_td_arr ) ;

		var tab_row = group_tab_obj.insertRow ( group_tab_obj.rows.length ) ;
		tab_row.style.display = "none" ;
		var c0 = tab_row.insertCell( 0 ) ;
		c0.innerHTML = '' ;
		var c1 = tab_row.insertCell( 1 ) ;
		c1.colSpan = "3" ;
		
		var temp_div = document.createElement("div") ;
		temp_div.style.marginBottom = '10px' ;
		temp_div.style.border = '#D2D2D2 solid 1px' ;
		temp_div.style.background = '#FFF' ;
		
		var temp_table = document.createElement("table") ;
		temp_table.className = 'table table-striped' ;
		temp_table.style.margin = '0px' ;
		
		var temp_table_th = null ;
		var tab_row = temp_table.insertRow ( temp_table.rows.length ) ;
		temp_table_th = document.createElement("th") ;
		temp_table_th.innerHTML = '<input type="checkbox" onchange="set_all_checkbox2(this.parentNode.parentNode.parentNode,this);">' ;
		tab_row.appendChild( temp_table_th ) ;
		
		temp_table_th = document.createElement("th") ;
		temp_table_th.innerHTML = '<a href="#" onclick="">IP&nbsp;<span class="caret"></span></a>' ;
		tab_row.appendChild( temp_table_th ) ;
		
		temp_table_th = document.createElement("th") ;
		temp_table_th.innerHTML = '<a href="#" onclick="">主机名&nbsp;<span class="caret"></span></a>' ;
		tab_row.appendChild( temp_table_th ) ;
		
		temp_table_th = document.createElement("th") ;
		temp_table_th.innerHTML = '<a href="#" onclick="">端口&nbsp;<span class="caret"></span></a>' ;
		tab_row.appendChild( temp_table_th ) ;
		
		temp_table_th = document.createElement("th") ;
		temp_table_th.innerHTML = '<a href="#" onclick="">路径&nbsp;<span class="caret"></span></a>' ;
		tab_row.appendChild( temp_table_th ) ;
		
		temp_table_th = document.createElement("th") ;
		temp_table_th.style.display = "none" ;
		temp_table_th.innerHTML = temp_div_str ;
		tab_row.appendChild( temp_table_th ) ;

		
		temp_div.appendChild( temp_table ) ;
		
		c1.appendChild( temp_div ) ;
	}
}

//配置主机，把配置写到主机列表
function set_host_conf_table()
{
	var table_obj = document.getElementById('step_2_tab_table') ;
	var input_obj = document.getElementById('host_config_table').getElementsByTagName('input') ;
	
	var path = input_obj.item(0).value ;
	var group = input_obj.item(1).value ;
	var user = input_obj.item(2).value ;
	var password = input_obj.item(3).value ;
	
	var table_len = table_obj.rows.length ;
	for ( var i = 1; i < table_len; ++i )
	{
		var input_box = table_obj.rows[i].cells[0].getElementsByTagName('input').item(0) ;
		if ( input_box.checked == true )
		{
			table_obj.rows[i].cells[3].innerHTML = path ;
			table_obj.rows[i].cells[4].innerHTML = group ;
			table_obj.rows[i].cells[5].innerHTML = user ;
			table_obj.rows[i].cells[6].innerHTML = password ;
		}
	}
}

//获取group的数组
function get_group_list()
{
	var group_tab_obj = document.getElementById("step_5_tab_table") ;
	var group_tab_len = group_tab_obj.rows.length ;
	var group_arr = new Array() ;
	
	for ( var i = 2; i < group_tab_len; i += 2 )
	{
		group_arr.push( group_tab_obj.rows[i].cells[2].innerHTML ) ;
	}
	
	return group_arr ;
}

//步骤7 折叠
function fold_table_in_td( td_a_obj )
{
	var td_obj = td_a_obj.parentNode ;
	var tr_obj = td_obj.parentNode ;
	var table_obj = tr_obj.parentNode ;
	if ( td_obj.getElementsByTagName('span').item(0).className == "glyphicon glyphicon-chevron-right" )
	{
		td_obj.getElementsByTagName('span').item(0).className = "glyphicon glyphicon-chevron-down" ;
		table_obj.rows[( parseInt( tr_obj.rowIndex + 1 ) )].style.display = "" ;
	}
	else
	{
		td_obj.getElementsByTagName('span').item(0).className = "glyphicon glyphicon-chevron-right" ;
		table_obj.rows[( parseInt( tr_obj.rowIndex + 1 ) )].style.display = "none" ;
	}
}

//隐藏全部步骤框
function hiddendiv()
{
	var obj = null ;
	for ( var i = 1; ; ++i )
	{
		obj = document.getElementById("step_"+i) ;
		if ( !obj )
		{
			break;
		}
		obj.style.display = "none" ;
	}
}

//修改错误提示，并显示错误框
function show_error_win( str )
{
	document.getElementById("error_context").innerHTML = str ;
	$('#errorprowin').modal('show') ;
}

//填写日志到日志框
function show_diary_win( str )
{
	document.getElementById("diary_context").innerHTML += str ;
}

//获取当前第几步骤
function getCurrStep()
{
	var i = 1 ;
	for ( ; ; ++i )
	{
		var obj = document.getElementById("step_"+i) ;
		if ( obj )
		{
			if ( obj.style.display != "none" )
			{
				return i ;
			}
		}
		else
		{
			return -1 ;
		}
	}
}

//步骤选择器
function step_selector()
{
	var step_num = getCurrStep() ;
	if ( step_num == -1 )
	{
		alert ( "不存在的步骤" ) ;
	}
	else
	{
		var rc = true ;
		try
		{
			eval ( "rc = f_step_" + step_num + "();" ) ;
		}
		catch(e)
		{
			rc = false ;
		}
		if ( rc )
		{
			switchshow(step_num+1);
		}
	}
}

//步骤选择器 返回
function step_selector_return()
{
	var step_num = getCurrStep() ;
	--step_num ;
	if ( step_num >= 0 )
	{
		switchshow( step_num ) ;
	}
}

//第一步骤的函数
function f_step_1()
{
	var rc = true ;
	var path_obj = document.getElementById("step_1_path") ;

	//校验安装文件
	var post_context = 'common=' + convert2post( 'checkfile' ) + '&order=' + convert2post( path_obj.value ) ;

	//回调函数
	function callback_f_step_1( str )
	{
		try
		{
			var json_obj = eval('(' + str + ')');
			if ( json_obj['errno'] != 0 )
			{
				show_error_win( json_obj['output'] ) ;
				show_diary_win( json_obj['diary'] ) ;
				rc = false ;
			}
		}
		catch(e)
		{
			rc = false ;
		}
	}
	ajax2sendNew( 'POST', './shell/phpexec.php', post_context, false, callback_f_step_1, callback_err, callback_before, callback_complete ) ;
	
	if ( !rc )
	{
		return rc ;
	}
	
	//校验sdbcm端口
	var sdbcm_obj = document.getElementById("step_1_sdbcm") ;
	
	if ( sdbcm_obj.value == "" )
	{
		show_error_win( "sdbcm端口不能为空" ) ;
		rc = false ;
		return rc ;
	}
	
	return rc ;
}

//第二步骤的函数
function f_step_2()
{
	var rc = true ;
	var obj = document.getElementById( "step_2_tab_table" ) ;
	var table_row_len = obj.rows.length ;
	if ( table_row_len <= 1 )
	{
		show_error_win( '至少需要添加一台主机' ) ;
		rc = false ;
	}
	return rc ;
}

//第三步骤的函数
function f_step_3()
{
	var rc = true ;
	var obj = document.getElementById( "step_3_tab_table" ) ;
	var table_row_len = obj.rows.length ;
	if ( table_row_len <= 1 )
	{
		show_error_win( '至少需要一个协调节点' ) ;
		rc = false ;
	}
	//检查是否都配置好
	for ( var i = 1; i < table_row_len; ++i )
	{
		if ( obj.rows[i].cells[3].innerHTML == '' || obj.rows[i].cells[4].innerHTML == '' )
		{
			show_error_win( '表格第' + (i) + '行没有配置' ) ;
			rc = false ;
			return rc ;
		}
	}
	return rc ;
}

//第四步骤的函数
function f_step_4()
{
	var rc = true ;
	var obj = document.getElementById( "step_4_tab_table" ) ;
	var table_row_len = obj.rows.length ;
	if ( table_row_len <= 1 )
	{
		show_error_win( '至少需要一个编目节点' ) ;
		rc = false ;
	}
	//检查是否都配置好
	for ( var i = 1; i < table_row_len; ++i )
	{
		if ( obj.rows[i].cells[3].innerHTML == "" || obj.rows[i].cells[4].innerHTML == "" )
		{
			show_error_win( '表格第' + (i) + '行没有配置' ) ;
			rc = false ;
			return rc ;
		}
	}
	return rc ;
}

//第五步骤的函数
function f_step_5()
{
	var rc = true ;
	var obj = document.getElementById( "step_5_tab_table" ) ;
	var table_row_len = obj.rows.length ;
	if ( table_row_len <= 2 )
	{
		show_error_win( '至少需要一个分区组' ) ;
		rc = false ;
	}
	//检查是否都配置好
	for ( var i = 2; i < table_row_len; ++i )
	{
		var group_name = obj.rows[i].cells[2].innerHTML ;
		++i ;
		var child_table_obj = obj.rows[i].cells[1].getElementsByTagName("table").item(0) ;
		var child_table_len = child_table_obj.rows.length ;
		for ( var l = 1; l < child_table_len; ++l )
		{
			if ( child_table_obj.rows[l].cells[3].innerHTML == "" || child_table_obj.rows[l].cells[4].innerHTML == "" )
			{
				show_error_win( '分区组 ' + group_name + ' 的第' + (l) + '行没有配置' ) ;
				rc = false ;
				return rc ;
			}
		}
	}
	return rc ;
}

//第六步骤的函数
function f_step_6()
{
	var rc = true ;
	var return_j_o_t = null ;
	var host_list_obj  = document.getElementById("step_2_tab_table") ;
	var autostart = 1 ;
	if ( document.getElementById("step_1_autostart").value == 'true' )
	{
		autostart = 1 ;
	}
	else
	{
		autostart = 0 ;
	}
	
	//用ajax把配置都发送到php,php生成配置文件
	var order = '{"debug":"' + document.getElementById("step_1_debug").value + '","install":"' + document.getElementById("step_1_path").value + '","sdbcm":"' + document.getElementById("step_1_sdbcm").value + '","autostart":' + autostart + ',"group":"' ;
	var group_arr = get_group_list() ;
	var isfirst = true ;
	for( key in group_arr )
	{
		if ( isfirst )
		{
			isfirst = false ;
		}
		else
		{
			order = order + ' ' ;
		}
		order = order + '\\"' + group_arr[key] + '\\"' ;
	}
	order = order + '","host":[' ;
	
	isfirst = true ;
	var host_table_len = host_list_obj.rows.length ;
	for ( var i = 1; i < host_table_len; ++i )
	{
		if ( isfirst )
		{
			isfirst = false ;
		}
		else
		{
			order = order + ',' ;
		}
		order = order + '["' + host_list_obj.rows[i].cells[2].innerHTML + '","' + host_list_obj.rows[i].cells[3].innerHTML + '","' + host_list_obj.rows[i].cells[4].innerHTML + '","' + host_list_obj.rows[i].cells[5].innerHTML + '","' + host_list_obj.rows[i].cells[6].innerHTML + '"]' ;
	}
	
	function callback_f_step_6( str )
	{
		try
		{
			return_j_o_t = eval('(' + str + ')') ;
			rc = return_j_o_t['errno'] ;
		}
		catch(e)
		{
			return_j_o_t = eval('({"output":"","errno":"1","diary":""})') ;
			rc = 1 ;
		}
	}
	function callback_f_step_6_err()
	{
		rc = 1 ;
	}
	order = order + '],"node":' + get_all_node_to_json() + '}' ;
	var post_context = 'common=' + convert2post( 'postallnodeconf' ) + '&order=' + convert2post( order ) ;
	ajax2sendNew( 'POST', './shell/phpexec.php', post_context, false, callback_f_step_6, callback_f_step_6_err, callback_before, callback_complete ) ;
	return rc ;
}

//第七步骤的函数
function f_step_7()
{
	var rc = true ;
	return rc ;
}

//把所有节点转换成json
function get_all_node_to_json()
{
	var coord_list_obj  = document.getElementById("step_3_tab_table") ;
	var cata_list_obj   = document.getElementById("step_4_tab_table") ;
	var data_list_obj   = document.getElementById("step_5_tab_table") ;
	var json_str = '' ;
	var isfirst_1 = true ;
	
	json_str = json_str + '{"coord":[' ;
	var coord_table_row_len = coord_list_obj.rows.length ;
	for ( var i = 1; i < coord_table_row_len; ++i )
	{
		if ( isfirst_1 )
		{
			isfirst_1 = false ;
		}
		else
		{
			json_str = json_str + ',' ;
		}
		var temp_div_obj = coord_list_obj.rows[i].cells[5].getElementsByTagName('div') ;
		json_str = json_str + '{"hostname":"' ;
		json_str = json_str + replaceAllEx( coord_list_obj.rows[i].cells[2].innerHTML, '"', '\\"' ) ;
		json_str = json_str + '","conf":[' ;
		var isfirst_2 = true ;
		for ( var k = 0; k < temp_div_obj.length; ++k )
		{
			var temp_div_item_obj = temp_div_obj.item(k) ;
			if ( isfirst_2 )
			{
				isfirst_2 = false ;
			}
			else
			{
				json_str = json_str + ',' ;
			}
			json_str = json_str + '"' + replaceAllEx( temp_div_item_obj.innerHTML, '"', '\\"' ) + '"' ;
		}
		json_str = json_str + ']}' ;
	}
	json_str = json_str + '],"cata":[' ;
	isfirst_1 = true ;
	var cata_table_row_len = cata_list_obj.rows.length ;
	for ( var i = 1; i < cata_table_row_len; ++i )
	{
		if ( isfirst_1 )
		{
			isfirst_1 = false ;
		}
		else
		{
			json_str = json_str + ',' ;
		}
		var temp_div_obj = cata_list_obj.rows[i].cells[5].getElementsByTagName('div') ;
		json_str = json_str + '{"hostname":"' ;
		json_str = json_str + replaceAllEx( cata_list_obj.rows[i].cells[2].innerHTML, '"', '\\"' ) ;
		json_str = json_str + '","conf":[' ;
		var isfirst_2 = true ;
		for ( var k = 0; k < temp_div_obj.length; ++k )
		{
			var temp_div_item_obj = temp_div_obj.item(k) ;
			if ( isfirst_2 )
			{
				isfirst_2 = false ;
			}
			else
			{
				json_str = json_str + ',' ;
			}
			json_str = json_str + '"' + replaceAllEx( temp_div_item_obj.innerHTML, '"', '\\"' ) + '"' ;
		}
		json_str = json_str + ']}' ;
	}
	json_str = json_str + '],"data":[' ;
	isfirst_1 = true ;
	var data_table_row_len = data_list_obj.rows.length ;
	for ( var i = 2; i < data_table_row_len; ++i )
	{
		group_name = data_list_obj.rows[i].cells[2].innerHTML ;
		++i ;
		var child_tab_obj = data_list_obj.rows[i].cells[1].getElementsByTagName('table').item(0) ;
		var child_tab_len = child_tab_obj.rows.length ;
		for ( var l = 1; l < child_tab_len; ++l )
		{
			if ( isfirst_1 )
			{
				isfirst_1 = false ;
			}
			else
			{
				json_str = json_str + ',' ;
			}
			var temp_div_obj = child_tab_obj.rows[l].cells[5].getElementsByTagName('div') ;
			json_str = json_str + '{"hostname":"' ;
			json_str = json_str + replaceAllEx( child_tab_obj.rows[l].cells[2].innerHTML, '"', '\\"' ) ;
			json_str = json_str + '","groupname":"' ;
			json_str = json_str + replaceAllEx( group_name, '"', '\\"' ) ;
			json_str = json_str + '","conf":[' ;
			var isfirst_2 = true ;
			for ( var k = 0; k < temp_div_obj.length; ++k )
			{
				var temp_div_item_obj = temp_div_obj.item(k) ;
				if ( isfirst_2 )
				{
					isfirst_2 = false ;
				}
				else
				{
					json_str = json_str + ',' ;
				}
				json_str = json_str + '"' + replaceAllEx( temp_div_item_obj.innerHTML, '"', '\\"' ) + '"' ;
			}
			json_str = json_str + ']}' ;
		}
	}
	json_str = json_str + ']}' ;
	return json_str ;
}

//第二步骤的扫描函数
function f_scaning()
{
	var rc = true ;
	var scan_host = document.getElementById("scan_tab_1_host_text").value ;
	
	if ( scan_host == "" )
	{
		//alert( "请输入主机" ) ;
		//show_error_win( "请输入主机" ) ;
		rc = false ;
		return rc ;
	}

	document.getElementById("scan_load_pic").style.width = "0%" ;
	document.getElementById("scan_load_1").style.display = "" ;
	document.getElementById("scan_load_2").style.display = "none" ;
	document.getElementById("scan_load_2").innerHTML = '' ;
	document.getElementById("scan_tab_table").rows[0].cells[0].getElementsByTagName('input').item(0).checked = false ;

	json_obj = get_host_list( scan_host ) ;

	$('#scan_tab_ul a[href="#scan_tab_2"]').tab('show') ;
	var tab_obj = document.getElementById("scan_tab_table") ;
	var host_len = json_obj.length ;
	var cur_host_num = 0 ;
	var success_host = 0 ;
			
	var trs_obj = tab_obj.getElementsByTagName("tr");
	for(var i = trs_obj.length - 1; i > 0; i--)
	{
		tab_obj.deleteRow( i ) ;
	}
	
	//回调函数
	function callback_f_check_host( str )
	{
		try
		{
			return_j_o_t = eval('(' + str + ')') ;
		}
		catch(e)
		{
			return_j_o_t = eval('({"output":"","errno":"1","diary":""})') ;
		}
		var tab_row = tab_obj.insertRow ( tab_obj.rows.length ) ;
		var c1 = tab_row.insertCell( 0 ) ;
		if ( return_j_o_t['errno'] == 0 )
		{
			++success_host ;
			c1.innerHTML = '<input type="checkbox">' ;
		}
		else
		{
			c1.innerHTML = '<input type="checkbox" disabled>' ;
		}
		var c2 = tab_row.insertCell( 1 ) ;
		c2.innerHTML = return_j_o_t['ip'] ;
		var c3 = tab_row.insertCell( 2 ) ;
		c3.innerHTML = return_j_o_t['host'] ;
		var c4 = tab_row.insertCell( 3 ) ;
		if ( return_j_o_t['errno'] == 0 )
		{
			c4.innerHTML = "连接成功：响应时间 " + return_j_o_t['output'] + " 毫秒" ;
		}
		else
		{
			c4.innerHTML = return_j_o_t['output'] ;
		}
		++cur_host_num ;
		var load_num = parseInt(((cur_host_num/host_len)*100)) ;
		document.getElementById("scan_load_pic").style.width = load_num + "%" ;
		if ( cur_host_num == host_len )
		{
			document.getElementById("scan_load_1").style.display = "none" ;
			document.getElementById("scan_load_2").style.display = "" ;
			document.getElementById("scan_load_2").innerHTML = '已扫描 ' + host_len + ' 台主机，其中 ' + success_host + ' 台已连接成功连接SSH' ;
		}
		var scroll_obj = document.getElementById("scan_tab_scoll") ;
		scroll_obj.scrollTop = scroll_obj.scrollHeight ;
	}
	for ( var key in json_obj )
	{
		var return_j_o_t = null ;
		var post_context = 'common=' + convert2post( 'checkhost' ) + '&order=' + convert2post( (json_obj[key]+' '+'22') ) ;
		ajax2sendNew( 'POST', './shell/phpexec.php', post_context, true, callback_f_check_host, callback_err, callback_before, callback_complete ) ;
	}
	return rc ;
}

//根据表格第一列的checkbox来删除行
function delete_table_rows( table_name )
{
	var obj = document.getElementById( table_name ) ;
	for ( var i = 1; i < obj.rows.length; )
	{
		var td_0 = obj.rows[i].cells[0] ;
		var input_obj = td_0.getElementsByTagName('input').item(0) ;
		if ( input_obj.checked == true )
		{
			obj.deleteRow( i ) ;
			i = 1 ;
		}
		else
		{
			++i ;
		}
	}
}

//根据group表格第一列的checkbox来删除行(步骤五专用)
function delete_group_table_rows( table_name )
{
	var obj = document.getElementById( table_name ) ;
	for ( var i = 2; i < obj.rows.length; )
	{
		var td_0 = obj.rows[i].cells[0] ;
		var input_obj = td_0.getElementsByTagName('input').item(0) ;
		if ( input_obj && input_obj.checked == true )
		{
			obj.deleteRow( i ) ;
			obj.deleteRow( i ) ;
			break ;
		}
		else
		{
			i += 2 ;
		}
	}
}

//根据group表格第一列的checkbox来删除指定节点(步骤五专用)
function delete_group_table_node_rows( table_name )
{
	var obj = document.getElementById( table_name ) ;
	for ( var i = 2; i < obj.rows.length; )
	{
		var td_0 = obj.rows[i].cells[0] ;
		var td_3 = obj.rows[i].cells[3] ;
		var input_obj = td_0.getElementsByTagName('input').item(0) ;
		if ( input_obj && input_obj.checked == true )
		{
			++i;
			var temp_tab_obj = obj.rows[i].cells[1].getElementsByTagName('table').item(0) ;
			for ( var k = 1; k < temp_tab_obj.rows.length; )
			{
				var t_td_0 = temp_tab_obj.rows[k].cells[0] ;
				var temp_inp_obj = t_td_0.getElementsByTagName('input').item(0) ;
				if ( temp_inp_obj.checked == true )
				{
					temp_tab_obj.deleteRow( k ) ;
					k = 1 ;
				}
				else
				{
					++k ;
				}
			}
			td_3.innerHTML = temp_tab_obj.rows.length - 1 ;
			break ;
		}
		else
		{
			i += 2 ;
		}
	}
}

//根据group表格第一列的checkbox来删除所有节点(步骤五专用)
function delete_group_table_node_all_rows( table_name )
{
	var obj = document.getElementById( table_name ) ;
	for ( var i = 2; i < obj.rows.length; )
	{
		var td_0 = obj.rows[i].cells[0] ;
		var td_3 = obj.rows[i].cells[3] ;
		var input_obj = td_0.getElementsByTagName('input').item(0) ;
		if ( input_obj && input_obj.checked == true )
		{
			++i;
			var temp_tab_obj = obj.rows[i].cells[1].getElementsByTagName('table').item(0) ;
			for ( var k = 1; k < temp_tab_obj.rows.length; )
			{
				temp_tab_obj.deleteRow( k ) ;
			}
			td_3.innerHTML = 0 ;
			break ;
		}
		else
		{
			i += 2 ;
		}
	}
}

//group表格删除所有行(步骤五专用)
function delete_group_table_all_rows( table_name )
{
	var obj = document.getElementById( table_name ) ;
	for ( var i = 2; i < obj.rows.length; )
	{
		obj.deleteRow( i ) ;
	}
}

//删除表格所有行
function delete_table_all_rows( table_name )
{
	var obj = document.getElementById( table_name ) ;
	for ( var i = 1; i < obj.rows.length; )
	{
		obj.deleteRow( i ) ;
	}
}

//清除表格指定行的配置信息(步骤五专用)
function remove_group_table_rows_conf( table_name )
{
	var obj = document.getElementById( table_name ) ;
	for ( var i = 2; i < obj.rows.length; )
	{
		var td_0 = obj.rows[i].cells[0] ;
		var td_3 = obj.rows[i].cells[3] ;
		var input_obj = td_0.getElementsByTagName('input').item(0) ;
		if ( input_obj && input_obj.checked == true )
		{
			++i;
			var temp_tab_obj = obj.rows[i].cells[1].getElementsByTagName('table').item(0) ;
			for ( var k = 1; k < temp_tab_obj.rows.length; ++k )
			{
				var temp_inp_obj = temp_tab_obj.rows[k].cells[0].getElementsByTagName('input').item(0) ;
				if ( temp_inp_obj.checked == true )
				{
					temp_tab_obj.rows[k].cells[3].innerHTML = "" ;
					temp_tab_obj.rows[k].cells[4].innerHTML = "" ;
					var temp_tab_td_5 = temp_tab_obj.rows[k].cells[5] ;
					var div_item_obj = temp_tab_obj.rows[k].cells[5].getElementsByTagName('div') ;
					var div_item_len = div_item_obj.length ;
					for ( var l = 0; l < div_item_len; ++l )
					{
						temp_tab_td_5.getElementsByTagName('div').item(l).innerHTML = "" ;
					}
				}
			}
			break ;
		}
		else
		{
			i += 2 ;
		}
	}
}

//清除表格所有行的配置信息(步骤五专用)
function remove_group_table_all_rows_conf( table_name )
{
	var obj = document.getElementById( table_name ) ;
	for ( var i = 2; i < obj.rows.length; )
	{
		var td_0 = obj.rows[i].cells[0] ;
		var td_3 = obj.rows[i].cells[3] ;
		var input_obj = td_0.getElementsByTagName('input').item(0) ;
		if ( input_obj && input_obj.checked == true )
		{
			++i;
			var temp_tab_obj = obj.rows[i].cells[1].getElementsByTagName('table').item(0) ;
			for ( var k = 1; k < temp_tab_obj.rows.length; ++k )
			{
				temp_tab_obj.rows[k].cells[3].innerHTML = "" ;
				temp_tab_obj.rows[k].cells[4].innerHTML = "" ;
				var temp_tab_td_5 = temp_tab_obj.rows[k].cells[5] ;
				var div_item_obj = temp_tab_obj.rows[k].cells[5].getElementsByTagName('div') ;
				var div_item_len = div_item_obj.length ;
				for ( var l = 0; l < div_item_len; ++l )
				{
					temp_tab_td_5.getElementsByTagName('div').item(l).innerHTML = "" ;
				}
			}
			break ;
		}
		else
		{
			i += 2 ;
		}
	}
}

//清除表格指定行的配置信息
function remove_table_rows_conf( table_name )
{
	var obj = document.getElementById( table_name ) ;
	var table_row_len = obj.rows.length ;
	for ( var i = 1; i < table_row_len; ++i )
	{
		var td_0 = obj.rows[i].cells[0] ;
		var input_obj = td_0.getElementsByTagName('input').item(0) ;
		if ( input_obj.checked == true )
		{
			obj.rows[i].cells[3].innerHTML = "" ;
			obj.rows[i].cells[4].innerHTML = "" ;
			var div_item_obj = obj.rows[i].cells[5].getElementsByTagName('div') ;
			var div_item_len = div_item_obj.length ;
			for ( var k = 0; k < div_item_len; ++k )
			{
				obj.rows[i].cells[5].getElementsByTagName('div').item(k).innerHTML = "" ;
			}
		}
	}
}

//清除表格所有行的配置信息
function remove_table_all_rows_conf( table_name )
{
	var obj = document.getElementById( table_name ) ;
	var table_row_len = obj.rows.length ;
	for ( var i = 1; i < table_row_len; ++i )
	{
		obj.rows[i].cells[3].innerHTML = "" ;
		obj.rows[i].cells[4].innerHTML = "" ;
		var div_item_obj = obj.rows[i].cells[5].getElementsByTagName('div') ;
		var div_item_len = div_item_obj.length ;
		for ( var k = 0; k < div_item_len; ++k )
		{
			obj.rows[i].cells[5].getElementsByTagName('div').item(k).innerHTML = "" ;
		}
	}
}

//全选
function set_all_checkbox_true( table_name )
{
	var obj = document.getElementById( table_name ) ;
	var table_row_len = obj.rows.length ;
	var status = true ;
	for ( var i = 0; i < table_row_len; ++i )
	{
		var td_0 = obj.rows[i].cells[0] ;
		var input_obj = td_0.getElementsByTagName('input').item(0) ;
		if ( input_obj )
		{
			input_obj.checked = status ;
		}
	}
}

//反选
function set_all_checkbox_false( table_name )
{
	var obj = document.getElementById( table_name ) ;
	var table_row_len = obj.rows.length ;
	
	for ( var i = 0; i < table_row_len; ++i )
	{
		var td_0 = obj.rows[i].cells[0] ;
		var input_obj = td_0.getElementsByTagName('input').item(0) ;
		if ( input_obj )
		{
			if ( i == 0 )
			{
				input_obj.checked = false ;
			}
			else
			{
				input_obj.checked = !input_obj.checked ;
			}
		}
	}
}

//选择未配置的
function set_all_checkbox_not_set( table_name )
{
	var obj = document.getElementById( table_name ) ;
	var table_row_len = obj.rows.length ;
	
	for ( var i = 0; i < table_row_len; ++i )
	{
		var td_0 = obj.rows[i].cells[0] ;
		var input_obj = td_0.getElementsByTagName('input').item(0) ;
		if ( i == 0 )
		{
			input_obj.checked = false ;
		}
		else
		{
			if ( obj.rows[i].cells[3].innerHTML == "" || obj.rows[i].cells[4].innerHTML == "" )
			{
				input_obj.checked = true ;
			}
			else
			{
				input_obj.checked = false ;
			}
		}
	}
}

//选择已配置的
function set_all_checkbox_is_set( table_name )
{
	var obj = document.getElementById( table_name ) ;
	var table_row_len = obj.rows.length ;
	
	for ( var i = 0; i < table_row_len; ++i )
	{
		var td_0 = obj.rows[i].cells[0] ;
		var input_obj = td_0.getElementsByTagName('input').item(0) ;
		if ( i == 0 )
		{
			input_obj.checked = false ;
		}
		else
		{
			if ( obj.rows[i].cells[3].innerHTML != "" && obj.rows[i].cells[4].innerHTML != "" )
			{
				input_obj.checked = true ;
			}
			else
			{
				input_obj.checked = false ;
			}
		}
	}
}

//根据表格头的多选框控制表格内全部多选框
function set_all_checkbox( table_name, title_checkbox_obj )
{
	var obj = document.getElementById( table_name ) ;
	var table_row_len = obj.rows.length ;
	var status = title_checkbox_obj.checked ;
	for ( var i = 1; i < table_row_len; ++i )
	{
		var td_0 = obj.rows[i].cells[0] ;
		var input_obj = td_0.getElementsByTagName('input').item(0) ;
		if ( input_obj )
		{
			if ( !input_obj.disabled )
			{
				input_obj.checked = status ;
			}
		}
	}
}

//根据表格头的多选框控制表格内全部多选框
function set_all_checkbox2( table_obj, title_checkbox_obj )
{
	var obj = table_obj ;
	var table_row_len = obj.rows.length ;
	var status = title_checkbox_obj.checked ;
	for ( var i = 1; i < table_row_len; ++i )
	{
		var td_0 = obj.rows[i].cells[0] ;
		var input_obj = td_0.getElementsByTagName('input').item(0) ;
		if ( input_obj )
		{
			if ( !input_obj.disabled )
			{
				input_obj.checked = status ;
			}
		}
	}
}

//表格列做排序
//参数1 按钮对象
//参数2 表格id
//参数3 根据表格第几列做排序
function table_sort_col( button_obj, table_name, table_col )
{
	var sort_type = true ;
	var obj = button_obj ;
	var obj_str = obj.innerHTML ;
	str_arr = obj_str.split("&nbsp;") ;
	if ( str_arr[1] == '<span class="caret"></span>' )
	{
		table_sort( table_name, false, table_col ) ;
		obj.innerHTML = str_arr[0] + '&nbsp;<span class="dropup"><span class="caret"></span></span>' ;
	}
	else
	{
		table_sort( table_name, true, table_col ) ;
		obj.innerHTML = str_arr[0] + '&nbsp;<span class="caret"></span>' ;
	}
}

//表格排序
//参数1 表格id
//参数2 排序 true:正序 false:逆序
//参数3 根据表格第几列做排序
function table_sort( table_name, sort_type, table_col )
{
	var obj = document.getElementById( table_name ) ;
	var table_row_len = obj.rows.length ;
	var table_arr = new Array() ;
	for ( var i = 1; i < table_row_len; ++i )
	{
		table_arr.push( obj.rows[i] ) ;
	}
	table_arr.sort( sort_compate ) ;
	
	var trs_obj = obj.getElementsByTagName("tr");
	for(var i = trs_obj.length - 1; i > 0; i--)
	{
		obj.deleteRow( i ) ;
	}
	
	for ( var key in table_arr )
	{
		var tab_row = obj.insertRow ( obj.rows.length ) ;
		tab_row.innerHTML = table_arr[key].innerHTML ;
	}
	
	
	function sort_compate( obj_1, obj_2 )
	{
		var str_1 = obj_1.cells[table_col].innerHTML ;
		var str_2 = obj_2.cells[table_col].innerHTML ;
		if ( sort_type )
		{
			return str_1.localeCompare( str_2 ) ;
		}
		else
		{
			return str_2.localeCompare( str_1 ) ;
		}
	}
}

//第二步骤，把连接成功的主机添加到主机列表
function add_link_host_to_list( source_name, dest_name )
{
	var source_obj = document.getElementById( source_name ) ;
	var dest_obj   = document.getElementById( dest_name ) ;

	var table_row_len = source_obj.rows.length ;
	for ( var i = 1; i < table_row_len; ++i )
	{
		var td_0 = source_obj.rows[i].cells[0] ;
		var input_obj = td_0.getElementsByTagName('input').item(0) ;
		if ( input_obj.checked == true && add_list_filter_repeat( source_obj.rows[i].cells[2].innerHTML, dest_obj ) )
		{
			var tab_row = dest_obj.insertRow ( dest_obj.rows.length ) ;
			var c0 = tab_row.insertCell( 0 ) ;
			c0.innerHTML = '<input type="checkbox">' ;
			var c1 = tab_row.insertCell( 1 ) ;
			c1.innerHTML = source_obj.rows[i].cells[1].innerHTML ;
			var c2 = tab_row.insertCell( 2 ) ;
			c2.innerHTML = source_obj.rows[i].cells[2].innerHTML ;
			var c3 = tab_row.insertCell( 3 ) ;
			c3.innerHTML = "/opt/sequoiadb" ;
			var c4 = tab_row.insertCell( 4 ) ;
			c4.innerHTML = "sdbadmin_group" ;
			var c5 = tab_row.insertCell( 5 ) ;
			c5.innerHTML = "sdbadmin" ;
			var c6 = tab_row.insertCell( 6 ) ;
			c6.innerHTML = "sequoiadb" ;
		}
	}
}

//批量添加节点
function add_host_to_node( source_name, button_obj )
{
	var source_obj = document.getElementById( source_name ) ;
	var dest_type_name_arr  = button_obj.value.split("|");
	var dest_type	= dest_type_name_arr[0] ;
	var dest_obj   = document.getElementById( dest_type_name_arr[1] ) ;

	if ( dest_type == "data" )
	{
		var table_row_len = source_obj.rows.length ;
		var table_row0_div_len = dest_obj.rows[1].cells[0].getElementsByTagName("div").length ;
		
		for ( var i = 1; i < table_row_len; ++i )
		{
			var td_0 = source_obj.rows[i].cells[0] ;
			var input_obj = td_0.getElementsByTagName('input').item(0) ;
			if ( input_obj && input_obj.checked == true )
			{
				var p_dest_len = dest_obj.rows.length ;
				var temp_table_len = 0 ;
				for ( var l = 2; l < p_dest_len; ++l )
				{
					var d_td_0 = dest_obj.rows[l].cells[0] ;
					var input_obj = d_td_0.getElementsByTagName('input').item(0) ;
				
					if ( input_obj && input_obj.checked == true )
					{
						++l ;
						var temp_table_obj = dest_obj.rows[l].cells[1].getElementsByTagName('table').item(0) ;
						var tab_row = temp_table_obj.insertRow ( temp_table_obj.rows.length ) ;
						var c0 = tab_row.insertCell( 0 ) ;
						c0.innerHTML = '<input type="checkbox">' ;
						var c1 = tab_row.insertCell( 1 ) ;
						c1.innerHTML = source_obj.rows[i].cells[1].innerHTML ;
						var c2 = tab_row.insertCell( 2 ) ;
						c2.innerHTML = source_obj.rows[i].cells[2].innerHTML ;
						var c3 = tab_row.insertCell( 3 ) ;
						c3.innerHTML = '' ;
						var c4 = tab_row.insertCell( 4 ) ;
						c4.innerHTML = '';
						var c5 = tab_row.insertCell( 5 ) ;
						c5.style.display = 'none' ;
						for ( var k = 0; k < table_row0_div_len; ++k )
						{
							var temp_div = document.createElement("div") ;
							c5.appendChild( temp_div ) ;
						}
						temp_table_len = temp_table_obj.rows.length - 1 ;
						dest_obj.rows[l-1].cells[3].innerHTML = temp_table_len ;
					}
				}
			}
		}
	}
	else
	{
		var table_row_len = source_obj.rows.length ;
		var table_row0_div_len = dest_obj.rows[0].cells[5].getElementsByTagName("div").length ;
		
		for ( var i = 1; i < table_row_len; ++i )
		{
			var td_0 = source_obj.rows[i].cells[0] ;
			var input_obj = td_0.getElementsByTagName('input').item(0) ;
			if ( input_obj.checked == true )
			{
				var tab_row = dest_obj.insertRow ( dest_obj.rows.length ) ;
				var c0 = tab_row.insertCell( 0 ) ;
				c0.innerHTML = '<input type="checkbox">' ;
				var c1 = tab_row.insertCell( 1 ) ;
				c1.innerHTML = source_obj.rows[i].cells[1].innerHTML ;
				var c2 = tab_row.insertCell( 2 ) ;
				c2.innerHTML = source_obj.rows[i].cells[2].innerHTML ;
				var c3 = tab_row.insertCell( 3 ) ;
				c3.innerHTML = '' ;
				var c4 = tab_row.insertCell( 4 ) ;
				c4.innerHTML = '';
				var c5 = tab_row.insertCell( 5 ) ;
				c5.style.display = "none" ;
				for ( var k = 0; k < table_row0_div_len; ++k )
				{
					var temp_div = document.createElement("div") ;
					c5.appendChild( temp_div ) ;
				}
			}
		}
	}
}

//批量配置节点弹窗的保存按钮事件
function save_button_to_table( button_obj, source_table_name )
{
	var source_tab_obj = document.getElementById( source_table_name ) ;
	var dest_type_name_arr  = button_obj.value.split("|");
	var dest_type	= dest_type_name_arr[0] ;
	var dest_table_name = dest_type_name_arr[1] ;
	var dest_tab_obj   = document.getElementById( dest_table_name ) ;
	
	if ( dest_type == "data" )
	{
		var table_row_len = dest_tab_obj.rows.length ;
		for ( var i = 2; i < table_row_len; ++i )
		{
			var group_name = dest_tab_obj.rows[i].cells[2].innerHTML ;
			var par_inp_obj = dest_tab_obj.rows[i].cells[0].getElementsByTagName('input').item(0) ;
			++i ;
			if ( par_inp_obj && par_inp_obj.checked == true )
			{
				var td_1 = dest_tab_obj.rows[i].cells[1] ;
				var temp_table_obj = td_1.getElementsByTagName('table').item(0) ;
				
				var temp_table_len = temp_table_obj.rows.length ;
				for ( var l = 1; l < temp_table_len; ++l )
				{
					var t_td_0 = temp_table_obj.rows[l].cells[0] ;
					var input_obj = t_td_0.getElementsByTagName('input').item(0) ;
					
					if ( input_obj && input_obj.checked == true )
					{
						var t_td_3 = temp_table_obj.rows[l].cells[3] ;
						var t_td_4 = temp_table_obj.rows[l].cells[4] ;
						var t_td_5 = temp_table_obj.rows[l].cells[5] ;
						var conf_div = t_td_5.getElementsByTagName('div') ;
						var source_table_row_len = source_tab_obj.rows.length ;
						for ( var k = 1; k < source_table_row_len; ++k )
						{
							var source_td_1 = source_tab_obj.rows[k].cells[1] ;
							var input_obj = source_td_1.getElementsByTagName('input').item(0) ;
							if ( !input_obj )
							{
								input_obj = source_td_1.getElementsByTagName('textarea').item(0) ;
								if ( !input_obj )
								{
									input_obj = source_td_1.getElementsByTagName('select').item(0) ;
									if( !input_obj )
									{
										continue ;
									}
								}
							}
							if ( source_tab_obj.rows[k].cells[0].innerHTML == "svcname" )
							{
								t_td_3.innerHTML = input_obj.value ;
							}
							else if ( source_tab_obj.rows[k].cells[0].innerHTML == "dbpath" )
							{
								t_td_4.innerHTML = input_obj.value ;
							}
							
							if ( source_tab_obj.rows[k].cells[0].innerHTML == "groupname" )
							{
								conf_div.item(k-1).innerHTML = group_name ;
							}
							else
							{
								conf_div.item(k-1).innerHTML = input_obj.value ;
							}
						}
					}
				}
			}
		}
	}
	else
	{
		var table_row_len = dest_tab_obj.rows.length ;
		for ( var i = 1; i < table_row_len; ++i )
		{
			var td_0 = dest_tab_obj.rows[i].cells[0] ;
			var input_obj = td_0.getElementsByTagName('input').item(0) ;
			if ( input_obj.checked == true )
			{
				var td_3 = dest_tab_obj.rows[i].cells[3] ;
				var td_4 = dest_tab_obj.rows[i].cells[4] ;
				var td_5 = dest_tab_obj.rows[i].cells[5] ;
				var conf_div = td_5.getElementsByTagName('div') ;
				var source_table_row_len = source_tab_obj.rows.length ;
				for ( var k = 1; k < source_table_row_len; ++k )
				{
					var source_td_1 = source_tab_obj.rows[k].cells[1] ;
					var input_obj = source_td_1.getElementsByTagName('input').item(0) ;
					if ( !input_obj )
					{
						input_obj = source_td_1.getElementsByTagName('textarea').item(0) ;
						if ( !input_obj )
						{
							input_obj = source_td_1.getElementsByTagName('select').item(0) ;
							if( !input_obj )
							{
								continue ;
							}
						}
					}
					if ( source_tab_obj.rows[k].cells[0].innerHTML == "svcname" )
					{
						td_3.innerHTML = input_obj.value ;
					}
					else if ( source_tab_obj.rows[k].cells[0].innerHTML == "dbpath" )
					{
						td_4.innerHTML = input_obj.value ;
					}
					else if ( source_tab_obj.rows[k].cells[0].innerHTML == "groupname" )
					{
						var td_6 = dest_tab_obj.rows[i].cells[6] ;
						if ( td_6 )
						{
							td_6.innerHTML = input_obj.value ;
						}
					}
					conf_div.item(k-1).innerHTML = input_obj.value ;
				}
			}
		}
	}
}

//点批量配置节点的事件
function batch_conf_node_buttun( table_name, tr_class_name, button_id, value )
{
	set_button_value( button_id, value ) ;
	hidden_table_tr( table_name, tr_class_name ) ;
}

//传递指定表格id到按钮value上
function set_button_value( button_id, value )
{
	document.getElementById(button_id).value = value ;
}

//把配置表格中指定class名字的隐藏
function hidden_table_tr( table_name, tr_class_name )
{
	var obj = document.getElementById( table_name ).getElementsByTagName( "tr" ) ;
	var tr_len = obj.length ;
	for ( var i = 1; i < tr_len; ++i )
	{
		if( obj.item(i).className == tr_class_name || obj.item(i).className == "options_conf_all" )
		{
			obj.item(i).style.display = "" ;
		}
		else
		{
			obj.item(i).style.display = "none" ;
		}
	}
}

//把主机列表加载到节点列表中
function add_host_list_to_node_list( host_table_name, node_table_name, button_id, value )
{
	set_button_value( button_id, value ) ;
	var host_tab_obj = document.getElementById( host_table_name ) ;
	var node_tab_obj = document.getElementById( node_table_name ) ;
	var table_row_len = host_tab_obj.rows.length ;
	
	node_tab_obj.rows[0].cells[0].getElementsByTagName('input').item(0).checked = false ;
	
	var trs_obj = node_tab_obj.getElementsByTagName("tr");
	for(var i = trs_obj.length - 1; i > 0; i--)
	{
		node_tab_obj.deleteRow( i ) ;
	}
	
	for ( var i = 1; i < table_row_len; ++i )
	{
		var tab_row = node_tab_obj.insertRow ( node_tab_obj.rows.length ) ;
		var c0 = tab_row.insertCell( 0 ) ;
		c0.innerHTML = '<input type="checkbox">' ;
		var c1 = tab_row.insertCell( 1 ) ;
		c1.innerHTML = host_tab_obj.rows[i].cells[1].innerHTML ;
		var c2 = tab_row.insertCell( 2 ) ;
		c2.innerHTML = host_tab_obj.rows[i].cells[2].innerHTML ;
	}
}

//第二步骤，添加主机列表，去重
function add_list_filter_repeat( host, dest_obj )
{
	var table_row_len = dest_obj.rows.length ;
	for ( var i = 1; i < table_row_len; ++i )
	{
		if ( host == dest_obj.rows[i].cells[2].innerHTML )
		{
			return false ;
		}
	}
	return true ;
}

//解析ip ip段 hostname  hostname段，返回数组
function get_host_list( str )
{
	var link_search = new Array() ;
	var ip_search = new Array() ;
   var host_search = new Array() ;
	//识别主机字符串，扫描主机
	var reg = new RegExp(/^(((2[0-4]\d|25[0-5]|[01]?\d\d?)|(\[[ ]*(2[0-4]\d|25[0-5]|[01]?\d\d?)[ ]*\-[ ]*(2[0-4]\d|25[0-5]|[01]?\d\d?)[ ]*\]))\.){3}((2[0-4]\d|25[0-5]|[01]?\d\d?)|(\[(2[0-4]\d|25[0-5]|[01]?\d\d?)\-(2[0-4]\d|25[0-5]|[01]?\d\d?)\]))$/) ;
	str.replace( /(^\s*)|(\s*$)/g, "" ) ;
	var matches = new Array() ;
	if ( ( matches = reg.exec( str ) ) != null )
	{
		//ip区间
		var ip_arr = str.split(".") ;
		for ( ipsub in ip_arr )
		{
			reg = new RegExp(/^((2[0-4]\d|25[0-5]|[01]?\d\d?)|(\[[ ]*(2[0-4]\d|25[0-5]|[01]?\d\d?)[ ]*\-[ ]*(2[0-4]\d|25[0-5]|[01]?\d\d?)[ ]*\]))$/) ;
			if ( ( matches = reg.exec( ip_arr[ipsub] ) ) != null )
			{
				//匹配每个数值
				if ( typeof( matches[4] ) == "undefined" || typeof( matches[5] ) == "undefined" )
				{
					//这是一个数字 192
					ip_search.push( matches[0] ) ;
				}
				else
				{
					//这是一个区间 [1-10]
					ip_search.push( new Array( matches[4], matches[5] ) ) ;
				}
			}
		}
		/*for ( keys in ip_search )
		{
			alert ( ip_search[keys] ) ;
		}*/
	}
	else
	{
		//主机名
		reg = new RegExp(/^((.*)(\[[ ]*(\d+)[ ]*\-[ ]*(\d+)[ ]*\])(.*))$/) ;
		if ( ( matches = reg.exec( str ) ) != null )
		{
			host_search.push( matches[2] ) ;
			host_search.push( matches[4] ) ;
			host_search.push( matches[5] ) ;
			host_search.push( matches[6] ) ;
		}
		else
		{
			host_search = str ;
		}
	}

	if ( ip_search.length > 0 )
	{
		//遍历数组，把IP段转成每个IP存入数组
		for( var i = ( isArray( ip_search[0] ) ? parseInt(ip_search[0][0]) : 0 ), i_end = ( isArray( ip_search[0] ) ? parseInt(ip_search[0][1]) : 0 ); i <= i_end; ++i )
		{
			for( var j = ( isArray( ip_search[1] ) ? parseInt(ip_search[1][0]) : 0 ), j_end = ( isArray( ip_search[1] ) ? parseInt(ip_search[1][1]) : 0 ); j <= j_end; ++j )
			{
				for( var k = ( isArray( ip_search[2] ) ? parseInt(ip_search[2][0]) : 0 ), k_end = ( isArray( ip_search[2] ) ? parseInt(ip_search[2][1]) : 0 ); k <= k_end; ++k )
				{
					for( var l = ( isArray( ip_search[3] ) ? parseInt(ip_search[3][0]) : 0 ), l_end = ( isArray( ip_search[3] ) ? parseInt(ip_search[3][1]) : 0 ); l <= l_end; ++l )
					{
						link_search.push( (( isArray( ip_search[0] ) ? i : ip_search[0] )+'.'+( isArray( ip_search[1] ) ? j : ip_search[1] )+'.'+( isArray( ip_search[2] ) ? k : ip_search[2] )+'.'+( isArray( ip_search[3] ) ? l : ip_search[3] )) ) ;
					}
				}
			}
		}
	}


	if ( host_search.length > 0 )
	{
		//转换hostname
		if ( isArray( host_search ) )
		{
			var str_start = host_search[0] ;
			var str_end   = host_search[3] ;
			var strlen_num = host_search[1].length ;
			var strlen_temp  = parseInt(host_search[1]).toString().length ;
			var need_add_zero = false ;
			if ( strlen_num > strlen_temp )
			{
				need_add_zero = true ;
			}
			for ( var i = parseInt(host_search[1]), i_end = parseInt(host_search[2]); i <= i_end ; ++i )
			{
				if ( need_add_zero && i.toString().length <= strlen_num )
				{
					link_search.push( str_start + pad(i,strlen_num) + str_end ) ;
				}
				else
				{
					link_search.push( str_start + i + str_end ) ;
				}
			}
		}
		else
		{
			link_search.push( host_search ) ;
		}
	}
	/*for ( keys in link_search )
	{
		alert ( link_search[keys] ) ;
	}*/
	return link_search ;
}







