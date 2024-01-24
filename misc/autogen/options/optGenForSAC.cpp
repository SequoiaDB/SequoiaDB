#include "optGenForSAC.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

using namespace boost::property_tree ;

#if defined (GENERAL_OPT_SAC_FILE)
IMPLEMENT_GENERATOR_AUTO_REGISTER( optGenForSAC, GENERAL_OPT_SAC_FILE ) ;
#endif

#define XML_FIELD_ROOTNAME          "Property"
#define XML_SKIP_FIELD              "<xmlattr>"
#define XML_FIELD_FILLER            "FILLER"
#define XML_FIELD_NAME              "Name"
#define XML_FIELD_WEB_NAME          "WebName"
#define XML_FIELD_TYPE              "Type"
#define XML_FIELD_DEFAULT           "Default"
#define XML_FIELD_VALID             "Valid"
#define XML_FIELD_DISPLAY           "Display"
#define XML_FIELD_DESC              "Desc"
#define XML_FIELD_LEVEL             "Level"
#define XML_FIELD_DYNAMIC_EDIT      "DynamicEdit"

#define PTREE_GET_VALUE( tree, name, key, type, required, defVal ) \
{\
   if ( required )\
   {\
      name = tree.second.get< type >( key ) ;\
   }\
   else\
   {\
      try\
      {\
         name = tree.second.get< type >( key ) ;\
      }\
      catch( ... )\
      {\
         name = defVal ;\
      }\
   }\
}

static const char *_skipList[] = {
   OPT_FILTER_FOR_SAC_LIST
} ;

#define SKIP_LIST_SIZE (sizeof(_skipList)/sizeof(const char*))

optGenForSAC::optGenForSAC() : _isFinish( false )
{
}

optGenForSAC::~optGenForSAC()
{
}

