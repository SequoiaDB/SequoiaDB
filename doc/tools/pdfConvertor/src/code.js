window.onload = function()
{
  function htmlEncode( str )
  {
	  str = str + '' ;
	  if( str.length == 0 ) return '' ;
	  var s = str.replace( /&/g, "&amp;" ) ;
	  s = s.replace( /</g, "&lt;" ) ;
	  s = s.replace( />/g, "&gt;" ) ;
	  s = s.replace( / /g, "&nbsp;" ) ;
	  s = s.replace( /'/g, "&#39;" ) ;
	  s = s.replace( /\"/g, "&quot;" ) ;
	  return s ;
  }
  function htmlDecode( str )
  {
	var s = '' ;
	str = str + '' ;
	if( str.length ==0 ) return '' ;
	s = str.replace( /&amp;/g, "&" ) ;
	s = s.replace( /&lt;/g, "<" ) ;
	s = s.replace( /&gt;/g, ">" ) ;
	s = s.replace( /&nbsp;/g, " " ) ;
	s = s.replace( /&#39;/g, "'" ) ;
	s = s.replace( /&quot;/g, "\"" ) ;
	s = s.replace( /<br>/g, "\n" ) ;
	return s ;
  }
  var codeList = document.getElementsByTagName( 'pre' ) ;
  var len = codeList.length ;

  for( var i = 0; i < len; ++i )
  {
	 if( codeList[i] && codeList[i].parentNode )
	 {
		if( codeList[i].className != 'lang-javascript' )
		{
		   continue ;
		}
		codeList[i].style.display = 'none' ;
		var str = htmlDecode( codeList[i].innerHTML ) ;
		var strArr = str.split( String.fromCharCode( 10 ) ) ;
		var len2 = strArr.length ;
		var div = document.createElement( 'div' ) ;
		var ol = document.createElement( 'ol' ) ;
		div.className = 'code' ;
		for( var k = 0; k < len2; ++k )
		{
		   var li = document.createElement( 'li' ) ;
		   li.innerHTML = htmlEncode( strArr[k] ) ;
		   if( k % 2 === 0 )
		   {
			  li.className = 'alt' ;
		   }
		   ol.appendChild( li ) ;
		}
		div.appendChild( ol ) ;
		codeList[i].parentNode.insertBefore( div, codeList[i] ) ;
	 }
  }
}
