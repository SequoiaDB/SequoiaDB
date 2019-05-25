#ifndef OptGen_H
#define OptGen_H

#include "core.hpp"
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

#define OPTXMLSRC "optlist.xml"
#define HEADERNAME "pmdOptions"
#define HEADERPATHCPP "../../SequoiaDB/engine/include/"HEADERNAME".hpp"
#define HEADERPATHC "../../SequoiaDB/engine/include/"HEADERNAME".h"
#define COORDSAMPLEPATH "../../conf/samples/sdb.conf.coord"
#define CATALOGSAMPLEPATH "../../conf/samples/sdb.conf.catalog"
#define DATASAMPLEPATH "../../conf/samples/sdb.conf.data"
#define STANDALONESAMPLEPATH "../../conf/samples/sdb.conf.standalone"


#define OPTLISTTAG "optlist"
#define OPTTAG "opt"
#define NAMETAG "name"
#define LONGTAG "long"
#define SHORTTAG "short"
#define DESCRIPTIONTAG "description"
#define DEFAULTTAG "default"
#define CATALOGTAG "catalogdft"
#define COORDTAG "coorddft"
#define DATATAG "datadft"
#define STANDTAG "standdft"
#define TYPETAG "type"
#define HIDDENTAG "hidden"

#define NONETYPE "none"

class OptElement
{
public :
    std::string nametag ;
    std::string longtag ;
    std::string desctag ;
    std::string defttag ;
    std::string catatag ;
    std::string cordtag ;
    std::string datatag ;
	std::string standtag;
    std::string typetag ;
    BOOLEAN hiddentag ;
    CHAR shorttag ;
    OptElement ()
    {
       hiddentag = FALSE ;
       shorttag = 0 ;
    }
} ;
class OptGen
{
    const char* language;
    std::vector<OptElement*> optlist;
    void loadFromXML ();
    void genHeaderC () ;
    void genHeaderCpp () ;
    void genCatalogSample () ;
    void genCoordSample () ;
    void genDataSample () ;
	  void getStandAloneSample() ;
public:
    OptGen (const char* lang);
    void genConfPair ( std::ofstream &fout, std::string key, std::string value1,
                       std::string value2, std::string desc ) ;
    void genSampleHeader ( std::ofstream &fout ) ;
    ~OptGen () ;
    void run ();
};
#endif
