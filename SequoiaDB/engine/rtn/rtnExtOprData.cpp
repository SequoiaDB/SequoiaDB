#include "rtnExtOprData.hpp"

namespace engine
{
   _rtnExtOprData::_rtnExtOprData()
   {
   }

   _rtnExtOprData::~_rtnExtOprData()
   {
   }

   INT32 _rtnExtOprData::setOrigRecord( const BSONObj &record,
                                        BOOLEAN getOwned )
   {
      INT32 rc = SDB_OK ;

      rc = _setRecord( _origRecord, record, getOwned ) ;
      PD_RC_CHECK( rc, PDERROR, "Set original record failed[%d]", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   const BSONObj& _rtnExtOprData::getOrigRecord() const
   {
      return _origRecord ;
   }

   INT32 _rtnExtOprData::setNewRecord( const BSONObj &record, BOOLEAN getOwned )
   {
      INT32 rc = SDB_OK ;

      rc = _setRecord( _newRecord, record, getOwned ) ;
      PD_RC_CHECK( rc, PDERROR, "Set new record failed[%d]", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   const BSONObj& _rtnExtOprData::getNewRecord() const
   {
      return _newRecord ;
   }

   INT32 _rtnExtOprData::saveOprRecord( void *key, const BSONObj &record,
                                        BOOLEAN getOwned )
   {
      INT32 rc = SDB_OK ;

      try
      {
         if ( getOwned )
         {
            _oprRecordMap[key] = record.getOwned() ;
         }
         else
         {
            _oprRecordMap[key] = record ;
         }
      }
      catch ( const std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BSONObj _rtnExtOprData::getOprRecord( void *key ) const
   {
      OPR_RECORD_MAP_CITR itr = _oprRecordMap.find( key ) ;
      if ( itr != _oprRecordMap.end() )
      {
         return itr->second ;
      }
      else
      {
         return BSONObj() ;
      }
   }

   INT32 _rtnExtOprData::_setRecord( BSONObj &target, const BSONObj &source,
                                     BOOLEAN getOwned )
   {
      INT32 rc = SDB_OK ;

      try
      {
         if ( getOwned )
         {
            target = source.getOwned() ;
         }
         else
         {
            target = source ;
         }
      }
      catch ( const std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}
