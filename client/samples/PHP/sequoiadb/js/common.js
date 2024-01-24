
//把不需要显示的输入框隐藏
function showform(num) {
   var g = document.getElementById( "table1" );
   g.style.display = "block";
   document.getElementById( "space" ).style.display = "none";
   document.getElementById( "collection" ).style.display = "none";
   document.getElementById( "condition" ).style.display = "none";
   document.getElementById( "obj" ).style.display = "none";
   document.getElementById( "selected" ).style.display = "none";
   document.getElementById( "orderBy" ).style.display = "none";
   document.getElementById( "hint" ).style.display = "none";
   document.getElementById( "numToSkip" ).style.display = "none";
   document.getElementById( "numToReturn" ).style.display = "none";
   document.getElementById( "rule" ).style.display = "none";
   document.getElementById( "indexDef" ).style.display = "none";
   document.getElementById( "index_pName" ).style.display = "none";
   document.getElementById( "index_isUnique" ).style.display = "none";
   document.getElementById( "listType" ).style.display = "none";
   document.getElementById( "pName" ).style.display = "none";
   
   switch(num)
   {
      case 1:
         document.getElementById( "space" ).style.display = "block";
         document.getElementById( "work1" ).value = "create_coll_space";
         break;
      case 2:
         document.getElementById( "space" ).style.display = "block";
         document.getElementById( "work1" ).value = "del_coll_space";
         break;
      case 3:
         document.getElementById( "work1" ).value = "list_coll_space";
         break;
      case 4:
         document.getElementById( "space" ).style.display = "block";
         document.getElementById( "collection" ).style.display = "block";
         document.getElementById( "work1" ).value = "create_coll";
         break;
      case 5:
         document.getElementById( "space" ).style.display = "block";
         document.getElementById( "collection" ).style.display = "block";
         document.getElementById( "work1" ).value = "dele_coll";
         break;
      case 6:
         document.getElementById( "work1" ).value = "list_coll";
         break;
      case 7:
         document.getElementById( "space" ).style.display = "block";
         document.getElementById( "collection" ).style.display = "block";
         document.getElementById( "condition" ).style.display = "block";
         document.getElementById( "selected" ).style.display = "block";
         document.getElementById( "orderBy" ).style.display = "block";
         document.getElementById( "hint" ).style.display = "block";
         document.getElementById( "numToSkip" ).style.display = "block";
         document.getElementById( "numToReturn" ).style.display = "block";
         document.getElementById( "work1" ).value = "query";
         break;
      case 8:
         document.getElementById( "space" ).style.display = "block";
         document.getElementById( "collection" ).style.display = "block";
         document.getElementById( "obj" ).style.display = "block";
         document.getElementById( "work1" ).value = "insert";
         break;
      case 9:
         document.getElementById( "space" ).style.display = "block";
         document.getElementById( "collection" ).style.display = "block";
         document.getElementById( "condition" ).style.display = "block";
         document.getElementById( "hint" ).style.display = "block";
         document.getElementById( "rule" ).style.display = "block";
         document.getElementById( "work1" ).value = "update";
         break;
      case 10:
         document.getElementById( "space" ).style.display = "block";
         document.getElementById( "collection" ).style.display = "block";
         document.getElementById( "condition" ).style.display = "block";
         document.getElementById( "hint" ).style.display = "block";
         document.getElementById( "work1" ).value = "delete";
         break;
      case 11:
         document.getElementById( "space" ).style.display = "block";
         document.getElementById( "collection" ).style.display = "block";
         document.getElementById( "condition" ).style.display = "block";
         document.getElementById( "work1" ).value = "getcount";
         break;
      case 12:
         document.getElementById( "space" ).style.display = "block";
         document.getElementById( "collection" ).style.display = "block";
         document.getElementById( "indexDef" ).style.display = "block";
         document.getElementById( "index_pName" ).style.display = "block";
         document.getElementById( "index_isUnique" ).style.display = "block";
         document.getElementById( "work1" ).value = "create_index";
         break;
      case 13:
         document.getElementById( "space" ).style.display = "block";
         document.getElementById( "collection" ).style.display = "block";
         document.getElementById( "index_pName" ).style.display = "block";
         document.getElementById( "work1" ).value = "del_index";
         break;
      case 14:
         document.getElementById( "space" ).style.display = "block";
         document.getElementById( "collection" ).style.display = "block";
         document.getElementById( "condition" ).style.display = "block";
         document.getElementById( "selected" ).style.display = "block";
         document.getElementById( "orderBy" ).style.display = "block";
         document.getElementById( "hint" ).style.display = "block";
         document.getElementById( "numToSkip" ).style.display = "block";
         document.getElementById( "numToReturn" ).style.display = "block";
         document.getElementById( "work1" ).value = "query_current";
         break;
      case 15:
         document.getElementById( "space" ).style.display = "block";
         document.getElementById( "collection" ).style.display = "block";
         document.getElementById( "rule" ).style.display = "block";
         document.getElementById( "work1" ).value = "update_current";
         break;
      case 16:
         document.getElementById( "space" ).style.display = "block";
         document.getElementById( "collection" ).style.display = "block";
         document.getElementById( "work1" ).value = "del_current";
         break;
      case 17:
         document.getElementById( "listType" ).style.display = "block";
         document.getElementById( "condition" ).style.display = "block";
         document.getElementById( "selected" ).style.display = "block";
         document.getElementById( "orderBy" ).style.display = "block";
         document.getElementById( "work1" ).value = "getSnapshot";
         break;
      case 18:
         document.getElementById( "work1" ).value = "resetSnapshot";
         break;
      case 19:
         document.getElementById( "listType" ).style.display = "block";
         document.getElementById( "condition" ).style.display = "block";
         document.getElementById( "selected" ).style.display = "block";
         document.getElementById( "orderBy" ).style.display = "block";
         document.getElementById( "work1" ).value = "getList";
         break;
      case 20:
         document.getElementById( "space" ).style.display = "block";
         document.getElementById( "collection" ).style.display = "block";
         document.getElementById( "pName" ).style.display = "block";
         document.getElementById( "work1" ).value = "collectionRename";
         break;
      case 21:
         document.getElementById( "space" ).style.display = "block";
         document.getElementById( "collection" ).style.display = "block";
         document.getElementById( "pName" ).style.display = "block";
         document.getElementById( "work1" ).value = "getIndex";
         break;
   }
   document.getElementById( "output" ).innerHTML = ""; //清空输出的内容
}

