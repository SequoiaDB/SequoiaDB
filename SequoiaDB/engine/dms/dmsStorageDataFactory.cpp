#include "dmsStorageDataFactory.hpp"
#include "dmsStorageData.hpp"
#include "dmsStorageDataCapped.hpp"

namespace engine
{
   _dmsStorageDataCommon* _dmsStorageDataFactory::createProduct(
                                                DMS_STORAGE_TYPE type,
                                                const CHAR *suFileName,
                                                dmsStorageInfo *info,
                                                _IDmsEventHolder *pEventHolder )
   {
      _dmsStorageDataCommon *data = NULL ;
      switch ( type )
      {
      case DMS_STORAGE_NORMAL:
         data = SDB_OSS_NEW dmsStorageData( suFileName, info, pEventHolder ) ;
         break ;
      case DMS_STORAGE_CAPPED:
         data = SDB_OSS_NEW dmsStorageDataCapped( suFileName, info,
                                                  pEventHolder ) ;
         break ;
      default:
         data = NULL ;
         goto error ;
      }

   done:
      return data ;
   error:
      goto done ;
   }

   dmsStorageDataFactory* getDMSStorageDataFactory()
   {
      static dmsStorageDataFactory dmsDataFactory ;
      return &dmsDataFactory ;
   }

}

