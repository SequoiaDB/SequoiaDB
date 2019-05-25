/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = sdbConsistencyInspect.hpp

   Descriptive Name = N/A

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   data insert/update/delete. This file does NOT include index logic.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/17/2014  LZ  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CONSISTENCY_INSPECT_HPP__
#define CONSISTENCY_INSPECT_HPP__

#include <iostream>
#include "pmdOptionsMgr.hpp"
#include "dms.hpp"
#include "client.hpp"
#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>

#ifdef _DEBUG
   #define OUTPUT_FUNCTION( str, funcName, rc ) \
      std::cout << str << funcName << " rc = " << rc << std::endl
#else
   #define OUTPUT_FUNCTION( str, funcName, rc )
#endif // _DEBUG

#define CHECK_VALUE( condition, label )   \
   do                                     \
   {                                      \
      if ( condition )                    \
      {                                   \
         goto label ;                     \
      }                                   \
   }while( FALSE )

#define CI_INSPECT_ERROR         0x10001000
#define CI_INSPECT_CL_NOT_FOUND  0x10001001

#define CI_USERNAME_SIZE OSS_MAX_PATHSIZE
#define CI_PASSWD_SIZE   OSS_MAX_PATHSIZE
#define CI_BUFFER_BLOCK      1024
#define CI_HEADER_SIZE       65536
#define CI_TAIL_SIZE         65536
#define CI_GROUPNAME_SIZE    OSS_MAX_GROUPNAME_SIZE
#define CI_HOSTNAME_SIZE     255
#define CI_SERVICENAME_SIZE  63
#define CI_VIEWOPTION_SIZE   63
#define CI_CS_NAME_SIZE      DMS_COLLECTION_SPACE_NAME_SZ
#define CI_CL_NAME_SIZE      DMS_COLLECTION_NAME_SZ
#define CI_ADDRESS_SIZE      ( CI_HOSTNAME_SIZE + CI_SERVICENAME_SIZE + 1 )
#define CI_CL_FULLNAME_SIZE  ( CI_CS_NAME_SIZE + CI_CL_NAME_SIZE + 1 )
#define CI_AUTH_SIZE         ( CI_USERNAME_SIZE + CI_PASSWD_SIZE + 1 )

CHAR g_username[ CI_USERNAME_SIZE + 1 ] = { 0 } ;
CHAR g_password[ CI_PASSWD_SIZE + 1 ] = { 0 } ;

#define CI_FILE_NAME       "inspect.bin"
#define CI_TMP_FILE        "inspect.bin.tmp.%d"
#define CI_FILE_REPORT     ".report"
#define CI_START_TMP_FILE  "inspect.start.tmp"
#define CI_ACTION_INSPECT  "inspect"
#define CI_ACTION_REPORT   "report"
#define CI_VIEW_GROUP      "group"
#define CI_VIEW_CL         "collection"
#define CI_COORD_DEFVAL    "localhost:11810"

#define CI_HEADER_EYECATCHER "SDBCI"
#define HEADER_PARSE_ERROR 1
#define CI_INVALID_LOOP 0
#define CI_MAIN_VERSION 0
#define CI_SUB_VERSION 1
#define CI_ACTION_SIZE 20
#define CI_EYECATCHER_SIZE 8
/*#define CI_HEAD_PADDING_SIZE ( ( CI_HEADER_SIZE )          - \
                               ( CI_HOSTNAME_SIZE + 1 )    - \
                               ( CI_SERVICENAME_SIZE + 1)  - \
                               ( CI_GROUPNAME_SIZE + 1 )   - \
                               ( CI_CS_NAME_SIZE + 1 )     - \
                               ( CI_CL_NAME_SIZE + 1 )     - \
                               ( OSS_MAX_PATHSIZE + 1 )    - \
                               ( OSS_MAX_PATHSIZE + 1 )    - \
                               ( CI_VIEWOPTION_SIZE + 1 )  - \
                               ( CI_EYECATCHER_SIZE )      - \
                               ( CI_ACTION_SIZE ) -12 )
*/
#define TAIL_PADDING_SIZE ( CI_TAIL_SIZE - sizeof(INT32) * 2 )

