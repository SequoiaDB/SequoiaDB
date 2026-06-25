/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = expOptions.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who          Description
   ====== =========== ============ =============================================
          28/07/2016  Lin Yuebang  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef EXP_OPTIONS_HPP_
#define EXP_OPTIONS_HPP_

//#include "core.hpp"
#include "oss.hpp"
#include "expCL.hpp"
#include "expHosts.hpp"
#include <boost/program_options.hpp>
#include <string>
#include <vector>

namespace exprt
{
   namespace po = boost::program_options ;
   using namespace std ;

   enum EXP_FILE_FORMAT
   {
      FORMAT_CSV = 0,
      FORMAT_JSON,

      // new format here

      FORMAT_COUNT
   } ;
   INT32 formatOfName( const string & name, EXP_FILE_FORMAT &format ) ;

   class expOptions : public SDBObject
   {
   public :
      expOptions() ;
      ~expOptions() ;
      // parse command-line, confifure-file will be parsed inside of parseCmd
      // when option '--conf' is specified in command-line
      INT32   parseCmd( INT32 argc, CHAR* argv[] ) ;
      // write options to configure-file which specified by option '--genconf'
      INT32   writeToConf( const expCLSet &clSet ) ;
      void    printHelpInfo() const ;
      BOOLEAN hasHelp() const ;
      BOOLEAN hasVersion() const ;
      BOOLEAN hasConf() const ;
      BOOLEAN hasGenConf() const ;

      inline const string &hostName()     const { return _hostName ; }
      inline const string &svcName()      const { return _svcName ; }
      inline const string &hostsString()  const { return _hostsString ; }
      inline const string &user()         const { return _user ; }
      inline const string &password()     const { return _password ; }
      inline const string &cipher()       const { return _cipherfile ; }
      inline const string &delRecord()    const { return _delRecord ; }
      inline const string &typeName()     const { return _typeName ; }
      inline const string &genConf()      const { return _genConf ; }
      inline const string &conf()         const { return _conf ; }
      //inline const string &csName()       const { return _csName ; }
      //inline const string &clName()       const { return _clName ; }
      inline const string &cscl()         const { return _cscl ; }
      inline const string &excludeCscl()  const { return _excludeCscl ; }
      inline const string &select()       const { return _select ; }
      inline const string &filter()       const { return _filter ; }
      inline const string &sort()         const { return _sort ; }
      inline const string &file()         const { return _file ; }
      inline const string &dir()          const { return _dir ; }
      inline UINT64  fileLimit()          const { return _fileLimit ; }
      inline INT64   skip()               const { return _skip; }
      inline INT64   limit()              const { return _limit; }
      inline string  delChar()            const { return _delChar ; }
      inline string  delField()           const { return _delField ; }
      inline BOOLEAN errorStop()          const { return _errorStop ; }
      inline BOOLEAN useSSL()             const { return _useSSL ; }
      inline BOOLEAN includeBinary()      const { return _includeBinary ; }
      inline BOOLEAN includeRegex()       const { return _includeRegex ; }
      inline BOOLEAN kickNull()           const { return _kickNull ; }
      inline BOOLEAN strict()             const { return _strict ; }
      inline BOOLEAN headLine()           const { return _headLine ; }
      inline BOOLEAN force()              const { return _force ; }
      inline BOOLEAN withId()             const { return _withId ; }
      inline EXP_FILE_FORMAT type()       const { return _type ; }
      inline const string &floatFmt()     const { return _floatFmt ; }
      inline BOOLEAN fullPrecision()      const { return _fullPrecision ; }
      inline BOOLEAN replace()            const { return _replace ; }

      inline const vector<string> &fieldsList() const
      {
         return _fields ;
      }

      inline const vector<Host> &hosts() const
      {
         return _hosts ;
      }

   private :
      INT32    _parseConf( const CHAR *confFileName ) ;
      BOOLEAN  _cmdHas( const CHAR *option ) const ;
      BOOLEAN  _confHas( const CHAR *option ) const ;
      BOOLEAN  _has( const CHAR *option ) const ;
      INT32    _setOptions( INT32 argc ) ;
      BOOLEAN  _checkDelimeters( string &stringDelimiter,
                                 string &fieldDelimiter,
                                 string &recordDelimiter ) ;
      INT32    _setDelOptions() ;
      INT32    _setConfOptions() ;
      INT32    _setCollectionOptions() ;
      INT32    _setFilePathOptions() ;
      template<typename T>
      T        _get( const CHAR* option ) const ;
   private :
      po::options_description _cmdDesc ;
      po::variables_map       _cmdVm ;
      BOOLEAN                 _cmdParsed ;
      po::options_description _confDesc ;
      po::variables_map       _confVm ;
      BOOLEAN                 _confParsed ;

      /* general */
      string               _hostName ;
      string               _svcName ;
      string               _hostsString ;
      vector<Host>         _hosts;
      string               _user ;
      string               _password ;
      string               _token ;
      string               _cipherfile ;
      string               _delRecord ;
      string               _typeName ;
      EXP_FILE_FORMAT      _type ;
      BOOLEAN              _withId ;
      BOOLEAN              _errorStop ;
      BOOLEAN              _useSSL ;
      UINT64               _fileLimit ;
      vector<string>       _fields ;
      string               _floatFmt ;
      BOOLEAN              _fullPrecision ;
      BOOLEAN              _replace ;

      /* single collection */
      string         _csName ;
      string         _clName ;
      string         _select ;
      string         _filter ;
      string         _sort ;
      string         _file ;
      INT64          _skip ;
      INT64          _limit ;

      /* multi collection */
      string         _cscl ;
      string         _excludeCscl ;
      string         _dir ;

      /* JSON */
      BOOLEAN        _strict ;

      /* CSV */
      string         _delChar ;
      string         _delField ;
      BOOLEAN        _headLine ;
      BOOLEAN        _includeBinary ;
      BOOLEAN        _includeRegex ;
      BOOLEAN        _force ;
      BOOLEAN        _kickNull ;
      BOOLEAN        _strictCheckDel ;

      /* conf */
      string         _conf ;
      string         _genConf ;
      BOOLEAN        _genFields ;
   } ;
}

#endif
