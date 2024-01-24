function ajax2send( obj, style, url, context, ajax, functions )
{
	if ( ajax == undefined )
	{
		ajax = true ;
	}
	if ( functions == undefined )
	{
		functions = "" ;
	}
	document.getElementById(obj).innerHTML = "<img src=\"images/loading.gif\" /> 载入中..." ;
	
	var xmlhttp;
	try
	{
		xmlhttp = new XMLHttpRequest();
	}
	catch (e)
	{
		try
		{
			xmlhttp = new ActiveXObject("Msxml2.XMLHTTP");
		}
		catch (e)
		{
			xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
		}
	}

	xmlhttp.onreadystatechange = function()
	{
		if (4 == xmlhttp.readyState)
		{
			if (200 == xmlhttp.status)
			{
				var Bodys = xmlhttp.responseText;
				document.getElementById(obj).innerHTML = Bodys ;
				if ( functions )
				{
					eval('(' + functions + ')');
				}
			}
		}
	}

	xmlhttp.open( style, url, ajax ) ;
	xmlhttp.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
	xmlhttp.send( context ) ;
}

function ajax2send2( style, url, context )
{
	var record ;
	var xmlhttp;
	try
	{
		xmlhttp = new XMLHttpRequest();
	}
	catch (e)
	{
		try
		{
			xmlhttp = new ActiveXObject("Msxml2.XMLHTTP");
		}
		catch (e)
		{
			xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
		}
	}

	xmlhttp.onreadystatechange = function()
	{
		if (4 == xmlhttp.readyState)
		{
			if (200 == xmlhttp.status)
			{
				record = xmlhttp.responseText;
				if ( record )
				{
					record = eval('(' + record + ')');
				}
			}
		}
	}

	xmlhttp.open( style, url, false ) ;
	xmlhttp.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
	xmlhttp.send( context ) ;
	return record ;
}

function ajax2send3( obj, style, url, context, ajax, functions )
{
	var record ;
	var xmlhttp;
	if ( ajax == undefined )
	{
		ajax = true ;
	}
	document.getElementById(obj).innerHTML = "<img src=\"images/loading.gif\" /> 载入中..." ;
	try
	{
		xmlhttp = new XMLHttpRequest();
	}
	catch (e)
	{
		try
		{
			xmlhttp = new ActiveXObject("Msxml2.XMLHTTP");
		}
		catch (e)
		{
			xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
		}
	}

	xmlhttp.onreadystatechange = function()
	{
		if (4 == xmlhttp.readyState)
		{
			if (200 == xmlhttp.status)
			{
				record = xmlhttp.responseText;
				if ( record )
				{
					try
					{
						record = eval('(' + record + ')');
						eval( functions );
					}
					catch(e)
					{
						document.getElementById(obj).innerHTML = "" ;
					}
				}
				else
				{
					document.getElementById(obj).innerHTML = "" ;
				}
			}
		}
	}

	xmlhttp.open( style, url, ajax ) ;
	xmlhttp.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
	xmlhttp.send( context ) ;
	return record ;
}

function ajax2send4( obj, style, url, context, ajax, functions )
{
	if ( ajax == undefined )
	{
		ajax = true ;
	}
	if ( functions == undefined )
	{
		functions = "" ;
	}
	//document.getElementById(obj).innerHTML = "<img src=\"images/loading.gif\" /> 载入中..." ;
	
	var xmlhttp;
	try
	{
		xmlhttp = new XMLHttpRequest();
	}
	catch (e)
	{
		try
		{
			xmlhttp = new ActiveXObject("Msxml2.XMLHTTP");
		}
		catch (e)
		{
			xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
		}
	}

	xmlhttp.onreadystatechange = function()
	{
		if (4 == xmlhttp.readyState)
		{
			if (200 == xmlhttp.status)
			{
				var Bodys = xmlhttp.responseText;
				Bodys = Bodys.replace(new RegExp("\r\n","gm"),"\\n");
				Bodys = Bodys.replace(new RegExp("\r","gm"),"");
				Bodys = Bodys.replace(new RegExp("\n","gm"),"\\n");
				if ( Bodys )
				{
					Bodys = eval('(' + Bodys + ')');
				}
				else
				{
					Bodys = eval( '( { } )' ) ;
				}
				
				if ( Bodys["context"] != "" )
				{
					document.getElementById(obj).innerHTML = Bodys["context"] ;
				}
				if ( Bodys["respond"] != "" )
				{
					var temp ;
					if ( Bodys["rc"] == 0 )
					{
						temp = '<div>' + Bodys["respond"] + "</div>" ;
					}
					else
					{
						temp = '<div style="color:#F00;">' + Bodys["respond"] + "</div>" ;
					}
					document.getElementById("context_respond").innerHTML += temp ;
					document.getElementById("context_respond").scrollTop = document.getElementById("context_respond").scrollHeight ;
				}
				if ( functions )
				{
					eval('(' + functions + ')');
				}
			}
		}
	}

	xmlhttp.open( style, url, ajax ) ;
	xmlhttp.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
	xmlhttp.send( context ) ;
}

// style 	 	发送类型 'post' 'get'
// url       	发送地址
// context   	post发送的内容
// async     	false:同步 true:异步
// function_1 	成功返回的回调函数
// function_2 	失败返回的回调函数
// function_3 	发送前回调函数
// function_4 	完成后的回调函数
function ajax2sendNew( style, url, context, async, function_1, function_2, function_3, function_4 )
{
	$.ajax({
		'url': url,
		'type': style,
		'data': context,
		'async': async,
		'dataType': 'text',
		'error': function_2,
		'success': function_1,
		'beforeSend':function_3,
		'complete':function_4
	});
/*
	var xmlhttp;
	try
	{
		xmlhttp = new XMLHttpRequest();
	}
	catch (e)
	{
		try
		{
			xmlhttp = new ActiveXObject("Msxml2.XMLHTTP");
		}
		catch (e)
		{
			xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
		}
	}

	xmlhttp.onreadystatechange = function()
	{
		if (4 == xmlhttp.readyState)
		{
			if (200 == xmlhttp.status)
			{
				record = xmlhttp.responseText;
				function_1( record ) ;
			}
			else
			{
				function_2() ;
			}
		}
	}

	xmlhttp.open( style, url, async ) ;

	xmlhttp.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded') ;
	xmlhttp.setRequestHeader('Connection', 'Close') ;
	
	xmlhttp.send( context ) ;*/
}