/*******************************************************************************

   Copyright (C) 2012-2014 SequoiaDB Ltd.

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
/*
@description: start om plugins
@modify list:
   2018-02-05 JiaWen He  Init
*/

var PD_LOGGER = new Logger( "startPlugins.js" ) ;

var rootPath = "../plugins" ;

function _throwError( rc, e, msg, isThrow )
{
   if( rc == SDB_OK )
   {
      rc = SDB_SYS ;
      error = new SdbError( rc, e.message ) ;
   }
   else
   {
      error = new SdbError( rc, msg ) ;
   }
   PD_LOGGER.log( PDERROR, error ) ;

   if( isThrow !== false )
   {
      throw error ;
   }
}

function _getPluginList()
{
   var plugins = [] ;
   var cursor = File.list( { "pathname": rootPath } ) ;

   while( record = cursor.next() )
   {
      var fileInfo = record.toObj() ;
      var fileName = fileInfo[FIELD_NAME2] ;

      if( File.isDir( rootPath + "/" + fileName ) )
      {
         plugins.push( fileName ) ;
      }
   }
   return plugins ;
}

function _stopPlugin( name, path )
{
   var rc = false ;
   var stopFile = path + "/stop.sh" ;

   try
   {
      rc = File.isFile( stopFile ) ;
   }
   catch( e )
   {
      _throwError( getLastError(), e,
                   sprintf( "plugin starter not found, name: ?", name ) ) ;
   }

   var c = new Cmd() ;

   try
   {
      c.run( stopFile ) ;
   }
   catch( e )
   {
      _throwError( c.getLastRet(), e, c.getLastOut() ) ;
   }

   var result = c.getLastOut() ;

   if( result.length > 0 )
   {
      PD_LOGGER.log( PDEVENT,
                     sprintf( "Plugin stop success, name: ?, result: ?",
                              name, result ) ) ;
   }
}

function _runPlugin( name, path )
{
   var rc = false ;
   var startFile = path + "/start.sh" ;

   try
   {
      rc = File.isFile( startFile ) ;
   }
   catch( e )
   {
      _throwError( getLastError(), e,
                   sprintf( "plugin starter not found, name: ?", name ) ) ;
   }

   var c = new Cmd() ;

   try
   {
      c.run( startFile ) ;
   }
   catch( e )
   {
      _throwError( c.getLastRet(), e, c.getLastOut() ) ;
   }

   var result = c.getLastOut() ;

   if( result.length > 0 )
   {
      PD_LOGGER.log( PDEVENT,
                     sprintf( "Plugin startup success, name: ?, result: ?",
                              name, result ) ) ;
   }
}

function _setPermissions( path )
{
   var c = new Cmd() ;
   var file = path + "/*.sh" ;

   try
   {
      c.run( "chmod u+x " + file ) ;
   }
   catch( e )
   {
   }
}

function _startPlugins( isChange )
{
   var pluginList = [] ;

   try
   {
      pluginList = _getPluginList() ;
   }
   catch( e )
   {
      _throwError( getLastError(), e,
                   sprintf( "Failed to get plugin list, path: ?", rootPath ) ) ;
   }

   if( pluginList.length == 0 )
   {
      PD_LOGGER.log( PDEVENT, "plugin not found" ) ;
      return;
   }

   for( var index in pluginList )
   {
      var pluginName = pluginList[index] ;
      var pluginPath = rootPath + "/" + pluginName + "/bin" ;

      _setPermissions( pluginPath ) ;

      if( isChange )
      {
         try
         {
            _stopPlugin( pluginName, pluginPath ) ;
         }
         catch( e )
         {
            _throwError( e.getErrCode(), e,
                         sprintf( "Failed to stop plugin, name: ?",
                                   pluginName ),
                         false ) ;
         }
      }

      try
      {
         _runPlugin( pluginName, pluginPath ) ;
      }
      catch( e )
      {
         _throwError( e.getErrCode(), e,
                      sprintf( "Failed to start plugin, name: ?",
                                pluginName ),
                      false ) ;
      }
   }
}

function _getOmConfig()
{
   var httpName = "8000" ;
   var cursor = Sdbtool.listNodes( { "type": "om", "expand": true } ) ;

   while( record = cursor.next() )
   {
      var omInfo = record.toObj() ;
      httpName = omInfo[FIELD_HTTP_NAME] ;
      break ;
   }

   return httpName ;
}

function _readPluginPublicConfig()
{
   var configContext = "" ;

   try
   {
      var f = new File( rootPath + "/plugin.conf", 0700, SDB_FILE_READONLY ) ;
      configContext = f.read() ;
      f.close() ;
   }
   catch( e )
   {
   }
   return configContext ;
}

function _writePluginPublicConfig()
{
   var configStr = _readPluginPublicConfig() ;

   try
   {
      var omhttpname = _getOmConfig() ;
      var context = "omhttpname = " + omhttpname ;
      var isChange = configStr !== context ;

      if( isChange )
      {
         var f = new File( rootPath + "/plugin.conf", 0700,
                           SDB_FILE_REPLACE | SDB_FILE_WRITEONLY ) ;
         f.write( context ) ;
         f.close() ;

         PD_LOGGER.log( PDEVENT, "Success to generate plugins config file" ) ;
      }

      return isChange ;
   }
   catch( e )
   {
      _throwError( getLastError(), e,
                   "Failed to generating plugins config file" ) ;
   }

   return true ;
}

function main()
{
   var result = {} ;

   PD_LOGGER.log( PDEVENT, "Begin to start plugins" ) ;

   var isChange = _writePluginPublicConfig() ;

   if( isChange )
   {
      PD_LOGGER.log( PDEVENT, "Config changes, restart the plugin" ) ;
   }
   else
   {
      PD_LOGGER.log( PDEVENT, "Config has not changed, startup plugin" ) ;
   }

   _startPlugins( isChange ) ;

   PD_LOGGER.log( PDEVENT, "finish start plugins" ) ;

   return result ;
}

main() ;
 