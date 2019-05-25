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

   Source File Name = pmdEntryPoint.hpp

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
#ifndef PMD_ENTRY_POINT_HPP__
#define PMD_ENTRY_POINT_HPP__

#include "oss.hpp"
#include "sdbInterface.hpp"
#include <map>
#include <string>

using namespace std ;

namespace engine
{

   class _pmdEDUCB ;
   /*
      PMD Entry point function define
   */
   typedef INT32 (*pmdEntryPointFunc)( _pmdEDUCB *cb, void *pData ) ;

   /*
      _pmdEPItem define
   */
   struct _pmdEPItem
   {
      INT32                _type ;
      pmdEntryPointFunc    _pFunc ;

      BOOLEAN              _isSystem ;
      string               _name ;

      _pmdEPItem()
      {
         _type = -1 ;
         _pFunc = NULL ;
         _isSystem = FALSE ;
      }

      BOOLEAN isSystem() const
      {
         return _isSystem ;
      }
      BOOLEAN isPoolable() const
      {
         return !_isSystem ;
      }
   } ;
   typedef _pmdEPItem pmdEPItem ;

   /*
      _pmdEPFactory define
   */
   class _pmdEPFactory : public SDBObject
   {
      friend class _pmdEPAssit ;

      typedef std::map< INT32, pmdEPItem >      MAP_ITEM ;
      typedef MAP_ITEM::iterator                MAP_ITEM_IT ;
      typedef MAP_ITEM::const_iterator          MAP_ITEM_CIT ;

      public:
         _pmdEPFactory() ;
         ~_pmdEPFactory() ;

      public:

         const pmdEPItem*     getItem( INT32 type ) const ;
         const pmdEPItem*     getItemByName( const CHAR *pName ) const ;

         BOOLEAN              isSystem( INT32 type ) const ;
         BOOLEAN              isSystem( const CHAR *pName ) const ;
         const CHAR*          type2Name( INT32 type ) const ;
         INT32                name2Type( const CHAR *pName ) const ;
         pmdEntryPointFunc    getEntry( INT32 type ) const ;
         pmdEntryPointFunc    getEntry( const CHAR *pName ) const ;

      protected:
         INT32                regItem( INT32 type,
                                       BOOLEAN isSystem,
                                       pmdEntryPointFunc pFunc,
                                       const CHAR *pName ) ;

      private:
         MAP_ITEM                _mapItems ;

   } ;
   typedef _pmdEPFactory pmdEPFactory ;

   /*
      Global Function
   */
   pmdEPFactory& pmdGetEPFactory() ;
   const CHAR* getEDUName( INT32 type ) ;
   BOOLEAN     isSystemEDU( INT32 type ) ;

   /*
      _pmdEPAssit
   */
   class _pmdEPAssit
   {
      public:
         _pmdEPAssit( INT32 type,
                      BOOLEAN isSystem,
                      pmdEntryPointFunc pFunc,
                      const CHAR *pName ) ;
         ~_pmdEPAssit() ;
   } ;
   typedef _pmdEPAssit pmdEPAssit ;

   #define _PMD_MAKE_JOIN( A, B )      A##B
   #define _PMD_MAKE_JOIN2( A, B )     _PMD_MAKE_JOIN( A, B )
   #define _PMD_UNIQUE_NAME( A )       _PMD_MAKE_JOIN2( A, __LINE__ )

   #define PMD_DEFINE_ENTRYPOINT( type, isSystem, func, name ) \
      pmdEPAssit _a_##type##_( type, isSystem, func, name )

}

#endif //PMD_ENTRY_POINT_HPP__

