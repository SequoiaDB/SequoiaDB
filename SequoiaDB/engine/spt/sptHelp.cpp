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

   Source File Name = sptHelp.cpp

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
#include "ossUtil.h"
#include "sptWords.hpp"
#include "sptHelp.hpp"
#include "sptParseMandoc.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <boost/algorithm/string.hpp>

using std::cout ;
using std::endl ;

#define SPT_BRIEF_ALIGNED             2
#define SPT_BRIEF_SIZE                48

namespace engine
{

   string _sptHelp::_lang = "en" ;
   
   _sptHelp::_sptHelp()
   {      
      _meta = sptClassMetaInfo( _lang ) ;
   }

   _sptHelp& _sptHelp::getInstance()
   {
      static _sptHelp help ;
      return help ;
   }

   void _sptHelp::setLanguage( const string &lang )
   {
      _lang = lang == "cn" ? "cn" : "en" ;
   }
   
   // "fuzzyFuncName" can be "Oma/Oma::createCoord/createCoord" or something like 
   // "create" for fuzzy searching 
   INT32 _sptHelp::displayManual( const string &fuzzyFuncName,
                                  const string &matcher,
                                  BOOLEAN isInstance )
   {
#if defined ( SDB_SHELL )
      INT32 rc = SDB_OK ;
      string filePath ;
      vector<string> vec ;
      
      rc = _meta.queryFuncInfo( fuzzyFuncName, matcher, isInstance, vec ) ;
      if ( rc )
      {
         goto error ;
      }
      // check
      if ( vec.size() == 0 )
      {
         stringstream ss ;
         if ( matcher == "" )
         {
            ss << "No manual for method \"" << fuzzyFuncName.c_str() << "\"." ; 
         }
         else
         {
            if ( isInstance )
            {
               ss << "No manual for method \"" << fuzzyFuncName.c_str() 
                  << "\" in current object." ;
            }
            else
            {
               ss << "No manual for method \"" << fuzzyFuncName.c_str() 
                  << "\" in class " << matcher << "." ; 
            }  
         }
         cout << ss.str().c_str() << endl ;
         goto done ;
      }
      if ( vec.size() > 1 )
      {
         vector<string> v_tmp ;
         vector<string>::iterator it ;
         string funcName ;
         std::size_t pos = 0 ;
         // we don't want to show a method like "Oma::createOM" in Oma class
         // or in Oma instance
         for ( it = vec.begin(); it != vec.end(); it++ )
         {
            if ( matcher == "" )
            {
               if ( std::string::npos != it->find( SPT_GLOBAL_CLASS ) )
               {
                  pos = it->find(SPT_CLASS_SEPARATOR) + 
                     ossStrlen( SPT_CLASS_SEPARATOR ) ;
                  funcName = it->substr( pos ) ;               
               }
               else
               {
                  funcName = *it ;
               }
            }
            else
            {
               pos = it->find(SPT_CLASS_SEPARATOR) + 
                  ossStrlen( SPT_CLASS_SEPARATOR ) ;
               funcName = it->substr( pos ) ;
            }
            // if the fuzzyFuncName is full match with the function's name,
            // such as fuzzyFuncName is "find", and vector contains 
            // "find,findOne", let's display the manual of "find"
            if ( fuzzyFuncName == funcName )
            {
               string funcFullName = *it ;
               vec.clear() ;
               v_tmp.clear() ;
               vec.push_back( funcFullName ) ;
               break ;
            }
            else
            {
               v_tmp.push_back( funcName ) ;
            }
         }
         // try to display multi matching methods
         if ( v_tmp.size() > 1 )
         {
            if ( matcher == "" )
            {
               ossPrintf( "%d methods related to method \"%s\", please "
                          "fill in the full name: "OSS_NEWLINE,
                          (INT32)v_tmp.size(), fuzzyFuncName.c_str() );
            }
            else
            {
               if ( isInstance )
               {
                  ossPrintf( "%d methods related to method \"%s\" in current "
                             "object, please fill in the full name: "OSS_NEWLINE,
                             (INT32)v_tmp.size(), fuzzyFuncName.c_str() );
               }
               else
               {
                  ossPrintf( "%d methods related to method \"%s\" in class "
                             "%s, please fill in the full name: "OSS_NEWLINE,
                             (INT32)v_tmp.size(), fuzzyFuncName.c_str(),
                             matcher.c_str() );
               }
            }
            for ( it = v_tmp.begin(); it != v_tmp.end(); it++ )
            {
               ossPrintf( "    %s"OSS_NEWLINE, (*it).c_str() );
            }
            goto done ;
         }
      }
      // get troff file
      rc = _meta.getTroffFile( vec[0], filePath ) ;
      if ( rc )
      {
         goto error ;
      }
      // display manual
#if defined ( _WINDOWS )
      rc = sptParseMandoc::getInstance().parse( filePath.c_str() ) ;
#else
      ossResetTty();
      rc = sptParseMandoc::getInstance().parse( filePath.c_str() ) ;
      ossResetTty();
#endif
      if ( rc != SDB_OK )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
#else
   return SDB_OK ;
#endif // if defined ( SDB_SHELL )
   }


