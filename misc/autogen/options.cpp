#include "config.h"
#include "options.hpp"
#include <boost/program_options.hpp>
#include <boost/exception/diagnostic_information.hpp>

namespace po = boost::program_options ;

static const char *_langList[] = {
   LANG_LIST
} ;

#define LANG_LIST_SIZE (sizeof(_langList)/sizeof(const char*))

cmdOptions::cmdOptions() : _level( 2 ),
                           _force( false ),
                           _lang( LANG_EN )
{
}

cmdOptions::~cmdOptions()
{
}

int cmdOptions::parse( int argc, char *argv[] )
{
   int rc = 0 ;
   po::options_description desc( "Options" ) ;
   po::variables_map vm ;

   desc.add_options()
      ( "help,h",       "help message" )
      ( "level,v",      po::value<int>(),     "Diagnostic level,default:2,value range:[0-3]")
      ( "language,l",   po::value<string>(),  "set language, default: en")
      ( "force,f",      po::value<bool>(),    "set replace file, default: false") ;

   try
   {
      po::store( po::parse_command_line( argc, argv, desc ), vm ) ;
      po::notify( vm ) ;
   }
   catch ( boost::exception& e )
   {
      cout << "Error: " << boost::diagnostic_information(e) << endl ;
      rc = 1 ;
      goto error ;
   }

   if ( vm.count( "help" ) )
   {
      std::cout << desc << endl;
      return -1 ;
   }

   if ( vm.count( "level" ) )
   {
      _level = vm["level"].as<int>() ;

      if ( _level < 0 )
      {
         _level = 0 ;
      }
      else if ( _level > 3 )
      {
         _level = 3 ;
      }
   }

   if ( vm.count( "force" ) )
   {
      _force = vm["force"].as<bool>() ;
   }

   if ( vm.count( "language" ) )
   {
      bool isFind = false ;
      _lang = vm["language"].as<string>() ;

      for ( int i = 0; i < LANG_LIST_SIZE; ++i )
      {
         if ( _lang == _langList[i] )
         {
            isFind = true ;
            break ;
         }
      }

      if ( isFind == false )
      {
         cout << "Error: invalid language: " << _lang << endl ;
         rc = 1 ;
         goto error ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

cmdOptions *getCmdOptions()
{
   static cmdOptions opt ;
   return &opt ;
}