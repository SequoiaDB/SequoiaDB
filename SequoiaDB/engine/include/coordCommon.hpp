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
#include "pmdEDU.hpp"

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
         _role[ SDB_ROLE_COORD ] = 1 ;
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
      void setParseRole ( INT32 ( &role )[ SDB_ROLE_MAX ] )
      {
         if ( OSS_BIT_TEST( _parseMask, COORD_CTRL_MASK_ROLE ) )
         {
            for ( INT32 i = 0 ; i < SDB_ROLE_MAX ; ++ i )
            {
               if ( _role[ i ] != role[ i ] )
               {
                  _role[ i ] = 0 ;
               }
            }
         }
         else
         {
            ossMemcpy( _role, role, sizeof( role ) ) ;
            OSS_BIT_SET( _parseMask, COORD_CTRL_MASK_ROLE ) ;
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
                                 vector< const CHAR* > &vecNodeName,
                                 vector< INT32 > & vecInstanceID,
                                 BSONObj *pNewObj = NULL,
                                 BOOLEAN strictCheck = FALSE ) ;

   void     coordFilterGroupsByRole( CoordGroupList &groupList,
                                     INT32 *pRoleFilter ) ;

   void     coordFilterNodesByRole( SET_ROUTEID &nodes,
                                    INT32 *pRoleFilter ) ;

   INT32    coordCheckNodeName( const CHAR *pNodeName ) ;
   INT32    coordCheckNodeName( const vector< const CHAR* > &vecNodeName ) ;
   INT32    coordCheckInstanceID ( const vector< INT32 > & vecInstanceID ) ;

   BOOLEAN  coordMatchNodeName( const CHAR *pNodeName,
                                const CHAR *pHostName,
                                const CHAR *pSvcName ) ;

   BOOLEAN  coordMatchNodeName( const vector< const CHAR* > &vecNodeName,
                                const CHAR *pHostName,
                                const CHAR *pSvcName ) ;

   INT32 coordInvalidateSequenceCache( CoordCataInfoPtr cataPtr,
                                       _pmdEDUCB *cb ) ;

   /*
      Coord Show Error Control
    */
   #define COORD_SHOWERROR                "ShowError"
   #define COORD_SHOWERROR_VALUE_SHOW     "show"
   #define COORD_SHOWERROR_VALUE_ONLY     "only"
   #define COORD_SHOWERROR_VALUE_IGNORE   "ignore"
   #define COORD_SHOWERRORMODE            "ShowErrorMode"
   #define COORD_SHOWERRORMODE_VALUE_AGGR "aggr"
   #define COORD_SHOWERRORMODE_VALUE_FLAT "flat"

   #define COORD_MASK_SHOWERROR_SHOW         0x00000001
   #define COORD_MASK_SHOWERROR_ONLY         0x00000002
   #define COORD_MASK_SHOWERROR_IGNORE       0x00000004
   #define COORD_MASK_SHOWERRORMODE_AGGR     0x00000008
   #define COORD_MASK_SHOWERRORMODE_FLAT     0x00000010
   #define COORD_MASK_SHOWERROR_ALL          0xFFFFFFFF

   #define COORD_MASK_SHOWERROR     ( COORD_MASK_SHOWERROR_SHOW |\
                                      COORD_MASK_SHOWERROR_ONLY |\
                                      COORD_MASK_SHOWERROR_IGNORE )

   #define COORD_MASK_SHOWERRORMODE ( COORD_MASK_SHOWERRORMODE_AGGR |\
                                      COORD_MASK_SHOWERRORMODE_FLAT )

   enum COORD_SHOWERROR_TYPE
   {
      COORD_SHOWERROR_SHOW ,
      COORD_SHOWERROR_IGNORE ,
      COORD_SHOWERROR_ONLY
   } ;

   enum COORD_SHOWERRORMODE_TYPE
   {
      COORD_SHOWERRORMODE_AGGR ,
      COORD_SHOWERRORMODE_FLAT
   } ;

   INT32 coordParseShowErrorHint ( const BSONObj & hint,
                                   UINT32 mask,
                                   COORD_SHOWERROR_TYPE & showError,
                                   COORD_SHOWERRORMODE_TYPE & showErrorMode ) ;

}

#endif // COORD_COMMON_HPP__

