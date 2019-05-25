#ifndef DBConfForWeb_H
#define DBConfForWeb_H

#include "core.hpp"
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

#define OPTXMLSRCFILE          "optlist.xml"
#define OPTOTHERINFOFORWEBFILE "optOtherInfoForWeb.xml"

#define OPT_SUPPLEMENTFILE "../../doc/src/document/database_management/runtime_configuration_supplement.md"

#define OPT_MDPATH "../../doc/src/document/database_management/runtime_configuration.md"

class OptEle
{
public:
	std::string nametag ;
    std::string longtag ;
    std::string shorttag ;
    std::string typeofwebtag ;
	std::string reloadabletag ;
	std::string reloadstrategytag ;
    std::string detailtag ;
    BOOLEAN hiddentag ;
    OptEle()
    {
       longtag = "--" ;
       shorttag = "-" ;
       hiddentag = FALSE ;
    }
} ;

class OptOtherInfoEle
{
public:
    std::string titletag ;
    std::string subtitletag ;
    std::string stentry_nametag ;
    std::string stentry_acronymtag ;
    std::string stentry_typetag ;
	std::string stentry_reloadabletag ;
	std::string stentry_reloadstrategytag ;
    std::string stentry_desttag ;
    std::string firsttag ;
    std::string secondtag ;
} ;

class OptGenForWeb
{
    const char *language ;
    std::vector<OptOtherInfoEle*> optOtherInfo ;
    std::vector<OptEle*> optlist ;
    void loadOtherInfoFromXML () ;
    void loadFromXML () ;
	INT32 parseOptListTag( boost::property_tree::ptree::value_type &v ) ;
    std::string genOptions () ;
	std::string genSupplement() ;
    void gendoc () ;

public:
    OptGenForWeb ( const char* lang ) ;
    ~OptGenForWeb () ;
    void run () ;
};

#endif