//创建发送对象
function createXMLHttpObject()
{
   if ( window.XMLHttpRequest )
   {
      return new XMLHttpRequest();
   }
   else
   {
      var MSXML = [ 'MSXML2.XMLHTTP.5.0', 'MSXML2.XMLHTTP.4.0', 'MSXML2.XMLHTTP.3.0', 'MSXML2.XMLHTTP', 'Microsoft.XMLHTTP' ];
      for ( var n = 0; n < MSXML.length; n++ )
      {
         try
         {
            return new ActiveXObject ( MSXML[ n ] ) ;
         }
         catch ( e )
         {
         }
      }
   }
}

function sendMsg()
{
   var xmlHttp = createXMLHttpObject ();
   if ( xmlHttp )
   {
      var g = document.getElementById( "table1" );
      g.style.display = "none";    //隐藏输入框，只显示要输出的内容
      var a1 = document.getElementById( "addr1" ).value;
      var a2 = document.getElementById( "space1" ).value;
      var a3 = document.getElementById( "collection1" ).value;
      var a4 = document.getElementById( "condition1" ).value;
      var a5 = document.getElementById( "selected1" ).value;
      var a6 = document.getElementById( "orderBy1" ).value;
      var a7 = document.getElementById( "hint1" ).value;
      var a8 = document.getElementById( "numToSkip1" ).value;
      var a9 = document.getElementById( "numToReturn1" ).value;
      var a10 = document.getElementById( "rule1" ).value;
      var a11 = document.getElementById( "indexDef1" ).value;
      var a12 = document.getElementById( "index_pName1" ).value;
      var a13 = document.getElementById( "index_isUnique1" ).value;
      var a14 = document.getElementById( "obj1" ).value;
      var a15 = document.getElementById( "listType1" ).value;
      var a16 = document.getElementById( "pName1" ).value;
      var work = document.getElementById( "work1" ).value;
      xmlHttp.open('POST','server.php',true); //异步发送
      xmlHttp.setRequestHeader('Content-Type','application/x-www-form-urlencoded') ;
      var sendData = 'pName=' + a16 + '&listType=' + a15 + '&addr=' + a1 + "&work=" + work + "&space=" + a2 + "&collectname=" + a3 + "&condition=" + a4 + "&selected=" + a5 + "&orderBy=" + a6 + "&hint=" + a7 + "&numToSkip=" + a8 + "&numToReturn=" + a9 + "&rule=" + a10 + "&indexDef=" + a11 + "&indexpName=" + a12 + "&isUnique=" + a13 + "&obj=" + a14 ;
      xmlHttp.send ( sendData ) ;
      xmlHttp.onreadystatechange=function()
         {
            if ( xmlHttp.readyState == 4 )
            {
               if ( xmlHttp.status == 200 )
               {
                  //正确返回
                  document.getElementById( "output" ).innerHTML = xmlHttp.responseText;
               }
               else
               {
                  //出错
                  document.getElementById( "output" ).innerHTML = "出错 " + xmlHttp.statusText;
               }
            }
            else
            {
               //正在提交
               document.getElementById( "output" ).innerHTML = "正在提交数据...";
            }
         }
   }
   else
   {
      //浏览器不支持  
      document.getElementById( "output" ).innerHTML = "浏览器不支持";
   }
}