#define CI_GROUP_HEADER_SIZE ( ( CI_GROUPNAME_SIZE + 1 ) + \
                                 sizeof( INT32 )         + \
                                 sizeof( UINT32 ) * 2 )
#define CI_CL_HEADER_SIZE ( ( CI_CL_FULLNAME_SIZE + 1 ) * 2 + sizeof( UINT32 ) )

#define CI_NODE_SIZE ( ( CI_HOSTNAME_SIZE + 1 )    + \
                       ( CI_SERVICENAME_SIZE + 1 ) + \
                         sizeof( INT32 ) * 3 )

#define DELETE_PTR(ptr)       \
do                            \
{                             \
   if ( NULL != ptr )         \
   {                          \
      delete ptr ;            \
      ptr = NULL ;            \
   }                          \
} while (FALSE);

#define MAX_NODE_COUNT 7

#define LSHIFT(x) ( 1 << x )
#define ALL_THE_SAME_BIT 7

template < class TNode >
class ciLinkList
{
public:
  friend ostream &operater( ostream &os, const ciLinkList< TNode > &link )
  {
     TNode *node = link.getHead() ;

     if ( NULL != node )
     {
        os << *node << std::endl ;
     }

     return os ;
  }

   ciLinkList() : _count( 0 ), _head( NULL ), _end( NULL ), _curNode( _head )
   {
   }

   ~ciLinkList()
   {
      if ( NULL != _head )
      {
         delete _head ;
         _head = NULL ;
      }
   }

   TNode *createNode()
   {
      TNode *newNode = new TNode() ;
      return newNode ;
   }

   TNode *getHead() const
   {
      return _head ;
   }

   void add( TNode *node )
   {
      if ( NULL == _head)
      {
         setHead( node ) ;
      }
      else
      {
         _end->_next = node ;
         _end = node ;
      }
      ++_count ;
   }

   TNode *next()
   {
      _curNode = _curNode->_next ;
      return _curNode ;
   }

   void resetCurrentNode()
   {
      _curNode = _head ;
   }

   UINT32 count()
   {
      return _count ;
   }

   void clear()
   {
      if ( NULL != _head )
      {
         delete _head ;
         _head = NULL ;
      }

      _end = NULL ;
      _curNode = _head ;
      _count = 0 ;
   }

private:
   void setHead( TNode *node )
   {
      _head = node ;
      _end = _head ;
      _curNode = _head ;
   }

private:
   UINT32  _count ;
   TNode  *_head ;
   TNode  *_end ;
   TNode  *_curNode ;
} ;

struct _ciHeader
{
   INT32 _mainVersion ;
   INT32 _subVersion ;
   INT32 _loop ;
   UINT64 _tailSize;
   CHAR  _eyeCatcher[ CI_EYECATCHER_SIZE ] ;
   CHAR  _action[ CI_ACTION_SIZE ] ;
   CHAR  _coordAddr[ CI_HOSTNAME_SIZE + 1 ] ;
   CHAR  _serviceName[ CI_SERVICENAME_SIZE + 1 ] ;
   CHAR  _groupName[ CI_GROUPNAME_SIZE + 1 ] ;
   CHAR  _csName[ CI_CS_NAME_SIZE + 1 ] ;
   CHAR  _clName[ CI_CL_NAME_SIZE + 1 ] ;
   CHAR  _filepath[ OSS_MAX_PATHSIZE + 1 ] ;
   CHAR  _outfile[ OSS_MAX_PATHSIZE + 1 ] ;
   CHAR  _view[ CI_VIEWOPTION_SIZE + 1 ] ;
   _ciHeader() : _mainVersion( CI_MAIN_VERSION ),
                 _subVersion( CI_SUB_VERSION ),
                 _loop( CI_INVALID_LOOP ),
                 _tailSize( 0 )
   {
      ossMemset( _action,      0, CI_ACTION_SIZE ) ;
      ossMemset( _eyeCatcher,  0, CI_EYECATCHER_SIZE ) ;
      ossMemset( _coordAddr,   0, CI_HOSTNAME_SIZE + 1 ) ;
      ossMemset( _serviceName, 0, CI_SERVICENAME_SIZE + 1 ) ;
      ossMemset( _groupName,   0, CI_GROUPNAME_SIZE + 1 ) ;
      ossMemset( _csName,      0, CI_CS_NAME_SIZE + 1 ) ;
      ossMemset( _clName,      0, CI_CL_NAME_SIZE + 1 ) ;
      ossMemset( _filepath,    0, OSS_MAX_PATHSIZE + 1 ) ;
      ossMemset( _outfile,     0, OSS_MAX_PATHSIZE + 1 ) ;
      ossMemset( _view,        0, CI_VIEWOPTION_SIZE + 1 ) ;

      ossMemcpy( _eyeCatcher, CI_HEADER_EYECATCHER, CI_EYECATCHER_SIZE ) ;
   }
} ;
typedef _ciHeader ciHeader ;

