#ifndef RTN_DICTCREATOR_JOB_HPP_
#define RTN_DICTCREATOR_JOB_HPP_

#include "dmsStorageUnit.hpp"
#include "rtnBackgroundJobBase.hpp"
#include "utilLZWDictionary.hpp"

namespace engine
{
   #define RTN_DEFAULT_DICT_SCAN_INTERVAL ( OSS_ONE_SEC * 5 )
   #define RTN_DICT_CREATE_COND_NOT_MATCH -1

   class _rtnDictCreatorJob : public _rtnBaseJob
   {
   public:
      _rtnDictCreatorJob ( UINT32 scanInterval ) ;
      virtual ~_rtnDictCreatorJob () ;
   public :
      virtual RTN_JOB_TYPE type () const ;
      virtual const CHAR* name() const ;
      virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
      virtual INT32 doit () ;
   private:
      INT32 _checkAndCreateDictForCL( dmsDictJob job );
      BOOLEAN _conditionMatch( dmsStorageUnit *su, UINT16 mbID ) ;
      INT32 _createDict( dmsStorageDataCommon *sd, dmsMBContext *context ) ;
      INT32 _transferDict( dmsStorageDataCommon *sd, dmsMBContext *context,
                           CHAR *dictStream, UINT32 dictSize ) ;
   private:
      utilDictCreator *_creator ;
      UINT32 _scanInterval ;
   } ;
   typedef _rtnDictCreatorJob rtnDictCreatorJob ;

   INT32 startDictCreatorJob ( EDUID *pEDUID,
                               UINT32 scanInterval = RTN_DEFAULT_DICT_SCAN_INTERVAL ) ;
}

#endif /* RTN_DICT_CREATOR_JOB_HPP_ */

