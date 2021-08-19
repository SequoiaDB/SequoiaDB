/******************************************************************************

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

   Source File Name = mthSActionFunc.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MTH_SACTIONFUNC_HPP_
#define MTH_SACTIONFUNC_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "mthCommon.hpp"
#include "../bson/bson.hpp"

namespace engine
{
   class _mthSAction ;
   typedef INT32 (*MTH_SACTION_BUILD)( const CHAR *,
                                       const bson::BSONElement &,
                                       _mthSAction *,
                                       bson::BSONObjBuilder & ) ;

   typedef INT32 (*MTH_SACTION_GET)( const CHAR *,
                                     const bson::BSONElement &,
                                     _mthSAction *,
                                     bson::BSONElement & ) ;

   INT32 mthIncludeBuild( const CHAR *,
                          const bson::BSONElement &,
                          _mthSAction *,
                          bson::BSONObjBuilder & ) ;

   INT32 mthIncludeGet( const CHAR *,
                        const bson::BSONElement &,
                        _mthSAction *,
                        bson::BSONElement & ) ;

   INT32 mthDefaultBuild( const CHAR *,
                          const bson::BSONElement &,
                          _mthSAction *,
                          bson::BSONObjBuilder & ) ;

   INT32 mthDefaultGet( const CHAR *,
                        const bson::BSONElement &,
                        _mthSAction *,
                        bson::BSONElement & ) ;

   INT32 mthSliceBuild( const CHAR *,
                        const bson::BSONElement &,
                        _mthSAction *,
                        bson::BSONObjBuilder & ) ;

   INT32 mthSliceGet( const CHAR *,
                      const bson::BSONElement &,
                      _mthSAction *,
                      bson::BSONElement & ) ;

   INT32 mthElemMatchBuild( const CHAR *,
                            const bson::BSONElement &,
                            _mthSAction *,
                            bson::BSONObjBuilder & ) ;

   INT32 mthElemMatchGet( const CHAR *,
                          const bson::BSONElement &,
                          _mthSAction *,
                          bson::BSONElement & ) ;

   INT32 mthElemMatchOneBuild( const CHAR *,
                               const bson::BSONElement &,
                               _mthSAction *,
                               bson::BSONObjBuilder & ) ;

   INT32 mthElemMatchOneGet( const CHAR *,
                             const bson::BSONElement &,
                             _mthSAction *,
                             bson::BSONElement & ) ;

   INT32 mthAbsBuild( const CHAR *,
                      const bson::BSONElement &,
                      _mthSAction *,
                      bson::BSONObjBuilder & ) ;

   INT32 mthAbsGet( const CHAR *,
                    const bson::BSONElement &,
                    _mthSAction *,
                    bson::BSONElement & ) ;

   INT32 mthCeilingBuild( const CHAR *,
                          const bson::BSONElement &,
                          _mthSAction *,
                          bson::BSONObjBuilder & ) ;

   INT32 mthCeilingGet( const CHAR *,
                        const bson::BSONElement &,
                        _mthSAction *,
                        bson::BSONElement & ) ;

   INT32 mthFloorBuild( const CHAR *,
                        const bson::BSONElement &,
                        _mthSAction *,
                        bson::BSONObjBuilder & ) ;

   INT32 mthFloorGet( const CHAR *,
                      const bson::BSONElement &,
                      _mthSAction *,
                      bson::BSONElement & ) ;

   INT32 mthModBuild( const CHAR *,
                      const bson::BSONElement &,
                      _mthSAction *,
                      bson::BSONObjBuilder & ) ;

   INT32 mthModGet( const CHAR *,
                    const bson::BSONElement &,
                    _mthSAction *,
                    bson::BSONElement & ) ;

   INT32 mthCastBuild( const CHAR *,
                       const bson::BSONElement &,
                       _mthSAction *,
                       bson::BSONObjBuilder & ) ;

   INT32 mthCastGet( const CHAR *,
                     const bson::BSONElement &,
                     _mthSAction *,
                     bson::BSONElement & ) ;

   INT32 mthSubStrBuild( const CHAR *,
                         const bson::BSONElement &,
                         _mthSAction *,
                         bson::BSONObjBuilder & ) ;

   INT32 mthSubStrGet( const CHAR *,
                       const bson::BSONElement &,
                       _mthSAction *,
                       bson::BSONElement & ) ;

   INT32 mthStrLenBuild( const CHAR *,
                         const bson::BSONElement &,
                         _mthSAction *,
                         bson::BSONObjBuilder & ) ;

   INT32 mthStrLenGet( const CHAR *,
                       const bson::BSONElement &,
                       _mthSAction *,
                       bson::BSONElement & ) ;

   INT32 mthLowerBuild( const CHAR *,
                        const bson::BSONElement &,
                        _mthSAction *,
                        bson::BSONObjBuilder & ) ;

   INT32 mthLowerGet( const CHAR *,
                      const bson::BSONElement &,
                      _mthSAction *,
                      bson::BSONElement & ) ;

   INT32 mthUpperBuild( const CHAR *,
                        const bson::BSONElement &,
                        _mthSAction *,
                        bson::BSONObjBuilder & ) ;

   INT32 mthUpperGet( const CHAR *,
                      const bson::BSONElement &,
                      _mthSAction *,
                      bson::BSONElement & ) ;

   INT32 mthTrimBuild( const CHAR *,
                       const bson::BSONElement &,
                       _mthSAction *,
                       bson::BSONObjBuilder & ) ;

   INT32 mthTrimGet( const CHAR *,
                     const bson::BSONElement &,
                     _mthSAction *,
                     bson::BSONElement & ) ;

   INT32 mthLTrimBuild( const CHAR *,
                        const bson::BSONElement &,
                        _mthSAction *,
                        bson::BSONObjBuilder & ) ;

   INT32 mthLTrimGet( const CHAR *,
                      const bson::BSONElement &,
                      _mthSAction *,
                      bson::BSONElement & ) ;

   INT32 mthRTrimBuild( const CHAR *,
                        const bson::BSONElement &,
                        _mthSAction *,
                        bson::BSONObjBuilder & ) ;

   INT32 mthRTrimGet( const CHAR *,
                      const bson::BSONElement &,
                      _mthSAction *,
                      bson::BSONElement & ) ;

   INT32 mthAddBuild( const CHAR *,
                      const bson::BSONElement &,
                      _mthSAction *,
                      bson::BSONObjBuilder & ) ;

   INT32 mthAddGet( const CHAR *,
                    const bson::BSONElement &,
                    _mthSAction *,
                    bson::BSONElement & ) ;

   INT32 mthSubtractBuild( const CHAR *,
                           const bson::BSONElement &,
                           _mthSAction *,
                           bson::BSONObjBuilder & ) ;

   INT32 mthSubtractGet( const CHAR *,
                         const bson::BSONElement &,
                         _mthSAction *,
                         bson::BSONElement & ) ;

   INT32 mthMultiplyBuild( const CHAR *,
                           const bson::BSONElement &,
                           _mthSAction *,
                           bson::BSONObjBuilder & ) ;

   INT32 mthMultiplyGet( const CHAR *,
                         const bson::BSONElement &,
                         _mthSAction *,
                         bson::BSONElement & ) ;

   INT32 mthDivideBuild( const CHAR *,
                         const bson::BSONElement &,
                         _mthSAction *,
                         bson::BSONObjBuilder & ) ;

   INT32 mthDivideGet( const CHAR *,
                       const bson::BSONElement &,
                       _mthSAction *,
                       bson::BSONElement & ) ;

   INT32 mthSizeBuild( const CHAR *fieldName, const bson::BSONElement &e,
                       _mthSAction *action, bson::BSONObjBuilder &builder ) ;

   INT32 mthSizeGet( const CHAR *fieldName, const bson::BSONElement &in,
                     _mthSAction *action, bson::BSONElement &out ) ;

   INT32 mthTypeBuild( const CHAR *fieldName, const bson::BSONElement &e,
                       _mthSAction *action, bson::BSONObjBuilder &builder ) ;

   INT32 mthTypeGet( const CHAR *fieldName, const bson::BSONElement &in,
                     _mthSAction *action, bson::BSONElement &out ) ;

}

#endif

