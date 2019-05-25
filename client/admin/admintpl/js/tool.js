function showtool( obj1, obj2 )
{
	var b1 = document.getElementById(obj1) ;
	var b2 = document.getElementById(obj2) ;
	
	if ( b1.innerHTML == "隐藏工具栏" )
	{
		b2.style.display = "none" ;
		b1.innerHTML = "显示工具栏" ;
		var strHeight = (window.screen.availHeight-200) + "px" ;
		document.getElementById("left_list").style.height = strHeight ;
		document.getElementById("right_context").style.height = strHeight ;
		document.getElementById("index").style.marginTop = "13px" ;
	}
	else
	{
		b2.style.display = "block" ;
		b1.innerHTML = "隐藏工具栏" ;
		var strHeight = (window.screen.availHeight-270) + "px" ;
		document.getElementById("left_list").style.height = strHeight ;
		document.getElementById("right_context").style.height = strHeight ;
		document.getElementById("index").style.marginTop = "0px" ;
	}
}

function loadconnection( button_obj )
{
	var table_row_obj = button_obj.parentNode.parentNode ;
	var add = table_row_obj.getElementsByTagName('td').item(0).innerHTML ;
	var represent = table_row_obj.getElementsByTagName('td').item(1).innerHTML ;
	var user = table_row_obj.getElementsByTagName('td').item(2).innerHTML ;
	var pwd = table_row_obj.getElementsByTagName('td').item(3).innerHTML ;
	document.getElementById("connect_address").value    = html_encode( add ) ;
	document.getElementById("connect_represent").value  = html_encode( represent ) ;
	document.getElementById("connect_user").value       = html_encode( user ) ;
	document.getElementById("connect_password").value   = html_encode( pwd ) ;
}

function deleteconnect( button_obj )
{
	var img = document.getElementById("connect_return_load") ;
	var obj = document.getElementById("connect_return") ;
	var table_row_obj = button_obj.parentNode.parentNode ;
	var add = table_row_obj.getElementsByTagName('td').item(0).innerHTML ;
	var order = "connectmodel=delete&connectaddress=" + convert2post( html_encode( add ) ) ;
	function callback_deleteconnect_before( xhr )
	{
		img.style.display = '' ;
	}
	function callback_deleteconnect_complete( xhr, ts )
	{
		img.style.display = 'none' ;
		obj.innerHTML = '' ;
		showconnectmanager() ;
	}
	ajax2sendNew( 'post','index.php?p=connectlist&m=ajax_f', order, true, callback_success, callback_err, callback_deleteconnect_before, callback_deleteconnect_complete ) ;
}

function updateconnect()
{
	var kep  = document.getElementById("keeppassword").checked ;
	var add  = document.getElementById("connect_address").value ;
	var user = document.getElementById("connect_user").value ;
	var pwd  = document.getElementById("connect_password").value ;
	var rep  = document.getElementById("connect_represent").value ;
	
	var obj  = document.getElementById("connect_return") ;
	var img  = document.getElementById("connect_return_load") ;
	
	obj.innerHTML = '' ;
	if ( kep == false )
	{
		pwd = "" ;
	}
	if ( add == "" )
	{
		obj.innerHTML = '<div class="alert alert-danger alert-dismissable"><button type="button" class="close" data-dismiss="alert" aria-hidden="true">&times;</button>请输入连接地址</div>' ;
	}
	else
	{
		function callback_updateconnect_before( xhr )
		{
			img.style.display = '' ;
		}
		function callback_updateconnect_err()
		{
			obj.innerHTML = '<div class="alert alert-danger alert-dismissable"><button type="button" class="close" data-dismiss="alert" aria-hidden="true">&times;</button>网络错误</div>' ;
		}
		function callback_updateconnect_complete()
		{
			img.style.display = 'none' ;
			showconnectmanager() ;
		}
		var order = 'connectmodel=update&connectaddress=' + convert2post( add ) + '&connectuser=' + convert2post( user ) + '&connectpwd=' + convert2post( pwd ) + "&represent=" + convert2post( rep ) ;
		ajax2sendNew( 'post','index.php?p=connectlist&m=ajax_f', order, true, callback_success, callback_updateconnect_err, callback_updateconnect_before, callback_updateconnect_complete ) ;
	}
}

