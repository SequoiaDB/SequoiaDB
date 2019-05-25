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

*******************************************************************************/

#include "ossTypes.hpp"
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/lexical_cast.hpp>

#include "runner.hpp"

namespace po = boost::program_options;
using namespace sdbclient;
using namespace std;

#define ADD_PARAM_OPTIONS_BEGIN( desc )\
           desc.add_options()
#define ADD_PARAM_OPTIONS_END ;

#define COMMANDS_STRING( a, b ) (string(a) +string( b)).c_str()

#define OPTION_HELP "help"
#define OPTION_INSERT "insert"
#define OPTION_UPDATE "update"
#define OPTION_QUERY "query"
#define OPTION_DELETE "delete"
#define OPTION_CS "cs"
#define OPTION_CS_NUM "cs_num"
#define OPTION_COLLECTION "collection"
#define OPTION_COLLECTION_NUM "collection_num"
#define OPTION_INDEX_SCALE "index_scale"
#define OPTION_THREAD "thread"
#define OPTION_HOST "host"
#define OPTION_PORT "port"

#define COMMANDS_OPTIONS \
        ( COMMANDS_STRING(OPTION_HELP, ",h"),                          "help" )\
        ( COMMANDS_STRING(OPTION_INSERT, ",i"), boost::program_options::value<UINT64>(), "<insert num>" )\
        ( COMMANDS_STRING(OPTION_UPDATE, ",u"), boost::program_options::value<UINT64>(), "<update num>" )\
        ( COMMANDS_STRING(OPTION_DELETE, ",d"), boost::program_options::value<UINT64>(), "<delete num>" )\
        ( COMMANDS_STRING(OPTION_QUERY, ",q"), boost::program_options::value<UINT64>(), "<query num>" )\
        ( COMMANDS_STRING(OPTION_CS, ",s"), boost::program_options::value<string>(), "<collection space prifix>" )\
        ( COMMANDS_STRING(OPTION_CS_NUM, ",n"), boost::program_options::value<UINT32>(), "<num>" )\
        ( COMMANDS_STRING(OPTION_COLLECTION, ",c"), boost::program_options::value<string>(), "<collection prifix>" )\
        ( COMMANDS_STRING(OPTION_COLLECTION_NUM, ",o"), boost::program_options::value<UINT32>(), "<num>" )\
        ( COMMANDS_STRING(OPTION_INDEX_SCALE, ",e"), boost::program_options::value<UINT32>(),\
        "<delete, update, query with index, 0 <= input <= 100>" )\
        ( COMMANDS_STRING(OPTION_THREAD, ",t"), boost::program_options::value<UINT32>(), "<thread num>" )\
        ( COMMANDS_STRING(OPTION_HOST, ",m"), boost::program_options::value<string>(), "<host>" )\
        ( COMMANDS_STRING(OPTION_PORT, ",p"), boost::program_options::value<UINT16>(), "<port>" )

INT32 init(INT32 argc, CHAR **argv, executionPlan &plan)
{
   INT32 rc = SDB_OK;
   po::options_description desc ( "Command options" ) ;
   po::variables_map vm ;
   ADD_PARAM_OPTIONS_BEGIN( desc )
      COMMANDS_OPTIONS
   ADD_PARAM_OPTIONS_END
   try
   {
      po::store ( po::parse_command_line ( argc,
                                           argv, desc), vm ) ;
      po::notify ( vm ) ;
   }
   catch ( po::unknown_option &e )
   {
      std::cerr <<  "Unknown argument: "
                << e.get_option_name () << std::endl ;
      rc = SDB_INVALIDARG;
      goto error;
   }
   catch ( po::invalid_option_value &e )
   {
      std::cerr <<  "Invalid argument: "
                << e.get_option_name () << std::endl ;
      rc = SDB_INVALIDARG;
      goto error;
   }
   catch( po::error &e )
   {
      std::cerr << e.what () << std::endl ;
      rc = SDB_INVALIDARG;
      goto error;
   }

   if (vm.count(OPTION_HELP))
   {
      cout<< desc << endl;
      rc = SDB_PMD_HELP_ONLY;
      goto done;
   }

   if (vm.count(OPTION_INSERT))
   {
      plan._insert = vm[OPTION_INSERT].as<UINT64>();
   }
   if (vm.count(OPTION_UPDATE))
   {
      plan._update = vm[OPTION_UPDATE].as<UINT64>();
   }
   if (vm.count(OPTION_DELETE))
   {
      plan._delete = vm[OPTION_DELETE].as<UINT64>();
   }
   if (vm.count(OPTION_QUERY))
   {
      plan._query = vm[OPTION_QUERY].as<UINT64>();
   }
   if (vm.count(OPTION_CS))
   {
      plan._cs = vm[OPTION_CS].as<string>();
   }
   if (vm.count(OPTION_CS_NUM))
   {
      plan._csNum = vm[OPTION_CS_NUM].as<UINT32>();
   }
   if (vm.count(OPTION_COLLECTION))
   {
      plan._collection = vm[OPTION_COLLECTION].as<string>();
   }
   if (vm.count(OPTION_COLLECTION_NUM))
   {
      plan._collectionNum = vm[OPTION_COLLECTION_NUM].as<UINT32>();
   }
   if (vm.count(OPTION_THREAD))
   {
      plan._thread = vm[OPTION_THREAD].as<UINT32>();
   }
   if (vm.count(OPTION_INDEX_SCALE))
   {
      UINT32 scale = vm[OPTION_INDEX_SCALE].as<UINT32>();
      if (100 < scale)
      {
         std::cerr << "invalid index_scale, 0 <= scale <=100" << endl;
         rc = SDB_INVALIDARG;
         goto error;
      }
      plan._scale = scale;
   }
   if ( vm.count(OPTION_HOST))
   {
      plan._host = vm[OPTION_HOST].as<string>() ;
   }
   else
   {
      plan._host = "localhost" ;
   }
   if ( vm.count(OPTION_PORT))
   {
      plan._port = vm[OPTION_PORT].as<UINT16>() ;
   }
   else
   {
      plan._port = 50000 ;
   }

done:
   return rc;
error:
   goto done;
}


INT32 main( INT32 argc, CHAR **argv )
{
   INT32 rc = SDB_OK;
   executionPlan plan;
   caseRunner r;
   rc = init(argc, argv, plan);
   if ( SDB_PMD_HELP_ONLY == rc )
   {
      rc = SDB_OK;
      goto done;
   }
   else if (SDB_OK != rc)
   {
      goto error;
   }
   else
   {
   }

   rc = r.run(plan);

done:
   return rc;
error:
   goto done;
}