struct _ciGroupHeader
{
   INT32   _groupID ;
   UINT32  _nodeCount ;
   UINT32  _clCount ;
   CHAR    _groupName[ CI_GROUPNAME_SIZE + 1 ] ;
   _ciGroupHeader() : _groupID( 0 ),
                      _nodeCount( 0 ),
                      _clCount( 0 )
   {
      ossMemset( _groupName, 0, CI_GROUPNAME_SIZE + 1 ) ;
   }
} ;
typedef _ciGroupHeader ciGroupHeader ;

struct _ciClHeader
{
   UINT32 _recordCount ;
   CHAR   _fullname[ CI_CL_FULLNAME_SIZE + 1 ] ;
   CHAR   _mainClName[ CI_CL_FULLNAME_SIZE + 1 ] ;
   _ciClHeader() : _recordCount( 0 )
   {
      ossMemset( _fullname, 0, CI_CL_FULLNAME_SIZE + 1 ) ;
      ossMemset( _mainClName, 0, CI_CL_FULLNAME_SIZE + 1 ) ;
   }
} ;
typedef _ciClHeader ciClHeader ;

struct _ciGroup
{
   INT32     _groupID ;
   CHAR      _groupName[ CI_GROUPNAME_SIZE + 1 ] ;
   _ciGroup *_next ;
   _ciGroup() : _groupID( 0 ), _next( NULL )
   {
      ossMemset( _groupName, 0, CI_GROUPNAME_SIZE + 1 ) ;
   }

   ~_ciGroup()
   {
      while (NULL != _next)
      {
         _ciGroup* ptr = _next ;
         _next = _next->_next ;
         ptr->_next = NULL ;

         delete ptr ;
         ptr = NULL ;
      }
   }
} ;
typedef _ciGroup ciGroup ;

struct _ciNode
{
   enum 
   {
      STATE_NORMAL = 0,    // normal
      STATE_DISCONN,       // failed to connect to
      STATE_CLNOTEXIST,    // the corresponding collection does not exist
      STATE_CLFAILED,      // failed to get the corresponding collection
      STATE_CUSURFAILED,    // failed to get cusur of the collection

      STATE_COUNT
   } ;
   static const CHAR *stateDesc[STATE_COUNT] ;

   INT32           _index ;
   INT32           _nodeID ;
   INT32           _state ; 
   sdbclient::sdb *_db ;
   _ciNode        *_next ;
   CHAR            _hostname[ CI_HOSTNAME_SIZE + 1 ] ;
   CHAR            _serviceName[ CI_SERVICENAME_SIZE + 1 ] ;
   _ciNode() : _index( 0 ), _nodeID( 0 ), 
               _state( STATE_NORMAL ), _db( NULL ), _next( NULL )
   {
      ossMemset( _hostname, 0, CI_HOSTNAME_SIZE + 1 ) ;
      ossMemset( _serviceName, 0, CI_SERVICENAME_SIZE + 1 ) ;
   }

   ~_ciNode()
   {
      if ( NULL != _db )
      {
         delete _db ;
         _db = NULL ;
      }

      while (NULL != _next)
      {
         _ciNode* ptr = _next ;
         _next = _next->_next ;
         ptr->_next = NULL ;

         delete ptr ;
         ptr = NULL ;
      }
   }
} ;
typedef _ciNode ciNode ;

