#ifndef RTN_EXTOPR_DATA__
#define RTN_EXTOPR_DATA__

#include "ossMemPool.hpp"
#include "utilPooledObject.hpp"
#include "../bson/bson.hpp"

using bson::BSONObj ;

namespace engine
{
   typedef void *    RTN_EXTOPR_HANDLE ;

   class _rtnExtOprData : public utilPooledObject
   {
      typedef ossPoolMap<void *, BSONObj>       OPR_RECORD_MAP ;
      typedef OPR_RECORD_MAP::const_iterator    OPR_RECORD_MAP_CITR ;
   public:
      _rtnExtOprData() ;
      virtual ~_rtnExtOprData() ;

   public:
      INT32 setOrigRecord( const BSONObj &record, BOOLEAN getOwned = FALSE ) ;
      INT32 setNewRecord( const BSONObj &record, BOOLEAN getOwned = FALSE ) ;

      const BSONObj& getOrigRecord() const ;
      const BSONObj& getNewRecord() const ;

      INT32 saveOprRecord( void *key, const BSONObj &value,
                         BOOLEAN getOwned = FALSE ) ;
      BSONObj getOprRecord( void *key ) const ;

   private:
      INT32 _setRecord( BSONObj &target, const BSONObj &source,
                        BOOLEAN getOwned ) ;

   private:
      BSONObj _origRecord ;
      BSONObj _newRecord ;
      OPR_RECORD_MAP _oprRecordMap ;
   } ;
   typedef _rtnExtOprData rtnExtOprData ;
}

#endif /* RTN_EXTOPR_DATA__ */