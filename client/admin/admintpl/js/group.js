function senddata ( group, hostname, servicename, isstart )
{
	document.getElementById("context").style.display = "block" ;
	document.getElementById("context2").style.display = "none" ;
	if ( group != "" )
	{
		document.getElementById("right_title").innerHTML = "分区组 " + group ;
		document.getElementById("h-group").value = group ;
		show_group_tool_1( isstart );
	}
	if ( hostname != "" && servicename != "" )
	{
		document.getElementById("right_title").innerHTML = "分区组 " + group + " - 节点 " + hostname + " : " + servicename ;
		document.getElementById("h-node").value = hostname + ":" + servicename ;
		show_group_tool_2( isstart );
	}
	group = convert2post( group ) ;
	hostname = convert2post( hostname ) ;
	servicename = convert2post( servicename ) ;
	ajax2send( "context", "post", "index.php?p=group&m=ajax_r", "group=" + group + "&hostname=" + hostname + "&node=" + servicename ) ;
}

function showhide_tb_group(num,style)
{
	var name = "tb-group-" + num ;
	if ( style == true )
	{
		document.getElementById(name).style.display = "block" ;
	}
	else
	{
		document.getElementById(name).style.display = "none" ;
	}
}

function batch_toggle_tb_group ( arr )
{
	var obj = null ;
	for ( var i = 0; ; ++i )
	{
		obj = document.getElementById( "tb-group-" + i ) ;
		if ( !obj )
		{
			break ;
		}
		if ( array_is_exists ( arr, i ) )
		{
			showhide_tb_group ( i, true ) ;
		}
		else
		{
			showhide_tb_group ( i, false ) ;
		}
	}
}

function ajaxsendgroupcommon()
{
	var style  = document.getElementById("h-type").value ;
	var gname = document.getElementById("h-group").value ;
	var nname = document.getElementById("h-node").value ;
	style = convert2post( style ) ;
	gname = convert2post( gname ) ;
	nname = convert2post( nname ) ;
	if ( style == "creategroup" )
	{
		var groupname = document.getElementById("tool_group_groupname").value ;
		groupname = convert2post( groupname ) ;
		ajax2send ( "tool_group_respond", "post", "index.php?p=group&m=ajax_w", "type=" + style + "&groupname=" + groupname ) ;
		ajax2send4 ( "tool_group_respond", "post", "index.php?p=group&m=ajax_w", "type=" + style + "&groupname=" + groupname, true, 'getleftlist( "pictree", "group" )' ) ;
	}
	else if ( style == "createcatalog" )
	{
		var hostname     = document.getElementById("tool_group_hostname").value ;
		var serivcename  = document.getElementById("tool_group_servicename").value ;
		var databasepath = document.getElementById("tool_group_databasepath").value ;
		var config       = document.getElementById("tool_group_config").value ;
		hostname = convert2post( hostname ) ;
		serivcename = convert2post( serivcename ) ;
		databasepath = convert2post( databasepath ) ;
		config = convert2post( config ) ;
		ajax2send ( "tool_group_respond", "post", "index.php?p=group&m=ajax_w", "type=" + style + "&groupname=" + gname + "&hostname=" + hostname + "&serivcename=" + serivcename + "&databasepath=" + databasepath + "&config=" + config ) ;
	}
	else if ( style == "createnode" )
	{
		var hostname     = document.getElementById("tool_group_hostname").value ;
		var serivcename  = document.getElementById("tool_group_servicename").value ;
		var databasepath = document.getElementById("tool_group_databasepath").value ;
		var config       = document.getElementById("tool_group_config").value ;
		hostname = convert2post( hostname ) ;
		serivcename = convert2post( serivcename ) ;
		databasepath = convert2post( databasepath ) ;
		config = convert2post( config ) ;
		ajax2send ( "tool_group_respond", "post", "index.php?p=group&m=ajax_w", "type=" + style + "&groupname=" + gname + "&hostname=" + hostname + "&serivcename=" + serivcename + "&databasepath=" + databasepath + "&config=" + config ) ;
	}
	else if ( style == "startgroup" )
	{
		ajax2send ( "tool_group_respond", "post", "index.php?p=group&m=ajax_w", "type=" + style + "&groupname=" + gname ) ;
	}
	else if ( style == "stopgroup" )
	{
		ajax2send ( "tool_group_respond", "post", "index.php?p=group&m=ajax_w", "type=" + style + "&groupname=" + gname ) ;
	}
	else if ( style == "startnode" )
	{
		ajax2send ( "tool_group_respond", "post", "index.php?p=group&m=ajax_w", "type=" + style + "&groupname=" + gname + "&nodename=" + nname ) ;
	}
	else if ( style == "stopnode" )
	{
		ajax2send ( "tool_group_respond", "post", "index.php?p=group&m=ajax_w", "type=" + style + "&groupname=" + gname + "&nodename=" + nname ) ;
	}
}

function toggle_tb_group(style)
{
	document.getElementById("tool_group_respond").innerHTML = "" ;
	document.getElementById("h-type").value = style ;
	if ( style == "creategroup" )
	{
		document.getElementById("tb-group-title").innerHTML = "创建分区组" ;
		batch_toggle_tb_group ( [ 0 ] ) ;
	}
	else if ( style == "createcatalog" )
	{
		document.getElementById("tb-group-title").innerHTML = "创建编目组" ;
		batch_toggle_tb_group ( [ 1, 2, 3, 4 ] ) ;
	}
	else if ( style == "createnode" )
	{
		document.getElementById("tb-group-title").innerHTML = "创建节点" ;
		batch_toggle_tb_group ( [ 1, 2, 3, 4 ] ) ;;
	}
	else if ( style == "startgroup" )
	{
		document.getElementById("tb-group-title").innerHTML = "启动分区组" ;
		batch_toggle_tb_group ( [ ] ) ;
	}
	else if ( style == "stopgroup" )
	{
		document.getElementById("tb-group-title").innerHTML = "停止分区组" ;
		batch_toggle_tb_group ( [ ] ) ;
	}
	else if ( style == "startnode" )
	{
		document.getElementById("tb-group-title").innerHTML = "启动节点" ;
		batch_toggle_tb_group ( [ ] ) ;
	}
	else if ( style == "stopnode" )
	{
		document.getElementById("tb-group-title").innerHTML = "停止节点" ;
		batch_toggle_tb_group ( [ ] ) ;
	}
}