#define NONE_NAME "None"
struct _ciCollection
{
   _ciCollection *_next ;
   CHAR           _clName[ CI_CL_FULLNAME_SIZE + 1 ] ;
   CHAR           _mainClName[ CI_CL_FULLNAME_SIZE + 1 ] ;
   _ciCollection() : _next( NULL )
   {
      ossMemset( _clName, 0, CI_CL_FULLNAME_SIZE + 1 ) ;
      ossMemset( _mainClName, 0, CI_CL_FULLNAME_SIZE + 1 ) ;
      ossMemcpy( _mainClName, NONE_NAME, ossStrlen( NONE_NAME ) ) ;
   }
   ~_ciCollection()
   {
      while (NULL != _next)
      {
         _ciCollection* ptr = _next ;
         _next = _next->_next ;
         ptr->_next = NULL ;

         delete ptr ;
         ptr = NULL ;
      }
   }
} ;
typedef _ciCollection ciCollection ;

struct _ciRecord
{
   CHAR            _state ;
   INT32           _len ;
   _ciRecord      *_next ;
   bson::BSONObj   _bson ;
   _ciRecord() : _state( 0 ), _len( 0 ), _next( NULL )
   {}
   ~_ciRecord()
   {
      while (NULL != _next)
      {
         _ciRecord* ptr = _next ;
         _next = _next->_next ;
         ptr->_next = NULL ;

         delete ptr ;
         ptr = NULL ;
      }
   }
} ;
typedef _ciRecord ciRecord ;

struct _ciCursor
{
   INT32                 _nodeID ;
   INT32                 _index ;
   _ciCursor            *_next ;
   sdbclient::sdb       *_db ;
   sdbclient::sdbCursor *_cursor ;
   _ciCursor() : _nodeID( 0 ), _index( 0 ), _next( NULL ),
                 _db( NULL ), _cursor( NULL )
   {
   }
   ~_ciCursor()
   {
      if ( NULL != _db )
      {
         _db = NULL ;
      }

      if ( NULL != _cursor )
      {
         delete _cursor ;
         _cursor = NULL ;
      }

      while (NULL != _next)
      {
         _ciCursor* ptr = _next ;
         _next = _next->_next ;
         ptr->_next = NULL ;

         delete ptr ;
         ptr = NULL ;
      }
   }
} ;
typedef _ciCursor ciCursor ;

struct _ciBson
{
   bson::BSONObj objs[ MAX_NODE_COUNT ] ;
} ;
typedef _ciBson ciBson ;

struct _ciOffset
{
   INT64      _offset ;
   _ciOffset *_next ;

   _ciOffset() : _offset( 0 ), _next( NULL )
   {}

   ~_ciOffset()
   {
      while (NULL != _next)
      {
         _ciOffset* ptr = _next ;
         _next = _next->_next ;
         ptr->_next = NULL ;

         delete ptr ;
         ptr = NULL ;
      }
   }
};
typedef _ciOffset ciOffset ;

typedef std::vector< std::string > subCl ;
typedef std::map< std::string, subCl > mainCl ;

struct _ciTail
{
   INT32  _exitCode ;
   UINT32 _groupCount ;
   UINT32 _clCount ;
   UINT32 _diffCLCount ;
   UINT32 _mainClCount ;
   INT64  _recordCount ;
   UINT64 _timeCount ;
   mainCl _mainCls ;
   ciLinkList< ciOffset > _groupOffset ;
   _ciTail() : _exitCode( 0 ), _groupCount( 0 ), _clCount( 0 ), _diffCLCount(0),
               _mainClCount( 0 ), _recordCount( 0 ), _timeCount( 0 )
   {}
} ;
typedef _ciTail ciTail ;

struct _ciState
{
   CHAR _state ;
   void set( INT32 index )
   {
      _state |= LSHIFT( index ) ;
   }
   void reset()
   {
      _state = 0 ;
   }
   BOOLEAN hit( INT32 index )
   {
      return ( 0 != ( _state & LSHIFT( index ) ) ) ;
   }
   _ciState() : _state( 0 )
   {}
   _ciState( CHAR st ) : _state( st )
   {}
} ;
typedef _ciState ciState ;

