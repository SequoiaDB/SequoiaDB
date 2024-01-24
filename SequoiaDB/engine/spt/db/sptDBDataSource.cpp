#include "sptDBDataSource.hpp"

using namespace sdbclient ;
using namespace bson ;

#define SPT_DATASOURCE_NAME      "SdbDataSource"

namespace engine
{
   JS_CONSTRUCT_FUNC_DEFINE( _sptDBDataSource, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptDBDataSource, destruct )
   JS_MEMBER_FUNC_DEFINE( _sptDBDataSource, alter )
   JS_MEMBER_FUNC_DEFINE( _sptDBDataSource, toString )

   JS_BEGIN_MAPPING( _sptDBDataSource, SPT_DATASOURCE_NAME )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_ADD_MEMBER_FUNC( "alter", alter )
      JS_ADD_MEMBER_FUNC( "toString", toString )
   JS_MAPPING_END()

   _sptDBDataSource::_sptDBDataSource( _sdbDataSource *pDataSource )
   {
      _dataSource.pDataSource = pDataSource ;
   }

   _sptDBDataSource::~_sptDBDataSource()
   {
   }

   INT32 _sptDBDataSource::construct( const _sptArguments &arg,
                                      _sptReturnVal &rval,
                                      bson::BSONObj &detail )
   {
      detail = BSON( SPT_ERR <<
                     "use of new SdbDataSource() is forbidden, you should use "
                     "other functions to produce a SdbDataSource object" ) ;
      return SDB_SYS ;
   }

   INT32 _sptDBDataSource::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptDBDataSource::alter( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;
      if ( arg.argc() != 1 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Need one argument" ) ;
         goto error ;
      }

      rc = arg.getBsonobj( 0, options ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Options must be obj" ) ;
         goto error ;
      }

      rc = _dataSource.alterDataSource( options ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Failed to alter data source" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptDBDataSource::toString( const _sptArguments & arg,
                                      _sptReturnVal & rval,
                                      bson::BSONObj & detail )
   {
      rval.getReturnVal().setValue( _dataSource.getName() ) ;
      return SDB_OK ;
   }
}