document.getElementsByClassName = function(cl) {
   var retnode = [] ;
   var myclass = new RegExp('\\b' + cl + '\\b');
   var elem = this.getElementsByTagName('*');
   for ( var j = 0; j < elem.length; j++ )
   {
      var classes = elem[j].className;
      if( myclass.test(classes) )
      {
         retnode.push(elem[j]);
      }
   }
   return retnode;
}

function HideAll () {
   var items = document.getElementsByClassName("optiton");
   for ( var j = 0; j < items.length; j++ )
   {
      items[j].style.display = "none";
   }
}

function setCookie ( sName, sValue, expireHours ) {
   var cookieString = sName + "=" + escape(sValue);
   if ( expireHours > 0 ) {
      var date = new Date();
      date.setTime(date.getTime + expireHours * 3600 * 1000);
      cookieString = cookieString + "; expire=" + date.toGMTString();
   }
   document.cookie = cookieString ;
}

function getCookie(sName) {
   var aCookie = document.cookie.split("; ");
   for ( var j = 0; j < aCookie.length; j++ )
   {
      var aCrumb = aCookie[j].split("=");
      if ( escape(sName) == aCrumb[0] ){
         return unescape(aCrumb[1]);
      }
   }
   return null;
}

window.onload = function () {
   var show_item = "opt_1";
   if ( getCookie("show_item") != null ) {
      show_item = "opt_" + getCookie("show_item");
   }
   
   document.getElementById(show_item).style.display = "block";
   var items = document.getElementsByClassName("title");
   for ( var j = 0; j < items.length; j++ )
   {
      items[j].onclick = function() {
         var o = document.getElementById( "opt_" + this.name );
         if(o.style.display != "block") {
            HideAll();
            o.style.display = "block";
            ("show_item",this.name);
         }
         else {
            o.style.display = "none";
         }
      }
   }
}