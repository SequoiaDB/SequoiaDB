#include "optGeneratorBase.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

using namespace boost::property_tree ;

// XML element

#define XML_FIELD_OPTLIST        "optlist"
#define XML_FIELD_NAME           "name"
#define XML_FIELD_LONG           "long"
#define XML_FIELD_SHORT          "short"
#define XML_FIELD_DESC           "description"
#define XML_FIELD_DEFAULT        "default"
#define XML_FIELD_CATA           "catalogdft"
#define XML_FIELD_COORD          "coorddft"
#define XML_FIELD_DATA           "datadft"
#define XML_FIELD_STAND          "standdft"
#define XML_FIELD_TYPE           "type"
#define XML_FIELD_HIDDEN         "hidden"
#define XML_FIELD_DETAIL         "detail"
#define XML_FIELD_RELOADABLE     "reloadable"
#define XML_FIELD_TYPEOFWEB      "typeofweb"
#define XML_FIELD_RELOADSTRATEGY "reloadstrategy"

#define XML_VALUE_NONE     "none"

#define PTREE_GET_VALUE( name, key, type, required, defVal ) \
{\
   if ( required )\
   {\
      name = v.second.get< type >( key ) ;\
   }\
   else\
   {\
      try\
      {\
         name = v.second.get< type >( key ) ;\
      }\
      catch( ... )\
      {\
         name = defVal ;\
      }\
   }\
}

optGeneratorBase::optGeneratorBase()
{
}

optGeneratorBase::~optGeneratorBase()
{
}

int optGeneratorBase::init()
{
   return _loadOptList( OPTLIST_FILE_PATH ) ;
}

int optGeneratorBase::_loadOptList( const char *path )
{
   int rc = 0 ;
   int i  = 0 ;
   ptree pt ;
   string configPath ;

   configPath = utilCatPath( _rootPath.c_str(), path ) ;
   configPath = utilGetRealPath2( configPath.c_str() ) ;

   try
   {
      read_xml( configPath.c_str(), pt ) ;
   }
   catch ( std::exception &e )
   {
      printLog( PD_ERROR ) << "Failed to read xml file: " << e.what() << endl ;
      rc = 1 ;
      goto error ;
   }

   try
   {
      BOOST_FOREACH( ptree::value_type &v, pt.get_child( XML_FIELD_OPTLIST ) )
      {
         OPTION_ELE optEle ;

         PTREE_GET_VALUE( optEle.nametag, XML_FIELD_NAME, string, true, "" ) ;
         PTREE_GET_VALUE( optEle.longtag, XML_FIELD_LONG, string, true, "" ) ;

         {
            string shorttag ;

            PTREE_GET_VALUE( shorttag, XML_FIELD_SHORT, string, false, "" ) ;

            optEle.shorttag = shorttag.c_str()[0] ;
         }

         for( i = 0; i < _langListSize(); ++i )
         {
            const char *lang = _getLang( i ) ;
            string key = XML_FIELD_DESC + string( "." ) + lang ;

            PTREE_GET_VALUE( optEle.desc[lang], key, string, false, "" ) ;
         }

         for( i = 0; i < _langListSize(); ++i )
         {
            const char *lang = _getLang( i ) ;
            string key = XML_FIELD_DETAIL + string( "." ) + lang ;

            PTREE_GET_VALUE( optEle.detail[lang], key, string, false, "" ) ;
         }

         for( i = 0; i < _langListSize(); ++i )
         {
            const char *lang = _getLang( i ) ;
            string key = XML_FIELD_RELOADABLE + string( "." ) + lang ;

            PTREE_GET_VALUE( optEle.reloadable[lang], key, string, false, "" ) ;
         }

         for( i = 0; i < _langListSize(); ++i )
         {
            const char *lang = _getLang( i ) ;
            string key = XML_FIELD_RELOADSTRATEGY + string( "." ) + lang ;

            PTREE_GET_VALUE( optEle.reloadstrategy[lang], key, string,
                             false, "" ) ;
         }

         PTREE_GET_VALUE( optEle.typeofweb, XML_FIELD_TYPEOFWEB, string, false, "" ) ;

         PTREE_GET_VALUE( optEle.defttag, XML_FIELD_DEFAULT, string, false, "" ) ;

         PTREE_GET_VALUE( optEle.catatag, XML_FIELD_CATA, string, false, "" ) ;

         PTREE_GET_VALUE( optEle.cordtag, XML_FIELD_COORD, string, false, "" ) ;

         PTREE_GET_VALUE( optEle.standtag, XML_FIELD_STAND, string, false, "" ) ;

         PTREE_GET_VALUE( optEle.datatag, XML_FIELD_DATA, string, false, "" ) ;

         PTREE_GET_VALUE( optEle.typetag, XML_FIELD_TYPE, string, false, "string" ) ;
         if ( XML_VALUE_NONE == optEle.typetag )
         {
            optEle.typetag = "" ;
         }

         {
            string hiddentag ;

            PTREE_GET_VALUE( hiddentag, XML_FIELD_HIDDEN, string, false, "false" ) ;
            utilStrToBool( hiddentag.c_str(), &optEle.hiddentag ) ;
         }

         _optionList.push_back( optEle ) ;
      }
   }
   catch ( std::exception &e )
   {
      printLog( PD_ERROR ) << "Failed to parse xml: " << e.what() << endl ;
      rc = 1 ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

/*
   type
      1: C-style comment
      2: python comment
*/
int optGeneratorBase::_buildStatement( int type, string &headerDesc )
{
   int rc = 0 ;
   char buff[2048] = { 0 } ;
   vector<string> statementLines ;
   string comment ;

   headerDesc = "" ;

   if ( type == 1 )
   {
      comment = "//" ;
      headerDesc += GLOBAL_LICENSE2"\n\n" ;
   }
   else if ( type == 2 )
   {
      vector<string> licenseLines ;

      comment = "#" ;

      licenseLines = stringSplit( GLOBAL_LICENSE, "\n" ) ;
      for( int i = 0; i < licenseLines.size(); ++i )
      {
         headerDesc += comment + licenseLines.at( i ) + "\n" ;
      }

      headerDesc += "\n\n" ;
   }
   else
   {
      rc = 1 ;
      goto error ;
   }

   statementLines = stringSplit( DEFAULT_FILE_STATEMENT, "\n" ) ;
   for( int i = 0; i < statementLines.size(); ++i )
   {
      headerDesc += comment + statementLines.at( i ) + "\n" ;
   }

   rc = (int)utilSnprintf( buff, 2048, headerDesc.c_str(),
                           utilGetCurrentYear(), OPTLIST_DESC_PATH ) ;
   if ( rc < 0 )
   {
      rc = 1 ;
      goto error ;
   }

   rc = 0 ;
   headerDesc = buff ;

done:
   return rc ;
error:
   goto done ;
}
