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

   Source File Name = sptHelp.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          6/4/2017    TZB  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPTHELP_HPP__
#define SPTHELP_HPP__

#include "sptClassMetaInfo.hpp"

namespace engine
{
   #define SPT_SYNOPSIS_INDENT           3
   #define SPT_BRIEF_INDENT              30
   
   class _sptHelp : public SDBObject
   {
   private:
      _sptHelp( const _sptHelp& ) ;
      _sptHelp& operator=( const _sptHelp& ) ;

   public:
      _sptHelp() ;
      ~_sptHelp() {}
      
   public:
      static _sptHelp&            getInstance() ;
      static void                 setLanguage( const string &lang ) ;
      INT32                       displayManual( const string &fuzzyFuncName,
                                                 const string &matcher,
                                                 BOOLEAN isInstance ) ;
      INT32                       displayMethod( const string &className,
                                                 BOOLEAN isInstance ) ;
      INT32                       displayGlobalMethod() ;
      
   private:
      INT32                       _displayConstructorMethod( const string &className,
                                          const vector<sptFuncMetaInfo> &input ) ;
      INT32                       _displayStaticMethod( const string &className,
                                          const vector<sptFuncMetaInfo> &input ) ;
      INT32                       _displayInstanceMethod( const string &className,
                                          const vector<sptFuncMetaInfo> &input ) ;
      INT32                       _displayMethod( const vector<sptFuncMetaInfo> &input,
                                                  INT32 synopsisIndent = SPT_SYNOPSIS_INDENT,
                                                  INT32 briefIndent = SPT_BRIEF_INDENT ) ;
      INT32                       _displayEntry( const vector<string> &synopsis, 
                                                 const string &brief,
                                                 INT32 synopsisIndent = SPT_SYNOPSIS_INDENT,
                                                 INT32 briefIndent = SPT_BRIEF_INDENT ) ;
      INT32                       _splitBrief( const string &brief,
                                               vector<string> &vec ) ;
      INT32                       _getSplitPosition( const CHAR *pos, 
                                                     INT32 lineLen, 
                                                     INT32 *offset ) ;
   private:
      static string               _lang ;
      sptClassMetaInfo            _meta ;
   } ;
   typedef class _sptHelp sptHelp ;

} // namespace
#endif // SPTHELP_HPP__