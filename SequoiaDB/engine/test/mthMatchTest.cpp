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
*******************************************************************************/

#include "core.hpp"
#include "mthMatcher.hpp"
#include "../bson/bson.h"
#include "../util/fromjson.hpp"
using namespace bson;
using namespace engine;
#define COMMENT_SYMBOL '#'
#define BUFFERSIZE 1023
char patternBuffer[BUFFERSIZE+1];
char compareBuffer[BUFFERSIZE+1];
void printHelp(char *pName)
{
   printf("Syntax: %s -p patternFile -c compareFile\n", pName) ;
}
int main (int argc, char** argv)
{
   FILE *pPatternFile = NULL ;
   FILE *pCompareFile = NULL ;
   if ( argc != 5 )
   {
      printHelp((char*)argv[0]) ;
      return 0 ;
   }
   for ( int i=1; i<argc; i++ )
   {
      if ( strcmp((char*)argv[i], "-p" ) == 0 )
      {
         pPatternFile = fopen((char*)argv[++i], "r");
         if (!pPatternFile)
         {
            printf("Failed to open file %s\n", (char*)argv[i]);
            return 0 ;
         }
      }
      else if ( strcmp((char*)argv[i], "-c" ) == 0 )
      {
         pCompareFile = fopen((char*)argv[++i], "r");
         if (!pCompareFile)
         {
            printf("Failed to open file %s\n", (char*)argv[i]);
            return 0 ;
         }
      }
      else
      {
         printHelp((char*)argv[0]) ;
         return  0 ;
      }
   }
   if ( !pPatternFile  || !pCompareFile )
   {
      printf("Not able to open both patternFile and compareFile\n") ;
      printHelp((char*)argv[0]);
      return 0 ;
   }

   int patternCount = 0 ;
   bool patternLoaded = true ;
   while ( NULL != fgets(patternBuffer, BUFFERSIZE, pPatternFile) )
   {
      if ( patternLoaded )
      {
         printf("Test Case # %d\n", patternCount) ;
         printf("---------------------------------------------------\n") ;
         patternCount ++ ;
         patternLoaded = false ;
      }
      char *p = &patternBuffer[0] ;
      // skip all space and tab
      while ( *p != 0 )
      {
         if ( *p == ' ' || *p == '\t' )
            p++ ;
         else
            break ;
      }
      // empty line?
      if ( *p == 0 || *p == '\n')
         continue ;
      // if this is comment?
      if ( *p == COMMENT_SYMBOL )
      {
         printf("\t%s",p) ;
         continue ;
      }

      if( p[strlen(p)-1] == '\n' )
         p[strlen(p)-1]=0 ;

      // create pattern object
      printf("\tOriginal: %s\n", p) ;
      BSONObj patternObj ;
      if ( SDB_OK != fromjson ( p, patternObj ) )
      {
         printf("\tError: failed to create pattern: %s\n", p);
         patternLoaded = true ;
         continue ;
      }
      patternLoaded = true ;
      if ( patternObj.isEmpty() )
      {
         printf("\tError: failed to create pattern: %s\n", p);
         continue ;
      }
      // print pattern object
      printf("\tParse To: %s\n", patternObj.toString().c_str());
      mthMatcher matcher ;
      if ( 0!=matcher.loadPattern(patternObj))
      {
         printf("\tError: failed to load pattern\n");
         continue ;
      }

      // load compares
      int compareCount = 0 ;
      bool compareLoaded = true ;
      fseek(pCompareFile,0,SEEK_SET);
      while ( NULL != fgets(compareBuffer, BUFFERSIZE, pCompareFile) )
      {
         if ( compareLoaded )
         {
            printf("\tMatcher # %d\n", compareCount) ;
            printf("\t>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n") ;
            compareCount ++ ;
            compareLoaded = false ;
         }
         char *q = &compareBuffer[0] ;
         // skip all space and tab
         while ( *q != 0 )
         {
            if ( *q == ' ' || *q == '\t' )
               q++ ;
            else
               break ;
         }
         // empty line?
         if ( *q == 0 || *q == '\n')
            continue ;
         // if this is comment?
         if ( *q == COMMENT_SYMBOL )
         {
            printf("\t\t%s",q) ;
            continue ;
         }

         if( q[strlen(q)-1] == '\n' )
            q[strlen(q)-1]=0 ;
         printf("\t\tOriginal: %s\n", q) ;
         BSONObj compareObj ;
         if ( SDB_OK != fromjson ( q, compareObj ) )
         {
            printf("\t\tError: failed to create pattern: %s\n", q);
            compareLoaded = true ;
            continue ;
         }
         compareLoaded = true ;
         if ( compareObj.isEmpty() )
         {
            printf("\t\tError: failed to create compare: %s\n", p);
            continue ;
         }
         // print compare object
         printf("\t\tParse To: %s\n", compareObj.toString().c_str());
         BOOLEAN result ;
         if ( 0 !=matcher.matches(compareObj, result))
         {
            printf("\t\tError: failed to load compare\n");
            continue;
         }
         printf("\t\tResult: %s\n", result?"Success":"Failed");
         printf("\n");
      }
      printf("\n");
   }
   fclose(pPatternFile);
   fclose(pCompareFile);
}
