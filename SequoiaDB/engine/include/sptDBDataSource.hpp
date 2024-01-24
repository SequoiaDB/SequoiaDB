#ifndef SPT_DB_DATASOURCE_HPP__
#define SPT_DB_DATASOURCE_HPP__

#include "client.hpp"
#include "sptApi.hpp"

using sdbclient::_sdbDataSource ;
using sdbclient::sdbDataSource ;

namespace engine
{
   #define SPT_DS_NAME_FIELD  "_name"
   class _sptDBDataSource : public SDBObject
   {
      JS_DECLARE_CLASS( _sptDBDataSource ) ;
   public:
      _sptDBDataSource( _sdbDataSource *pDataSource = NULL ) ;
      ~_sptDBDataSource() ;

   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;

      INT32 destruct() ;

      INT32 alter( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      INT32 toString( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

   private:
      sdbDataSource _dataSource ;
   } ;
   typedef _sptDBDataSource sptDBDataSource ;
}

#endif /* SPT_DB_DATASOURCE_HPP__ */
