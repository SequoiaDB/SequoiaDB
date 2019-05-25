#ifndef DMSSTORAGE_DATAFACTORY_HPP__
#define DMSSTORAGE_DATAFACTORY_HPP__

#include "dmsStorageDataCommon.hpp"

namespace engine
{
   class _dmsStorageDataFactory : public SDBObject
   {
   public:
      _dmsStorageDataFactory() {}
      ~_dmsStorageDataFactory() {}

      dmsStorageDataCommon* createProduct( DMS_STORAGE_TYPE type,
                                           const CHAR *suFileName,
                                           dmsStorageInfo *info,
                                           _IDmsEventHolder *pEventHolder ) ;
   } ;
   typedef _dmsStorageDataFactory dmsStorageDataFactory ;

   dmsStorageDataFactory* getDMSStorageDataFactory() ;
}

#endif /* DMSSTORAGE_DATAFACTORY_HPP__ */