function testconnect()
{
	var add  = document.getElementById("connect_address").value ;
	var user = document.getElementById("connect_user").value ;
	var pwd  = document.getElementById("connect_password").value ;
	
	var obj  = document.getElementById("connect_return") ;
	var img  = document.getElementById("connect_return_load") ;
	
	obj.innerHTML = '' ;
	if ( add == "" )
	{
		obj.innerHTML = '<div class="alert alert-danger alert-dismissable"><button type="button" class="close" data-dismiss="alert" aria-hidden="true">&times;</button>请输入连接地址</div>' ;
	}
	else
	{	
		function callback_testconnect_before( xhr )
		{
			img.style.display = '' ;
		}
		function callback_testconnect_success( str )
		{
			if ( str )
			{
				try
				{
					var json_obj = eval('(' + str + ')');
					if ( json_obj['errno'] == 0 )
					{
						obj.innerHTML = '<div class="alert alert-success alert-dismissable"><button type="button" class="close" data-dismiss="alert" aria-hidden="true">&times;</button>' + json_obj['message'] + '</div>' ;
					}
					else
					{
						obj.innerHTML = '<div class="alert alert-danger alert-dismissable"><button type="button" class="close" data-dismiss="alert" aria-hidden="true">&times;</button>' + json_obj['message'] + '</div>' ;
					}
				}
				catch(e)
				{
					obj.innerHTML = '<div class="alert alert-danger alert-dismissable"><button type="button" class="close" data-dismiss="alert" aria-hidden="true">&times;</button>系统错误</div>' ;
				}
			}
			else
			{
				obj.innerHTML = "" ;
			}
		}
		function callback_testconnect_err()
		{
			obj.innerHTML = '<div class="alert alert-danger alert-dismissable"><button type="button" class="close" data-dismiss="alert" aria-hidden="true">&times;</button>网络错误</div>' ;
		}
		function callback_testconnect_complete()
		{
			img.style.display = 'none' ;
		}
		var order = 'connectmodel=testlink&connectaddress=' + convert2post( add ) + '&connectuser=' + convert2post( user ) + '&connectpwd=' + convert2post( pwd ) ;
		ajax2sendNew( 'post','index.php?p=connectlist&m=ajax_f', order, true, callback_testconnect_success, callback_testconnect_err, callback_testconnect_before, callback_testconnect_complete ) ;
	}
}

function sessionconnect()
{
	var add  = document.getElementById("connect_address").value ;
	var user = document.getElementById("connect_user").value ;
	var pwd  = document.getElementById("connect_password").value ;
	
	var obj  = document.getElementById("connect_return") ;
	var img  = document.getElementById("connect_return_load") ;

	if ( add == "" )
	{
		obj.innerHTML = '<div class="alert alert-danger alert-dismissable"><button type="button" class="close" data-dismiss="alert" aria-hidden="true">&times;</button>请输入连接地址</div>' ;
	}
	else
	{
		function callback_sessionconnect_before( xhr )
		{
			img.style.display = '' ;
		}
		function callback_sessionconnect_success( str )
		{
			if ( str )
			{
				try
				{
					var json_obj = eval('(' + str + ')');
					if ( json_obj['errno'] == 0 )
					{
						tool_refresh_all ( location ) ;
					}
					else
					{
						obj.innerHTML = '<div class="alert alert-danger alert-dismissable"><button type="button" class="close" data-dismiss="alert" aria-hidden="true">&times;</button>' + json_obj['message'] + '</div>' ;
					}
				}
				catch(e)
				{
					obj.innerHTML = '<div class="alert alert-danger alert-dismissable"><button type="button" class="close" data-dismiss="alert" aria-hidden="true">&times;</button>系统错误</div>' ;
				}
			}
			else
			{
				obj.innerHTML = "" ;
			}
		}
		function callback_sessionconnect_err()
		{
			obj.innerHTML = '<div class="alert alert-danger alert-dismissable"><button type="button" class="close" data-dismiss="alert" aria-hidden="true">&times;</button>网络错误</div>' ;
		}
		function callback_sessionconnect_complete()
		{
			img.style.display = 'none' ;
		}
		//ajax2send("connect_return","post","index.php?p=connectlist&m=ajax_f","connectmodel=connect&connectaddress=" + add + "&connectuser=" + user + "&connectpwd=" + pwd, false ) ;
		var order = 'connectmodel=connect&connectaddress=' + convert2post( add ) + '&connectuser=' + convert2post( user ) + '&connectpwd=' + convert2post( pwd ) ;
		ajax2sendNew( 'post','index.php?p=connectlist&m=ajax_f', order, true, callback_sessionconnect_success, callback_sessionconnect_err, callback_sessionconnect_before, callback_sessionconnect_complete ) ;
	}
}

