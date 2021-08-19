/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = rtnCommandMon.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/09/2016  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_COMMAND_MON_HPP_
#define RTN_COMMAND_MON_HPP_

#include "rtnCommand.hpp"
#include "aggrBuilder.hpp"
#include "rtnFetchBase.hpp"

using namespace bson ;

namespace engine
{

   class _pmdEDUCB ;
   class _SDB_DMSCB ;
   class _SDB_RTNCB ;
   class _dpsLogWrapper ;
   class _rtnFetchBase ;

   /*
      _rtnMonInnerBase define
   */
   class _rtnMonInnerBase : public _rtnCommand, public _aggrCmdBase
   {
      protected:
         _rtnMonInnerBase () ;

         _rtnMonInnerBase ( const CHAR* name,
                            RTN_COMMAND_TYPE type,
                            INT32 fetchType,
                            UINT32 infoMask ) :
               _name( name ),
               _type( type ),
               _fetchType( fetchType ),
               _infoMask( infoMask ),
               _pDataProcessor( NULL ) {}

         virtual ~_rtnMonInnerBase () ;

      public:
         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;

         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;

         void setDataProcessor( IRtnMonProcessor *pDataProcessor )
         {
            _pDataProcessor = pDataProcessor ;
         }

      protected:
         INT32           _createFetch( _pmdEDUCB *cb,
                                       _rtnFetchBase **ppFetch ) ;

         const CHAR * name () { return _name ; }
         RTN_COMMAND_TYPE type () { return _type ; }
         INT32   _getFetchType() const { return _fetchType ; }
         UINT32  _addInfoMask() const { return _infoMask ; }
         virtual BOOLEAN _isCurrent() const = 0 ;
         virtual BOOLEAN _isDetail() const = 0 ;
         virtual BSONObj _getOptObj() const ;

      protected :
         // help functions
         BSONObj _getObjectFromHint ( const CHAR * fieldName ) const ;

      protected:
         INT64                _numToReturn ;
         INT64                _numToSkip ;
         const CHAR           *_matcherBuff ;
         const CHAR           *_selectBuff ;
         const CHAR           *_orderByBuff ;
         const CHAR           *_hintBuff ;
         const CHAR           *_name ;

         INT32                _flags ;
         RTN_COMMAND_TYPE     _type ;
         INT32                _fetchType ;
         UINT32               _infoMask ;
         IRtnMonProcessor    *_pDataProcessor ;
   } ;

   /*
      _rtnMonBase define
   */
   class _rtnMonBase : public _rtnMonInnerBase
   {
      protected:
         _rtnMonBase () ;

         _rtnMonBase(const CHAR* name,
                     const CHAR* intrName,
                     RTN_COMMAND_TYPE type,
                     INT32 fetchType,
                     UINT32 infoMask)
           : _rtnMonInnerBase( name, type, fetchType, infoMask ),
             _intrName(intrName)
         {}

         virtual ~_rtnMonBase () ;

      public:
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;

      private:
         const CHAR* _intrName ;

         virtual const CHAR *getIntrCMDName() { return _intrName ; }
   } ;

}

#endif //RTN_COMMAND_MON_HPP_

