var recordtree0 = new dTree( "recordtree0" ) ,
	recordtree1 = new dTree( "recordtree1" ) ,
	recordtree2 = new dTree( "recordtree2" ) ,
	recordtree3 = new dTree( "recordtree3" ) ,
	recordtree4 = new dTree( "recordtree4" ) ,
	recordtree5 = new dTree( "recordtree5" ) ,
	recordtree6 = new dTree( "recordtree6" ) , 
	recordtree7 = new dTree( "recordtree7" ) ,
	recordtree8 = new dTree( "recordtree8" ) ,
	recordtree9 = new dTree( "recordtree9" ) ,
	recordtree10 = new dTree( "recordtree10" ) ,
	recordtree11 = new dTree( "recordtree11" ) ,
	recordtree12 = new dTree( "recordtree12" ) ,
	recordtree13 = new dTree( "recordtree13" ) ,
	recordtree14 = new dTree( "recordtree14" ) ,
	recordtree15 = new dTree( "recordtree15" ) ,
	recordtree16 = new dTree( "recordtree16" ) ,
	recordtree17 = new dTree( "recordtree17" ) ,
	recordtree18 = new dTree( "recordtree18" ) ,
	recordtree19 = new dTree( "recordtree19" ) ;

$(document).ready(function()
{
	//$('#helplist').popover({placement:'auto',trigger:'hover',content:'这是列表，这里将会列出集合空间、集合。',container:'body'}) ;
	//$('#helpcontext').popover({placement:'auto',trigger:'hover',content:'这是主显示区，可以展示集合空间、集合、记录等详细信息。',container:'body'}) ;
	var heightnum = document.documentElement.clientHeight ;
	document.getElementById("left_list").style.height = (heightnum-220) + "px" ;
	document.getElementById("right_context").style.height = (heightnum - 280) + "px" ;
	window.onresize = function ()
	{
		var heightnum = document.documentElement.clientHeight ;
		document.getElementById("left_list").style.height = (heightnum-220) + "px" ;
		document.getElementById("right_context").style.height = (heightnum - 280) + "px" ;
	}
	getleftlist( "pictree", "data" ) ;
})

function showhide_tb_data(num,style)
{
	var name = "tb-data-" + num ;
	if ( style == true )
	{
		document.getElementById(name).style.display = "block" ;
	}
	else
	{
		document.getElementById(name).style.display = "none" ;
	}
}

function batch_toggle_tb_data ( arr )
{
	var obj = null ;
	for ( var i = 0; ; ++i )
	{
		obj = document.getElementById( "tb-data-" + i ) ;
		if ( !obj )
		{
			break ;
		}
		if ( array_is_exists ( arr, i ) )
		{
			showhide_tb_data ( i, true ) ;
		}
		else
		{
			showhide_tb_data ( i, false ) ;
		}
	}
}

function settempdata( cs, cl )
{
	if ( cs != "" )
	{
		document.getElementById("h-cs").value = cs ;
	}
	if ( cl != "" )
	{
		document.getElementById("h-cl").value = cl ;
	}
}

function senddata ( cs, cl, style )
{
	if ( style == "cldata" )
	{
		document.getElementById("h-skip").value = "20" ;
	}
	cs = convert2post( cs ) ;
	cl = convert2post( cl ) ;
	ajax2send( "context", "post", "index.php?p=data&m=ajax_r", "cs=" + cs + "&cl=" + cl + "&ty=" + style ) ;
}

function getnewdata( pon )
{
	var num = 20 ;
	var cs = document.getElementById("h-cs").value ;
	var cl = document.getElementById("h-cl").value ;
	if ( pon == 1 )
	{
		num = 20 ;
	}
	else
	{
		num = -20 ;
	}
	var skipnum = parseInt(document.getElementById("h-skip").value) + num ;
	document.getElementById("h-skip").value = skipnum ;
	cs = convert2post( cs ) ;
	cl = convert2post( cl ) ;
	ajax2send( "context", "post", "index.php?p=data&m=ajax_r", "cs=" + cs + "&cl=" + cl + "&sk=" + skipnum + "&ty=cldata" ) ;
}
function getnewdata2()
{
	var cs = document.getElementById("h-cs").value ;
	var cl = document.getElementById("h-cl").value ;
	var page = parseInt( document.getElementById("pagenum").value ) ;
	if ( page <=0 )
	{
		page = 1 ;
	}
	skipnum = page * 20 ;
	document.getElementById("h-skip").value = skipnum ;
	ajax2send( "context", "post", "index.php?p=data&m=ajax_r", "cs=" + cs + "&cl=" + cl + "&sk=" + skipnum + "&ty=cldata" ) ;
}

