#include "rcGeneratorBase.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

using namespace boost::property_tree ;

// xml element
#define XML_FIELD_CONSLIST    "rclist.conslist"
#define XML_FIELD_CODELIST    "rclist.codelist"
#define XML_FIELD_NAME        "name"
#define XML_FIELD_VALUE       "value"
#define XML_FIELD_DESCRIPTION "description"
#define XML_FIELD_CN          LANG_CN
#define XML_FIELD_EN          LANG_EN

// xml value
#define XML_VALUE_RESERVED_ERROR  "SDB_ERROR_RESERVED"

rcGeneratorBase::rcGeneratorBase() : _maxFieldWidth( 0 )
{
}

rcGeneratorBase::~rcGeneratorBase()
{
}

int rcGeneratorBase::init()
{
   return _loadRcList() ;
}

int rcGeneratorBase::_loadRcList()
{
   int rc = 0 ;
   ptree pt ;
   string configPath ;

   configPath = utilCatPath( _rootPath.c_str(), RC_FILE_PATH ) ;
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
      int errNum = 0 ;

      BOOST_FOREACH ( ptree::value_type &v, pt.get_child( XML_FIELD_CONSLIST ) )
      {
         pair<string, int> constant (
            v.second.get<string>( XML_FIELD_NAME ),
            v.second.get<int>( XML_FIELD_VALUE )
         ) ;

         _conslist.push_back( constant ) ;
      }

      BOOST_FOREACH( ptree::value_type &v, pt.get_child ( XML_FIELD_CODELIST ) )
      {
         RCInfo rcInfo ;
         ptree vv = v.second.get_child( XML_FIELD_DESCRIPTION ) ;

         rcInfo.name = v.second.get<string>( XML_FIELD_NAME ) ;
         rcInfo.desc_cn = vv.get<string>( XML_FIELD_CN ) ;
         rcInfo.desc_en = vv.get<string>( XML_FIELD_EN ) ;
         rcInfo.value = -( errNum + 1 ) ;

         if ( utilStrStartsWith( rcInfo.name, XML_VALUE_RESERVED_ERROR ) )
         {
            rcInfo.reserved = true ;
         }

         _rcInfoList.push_back( rcInfo ) ;

         ++errNum ;

         _maxFieldWidth = utilGetMaxInt( _maxFieldWidth, (int)rcInfo.name.length() ) ;
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
int rcGeneratorBase::_buildStatement( int type, string &headerDesc )
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
                           utilGetCurrentYear(), RC_DESC_PATH ) ;
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
