/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = coordCommon.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/05/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORD_COMMON_HPP__
#define COORD_COMMON_HPP__

#include "coordDef.hpp"
#include "rtnQueryOptions.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{

   /*
      Common define
   */
   #define COORD_SUBCL_NAME_DFT_LEN          ( 40 )

   /*
      Common functions
   */
   INT32    coordInitCataPtrFromObj( const BSONObj &obj,
                                     CoordCataInfoPtr &cataPtr ) ;

   BOOLEAN  coordIsCataAddrSame( const CoordVecNodeInfo &left,
                                 const CoordVecNodeInfo &right ) ;

   BOOLEAN  coordCataCheckFlag( INT32 flag ) ;

   BOOLEAN  coordCheckNodeReplyFlag( INT32 flag ) ;

   /*
      Coord parse mask define
   */
   #define  COORD_PARSE_MASK_ET_DFT          0x00000001
   #define  COORD_PARSE_MASK_IN_DFT          0x00000002
   #define  COORD_PARSE_MASK_ET_OPR          0x00000004
   #define  COORD_PARSE_MASK_IN_OPR          0x00000008

   #define  COORD_PARSE_MASK_ET              ( COORD_PARSE_MASK_ET_DFT|\
                                               COORD_PARSE_MASK_ET_OPR )
   #define  COORD_PARSE_MASK_ALL             0xFFFFFFFF

   INT32    coordParseBoolean( BSONElement &e, BOOLEAN &value, UINT32 mask ) ;
   INT32    coordParseInt( BSONElement &e, INT32 &value, UINT32 mask ) ;
   INT32    coordParseInt( BSONElement &e,
                           vector<INT32> &vecValue,
                           UINT32 mask ) ;
   INT32    coordParseString( BSONElement &e,
                              const CHAR *&value,
                              UINT32 mask ) ;
   INT32    coordParseString( BSONElement &e,
                              vector<const CHAR*> &vecValue,
                              UINT32 mask ) ;

   /*
      FILTER_BSON_ID define
   */
   enum FILTER_BSON_ID
   {
      FILTER_ID_MATCHER    = 1,
      FILTER_ID_SELECTOR,
      FILTER_ID_ORDERBY,
      FILTER_ID_HINT
   } ;

   /*
      NODE_SEL_STY define
   */
   enum NODE_SEL_STY
   {
      NODE_SEL_ALL         = 1,
      NODE_SEL_PRIMARY,
      NODE_SEL_SECONDARY,
      NODE_SEL_ANY
   } ;

   #define COORD_CTRL_MASK_GLOBAL         0x00000001
   #define COORD_CTRL_MASK_NODE_SELECT    0x00000002
   #define COORD_CTRL_MASK_ROLE           0x00000004
   #define COORD_CTRL_MASK_RAWDATA        0x00000008

   #define COORD_CTRL_MASK_ALL            0xFFFFFFFF
   /*
      _coordCtrlParam define
   */
   struct _coordCtrlParam
   {
      BOOLEAN           _isGlobal ;             // COORD_CTRL_MASK_GLOBAL
      FILTER_BSON_ID    _filterID ;
      NODE_SEL_STY      _emptyFilterSel ;       // COORD_CTRL_MASK_NODE_SELECT
      INT32             _role[ SDB_ROLE_MAX ] ; // COORD_CTRL_MASK_ROLE
      BOOLEAN           _rawData ;              // COORD_CTRL_MASK_RAWDATA

      UINT32            _parseMask ;

      /*
         Specila group and nodes
      */
      CoordGroupList    _specialGrps ;
      BOOLEAN           _useSpecialGrp ;
      SET_ROUTEID       _specialNodes ;
      BOOLEAN           _useSpecialNode ;

      _coordCtrlParam()
      {
         _isGlobal = TRUE ;
         _filterID = FILTER_ID_MATCHER ;
         _emptyFilterSel = NODE_SEL_ALL ;
         ossMemset( (void*)_role, 0, sizeof( _role ) ) ;
         _role[ SDB_ROLE_DATA ] = 1 ;
         _role[ SDB_ROLE_CATALOG ] = 1 ;
         _rawData = FALSE ;
         _parseMask = 0 ;

         _useSpecialGrp = FALSE ;
         _useSpecialNode = FALSE ;
      }

      void resetRole()
      {
         ossMemset( (void*)_role, 0, sizeof( _role ) ) ;
      }
      void setAllRole()
      {
         for ( INT32 i = 0 ; i < SDB_ROLE_MAX ; ++i )
         {
            _role[ i ] = 1 ;
         }
      }
   } ;
   typedef _coordCtrlParam coordCtrlParam ;

   BSONObj* coordGetFilterByID( FILTER_BSON_ID filterID,
                                rtnQueryOptions &queryOption ) ;

   INT32    coordParseControlParam( const BSONObj &obj,
                                    coordCtrlParam &param,
                                    UINT32 mask,
                                    BSONObj *pNewObj,
                                    BOOLEAN strictCheck ) ;

   INT32    coordParseGroupsInfo( const BSONObj &obj,
                                  vector< INT32 > &vecID,
                                  vector< const CHAR* > &vecName,
                                  BSONObj *pNewObj = NULL,
                                  BOOLEAN strictCheck = FALSE ) ;

   INT32    coordParseNodesInfo( const BSONObj &obj,
                                 vector< INT32 > &vecNodeID,
                                 vector< const CHAR* > &vecHostName,
                                 vector< const CHAR* > &vecSvcName,
                                 BSONObj *pNewObj = NULL,
                                 BOOLEAN strictCheck = FALSE ) ;

}

#endif // COORD_COMMON_HPP__