function encodeJsonComponent(json)
{
   
   json = replaceAllEx(json,'\n','[\\\\n]');
   json = replaceAllEx(json,'\r','[\\\\r]');
   json = replaceAllEx(json,'\t','[\\\\t]');
   json = replaceAllEx(json,'\b','[\\\\b]');
   json = replaceAllEx(json,'\f','[\\\\f]');
   json = replaceAllEx(json,'\'','[\\\\\']');
   //json = replaceAllEx(json,'\&','[\\\\\&]');
   //json = replaceAllEx(json,'\\','[\\\\]');
   
   return json;
}

function isArray(object)
{
    return  object && typeof object==='object' &&    
            typeof object.length==='number' &&  
            typeof object.splice==='function' &&     
            !(object.propertyIsEnumerable('length'));
}
function string2tree( obj1, obj2, num, newtree )
{
	obj1.innerHTML = obj2.innerHTML ;
	var tempstr = encodeJsonComponent ( obj2.innerHTML ) ;
	var json = eval('(' + tempstr + ')');
	var k = 0 ;
	newtree.add ( 0, -1, " JSON ", "", "", "", "images/tree/object.png", "images/tree/object.png" ) ;
	forstring2tree( json, newtree, k ) ;
	obj2.innerHTML = newtree ;
	function forstring2tree( json, newtree, k )
	{
		var oldnum = k ;
		for ( var key in json )
		{
			++k ;
			if ( typeof(json[key]) == "object" && !( json[key] === null ) )
			{
				if ( !isArray ( json[key] ) )
				{
					newtree.add ( k, oldnum, key, "", "", "", "images/tree/object.png", "images/tree/object.png" ) ;
				}
				else
				{
					newtree.add ( k, oldnum, key, "", "", "", "images/tree/array.png", "images/tree/array.png" ) ;
				}
				k = forstring2tree( json[key], newtree, k ) ;
			}
			else
			{
				if ( typeof ( json[key] ) == "string" )
				{
					newtree.add ( k, oldnum, key + " : \"" + replaceAllEx( json[key],'\"','[\\\"]' ) + "\"", "", "", "", "images/tree/string.png", "images/tree/string.png" ) ;
				}
				else if ( typeof ( json[key] ) == "boolean" )
				{
					newtree.add ( k, oldnum, key + " : " + json[key], "", "", "", "images/tree/bool.png", "images/tree/bool.png" ) ;
				}
				else if ( json[key] === null )
				{
					newtree.add ( k, oldnum, key + " : " + json[key], "", "", "", "images/tree/null.png", "images/tree/null.png" ) ;
				}
				else
				{
					newtree.add ( k, oldnum, key + " : " + json[key], "", "", "", "images/tree/number.png", "images/tree/number.png" ) ;
				}
			}
		}
		return k ;
	}
}

function tree2string( obj1, obj2 )
{
	obj2.innerHTML = obj1.innerHTML ;
	obj1.innerHTML = "1" ;
}

function convert_record_show( )
{
	var obj1 = null ;
	var obj2 = null ;
	var style = null ;
	for ( var i = 0; ; ++i )
	{
		obj1 = document.getElementById( "data_r_nature_" + i ) ;
		obj2 = document.getElementById( "data_r_record_" + i ) ;
		if ( !obj1 || !obj2 )
		{
			break ;
		}
		if ( style == null )
		{
			style = obj1.innerHTML ;
		}
		if ( style == "1" )
		{
			eval ( 'recordtree' + i + ' = null; recordtree' + i + ' = new dTree( "recordtree' + i + '" ); var temp = recordtree' + i + ';' ) ;
			string2tree ( obj1, obj2, i, temp ) ;
		}
		else
		{
			tree2string( obj1, obj2 ) ;
		}
	}
}

