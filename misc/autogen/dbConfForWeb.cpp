#include "dbConfForWeb.h"
#include "core.hpp"
#include "ossUtil.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <iterator>

#define OPTLISTTAG            "optlist"
#define NAMETAG               "name"
#define LONGTAG               "long"
#define SHORTTAG              "short"
#define TYPEOFWEBTAG          "typeofweb"
#define TYPEOFRELOADABLE      "reloadable"
#define TYPEOFRELOADSTRATEGY  "reloadstrategy"
#define HIDDENTAG             "hidden"
#define DETAILTAG             "detail"

#define OPTOTHERINFOFORWEBTAG       "optOtherInfoForWeb"
#define TITLETAG                    "title"
#define SUBTITLETAG                 "subtitle"
#define STHEADTAG                   "sthead"
#define STENTRY_NAMETAG             "stentry_name"
#define STENTRY_ACRONYMTAG          "stentry_acronym"
#define STENTRY_TYPETAG             "stentry_type"
#define STENTRY_RELOADABLETAG       "stentry_reloadable"
#define STENTRY_RELOADSTRATEGYTAG   "stentry_reloadstrategy"
#define STENTRY_DESTTAG             "stentry_dest"
#define NOTETAG                     "note"
#define NOTE_FIRSTTAG               "first"
#define NOTE_SECONDTAG              "second"

using namespace boost::property_tree;
using namespace std;

const CHAR *pLanguage[] = { "en", "cn" } ;

static string& replace_all ( string &str, const string& old_value, const string &new_value )
{
   for ( string::size_type pos(0) ; pos != string::npos; pos += new_value.length() )
   {
      if ( ( pos = str.find ( old_value, pos ) ) != string::npos )
         str.replace ( pos, old_value.length(), new_value ) ;
      else break ;
   }
   return str ;
}

OptGenForWeb::OptGenForWeb ( const char* lang ) : language( lang )
{
   loadFromXML () ;
   loadOtherInfoFromXML () ; 
}

OptGenForWeb::~OptGenForWeb ()
{
    vector<OptEle*>::iterator it ;
    vector<OptOtherInfoEle*>::iterator ite ;
    for ( it = optlist.begin(); it != optlist.end(); it++ )
    {
        delete *it ;
    }
    optlist.clear() ;
    for ( ite = optOtherInfo.begin(); ite != optOtherInfo.end(); ite++ )
    {
        delete *ite ;
    }
    optOtherInfo.clear() ;
}