#define CONSISTENCY_INSPECT_HELP      "help"
#define CONSISTENCY_INSPECT_VER       "version"
#define CONSISTENCY_INSPECT_ACTION    "action"
#define CONSISTENCY_INSPECT_COORD     "coord"
#define CONSISTENCY_INSPECT_LOOP      "loop"
#define CONSISTENCY_INSPECT_GROUP     "group"
#define CONSISTENCY_INSPECT_CS        "collectionspace"
#define CONSISTENCY_INSPECT_CL        "collection"
#define CONSISTENCY_INSPECT_FILE      "file"
#define CONSISTENCY_INSPECT_OUTPUT    "output"
#define CONSISTENCY_INSPECT_VIEW      "view"
#define CONSISTENCY_INSPECT_AUTH      "auth"

#define INSPECT_ADD_OPTIONS_BEGIN( desc ) desc.add_options()
#define INSPECT_ADD_OPTIONS_END ;
#define INSPECT_COMMANDS_STRING( a, b ) \
   ( std::string( a ) + std::string( b ) ).c_str()

#define INSPECT_OPTIONS \
   ( INSPECT_COMMANDS_STRING( CONSISTENCY_INSPECT_HELP, ",h" ), "show all command options" ) \
   ( INSPECT_COMMANDS_STRING( CONSISTENCY_INSPECT_VER, ",v" ), "show version of tool" ) \
   ( INSPECT_COMMANDS_STRING( CONSISTENCY_INSPECT_AUTH, ",u" ), boost::program_options::value< std::string >(), "auth, username:password, \"\":\"\" is set default" ) \
   ( INSPECT_COMMANDS_STRING( CONSISTENCY_INSPECT_ACTION, ",a" ), boost::program_options::value< std::string >(), "specify action, \"inspect\" or \"report\" supported, \"inspect\" is set default" ) \
   ( INSPECT_COMMANDS_STRING( CONSISTENCY_INSPECT_COORD, ",d" ), boost::program_options::value< std::string >(), "specify the coord address, default: \"localhost:11810\"" ) \
   ( INSPECT_COMMANDS_STRING( CONSISTENCY_INSPECT_LOOP, ",t" ), boost::program_options::value< INT32 >(), "specify times to loop" ) \
   ( INSPECT_COMMANDS_STRING( CONSISTENCY_INSPECT_GROUP, ",g" ),boost::program_options::value< std::string >(), "specify group name to be inspect" ) \
   ( INSPECT_COMMANDS_STRING( CONSISTENCY_INSPECT_CS, ",c" ), boost::program_options::value< std::string >(), "specify the collection space to be inspected") \
   ( INSPECT_COMMANDS_STRING( CONSISTENCY_INSPECT_CL, ",l" ), boost::program_options::value< std::string >(), "specify the collection to be inspected") \
   ( INSPECT_COMMANDS_STRING( CONSISTENCY_INSPECT_FILE, ",f" ), boost::program_options::value< std::string >(), "specify the file existed, when specified, other option will be ignored except options that \"-o\" and \"-f\" sepcified " ) \
   ( INSPECT_COMMANDS_STRING( CONSISTENCY_INSPECT_OUTPUT, ",o" ), boost::program_options::value< std::string >(), "specify the output file" ) \
   ( INSPECT_COMMANDS_STRING( CONSISTENCY_INSPECT_VIEW, ",w" ), boost::program_options::value< std::string >(), "specify the way to view the report, \"group\" or \"collection\" is avaliable, \"group\" is set default" )


class _sdbCi : public engine::_pmdCfgRecord
{
public:
   _sdbCi() ;
   ~_sdbCi() ;

   void displayArgs( const po::options_description &desc ) ;

public:
   INT32 init( INT32 argc, CHAR **argv,
               po::options_description &desc,
               po::variables_map &vm ) ;

