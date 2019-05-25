var runStatus = [ true, true, true, true ] ;
var b1 = [],
	b2 = [],
	b3 = [],
	b4 = [] ;
var oldValue = [ -1, -1, -1, -1 ] ;
var intervalID = null ;
var timeStep = 5 ;
var dataModel = "all" ;
var groupname = "" ;
var hostname  = "" ;
var svcname   = "" ;

$(document).ready(function()
{
	$('#runbutton_1').button('toggle') ;
	$('#runbutton_2').button('toggle') ;
	$('#runbutton_3').button('toggle') ;
	$('#runbutton_4').button('toggle') ;

	//getdata2drow2() ;
	//getdata2drow();
	//intervalID = setInterval( 'getdata2drow();', 1000 * 5 ) ; 
	
	var heightnum = document.documentElement.clientHeight ;
	document.getElementById("left_list").style.height = (heightnum-220) + "px" ;
	document.getElementById("right_context").style.height = (heightnum - 220) + "px" ;
	getleftlist( "pictree", "group" ) ;
})

window.onresize = function ()
{
	var heightnum = document.documentElement.clientHeight ;
	document.getElementById("left_list").style.height = (heightnum-220) + "px" ;
	document.getElementById("right_context").style.height = (heightnum - 220) + "px" ;
}

function chartToggle ( type, id )
{
	if ( type == 1 )
	{
		runStatus[id] = true ;
	}
	else if ( type == 2 )
	{
		runStatus[id] = false ;
	}
	else if ( type == 3 )
	{
		runStatus[id] = false ;
		changeChartOne ( id + 1 ) ;
	}
}

function changeChartOne( type )
{
	eval( "b" + type + "=[];oldValue[" + (type - 1) + "]=-1;" );
}

function changeChartModel ( t_model, t_groupname, t_hostname, t_svcname )
{
	document.getElementById("context").style.display = "none" ;
	document.getElementById("context2").style.display = "block" ;
	if ( intervalID != null )
	{
		clearInterval ( intervalID ) ;
	}
	var str = "当前监控：" ;
	if ( t_model == "all" )
	{
		str += "集群的性能" ;
	}
	else if ( t_model == "group" )
	{
		str += "分区组 " + t_groupname + " 的性能" ;
	}
	else if ( t_model == "node" )
	{
		str += "节点 " + t_hostname + ":" + t_svcname + " 的性能" ;
	}
	document.getElementById("right_title").innerHTML = str ;
	document.getElementById("dmonitor").style.display = "none" ;
	document.getElementById("pmonitor_1").style.display = "block" ;
	document.getElementById("pmonitor_2").style.display = "block" ;
	b1 = [],
	b2 = [],
	b3 = [],
	b4 = [] ;
	oldValue = [ -1, -1, -1, -1 ] ;
	
	dataModel = t_model ;
	groupname = t_groupname ;
	hostname  = t_hostname ;
	svcname   = t_svcname ;
	
	getdata2drow();
	
	intervalID = setInterval( 'getdata2drow();', 1000 * timeStep ) ;
}

function getdata2drow ()
{
	var jsonobj = ajax2send2( "post", "index.php?p=monitor&m=ajax_r", "model=" + dataModel + "&group=" + groupname + "&host=" + hostname + "&svc=" + svcname ) ;
	
	for ( var i = 1; i <= 4; ++i )
	{
		eval( 'if ( runStatus[' + (i-1) + '] ){b' + i + ' = convertChart ( b' + i + ', timeStep ) ;var str_' + i + ' = document.getElementById("selecter_' + i + '").value ;if ( oldValue[' + (i-1) + '] < 0 ){oldValue[' + (i-1) + '] = parseInt(jsonobj[str_' + i + ']===null?0:jsonobj[str_' + i + '])}b' + i + '.push ( [ 60, (parseInt(jsonobj[str_' + i + ']===null?0:jsonobj[str_' + i + '])-oldValue[' + (i-1) + '])/timeStep ] ) ;oldValue[' + (i-1) + '] = parseInt(jsonobj[str_' + i + ']===null?0:jsonobj[str_' + i + ']) ;drowLinePic( "myChart' + i + '", b' + i + ' );}' ) ;
	}

	/*b1 = convertChart ( b1, timeStep ) ;
	var str_1 = document.getElementById("selecter_1").value ;
	if ( oldValue[0] < 0 )
	{
		oldValue[0] = parseInt(jsonobj[str_1])
	}
	//b1.push ( [ 60, parseInt(jsonobj[str_1])-oldValue[0] ] ) ;
	b1.push ( [ 60, parseInt(jsonobj[str_1]) ] ) ;
	oldValue[0] = parseInt(jsonobj[str_1]) ;
	drowLinePic( "myChart1", b1 );*/
}