void OptGenForWeb::loadOtherInfoFromXML ()
{
    ptree pt ;
    try
    {
       read_xml ( OPTOTHERINFOFORWEBFILE, pt ) ;
    }
    catch ( std::exception &e )
    {
        cout << "Can not read xml file, not exist or wrong directory forOptGenForWeb: "
             << e.what() << endl ;
        exit ( 0 ) ;
    }

    OptOtherInfoEle *newele = new OptOtherInfoEle() ;
    if ( !newele )
    {
        cout << "Failed to allocate memory for OptOtherInfoEle!" << endl ;
        exit ( 0 ) ;
    }

    try
    {
        BOOST_FOREACH ( ptree::value_type &v, pt.get_child( OPTOTHERINFOFORWEBTAG ) )
        {
            if ( TITLETAG == v.first )
            {
                try
                {
                    newele->titletag = v.second.get<string>(language) ;
                }
                catch ( std::exception &e )
                {
                    delete newele ;
                    cout << "Wrong to get the title tag: " << e.what()
                         << endl ;
                    continue ;
                }
            }
            else if ( SUBTITLETAG == v.first )
            {
                try
                {
                    newele->subtitletag = v.second.get<string>(language) ;
                }
                catch ( std::exception &e )
                {
                    delete newele ;
                    cout << "Wrong to get the subtitle tag: " << e.what()
                         << endl ;
                    continue ;
                }
            }
            else if ( STHEADTAG == v.first )
            {
                try
                {
                    BOOST_FOREACH( ptree::value_type &v1, v.second )
                    {
                        if ( STENTRY_NAMETAG == v1.first )
                        {
                            newele->stentry_nametag = v1.second
                                           .get<string>(language) ;
                        }
                        else if ( STENTRY_ACRONYMTAG == v1.first )
                        {
                            newele->stentry_acronymtag = v1.second
                                           .get<string>(language) ;
                        }
                        else if ( STENTRY_TYPETAG == v1.first )
                        {
                            newele->stentry_typetag = v1.second
                                           .get<string>(language) ;
                        }
                        else if ( STENTRY_RELOADABLETAG == v1.first )
                        {
                            newele->stentry_reloadabletag = v1.second
                                           .get<string>(language) ;
                        }
                        else if ( STENTRY_RELOADSTRATEGYTAG == v1.first )
                        {
                            newele->stentry_reloadstrategytag = v1.second
                                           .get<string>(language) ;
                        }
                        else if ( STENTRY_DESTTAG == v1.first )
                        {
                            newele->stentry_desttag = v1.second
                                           .get<string>(language) ;
                        }
                    }
                }
                catch ( std::exception &e )
                {
                    delete newele ;
                    cout << "Wrong to get the stentry tags: " << e.what()
                         << endl ;
                    continue ;
                }
            }
            else if ( NOTETAG == v.first )
            {
                try
                {
                    BOOST_FOREACH( ptree::value_type &v2, v.second )
                    {
                        if ( NOTE_FIRSTTAG == v2.first )
                        {
                            newele->firsttag = v2.second
                                    .get<string>(language) ;
                        }
                        else if ( NOTE_SECONDTAG == v2.first )
                        {
                            newele->secondtag = v2.second
                                    .get<string>(language) ;
                        }
                    }
                }
                catch ( std::exception &e )
                {
                    delete newele ;
                    cout << "Wrong to get the note tags: " << e.what()
                         << endl ;
                    continue ;
                }
            }
        }
    }
    catch ( std::exception& e )
    {
        cout << "XML format error: " << e.what()
             << ", unknown node name or description language,please check!"
             << endl ;
        exit(0) ;
    }
    optOtherInfo.push_back ( newele ) ;
}

INT32 OptGenForWeb::parseOptListTag( ptree::value_type &v )
{
   INT32 rc = SDB_OK ;
   boost::optional<string> optionalString ;
   boost::optional<ptree &> optionalPtree ;
   ptree defaultTagValue ;
   defaultTagValue.put("en", "") ;
   defaultTagValue.put("cn", "") ;

   OptEle *newele = new ( std::nothrow ) OptEle() ;

   if ( !newele )
   {
       cout << "Failed to allocate memory for OptEle!" << endl ;
       rc = SDB_SYS ;
       goto error ;
   }

   optionalString = v.second.get_optional<string>(HIDDENTAG) ;
   if ( optionalString )
   {
      ossStrToBoolean( optionalString->c_str(), &( newele->hiddentag ) ) ;
   }

   if ( newele->hiddentag )
   {
      goto done ;
   }

   optionalString = v.second.get_optional<string>(NAMETAG) ;
   if ( optionalString )
   {
      newele->nametag = *optionalString ;
   }
   else
   {
      cout << "Name tag is required." << endl ;
      rc = SDB_SYS ;
      goto error ;
   }

   optionalString = v.second.get_optional<string>(LONGTAG) ;
   if ( optionalString )
   {
      newele->longtag += *optionalString ;
   }
   else
   {
      cout << "Long tag is required for " << newele->nametag << endl ;
      rc = SDB_SYS ;
      goto error ;
   }

   try
   {
      optionalPtree = v.second.get_child_optional(DETAILTAG) ;
      if ( optionalPtree )
      {
         newele->detailtag = optionalPtree->get<string>(language) ;
      }
      else
      {
         cout << "Detail tag is required for " << newele->longtag << endl ;
         rc = SDB_SYS ;
         goto error ;
      }

      optionalString = v.second.get_optional<string>(SHORTTAG) ;
      if ( optionalString )
      {
         newele->shorttag += *optionalString ;
      }
      else
      {
         newele->shorttag = "";
      }

      newele->typeofwebtag = v.second.get(TYPEOFWEBTAG, "--") ;

      newele->reloadabletag = v.second.get_child_optional(TYPEOFRELOADABLE)
                               .value_or(defaultTagValue).get<string>(language) ;

      newele->reloadstrategytag = v.second.get_child_optional(TYPEOFRELOADSTRATEGY)
                                   .value_or(defaultTagValue).get<string>(language) ;
   }
   catch ( std::exception &e )
   {
      cout << "XML format error: " << e.what()
           << ", unknown description language for "
           << newele->longtag << endl ;
      rc = SDB_SYS ;
      goto error ;
   }

   optlist.push_back ( newele ) ;
   newele = NULL ;

done:
   if ( newele )
   {
      delete newele ;
   }

   return rc;
error:

   goto done;
}

