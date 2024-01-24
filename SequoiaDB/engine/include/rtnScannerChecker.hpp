/*******************************************************************************


   Copyright (C) 2023-present SequoiaDB Ltd.

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

   Source File Name = rtnScannerChecker.hpp

   Descriptive Name = RunTime Scanner Checker

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/26/2022  HGM first init

   Last Changed =

*******************************************************************************/

#ifndef RTN_SCANNER_CHECKER_HPP_
#define RTN_SCANNER_CHECKER_HPP_

#include "pmdEDU.hpp"
#include "dmsScanner.hpp"

namespace engine
{

   /*
      _rtnScannerChecker define
    */
   // scanner checker to check if scanner is interrupted
   // use the place holder context to hold position for scanner
   class _rtnScannerChecker : public utilPooledObject,
                              public IDmsScannerChecker
   {
   public:
      _rtnScannerChecker( pmdEDUCB *cb ) ;
      virtual ~_rtnScannerChecker() ;

   public:
      virtual INT32 open( UINT32 suLID,
                          UINT32 mbLID,
                          const CHAR *csName,
                          const CHAR *clShortName,
                          const CHAR *optrDesc ) ;
      virtual void  release() ;
      virtual BOOLEAN needInterrupt() ;

   protected:
      INT32 _open( UINT32 suLID,
                   UINT32 mbLID,
                   const CHAR *csName,
                   const CHAR *clShortName,
                   const CHAR *optrDesc ) ;
      void _release() ;

   protected:
      pmdEDUCB * _eduCB ;
      INT64      _contextID ;
   } ;

   typedef class _rtnScannerChecker rtnScannerChecker ;

   /*
      _rtnScannerCheckerCreator define
    */
   class _rtnScannerCheckerCreator : public _IDmsScannerCheckerCreator
   {
   public:
      _rtnScannerCheckerCreator() {}
      virtual ~_rtnScannerCheckerCreator() {}

      INT32 createChecker( UINT32 suLID,
                           UINT32 mbLID,
                           const CHAR *csName,
                           const CHAR *clShortName,
                           const CHAR *optrDesc,
                           pmdEDUCB *cb,
                           IDmsScannerChecker **ppChecker ) ;
      void releaseChecker( IDmsScannerChecker *pChecker ) ;
   } ;

   typedef class _rtnScannerCheckerCreator rtnScannerCheckerCreator ;

}

#endif // RTN_SCANNER_CHECKER_HPP_