   INT32 _sptHelp::displayMethod( const string &className, 
                                  BOOLEAN isInstance )
   {
#if defined ( SDB_SHELL )
      INT32 rc = SDB_OK ;
      vector<sptFuncMetaInfo> vec ;

      if ( TRUE == isInstance )
      {
         rc = _meta.getMetaInfo( className, 
                                 SPT_FUNC_INSTANCE, vec ) ;
         if ( rc )
         {
            goto error ;
         }
         rc = _displayInstanceMethod( className, vec ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      else
      {
         // display constructor methods
         rc = _meta.getMetaInfo( className, SPT_FUNC_CONSTRUCTOR, vec ) ;
         if ( rc ) goto error ;
         if ( vec.size() > 0 )
         {
            rc = _displayConstructorMethod( className, vec ) ;
            if ( rc ) goto error ;
         }
         // display static methods
         vec.clear() ;
         rc = _meta.getMetaInfo( className, SPT_FUNC_STATIC, vec ) ;
         if ( rc ) goto error ;
         if ( vec.size() > 0 )
         {
            rc = _displayStaticMethod( className, vec ) ;
            if ( rc ) goto error ;
         }
         // display instance methods
         vec.clear() ;
         rc = _meta.getMetaInfo( className, 
                                 SPT_FUNC_INSTANCE, vec ) ;
         if ( rc ) goto error ;
         if ( vec.size() > 0 )
         {
            rc = _displayInstanceMethod( className, vec ) ;
            if ( rc ) goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
#else
   return SDB_OK ;
#endif // if defined ( SDB_SHELL )
   }

   INT32 _sptHelp::displayGlobalMethod()
   {
#if defined ( SDB_SHELL )
      INT32 rc = SDB_OK ;
      vector<sptFuncMetaInfo> vec ;
      
      rc = _meta.getMetaInfo( "Global", 
                              SPT_FUNC_CONSTRUCTOR | 
                              SPT_FUNC_STATIC |
                              SPT_FUNC_INSTANCE, vec ) ;
      if ( rc ) goto error ;
      if ( vec.size() > 0 )
      {
         rc = _displayMethod( vec, 3, 54 ) ;
         if ( rc ) goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
#else
   return SDB_OK ;
#endif // if defined ( SDB_SHELL )
   }

   INT32 _sptHelp::_displayConstructorMethod( const string &className,
                                          const vector<sptFuncMetaInfo> &input )
   {
      cout << endl ;
      cout << setw( SPT_SYNOPSIS_INDENT ) << " " 
         << "--Constructor methods for class \"" 
         << className << "\":" << endl ;
      return _displayMethod( input ) ;
   }

   INT32 _sptHelp::_displayStaticMethod( const string &className,
                                         const vector<sptFuncMetaInfo> &input )
   {
      INT32 rc = SDB_OK ;
      vector<string> vec ;
      vector<sptFuncMetaInfo>::const_iterator it ;

      cout << endl ;
      cout << setw( SPT_SYNOPSIS_INDENT ) << " " 
         << "--Static methods for class \"" 
         <<  className << "\":" << endl ;
      for ( it = input.begin(); it != input.end(); it++ )
      {
         vector<string> v = it->syntax ;
         vector<string>::iterator itr = v.begin() ;
         for( ; itr != v.end(); itr++ )
         {
            *itr = className + "." + *itr ;
         }
         rc = _displayEntry( v, it->desc ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptHelp::_displayInstanceMethod( const string &className,
                                           const vector<sptFuncMetaInfo> &input )
   {
      cout << endl ;
      cout << setw( SPT_SYNOPSIS_INDENT ) << " " 
         << "--Instance methods for class \"" 
         <<  className << "\":" << endl ;
      return _displayMethod( input ) ;
   }

   INT32 _sptHelp::_displayMethod( const vector<sptFuncMetaInfo> &input,
                                   INT32 synopsisIndent,
                                   INT32 briefIndent )
   {
      INT32 rc = SDB_OK ;
      vector<sptFuncMetaInfo>::const_iterator it ;

      for ( it = input.begin(); it != input.end(); it++ )
      {
         rc = _displayEntry( it->syntax, it->desc, 
                             synopsisIndent, briefIndent ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      
   done:
      return rc ;
   error:
      goto done ;
   }

   // i am not going to display cutline in windows system
   INT32 _sptHelp::_displayEntry( const vector<string> &synopsis, 
                                  const string &brief,
                                  INT32 synopsisIndent,
                                  INT32 briefIndent )
   {
      INT32 rc = SDB_OK ;
      INT32 synopsisLen = 0 ;
      string lastSynopsis ;
      vector<string> vec ;
      vector<string>::iterator it ;

      // check arguments
      if ( synopsis.size() < 1 )
      {
         ossPrintf( "Invalid argument, %s:%d"OSS_NEWLINE,
                     __FILE__, __LINE__ ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      // when we have several synopsises, we dispay n-1 pieces first.
      if ( synopsis.size() > 1 )
      {
         INT32 i = 0 ;
         while( i < (INT32)( synopsis.size() - 1 ) )
         {
            cout << setw( synopsisIndent ) << " " << synopsis[i] << endl ; 
            i++ ;
         }
      }
      // we are going to display the last one
      lastSynopsis = synopsis[synopsis.size() - 1] ;
      synopsisLen = synopsisIndent + lastSynopsis.length() + 1 ;
      // in this case, display like this:
      //                                 synopsis  - brief
      if ( synopsisLen < briefIndent )
      {
         rc = _splitBrief( brief, vec ) ;
         if ( rc )
         {
            goto error ;
         }
         it = vec.begin() ;
         // display the first line
         cout << setw( synopsisIndent ) << " " << lastSynopsis.c_str() 
              << setw( briefIndent - synopsisLen + 1 ) << " " << *it << endl ;
         for( it++; it != vec.end(); it++ )
         {
            cout << setw( briefIndent + SPT_BRIEF_ALIGNED ) << " " << *it << endl ;
         }
      }
      // in this case, display like this:
      //                                 synopsis
      //                                          - brief
      else
      {
         // display the first part
         cout << setw( synopsisIndent ) << " " << lastSynopsis.c_str() << endl ;
         // display the second part
         rc = _splitBrief( brief, vec ) ;
         if ( rc )
         {
            goto error ;
         }
         // display the first line
         if ( vec.size() > 0 )
         {
            cout << setw( briefIndent ) << " " << vec[0] << endl ;
         }
         // display the rest
         it = vec.begin() ;
         for ( it++; it != vec.end(); it++ )
         {
            cout << setw( briefIndent + SPT_BRIEF_ALIGNED ) << " " << *it << endl ;
         }
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sptHelp::_splitBrief( const string &brief, vector<string> &output )
   {
      INT32 rc = SDB_OK ;

      if ( "en" == _lang )
      {
         const CHAR *pb = brief.c_str() ;
         INT32 offset = -1 ;
         string str ;
         // check argument
         if ( !pb )
         {
            ossPrintf( "Invalid argument, %s:%d"OSS_NEWLINE,
                        __FILE__, __LINE__ ) ;

            rc = SDB_INVALIDARG ;
            goto error ;
         }
         while ( TRUE )
         {
            rc = _getSplitPosition( pb, SPT_BRIEF_SIZE, &offset ) ;
            if ( rc )
               goto error ;
            // if offset == 0, means we finish going through the whole brief 
            // and the whole brief has been insert into vec
            if ( 0 == offset )
            {
               str = string( pb, ossStrlen(pb) ) ;
               boost::trim_left( str ) ;
               output.push_back( str ) ;
               break ;
            }
            str = string ( pb, offset ) ;
            boost::trim_left( str ) ;
            output.push_back( str ) ;
            pb = pb + offset ;
            offset = -1 ;
         }
      }
      else
      {
         sdbSplitWords( brief, SPT_BRIEF_SIZE, output ) ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sptHelp::_getSplitPosition( const CHAR *pos, 
                                      INT32 lineLen, INT32 *offset )
   {
      INT32 rc = SDB_OK ;
      const CHAR *end = NULL ;
      const CHAR *mark = NULL ;
      INT32 left = 0 ;
      // check argument
      if ( !pos || lineLen <= 0 )
      {
         rc = SDB_INVALIDARG ;
         ossPrintf( "Invalid argument, %s:%d"OSS_NEWLINE,
                     __FILE__, __LINE__ ) ;
         goto error ;
      }
      left = ossStrlen( pos ) + 1 ;
      if ( left <= lineLen )
      {
         // means pos point to the last part of brief
         *offset = 0 ;
      }
      else
      {
         mark = end = pos + lineLen ;
         while( ( *end != ' ' ) && ( end > pos ) )
         {
            --end ;
         }
         if ( mark > end )
         {
            // we want to let ' ' in the end of a line
            ++end ;
         }
         // set which position to split a new line
         *offset = (end == pos) ? lineLen : end - pos ;
      }
   done :
      return rc ;
   error :
      goto done ;
   }

   
} // namespace