void OptGenForWeb::loadFromXML ()
{
   ptree pt ;

   try
   {
     read_xml ( OPTXMLSRCFILE, pt ) ;
   }
   catch ( std::exception &e )
   {
     cout << "Can not read src xml file, not exist or wrong directory for OptGenForWeb: "
          << e.what() << endl ;
     exit ( 1 ) ;
   }

   try
   {
      BOOST_FOREACH ( ptree::value_type &v, pt.get_child( OPTLISTTAG ) )
      {
         if ( SDB_OK != parseOptListTag( v ) )
         {
            exit ( 1 ) ;
         }
      }
   }
   catch ( std::exception &e )
   {
      cout << "XML format error: " << e.what()
           << ", cannot find tag " << OPTLISTTAG
           << endl ;
      exit ( 1 ) ;
   }

}

string OptGenForWeb::genOptions ()
{
    vector<OptEle*>::iterator it ;
    vector<OptOtherInfoEle*>::iterator ite ;
    ostringstream oss ;

    ite = optOtherInfo.begin() ;
    if ( optOtherInfo.end() == ite )
    {
        cout << "Nothing in 'optOtherInfo'." << endl ;
        exit( 0 ) ;
    }

    oss << "##" << (*ite)->subtitletag << "##" << endl ;
    oss << endl ;

    oss << "|" << (*ite)->stentry_nametag
        << "|" << (*ite)->stentry_acronymtag
        << "|" << (*ite)->stentry_typetag
        << "|" << (*ite)->stentry_reloadabletag
        << "|" << (*ite)->stentry_reloadstrategytag
        << "|" << (*ite)->stentry_desttag
        << "|" << endl ;
    oss << "|---|---|---|---|---|---|" << endl ;

    for ( it = optlist.begin(); it != optlist.end(); it++ )
    {
        string detail = replace_all( (*it)->detailtag, "\n", "<br/>" );
        detail = replace_all( detail, "|", "\\|" );

        oss << "|" << (*it)->longtag
            << "|" << (*it)->shorttag
            << "|" << (*it)->typeofwebtag
            << "|" << (*it)->reloadabletag
            << "|" << (*it)->reloadstrategytag
            << "|" << detail
            << "|" << endl ;
    }
    oss << endl ;

    if( !(*ite)->firsttag.empty() )
    {
        oss << ">**Note:**  " << endl ;
        oss << ">1. " << (*ite)->firsttag << "  " << endl ;
        if( !(*ite)->secondtag.empty() )
        {
            oss << ">2. " << (*ite)->secondtag << "  " << endl ;
        }
    }
    oss << endl ;
    return oss.str() ;
}

string OptGenForWeb::genSupplement()
{
    string str ;

    if ( 0 == strcmp( pLanguage[0], language ) )
    {
    }
    else if ( 0 == strcmp( pLanguage[1], language ) )
    {
        ifstream fin( OPT_SUPPLEMENTFILE ) ;
        str = string( istreambuf_iterator< char >( fin ),
                      istreambuf_iterator< char >() ) ;
    }
    else
    {
        cout << "The language is not support: " << language << endl ;
    }

    return str ;
}

void OptGenForWeb::gendoc()
{
    string optStr ;
    string suppleStr ;
    string fileName ;

    if ( 0 == strcmp( pLanguage[0], language ) )
    {
        fileName = string( OPT_MDPATH ) ;
    }
    else if ( 0 == strcmp( pLanguage[1], language ) )
    {
        fileName = string( OPT_MDPATH ) ;
    }
    else
    {
        cout << "The language is not support: " << language << endl ;
    }

    optStr = genOptions() ;
    if ( "" == optStr )
    {
        cout << "Failed to generate database configuration options." << endl ;
        exit ( 0 ) ;
    }

    suppleStr = genSupplement() ;
    ofstream fout( fileName.c_str() ) ;

    fout << optStr << suppleStr << endl ;
}

void OptGenForWeb::run ()
{
   gendoc () ;
}

