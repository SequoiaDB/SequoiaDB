#include "optGenForDoc.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

using namespace boost::property_tree ;

#define XML_FIELD_ROOTNAME             "optOtherInfoForWeb"
#define XML_FIELD_TITLE                "title"
#define XML_FIELD_SUBTITLE             "subtitle"
#define XML_FIELD_STHEAD               "sthead"
#define XML_FIELD_STE_NAME             "stentry_name"
#define XML_FIELD_STE_ACRONYM          "stentry_acronym"
#define XML_FIELD_STE_TYPE             "stentry_type"
#define XML_FIELD_STE_RELOADABLE       "stentry_reloadable"
#define XML_FIELD_STE_RELOADSTRATEGY   "stentry_reloadstrategy"
#define XML_FIELD_STE_DESC             "stentry_dest"
#define XML_FIELD_NOTE                 "note"
#define XML_FIELD_FIRST                "first"
#define XML_FIELD_SECOND               "second"
#define XML_FIELD_THIRD                "third"


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
   OPT_FILTER_FOR_DOC_LIST
} ;

#define SKIP_LIST_SIZE (sizeof(_skipList)/sizeof(const char*))

#if defined (GENERAL_OPT_DOC_FILE)
IMPLEMENT_GENERATOR_AUTO_REGISTER( optGenForDoc, GENERAL_OPT_DOC_FILE ) ;
#endif

optGenForDoc::optGenForDoc() : _isFinish( false ),
                               _tmpLang( LANG_CN )
{
}

optGenForDoc::~optGenForDoc()
{
}

int optGenForDoc::init()
{
   int rc = 0 ;

   rc = _loadOptList( OPTLIST_FILE_PATH ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _loadOptOtherInfo() ;
   if ( rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

bool optGenForDoc::hasNext()
{
   return !_isFinish ;
}

int optGenForDoc::outputFile( int id, fileOutStream &fout, string &outputPath )
{
   int rc = 0 ;
   int i  = 0 ;
   int k  = 0 ;
   int listSize = (int)_optionList.size() ;
   const OPTION_OTHER_INFO_ELE &otherInfo = _optionOtherList[0] ;
   string srcContent ;

   outputPath = OPT_RUNTIME_CONFIG_PATH ;

   fout	<< "|" << otherInfo.name
        << "|" << otherInfo.acronym
        << "|" << otherInfo.type
        << "|" << otherInfo.reloadable
        << "|" << otherInfo.reloadstrategy
        << "|" << otherInfo.desc
        << "|" << endl
        << "|---|---|---|---|---|---|"
        << endl ;

   for ( i = 0; i < listSize; ++i )
   {
      bool isSkip = false ;
      OPTION_ELE optInfo = _optionList[i] ;
      string detail = optInfo.getDetail( _tmpLang ) ;
      string longtag ;
      string shorttag ;

      if ( optInfo.hiddentag )
      {
         continue ;
      }

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

      detail = utilReplaceAll( detail, "\n", "<br/>" ) ;
      detail = utilReplaceAll( detail, "|", "\\|" ) ;

      if ( optInfo.shorttag != 0 )
      {
         shorttag += "-" ;
         shorttag += optInfo.shorttag ;
      }

      longtag = "--" + optInfo.longtag ;

      fout << "|" << longtag
           << "|" << shorttag
           << "|" << optInfo.typeofweb
           << "|" << optInfo.getReloadable( _tmpLang )
           << "|" << optInfo.getReloadstrategy( _tmpLang )
           << "|" << detail
           << "|\n" ;
   }

   fout << "\n" ;

   if( !otherInfo.first.empty() )
   {
      fout << ">**Note:**  " << endl ;
      fout << ">1. " << otherInfo.first << "  " << endl ;
      if( !otherInfo.second.empty() )
      {
         fout << ">2. " << otherInfo.second << "  " << endl ;
      }
      if( !otherInfo.third.empty() )
      {
         fout << ">3. " << otherInfo.third << "  " << endl ;
      }
   }

   fout << "\n" ;

done:
   _isFinish = true ;
   return rc ;
error:
   goto done ;
}

int optGenForDoc::_loadOptOtherInfo()
{
   int rc = 0 ;
   ptree pt ;
   string configPath ;

   configPath = utilCatPath( _rootPath.c_str(), OPT_OTHER_DOC_FILE_PATH ) ;
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
      OPTION_OTHER_INFO_ELE ele ;

      BOOST_FOREACH ( ptree::value_type &v, pt.get_child( XML_FIELD_ROOTNAME ) )
      {
         if ( XML_FIELD_TITLE == v.first )
         {
            PTREE_GET_VALUE( v, ele.title, _tmpLang, string, true, "" ) ;
         }
         else if ( XML_FIELD_SUBTITLE == v.first )
         {
            PTREE_GET_VALUE( v, ele.subTitle, _tmpLang, string, true, "" ) ;
         }
         else if ( XML_FIELD_STHEAD == v.first )
         {
            BOOST_FOREACH( ptree::value_type &v1, v.second )
            {
               if ( XML_FIELD_STE_NAME == v1.first )
               {
                  PTREE_GET_VALUE( v1, ele.name, _tmpLang, string, false, "" ) ;
               }
               else if ( XML_FIELD_STE_ACRONYM == v1.first )
               {
                  PTREE_GET_VALUE( v1, ele.acronym, _tmpLang, string,
                                   false, "" ) ;
               }
               else if ( XML_FIELD_STE_TYPE == v1.first )
               {
                  PTREE_GET_VALUE( v1, ele.type, _tmpLang, string, false, "" ) ;
               }
               else if ( XML_FIELD_STE_RELOADABLE == v1.first )
               {
                  PTREE_GET_VALUE( v1, ele.reloadable, _tmpLang, string,
                                   false, "" ) ;
               }
               else if ( XML_FIELD_STE_RELOADSTRATEGY == v1.first )
               {
                  PTREE_GET_VALUE( v1, ele.reloadstrategy, _tmpLang, string,
                                   false, "" ) ;
               }
               else if ( XML_FIELD_STE_DESC == v1.first )
               {
                  PTREE_GET_VALUE( v1, ele.desc, _tmpLang, string, false, "" ) ;
               }
            }
         }
         else if ( XML_FIELD_NOTE == v.first )
         {
            BOOST_FOREACH( ptree::value_type &v2, v.second )
            {
               if ( XML_FIELD_FIRST == v2.first )
               {
                  PTREE_GET_VALUE( v2, ele.first, _tmpLang, string, false, "" ) ;
               }
               else if ( XML_FIELD_SECOND == v2.first )
               {
                  PTREE_GET_VALUE( v2, ele.second, _tmpLang, string, false, "" ) ;
               }
               else if ( XML_FIELD_THIRD == v2.first )
               {
                  PTREE_GET_VALUE( v2, ele.third, _tmpLang, string, false, "" ) ;
               }
            }
         }
         else
         {
            printLog( PD_ERROR ) << "Warning: unknow key, key = " << v.first
                                 << ", path = " << configPath << endl ;
            continue ;
         }
      }
      _optionOtherList.push_back ( ele ) ;
   }
   catch ( std::exception &e )
   {
      printLog( PD_ERROR ) << "Failed to parse xml: " << e.what() << endl ;
      rc = 1 ;
      goto error ;
   }

   if ( _optionOtherList.size() == 0 )
   {
      printLog( PD_ERROR ) << OPT_OTHER_DOC_FILENAME" is empty: path = "
                           << configPath << endl ;
      rc = 1 ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}