function drowLinePic( obj, b )
{
	var ctx = document.getElementById(obj) ;
	var maxvalue = 1 ;
	for ( var i = 0; i < b.length; ++i )
	{
		var temp = b[i] ;
		if ( temp[1] >= maxvalue )
		{
			var temp2 = parseInt ( temp[1] / 10 ) ;
			if ( temp2 <= 0 )
			{
				temp2 = 1 ;
			}
			maxvalue = temp[1] + temp2 ;
		}
	}
	Flotr.draw( ctx, [ b ], { yaxis : { min : 0, max : maxvalue, autoscale : true },
							  mouse : { track : true, relative : true, trackFormatter : function (o) { return parseInt(o.y); }, position : 'ne' },
							  xaxis: { min : 0, noTicks : 10, autoscale : false, showLabels : false } } ) ;
}

function changeChartModel2 ( t_model, t_groupname, t_hostname, t_svcname )
{
	document.getElementById("context").style.display = "none" ;
	document.getElementById("context2").style.display = "block" ;
	if ( intervalID != null )
	{
		clearInterval ( intervalID ) ;
	}
	var str = "当前监控：" ;
	if ( t_model == "all" )
	{
		str += "集群的数据" ;
	}
	else if ( t_model == "group" )
	{
		str += "分区组 " + t_groupname + " 的数据" ;
	}
	else if ( t_model == "node" )
	{
		str += "节点 " + t_hostname + ":" + t_svcname + " 的数据" ;
	}
	document.getElementById("right_title").innerHTML = str ;
	document.getElementById("dmonitor").style.display = "block" ;
	document.getElementById("pmonitor_1").style.display = "none" ;
	document.getElementById("pmonitor_2").style.display = "none" ;
	b1 = [],
	b2 = [],
	b3 = [],
	b4 = [] ;
	oldValue = [ -1, -1, -1, -1 ] ;
	
	dataModel = t_model ;
	groupname = t_groupname ;
	hostname  = t_hostname ;
	svcname   = t_svcname ;
	
	getdata2drow2();
}

function getdata2drow2()
{
	dataModel = convert2post( dataModel ) ;
	groupname = convert2post( groupname ) ;
	hostname = convert2post( hostname ) ;
	svcname = convert2post( svcname ) ;
	var jsonobj = ajax2send2( "post", "index.php?p=monitor&m=ajax_r", "model=" + dataModel + "&group=" + groupname + "&host=" + hostname + "&svc=" + svcname ) ;
	var d1 = [ { data : [[1,parseInt(jsonobj["TotalInsert"]===null?0:jsonobj["TotalInsert"])]], label : "Insert" },
		   	   { data : [[3,parseInt(jsonobj["TotalDelete"]===null?0:jsonobj["TotalDelete"])]], label : "Delete" },
			   { data : [[5,parseInt(jsonobj["TotalUpdate"]===null?0:jsonobj["TotalUpdate"])]], label : "Update" },
			   { data : [[7,parseInt(jsonobj["TotalRead"]===null?0:jsonobj["TotalRead"])]],   label : "Query"   },
			   { data : [[9,parseInt(jsonobj["ReplInsert"]===null?0:jsonobj["ReplInsert"])]],  label : "Insert(Replica)"  },
			   { data : [[11,parseInt(jsonobj["ReplDelete"]===null?0:jsonobj["ReplDelete"])]], label : "Delete(Replica)"  },
			   { data : [[13,parseInt(jsonobj["ReplUpdate"]===null?0:jsonobj["ReplUpdate"])]], label : "Update(Replica)"  } ]
	drowLinePic2( "dmonitor", d1 );
}

function drowLinePic2( obj, b )
{
	var labels = [ '', 'Insert', '', 'Delete', '', 'Update', '', 'Query', '', 'Insert(Replica)', '', 'Delete(Replica)', '', 'Update(Replica)', '' ] ;
	var ctx = document.getElementById(obj) ;
	Flotr.draw( ctx, b, { bars : { show : true, horizontal : false, shadowSize : 0, barWidth : 1 },
							 mouse : { track : true, relative : true, trackFormatter  : function (o) { return parseInt(o.y); }, position : 'ne' },
							 legend: { labelFormatter : function(label){ return label; }, position : 'ne', backgroundColor : '#D2E8FF' },
							 xaxis : { noTicks : 14, min : 0, max : 14, tickFormatter : function (x) { var x = parseInt(x);return labels[x]; } },
      						 yaxis : { min : 0, autoscaleMargin : 1 } } ) ;
}