function toggle_tb_data(style)
{
	document.getElementById("tool_data_respond").innerHTML = "" ;
	document.getElementById("h-type").value = style ;
	if ( style == "insert" )
	{
		document.getElementById("tb-data-title").innerHTML = "插入记录" ;
		batch_toggle_tb_data( [ 0 ] ) ;
	}
	else if ( style == "delete" )
	{
		document.getElementById("tb-data-title").innerHTML = "删除记录" ;
		batch_toggle_tb_data( [ 4, 7 ] ) ;
	}
	else if ( style == "update" )
	{
		document.getElementById("tb-data-title").innerHTML = "更新记录" ;
		batch_toggle_tb_data( [ 1, 4, 7 ] ) ;
	}
	else if ( style == "find" )
	{
		document.getElementById("tb-data-title").innerHTML = "查询记录" ;
		batch_toggle_tb_data( [ 4, 5, 6, 7, 8, 9, 18 ] ) ;
	}
	else if ( style == "createcs" )
	{
		document.getElementById("tb-data-title").innerHTML = "创建集合空间" ;
		batch_toggle_tb_data( [ 10, 11 ] ) ;
	}
	else if ( style == "dropcs" )
	{
		document.getElementById("tb-data-title").innerHTML = "删除集合空间" ;
		batch_toggle_tb_data( [ ] ) ;
	}
	else if ( style == "createcl" )
	{
		document.getElementById("tb-data-title").innerHTML = "创建集合" ;
		batch_toggle_tb_data( [ 12, 13 ] ) ;
	}
	else if ( style == "dropcl" )
	{
		document.getElementById("tb-data-title").innerHTML = "删除集合" ;
		batch_toggle_tb_data( [ ] ) ;
	}
	else if ( style == "split" )
	{
		document.getElementById("tb-data-title").innerHTML = "集合切分" ;
		batch_toggle_tb_data( [ 2, 3, 4, 19 ] ) ;
	}
	else if ( style == "count" )
	{
		document.getElementById("tb-data-title").innerHTML = "获取记录数" ;
		batch_toggle_tb_data( [ 4 ] ) ;
	}
	else if ( style == "createindex" )
	{
		document.getElementById("tb-data-title").innerHTML = "创建索引" ;
		batch_toggle_tb_data( [ 14, 15, 16, 17 ] ) ;
	}
	else if ( style == "deleteindex" )
	{
		document.getElementById("tb-data-title").innerHTML = "删除索引" ;
		batch_toggle_tb_data( [ 15 ] ) ;
	}
}