   INT32 handle( const po::options_description &desc,
                 const po::variables_map &vm) ;

private:
   INT32 splitAddr() ;
   INT32 splitAuth() ;
   INT32 inspect() ;
   INT32 report ( const CHAR *inFile, const CHAR *reportFile,
                  CHAR *&tailBuffer, INT64 &tailBufferSize ) ;
   INT32 report2( const CHAR *inFile, const CHAR *reportFile,
                  CHAR *&tailBuffer, INT64 &tailBufferSize ) ;

private:
   virtual INT32 doDataExchange( engine::pmdCfgExchange *pEx ) ;
   virtual INT32 postLoaded( engine::PMD_CFG_STEP step ) ;
   virtual INT32 preSaving() ;

private:
   ciHeader _header ;
   CHAR     _coordAddr[ CI_ADDRESS_SIZE + 1 ] ;
   CHAR     _auth[ CI_AUTH_SIZE + 1 ] ;
} ;
typedef _sdbCi sdbCi ;


#ifdef _DEBUG
inline ostream &operator<< ( ostream &os, const ciHeader &header )
{
   os << header._eyeCatcher  << std::endl
      << header._mainVersion << std::endl
      << header._subVersion  << std::endl
      << header._loop        << std::endl
      << header._coordAddr   << std::endl
      << header._serviceName << std::endl
      << g_username          << std::endl
      << g_password          << std::endl
      << header._groupName   << std::endl
      << header._csName      << std::endl
      << header._clName      << std::endl
      << header._filepath    << std::endl
      << header._view        << std::endl ;

   return os ;
}

inline ostream &operator<< ( ostream &os, const ciTail &tail )
{
   os << tail._exitCode   << std::endl
      << tail._groupCount << std::endl ;

   return os ;
}

inline ostream &operator<< ( ostream &os, const ciGroupHeader &header )
{
   os << header._groupID   << std::endl
      << header._groupName << std::endl
      << header._nodeCount << std::endl ;

   return os ;
}

inline ostream &operator<< ( ostream &os, const ciClHeader &header )
{
   os << header._fullname    << std::endl
      << header._recordCount << std::endl ;

   return os ;
}

inline ostream &operator<< ( ostream &os, const ciGroup &group )
{
   os << group._groupID   << std::endl
      << group._groupName << std::endl ;

   if ( NULL != group._next )
   {
      os << *group._next  << std::endl ;
   }

   return os ;
}

inline ostream &operator<< ( ostream &os, const ciNode &node )
{
   os << node._index       << std::endl
      << node._nodeID      << std::endl
      << node._hostname    << std::endl
      << node._serviceName << std::endl ;

   if ( NULL != node._next )
   {
      os << *node._next    << std::endl ;
   }

   return os ;
}

inline ostream &operator<< ( ostream &os, const ciCollection &cl )
{
   os << cl._clName  << std::endl ;

   if ( NULL != cl._next )
   {
      os << *cl._next   << std::endl ;
   }

   return os ;
}

inline ostream &operator<< ( ostream &os, const ciCursor &cr )
{
   os << cr._index  << std::endl
      << cr._nodeID << std::endl
      << cr._db     << std::endl
      << cr._cursor << std::endl ;

   if ( NULL != cr._next )
   {
      os << *cr._next  << std::endl ;
   }

   return os ;
}

inline ostream &operator<< ( ostream &os, const ciRecord &doc )
{
   os << doc._bson.toString() << "  " ;
   for ( int idx = 0 ; idx < MAX_NODE_COUNT ; ++idx )
   {
      os << ( 0 != ( doc._state & LSHIFT( idx ) ) ) ;
   }

   if ( NULL != doc._next )
   {
      os << std::endl << *doc._next << std::endl ;
   }

   return os ;
}

inline ostream &operator<< ( ostream &os, const ciBson &obj )
{
   INT32 idx = 0 ;
   while ( idx < MAX_NODE_COUNT )
   {
      os << obj.objs[ idx ].toString() << std::endl ;
   }

   return os ;
}

inline ostream &operator<< ( ostream &os, const ciOffset &offset )
{
   os << offset._offset << std::endl ;

   if ( NULL != offset._next )
   {
      os << *offset._next  << std::endl ;
   }

   return os ;
}
#endif

#endif