function sessionconnect2( button_obj )
{
	//ajax2send("connect_return","post","index.php?p=connectlist&m=ajax_f","connectmodel=connect&connectaddress=" + add + "&connectuser=" + user + "&connectpwd=" + pwd, false ) ;
	var table_row_obj = button_obj.parentNode.parentNode ;
	var img = document.getElementById('connect_return_load') ;
	var obj = document.getElementById('connect_return') ;
	var add = table_row_obj.getElementsByTagName('td').item(0).innerHTML ;
	var user = table_row_obj.getElementsByTagName('td').item(2).innerHTML ;
	var pwd = table_row_obj.getElementsByTagName('td').item(3).innerHTML ;
	add = convert2post( add ) ;
	user = convert2post( user ) ;
	pwd = convert2post( pwd ) ;
	
	function callback_sessionconnect2_before( xhr )
	{
		img.style.display = '' ;
	}
	function callback_sessionconnect2_success( str )
	{
		try
		{
			var json_obj = eval('(' + str + ')');
			if ( json_obj['errno'] == 0 )
			{
				tool_refresh_all ( location ) ;
			}
			else
			{
				obj.innerHTML = '<div class="alert alert-danger alert-dismissable"><button type="button" class="close" data-dismiss="alert" aria-hidden="true">&times;</button> ' + json_obj['message'] + '</div>'
			}
		}
		catch(e)
		{
		}
	}
	function callback_sessionconnect2_err()
	{
		obj.innerHTML = '<div class="alert alert-danger alert-dismissable"><button type="button" class="close" data-dismiss="alert" aria-hidden="true">&times;</button> 网络错误</div>'
	}
   function callback_sessionconnect2_complete( xhr, ts )
	{
		img.style.display = 'none' ;
	}
	
	var order = "connectmodel=connect&connectaddress=" + add + "&connectuser=" + user + "&connectpwd=" + pwd ;
	ajax2sendNew( 'post', 'index.php?p=connectlist&m=ajax_f', order, true, callback_sessionconnect2_success, callback_sessionconnect2_err, callback_sessionconnect2_before, callback_sessionconnect2_complete ) ;
}

function convertChart ( data, step )
{
	var ary = [] ;
	if ( data.length == 0 )
	{
		for ( var i = 0 ; i <= 60 ; i+=step )
		{
			var temp = [i,0] ;
			data.push( temp ) ;
		}
	}
	for ( var i = 1 ; i < data.length ; ++i )
	{
		var temp = data[i] ;
		temp[0] -= step ;
		ary.push( temp ) ;
	}
	return ary ;
}
function tool_refresh_all ( lo )
{
	lo.reload() ;
}
function left_list_open_all ( b )
{
	b.openAll() ;
}

function left_list_close_all ( b )
{
	b.closeAll() ;
}
function sqlexecute ( obj1, obj2 )
{
	var str = document.getElementById(obj1).value ;
	ajax2send4 ( "context", "post", "index.php?p=sql&m=ajax_r", "sql=" + str ) ;
	$('#Modal_sql').modal('toggle');
}