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

   Source File Name = qgmPlMthMatcherFilter.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/29/2013  JHL  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef QGMPLMTHMATCHERFILTER_HPP__
#define QGMPLMTHMATCHERFILTER_HPP__

#include "qgmPlFilter.hpp"
#include "mthMatchTree.hpp"

namespace engine
{
   class qgmPlMthMatcherFilter : public _qgmPlFilter
   {
   public:
      qgmPlMthMatcherFilter( const qgmOPFieldVec &selector,
                           INT64 numSkip,
                           INT64 numReturn,
                           const qgmField &alias );

      INT32 loadPattern( bson::BSONObj matcher );

   private:
      virtual INT32 _fetchNext( qgmFetchOut & next );

   private:
      _mthMatchTree        _matcher;
   };
}

#endif