function ajaxsenddatacommon()
{
	var style  = document.getElementById("h-type").value ;
	var csname = document.getElementById("h-cs").value ;
	var clname = document.getElementById("h-cl").value ;
	style = convert2post( style ) ;
	csname = convert2post( csname ) ;
	clname = convert2post( clname ) ;
	if ( style == "insert" )
	{
		var record = document.getElementById("tool_data_record").value ;
		record = convert2post( record ) ;
		ajax2send4 ( "context", "post", "index.php?p=data&m=ajax_w", "type=" + style + "&csname=" + csname + "&clname=" + clname + "&record=" + record, true, 'senddata( "' + csname + '", "' + clname + '", "cldata" )' ) ;
		$('#myModal1').modal('toggle');
	}
	else if ( style == "delete" )
	{
		var condition = document.getElementById("tool_data_condition").value ;
		var hint      = document.getElementById("tool_data_hint").value ;
		condition = convert2post( condition ) ;
		hint = convert2post( hint ) ;
		ajax2send4 ( "context", "post", "index.php?p=data&m=ajax_w", "type=" + style + "&csname=" + csname + "&clname=" + clname + "&condition=" + condition + "&hint=" + hint, true, 'senddata( "' + csname + '", "' + clname + '", "cldata" )' ) ;
		$('#myModal1').modal('toggle');
	}
	else if ( style == "update" )
	{
		var rule      = document.getElementById("tool_data_rule").value ;
		var condition = document.getElementById("tool_data_condition").value ;
		var hint      = document.getElementById("tool_data_hint").value ;
		rule = convert2post( rule ) ;
		condition = convert2post( condition ) ;
		hint = convert2post( hint ) ;
		ajax2send4 ( "context", "post", "index.php?p=data&m=ajax_w", "type=" + style + "&csname=" + csname + "&clname=" + clname + "&rule=" + rule + "&condition=" + condition + "&hint=" + hint, true, 'senddata( "' + csname + '", "' + clname + '", "cldata" )' ) ;
		$('#myModal1').modal('toggle');
	}
	else if ( style == "find" )
	{
		var condition   = document.getElementById("tool_data_condition").value ;
		var selecter    = document.getElementById("tool_data_selecter").value ;
		var orderby     = document.getElementById("tool_data_orderby").value ;
		var hint        = document.getElementById("tool_data_hint").value ;
		var numtoskip   = document.getElementById("tool_data_numtoskip").value ;
		var numtoreturn = document.getElementById("tool_data_numbertoreturn").value ;
		condition = convert2post( condition ) ;
		selecter = convert2post( selecter ) ;
		orderby = convert2post( orderby ) ;
		hint = convert2post( hint ) ;
		numtoskip = convert2post( numtoskip ) ;
		numtoreturn = convert2post( numtoreturn ) ;
		ajax2send4 ( "context", "post", "index.php?p=data&m=ajax_w", "type=" + style + "&csname=" + csname + "&clname=" + clname + "&condition=" + condition + "&selecter=" + selecter + "&orderby=" + orderby + "&hint=" + hint + "&numtoskip=" + numtoskip + "&numtoreturn=" + numtoreturn, true ) ;
		$('#myModal1').modal('toggle');
	}
	else if ( style == "createcs" )
	{
		csname       = document.getElementById("tool_data_csname").value ;
		var pagesize = document.getElementById("tool_data_pagesize").value ;
		csname = convert2post( csname ) ;
		pagesize = convert2post( pagesize ) ;
		ajax2send4 ( "context", "post", "index.php?p=data&m=ajax_w", "type=" + style + "&csname=" + csname + "&pagesize=" + pagesize, true, 'getleftlist( "pictree", "data" )' ) ;
		$('#myModal1').modal('toggle');
	}
	else if ( style == "createcl" )
	{
		clname       = document.getElementById("tool_data_clname").value ;
		var optionss = document.getElementById("tool_data_options").value ;
		clname = convert2post( clname ) ;
		optionss = convert2post( optionss ) ;
		ajax2send4 ( "context", "post", "index.php?p=data&m=ajax_w", "type=" + style + "&csname=" + csname + "&clname=" + clname + "&options=" + optionss, true, 'getleftlist( "pictree", "data" )' ) ;
		$('#myModal1').modal('toggle');
	}
	else if ( style == "split" )
	{
		var sourcename = document.getElementById("tool_data_sourcename").value ;
		var destname = document.getElementById("tool_data_destname").value ;
		var condition = document.getElementById("tool_data_condition").value ;
		var endcondition = document.getElementById("tool_data_endcondition").value ;
		sourcename = convert2post( sourcename ) ;
		destname = convert2post( destname ) ;
		condition = convert2post( condition ) ;
		ajax2send4 ( "context", "post", "index.php?p=data&m=ajax_w", "type=" + style + "&csname=" + csname + "&clname=" + clname + "&sourcename=" + sourcename + "&destname=" + destname + "&condition=" + condition + "&endcondition=" + endcondition ) ;
		$('#myModal1').modal('toggle');
	}
	else if ( style == "createindex" )
	{
		var indexdef = document.getElementById("tool_data_indexdef").value ;
		var indexname = document.getElementById("tool_data_indexname").value ;
		var indexunique = document.getElementById("tool_data_indexunique").checked ;
		var indexenforced = document.getElementById("tool_data_indexenforced").checked ;
		indexdef = convert2post( indexdef ) ;
		indexname = convert2post( indexname ) ;
		ajax2send4 ( "context", "post", "index.php?p=data&m=ajax_w", "type=" + style + "&csname=" + csname + "&clname=" + clname + "&indexdef=" + indexdef + "&indexname=" + indexname + "&indexunique=" + indexunique + "&indexenforced=" + indexenforced ) ;
		$('#myModal1').modal('toggle');
	}
	else if ( style == "deleteindex" )
	{
		var indexname = document.getElementById("tool_data_indexname").value ;
		indexname = convert2post( indexname ) ;
		ajax2send4 ( "context", "post", "index.php?p=data&m=ajax_w", "type=" + style + "&csname=" + csname + "&clname=" + clname + "&indexname=" + indexname ) ;
		$('#myModal1').modal('toggle');
	}
	else if ( style == "count" )
	{
		var condition   = document.getElementById("tool_data_condition").value ;
		condition = convert2post( condition ) ;
		ajax2send ( "tool_data_respond", "post", "index.php?p=data&m=ajax_w", "type=" + style + "&csname=" + csname + "&clname=" + clname + "&condition=" + condition ) ;
	}
	else if ( style == "dropcs" )
	{
		ajax2send4 ( "context", "post", "index.php?p=data&m=ajax_w", "type=" + style + "&csname=" + csname, true, 'getleftlist( "pictree", "data" )' ) ;
		$('#myModal1').modal('toggle');
	}
	else if ( style == "dropcl" )
	{
		ajax2send4 ( "context", "post", "index.php?p=data&m=ajax_w", "type=" + style + "&csname=" + csname + "&clname=" + clname, true, 'getleftlist( "pictree", "data" )' ) ;
		$('#myModal1').modal('toggle');
	}
	
}