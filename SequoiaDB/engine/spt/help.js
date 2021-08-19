/*******************************************************************************
   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*******************************************************************************/
 const help = function( val ) {
   if ( val == undefined )
   {
      println("   --Connect to database:");
      println("   var db = new Sdb()                                 - Connect to database use default host");
      println("                                                        'localhost' and default port 11810.");
      println("   var db = new Sdb('localhost',11810)                - Connect to database use specified host and port.");
      println("   var db = new Sdb('ubuntu',11810,'admin','123')     - Connect to database with username and password.");
      println("   var db = new SecureSdb()                           - Connect to database securely use default host");
      println("                                                        'localhost' and default port 11810.");
      println("   var db = new SecureSdb('localhost',11810)          - Connect to database securely use specified host and port.");
      println("   var db = new SecureSdb('ubuntu',11810,'admin','123')");
      println("                                                      - Connect to database securely with username and password.");
      println("   --Get help information:");
      println("   help(<method>)                                     - Help on specified method, e.g. help(\'createCS\').");
      println("   db.help()                                          - Help on db methods.");
      println("   db.<csname>.help()                                 - Help on collection space methods.");
      println("   db.<csname>.<clname>.help()                        - Help on collection methods.");
      println("   help('help')                                       - For more detail of help.");
      println("");
      println("   --Global functions: ");
      println("   showClass([className])                             - Show all class name or class's function name.");
      globalHelp() ;
      println("");
      println("   --General commands: ");
      println("   clear                                              - Clear the terminal screen.");
      println("   history -c                                         - Clear the history.");
      println("   quit                                               - Exit.");
      println("");
      println("   --KeyBoard Shortcuts: ");
      println("   ctrl + a , <home>                                  - Move to the beginning of the line.");
      println("   ctrl + b , <left>                                  - Move backward by one character.");
      println("   ctrl + c                                           - Cancel or quit.");
      println("   ctrl + d , <delete>                                - Delete the current character.");
      println("   ctrl + e , <end>                                   - Move to the end of line.");
      println("   ctrl + f , <right>                                 - Move forward by one character.");
      println("   ctrl + g                                           - Stop reverse search.");
      println("   ctrl + h , <backspace>                             - Delete the previous character.");
      println("   ctrl + k                                           - Delete from current to the end of line.");
      println("   ctrl + l                                           - Clear the screen.");
      println("   ctrl + m , <enter>                                 - Accept the line.");
      println("   ctrl + n , <down>                                  - Move to the next line in history");
      println("   ctrl + p , <up>                                    - Move to the previous line in history.");
      println("   ctrl + r                                           - Reverse search.");
      println("   ctrl + t                                           - Swap the current and the previous charaters.");
      println("   ctrl + u                                           - Delete the whole line.");
      println("   ctrl + w                                           - Delete the previous word.");
      println("   ctrl + <left>                                      - Move backward by one word.");
      println("   ctrl + <right>                                     - Move forward by one word.");
   }
   else
   {
      globalHelp( val ) ;
   }
}

/*
const _help = function( className, funcName, isInstance ) {
   if ( isInstance == undefined )
   {
      isInstance = false ;
   }
   if ( funcName == undefined )
   {
	  displayMethod( className, isInstance ) ;
   }
   else
   {
      displayManual( funcName, className, isInstance ) ;
   }
}

// Sdb
Sdb.help = function( func ) {
   _help( "Sdb", func, false ) ;
}

Sdb.prototype.help = function( func ) {
   _help( "Sdb", func, true ) ;
}

// SdbNode
SdbNode.help = function( func ) {
   _help( "SdbNode", func, false ) ;
}

SdbNode.prototype.help = function( func ) {
   _help( "SdbNode", func, true ) ;
}

// SdbReplicaGroup
SdbReplicaGroup.help = function( func ) {
   _help( "SdbReplicaGroup", func, false ) ;
}

SdbReplicaGroup.prototype.help = function( func ) {
   _help( "SdbReplicaGroup", func, true ) ;
}

// SdbCS
SdbCS.help = function( func ) {
   _help( "SdbCS", func, false ) ;
}

SdbCS.prototype.help = function( func ) {
   _help( "SdbCS", func, true ) ;
}

// SdbCollection
SdbCollection.help = function( func ) {
   _help( "SdbCollection", func, false ) ;
}

SdbCollection.prototype.help = function( func ) {
   _help( "SdbCollection", func, true ) ;
}

// SdbCursor
SdbCursor.help = function( func ) {
   _help( "SdbCursor", func, false ) ;
}

SdbCursor.prototype.help = function( func ) {
   _help( "SdbCursor", func, true ) ;
}

// SdbDomain
SdbDomain.help = function( func ) {
   _help( "SdbDomain", func, false ) ;
}

SdbDomain.prototype.help = function( func ) {
   _help( "SdbDomain", func, true ) ;
}
*/

// SdbQuery
SdbQuery.prototype.help = function( func ) {
	if (undefined == func) {
	   	println("db.cs.cl.find().help(<method>) help on find methods.");
		println("--methods for modifiers:");
	   	println("hint([hint])                - Enumerate the result set according to the");
	    println("                            specified index.");
	   	println("limit([num])                - Limit the maximum number of returned records.");
	   	println("remove()                    - delete the queried document set.");
	   	println("skip([num])                 - It is used to specify the first returned record");
	    println("                            in the result set.");
	   	println("sort([sort])                - Sort the result set according to a specified");
	    println("                            field.");
	   	println("update(<update>,[returnNew])");
	    println("                            - update the queried document set.");
	   	println("--methods for cursor:");
	   	println("close()                     - Close the current cursor.");
	   	println("current()                   - Return the record that current cursor points");
	    println("                            to.");
	   	println("next()                      - Return the next record of the record that");
	    println("                            current cursor points to.");
	   	println("--methods for general:");
	    println("[i]                         - Access the result set by index.");
	    println("count()                     - Ignores skip() and limit(), return the amount");
	    println("                            of records that match the query condition.");
	    println("explain([option])           - Return the access plan of the current query.");
	   	println("size()                      - Honors skip() and limit(), return the amount of");
	    println("                            records from current cursor to final cursor.");
	    println("toArray()                   - Return result set in the type of array.");
	} else {
		SdbCursor.help( func ) ;
	}

}

/*
// SdbDC
SdbDC.help = function( func ) {
   _help( "SdbDC", func, false ) ;
}
*/

SdbDC.prototype.help = function( func ) {
   // _help( "SdbDC", func, true ) ;

   // TODO: add troff file of dc, and remove the follow info.
   if ( func == undefined )
   {
      println("DC methods:") ;
      println("   dc.help(<method>)           help on specified method of data center, e.g. dc.help(\'activate\')");
	  println("   createImage( <imageCatAddr> )  --eg: dc.createImage( \'192.168.20.106:30003\' )") ;
	  println("   removeImage()") ;
	  println("   attachGroups( [groupsMapObj] ) --eg: dc.attachGroups( {Groups:[[\'a\', \'a\'], [\'b\', \'b\']]} }" ) ;
	  println("   detachGroups( [groupsMapObj] ) --eg: dc.detachGroups( {Groups:[[\'a\',\'a\'], [\'b\', \'b\']]} )" ) ;
	  println("   enableImage()") ;
      println("   disableImage()");
	  println("   activate()") ;
	  println("   deactivate()") ;
      println("   enableReadonly()") ;
	  println("   disableReadonly()") ;
	  println("   getDetail()") ;
      println("   toString()") ;
   }
}