int optGenForSAC::init()
{
   int rc = 0 ;

   rc = _loadOptList( OPTLIST_FILE_PATH ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _loadConfigTemplateWithSAC() ;
   if ( rc )
   {
      goto error ;
   }

   rc = _mergeConfig() ;
   if ( rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

bool optGenForSAC::hasNext()
{
   return !_isFinish ;
}

int optGenForSAC::outputFile( int id, fileOutStream &fout, string &outputPath )
{
   int rc = 0 ;
   int i  = 0 ;
   int listSize = (int)_sacXmlList.size() ;
   const char *lang = _getLang( id ) ;

   outputPath = OPT_SAC_SDB_CONFIG_FILE_PATH"_" ;
   if ( LANG_CN == string( lang ) )
   {
      outputPath += "zh-CN" ;
   }
   else
   {
      outputPath += lang ;
   }
   outputPath += ".xml" ;

   fout << "<Property type=\"array\">" << endl ;

   for ( i = 0; i < listSize; ++i )
   {
      SAC_XML_ELE tmpInfo = _sacXmlList[i] ;

      fout << "   <FILLER>" << endl
           << "      <Name>"     << tmpInfo.name << "</Name>" << endl
           << "      <WebName>"  << tmpInfo.webName[lang] << "</WebName>" << endl
           << "      <Type>"     << tmpInfo.type << "</Type>" << endl
           << "      <Default>"  << tmpInfo.defaultVal << "</Default>" << endl
           << "      <Valid>"    << tmpInfo.valid << "</Valid>" << endl
           << "      <Display>"  << tmpInfo.display << "</Display>" << endl
           << "      <Edit>true</Edit>" << endl
           << "      <Desc>"     << tmpInfo.desc[lang] << "</Desc>" << endl
           << "      <reloadable>" << tmpInfo.reloadable[lang] << "</reloadable>" << endl
           << "      <reloadstrategy>" << tmpInfo.reloadstrategy[lang] << "</reloadstrategy>" << endl
           << "      <Level>"    << tmpInfo.level << "</Level>" << endl
           << "      <hidden>"    << ( tmpInfo.hidden ? "true" : "false" ) << "</hidden>" << endl
           << "   </FILLER>" << endl ;
   }

   fout << "</Property>" << endl ;

   if ( _langListSize() == id + 1 )
   {
      _isFinish = true ;
   }
   return rc ;
}

int optGenForSAC::_getTemplateConfigIndex( const string &name )
{
   int i = 0 ;
   int index = -1 ;
   int listSize = (int)_sacXmlList.size() ;

   for ( i = 0; i < listSize; ++i )
   {
      SAC_XML_ELE tmpInfo = _sacXmlList[i] ;

      if( tmpInfo.name == name )
      {
         index = i ;
         break ;
      }
   }

   return index ;
}

int optGenForSAC::_mergeConfig()
{
   int rc = 0 ;
   int i  = 0 ;
   int k  = 0 ;
   int index = -1 ;
   int listSize = (int)_optionList.size() ;

   for ( i = 0; i < listSize; ++i )
   {
      bool isSkip = false ;
      OPTION_ELE optInfo = _optionList[i] ;

      for ( k = 0; k < SKIP_LIST_SIZE; ++k )
      {
         if ( optInfo.longtag == _skipList[k] )
         {
            isSkip = true ;
            break ;
         }
      }

      if ( isSkip )
      {
         continue ;
      }

      index = _getTemplateConfigIndex( optInfo.longtag ) ;
      if ( -1 == index )
      {
         SAC_XML_ELE ele ;

         index = (int)_sacXmlList.size() ;

         ele.name = optInfo.longtag ;
         ele.level = 1 ;

         if ( optInfo.typeofweb == "num" )
         {
            ele.type = "int" ;
            ele.display = "edit box" ;
         }
         else if ( optInfo.typeofweb == "boolean" )
         {
            ele.type = "bool" ;
            ele.valid = "true,false" ;
            ele.display = "select box" ;
         }
         else
         {
            ele.type = "string" ;
            ele.display = "edit box" ;
         }

         for( k = 0; k < _langListSize(); ++k )
         {
            const char *lang = _getLang( k ) ;

            ele.webName[lang] = optInfo.longtag ;
         }

         _sacXmlList.push_back ( ele ) ;
      }

      if ( optInfo.defttag.length() > 0 &&
           _sacXmlList[index].defaultVal.length() == 0 )
      {
         _sacXmlList[index].defaultVal = optInfo.defttag ;
      }

      _sacXmlList[index].hidden = optInfo.hiddentag ;

      for( k = 0; k < _langListSize(); ++k )
      {
         const char *lang = _getLang( k ) ;

         _sacXmlList[index].desc[lang] = optInfo.desc[lang] ;
      }

      for( k = 0; k < _langListSize(); ++k )
      {
         const char *lang = _getLang( k ) ;

         _sacXmlList[index].reloadable[lang] = optInfo.reloadable[lang] ;
      }

      for( k = 0; k < _langListSize(); ++k )
      {
         const char *lang = _getLang( k ) ;

         _sacXmlList[index].reloadstrategy[lang] = optInfo.reloadstrategy[lang] ;
      }
   }

   return rc ;
}

int optGenForSAC::_loadConfigTemplateWithSAC()
{
   int rc = 0 ;
   int i  = 0 ;
   string configPath ;
   ptree pt ;

   configPath = utilCatPath( _rootPath.c_str(), OPT_SAC_SDB_XML_FILE_PATH ) ;
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
      BOOST_FOREACH ( ptree::value_type &v, pt.get_child( XML_FIELD_ROOTNAME ) )
      {
         SAC_XML_ELE ele ;

         if ( XML_FIELD_FILLER == v.first )
         {
            PTREE_GET_VALUE( v, ele.name, XML_FIELD_NAME, string, true, "" ) ;

            for( i = 0; i < _langListSize(); ++i )
            {
               const char *lang = _getLang( i ) ;
               string key = XML_FIELD_WEB_NAME + string( "." ) + lang ;

               PTREE_GET_VALUE( v, ele.webName[lang], key, string, false, "" ) ;
            }

            PTREE_GET_VALUE( v, ele.type, XML_FIELD_TYPE, string, true, "" ) ;
            PTREE_GET_VALUE( v, ele.defaultVal, XML_FIELD_DEFAULT, string, true, "" ) ;
            PTREE_GET_VALUE( v, ele.valid, XML_FIELD_VALID, string, true, "" ) ;
            PTREE_GET_VALUE( v, ele.display, XML_FIELD_DISPLAY, string, true, "" ) ;

            for( i = 0; i < _langListSize(); ++i )
            {
               const char *lang = _getLang( i ) ;
               string key = XML_FIELD_DESC + string( "." ) + lang ;

               PTREE_GET_VALUE( v, ele.desc[lang], key, string, false, "" ) ;
            }

            PTREE_GET_VALUE( v, ele.level, XML_FIELD_LEVEL, int, true, 0 ) ;
         }
         else if ( XML_SKIP_FIELD == v.first )
         {
            continue ;
         }
         else
         {
            printLog( PD_ERROR ) << "Warning: unknow key, key = " << v.first
                                 << ", path = " << configPath << endl ;
            continue ;
         }

         _sacXmlList.push_back ( ele ) ;
      }
   }
   catch ( std::exception &e )
   {
      printLog( PD_ERROR ) << "Failed to parse xml: " << e.what() << endl ;
      rc = 1 ;
      goto error ;
   }

   if ( _sacXmlList.size() == 0 )
   {
      printLog( PD_ERROR ) << OPT_SAC_SDB_XML_FILE_PATH " is empty: path = "
                           << configPath << endl ;
      rc = 1 ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}
