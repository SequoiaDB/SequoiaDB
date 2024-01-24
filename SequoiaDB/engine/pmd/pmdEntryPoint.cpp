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

   Source File Name = pmdEntryPoint.cpp

   Descriptive Name = Process MoDel Main

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for SequoiaDB,
   and all other process-initialization code.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          18/01/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdEntryPoint.hpp"
#include "pd.hpp"
#include "ossUtil.hpp"

namespace engine
{

   /*
      _pmdEPFactory implement
   */
   _pmdEPFactory::_pmdEPFactory()
   {
   }

   _pmdEPFactory::~_pmdEPFactory()
   {
      _mapItems.clear() ;
   }

   INT32 _pmdEPFactory::regItem( INT32 type,
                                 BOOLEAN isSystem,
                                 pmdEntryPointFunc pFunc,
                                 const CHAR *pName )
   {
      INT32 rc = SDB_OK ;
      pmdEPItem item ;

      if ( !pName || !pFunc )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      else if ( getItem( type ) || getItemByName( pName ) )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      item._type = type ;
      item._name = pName ;
      item._pFunc = pFunc ;
      item._isSystem = isSystem ;

      _mapItems[ type ] = item ;

   done:
      return rc ;
   error:
      goto done ;
   }

   const pmdEPItem* _pmdEPFactory::getItem( INT32 type ) const
   {
      MAP_ITEM_CIT cit = _mapItems.find( type ) ;
      if ( cit == _mapItems.end() )
      {
         return NULL ;
      }
      return &(cit->second) ;
   }

   const pmdEPItem* _pmdEPFactory::getItemByName( const CHAR *pName ) const
   {
      MAP_ITEM_CIT cit = _mapItems.begin() ;
      while( cit != _mapItems.end() )
      {
         if ( 0 == ossStrcasecmp( pName, cit->second._name.c_str() ) )
         {
            return &(cit->second) ;
         }
         ++cit ;
      }
      return NULL ;
   }

   BOOLEAN _pmdEPFactory::isSystem( INT32 type ) const
   {
      const pmdEPItem *pItem = getItem( type ) ;
      if (  pItem && pItem->_isSystem )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _pmdEPFactory::isSystem( const CHAR *pName ) const
   {
      const pmdEPItem *pItem = getItemByName( pName ) ;
      if (  pItem && pItem->_isSystem )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   const CHAR* _pmdEPFactory::type2Name( INT32 type ) const
   {
      const pmdEPItem *pItem = getItem( type ) ;
      if ( pItem )
      {
         return pItem->_name.c_str() ;
      }
      return "Unknow" ;
   }

   INT32 _pmdEPFactory::name2Type( const CHAR *pName ) const
   {
      const pmdEPItem *pItem = getItemByName( pName ) ;
      if ( pItem )
      {
         return pItem->_type ;
      }
      return -1 ;
   }

   pmdEntryPointFunc _pmdEPFactory::getEntry( INT32 type ) const
   {
      const pmdEPItem *pItem = getItem( type ) ;
      if ( pItem )
      {
         return pItem->_pFunc ;
      }
      return NULL ;
   }

   pmdEntryPointFunc _pmdEPFactory::getEntry( const CHAR *pName ) const
   {
      const pmdEPItem *pItem = getItemByName( pName ) ;
      if ( pItem )
      {
         return pItem->_pFunc ;
      }
      return NULL ;
   }

   pmdEPFactory& pmdGetEPFactory()
   {
      static pmdEPFactory s_epFactory ;
      return s_epFactory ;
   }

   const CHAR* getEDUName( INT32 type )
   {
      return pmdGetEPFactory().type2Name( type ) ;
   }

   BOOLEAN isSystemEDU( INT32 type )
   {
      return pmdGetEPFactory().isSystem( type ) ;
   }

   /*
      _pmdEPAssit
   */
   _pmdEPAssit::_pmdEPAssit( INT32 type,
                             BOOLEAN isSystem,
                             pmdEntryPointFunc pFunc,
                             const CHAR *pName )
   {
      INT32 rc = pmdGetEPFactory().regItem( type, isSystem, pFunc, pName ) ;
      SDB_ASSERT( SDB_OK == rc, "Register EntryPoint failed" ) ;
   }

   _pmdEPAssit::~_pmdEPAssit()
   {
   }

}


