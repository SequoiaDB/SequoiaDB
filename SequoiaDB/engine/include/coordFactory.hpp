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

   Source File Name = coordFactory.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/19/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORD_FACTORY_HPP__
#define COORD_FACTORY_HPP__

#include "coordOperator.hpp"
#include <string>
#include <map>

using namespace std ;

namespace engine
{

   /*
      Common define
   */
   typedef coordOperator* (*COORD_NEW_OPERATOR)() ;

   #define COORD_DECLARE_CMD_AUTO_REGISTER() \
      public: \
         static _coordOperator *newThis ()

   #define COORD_IMPLEMENT_CMD_AUTO_REGISTER(theClass, className, isReadOnly) \
      _coordOperator *theClass::newThis () \
      { \
         return SDB_OSS_NEW theClass() ;\
      } \
      _coordCommandAssit theClass##Assit ( className, isReadOnly, \
            (COORD_NEW_OPERATOR)theClass::newThis )

   /*
      _coordFactoryItem define
   */
   struct _coordFactoryItem
   {
      BOOLEAN                 _isReadOnly ;
      COORD_NEW_OPERATOR      _pFunc ;

      _coordFactoryItem()
      {
         _isReadOnly = FALSE ;
         _pFunc = NULL ;
      }
      _coordFactoryItem( BOOLEAN isReadOnly, COORD_NEW_OPERATOR pFunc )
      {
         _isReadOnly = isReadOnly ;
         _pFunc = pFunc ;
      }
   } ;
   typedef _coordFactoryItem coordFactoryItem ;

   /*
      _coordCommandFactory define
   */
   class _coordCommandFactory : public SDBObject
   {
      typedef map< string, coordFactoryItem >      MAP_COMMAND ;
      typedef MAP_COMMAND::iterator                MAP_COMMAND_IT ;

      friend class _coordCommandAssit ;

      public:
         _coordCommandFactory() ;
         ~_coordCommandFactory() ;

      public:
         INT32          create( const CHAR *pCmdName,
                                coordOperator *&pOperator ) ;
         void           release( coordOperator *pOperator ) ;

      protected:
         INT32          _register( const CHAR *pCmdName,
                                   BOOLEAN isReadOnly,
                                   COORD_NEW_OPERATOR pFunc ) ;

      private:
         MAP_COMMAND          _mapCommand ;

   } ;
   typedef _coordCommandFactory coordCommandFactory ;

   coordCommandFactory* coordGetFactory() ;

   /*
      _coordCommandAssit define
   */
   class _coordCommandAssit
   {
      public:
         _coordCommandAssit( const CHAR *pCmdName,
                             BOOLEAN isReadOnly,
                             COORD_NEW_OPERATOR pFunc ) ;
         _coordCommandAssit( COORD_NEW_OPERATOR pFunc ) ;
         ~_coordCommandAssit() ;
   } ;
   typedef _coordCommandAssit coordCommandAssit ;

}

#endif // COORD_FACTORY_HPP__
