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

   Source File Name = pmdOptionsParse.cpp

   Descriptive Name = Pmd Options Parse

   When/how to use: this program may be used on binary and text-formatted
   versions of RunTime component. This file contains structure for RunTime
   control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who        Description
   ====== =========== ========== ==========================================
          08/07/2020  FangJiabin Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdOptionsParse.hpp"
#include "msg.hpp"
#include "pmdOptions.h"

namespace engine
{
   #define PMD_MAX_LONG_STR_LEN   256

   INT32 pmdParsePreferInstStr( const CHAR *instanceStr,
                                ossPoolList< UINT8 > &instanceList,
                                PMD_PREFER_INSTANCE_TYPE &specInstance,
                                BOOLEAN &hasInvalidChar )
   {
      INT32   rc                = SDB_OK ;
      CHAR    *curPreferInstStr = NULL ;
      CHAR    *lastParsed       = NULL ;
      BOOLEAN hasParsed         = FALSE ;
      CHAR    preferInstStrCopy [ PMD_MAX_LONG_STR_LEN + 1 ] = { '\0' } ;

      if ( NULL == instanceStr )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "The preferred instance str can't be null, rc: %d",
                 rc ) ;
         goto error ;
      }

      instanceList.clear() ;

      if ( ossStrlen( instanceStr ) > PMD_MAX_LONG_STR_LEN )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "The length of preferd instance str must be less "
                 "than %d, rc: %d", PMD_MAX_LONG_STR_LEN, rc ) ;
         goto error ;
      }
      ossStrncpy( preferInstStrCopy, instanceStr, ossStrlen( instanceStr ) ) ;

      specInstance = PMD_PREFER_INSTANCE_TYPE_UNKNOWN ;
      curPreferInstStr = ossStrtok( preferInstStrCopy, ",", &lastParsed ) ;
      while ( NULL != curPreferInstStr && '\0' != curPreferInstStr[0] )
      {
         hasParsed = FALSE ;
         if ( 0 == ossStrcasecmp( curPreferInstStr,
                                  PREFER_INSTANCE_MASTER_STR ) &&
              PMD_PREFER_INSTANCE_TYPE_UNKNOWN == specInstance )
         {
            specInstance = PMD_PREFER_INSTANCE_TYPE_MASTER ;
            hasParsed = TRUE ;
         }
         else if ( 0 == ossStrcasecmp( curPreferInstStr,
                                       PREFER_INSTANCE_SLAVE_STR ) &&
                   PMD_PREFER_INSTANCE_TYPE_UNKNOWN == specInstance )
         {
            specInstance = PMD_PREFER_INSTANCE_TYPE_SLAVE ;
            hasParsed = TRUE ;
         }
         else if ( 0 == ossStrcasecmp( curPreferInstStr,
                                       PREFER_INSTANCE_ANY_STR ) &&
                   PMD_PREFER_INSTANCE_TYPE_UNKNOWN == specInstance )
         {
            specInstance = PMD_PREFER_INSTANCE_TYPE_ANYONE ;
            hasParsed = TRUE ;
         }
         else if ( 0 == ossStrcasecmp( curPreferInstStr,
                                       PREFER_INSTANCE_MASTER_SND_STR ) &&
                   PMD_PREFER_INSTANCE_TYPE_UNKNOWN == specInstance )
         {
            specInstance = PMD_PREFER_INSTANCE_TYPE_MASTER_SND ;
            hasParsed = TRUE ;
         }
         else if ( 0 == ossStrcasecmp( curPreferInstStr,
                                       PREFER_INSTANCE_SLAVE_SND_STR ) &&
                   PMD_PREFER_INSTANCE_TYPE_UNKNOWN == specInstance )
         {
            specInstance = PMD_PREFER_INSTANCE_TYPE_SLAVE_SND ;
            hasParsed = TRUE ;
         }
         else if ( 0 == ossStrcasecmp( curPreferInstStr,
                                       PREFER_INSTANCE_ANY_SND_STR ) &&
                   PMD_PREFER_INSTANCE_TYPE_UNKNOWN == specInstance )
         {
            specInstance = PMD_PREFER_INSTANCE_TYPE_ANYONE_SND ;
            hasParsed = TRUE ;
         }
         else
         {
            INT32 curPrefInstInt = ossAtoi( curPreferInstStr ) ;
            if( curPrefInstInt > NODE_INSTANCE_ID_MIN &&
                curPrefInstInt < NODE_INSTANCE_ID_MAX )
            {
               try
               {
                  instanceList.push_back( ( UINT8 )curPrefInstInt ) ;
               }
               catch( std::exception &e )
               {
                  rc = SDB_OOM ;
                  PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
                  goto error ;
               }
               hasParsed = TRUE ;
            }
            else
            {
               PD_LOG( PDWARNING, "Invalid instance id[%d]. The range of "
                       "instance id is [%d, %d]", curPrefInstInt,
                       PMD_PREFER_INSTANCE_TYPE_MIN + 1,
                       PMD_PREFER_INSTANCE_TYPE_MAX - 1 ) ;
            }
         }
         if( !hasParsed )
         {
            hasInvalidChar = TRUE ;
         }
         curPreferInstStr = ossStrtok( lastParsed, ",", &lastParsed ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 pmdParsePreferInstModeStr( const CHAR *instanceModeStr,
                                    PMD_PREFER_INSTANCE_MODE &instanceMode )
   {
      INT32 rc = SDB_OK ;

      if ( NULL == instanceModeStr )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "The preferred instance mode str can't be null, "
                 "rc: %d", rc ) ;
         goto error ;
      }

      instanceMode = PMD_PREFER_INSTANCE_MODE_UNKNOWN ;

      if ( 0 == ossStrcmp( instanceModeStr, PREFER_INSTANCE_RANDOM_STR ) )
      {
         instanceMode = PMD_PREFER_INSTANCE_MODE_RANDOM ;
      }
      else if ( 0 == ossStrcmp( instanceModeStr, PREFER_INSTANCE_ORDERED_STR ) )
      {
         instanceMode = PMD_PREFER_INSTANCE_MODE_ORDERED ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 pmdParsePreferConstraintStr( const CHAR *constraintStr,
                                      PMD_PREFER_CONSTRAINT &constraint )
   {
      INT32 rc = SDB_OK ;

      if ( NULL == constraintStr )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "The preferred constraint str can't be null, rc: %d",
                 rc ) ;
         goto error ;
      }

      constraint = PMD_PREFER_CONSTRAINT_UNKNOWN ;

      if ( '\0' == constraintStr[0] )
      {
         constraint = PMD_PREFER_CONSTRAINT_NONE ;
      }
      else if ( 0 == ossStrcasecmp( constraintStr,
                                    PREFER_CONSTRAINT_PRY_ONLY_STR ) )
      {
         constraint = PMD_PREFER_CONSTRAINT_PRY_ONLY ;
      }
      else if ( 0 == ossStrcasecmp( constraintStr,
                                    PREFER_CONSTRAINT_SND_ONLY_STR ) )
      {
         constraint = PMD_PREFER_CONSTRAINT_SND_ONLY ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* pmdPreferInstInt2String( PMD_PREFER_INSTANCE_TYPE preferInstInt )
   {
      switch ( preferInstInt )
      {
         case PMD_PREFER_INSTANCE_TYPE_MASTER :
            return PREFER_INSTANCE_MASTER_STR ;
         case PMD_PREFER_INSTANCE_TYPE_MASTER_SND :
            return PREFER_INSTANCE_MASTER_SND_STR ;
         case PMD_PREFER_INSTANCE_TYPE_SLAVE :
            return PREFER_INSTANCE_SLAVE_STR ;
         case PMD_PREFER_INSTANCE_TYPE_SLAVE_SND :
            return PREFER_INSTANCE_SLAVE_SND_STR ;
         case PMD_PREFER_INSTANCE_TYPE_ANYONE :
            return PREFER_INSTANCE_ANY_STR ;
         case PMD_PREFER_INSTANCE_TYPE_ANYONE_SND :
            return PREFER_INSTANCE_ANY_SND_STR ;
         default :
            break ;
      }
      return "Unknown" ;
   }

   const CHAR* pmdGetConfigAliasName( const CHAR* config )
   {
      if ( NULL == config )
      {
         return "" ;
      }

      // preferedinstance / preferredinstance
      if ( 0 == ossStrcmp( config, PMD_OPTION_PREFERREDINST ) )
      {
         return PMD_OPTION_PREFINST ;
      }
      else if ( 0 == ossStrcmp( config, PMD_OPTION_PREFINST ) )
      {
         return PMD_OPTION_PREFERREDINST ;
      }
      // preferedinstancemode / preferredinstancemode
      else if ( 0 == ossStrcmp( config, PMD_OPTION_PREFERREDINST_MODE ) )
      {
         return PMD_OPTION_PREFINST_MODE ;
      }
      else if ( 0 == ossStrcmp( config, PMD_OPTION_PREFINST_MODE ) )
      {
         return PMD_OPTION_PREFERREDINST_MODE ;
      }
      // preferedstrict / preferredstrict
      else if ( 0 == ossStrcmp( config, PMD_OPTION_PREFERREDINST_STRICT ) )
      {
         return PMD_OPTION_PREFINST_STRICT ;
      }
      else if ( 0 == ossStrcmp( config, PMD_OPTION_PREFINST_STRICT ) )
      {
         return PMD_OPTION_PREFERREDINST_STRICT ;
      }
      // --preferedperiod / --preferredperiod
      else if ( 0 == ossStrcmp( config, PMD_OPTION_PREFERREDINST_PERIOD ) )
      {
         return PMD_OPTION_PREFINST_PERIOD ;
      }
      else if ( 0 == ossStrcmp( config, PMD_OPTION_PREFINST_PERIOD ) )
      {
         return PMD_OPTION_PREFERREDINST_PERIOD ;
      }
      else
      {
         return "" ;
      }
   }
}

