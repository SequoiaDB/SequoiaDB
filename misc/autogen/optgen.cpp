#include "optgen.h"
#include "core.hpp"
#include "ossUtil.h"
#include <fstream>
#include <iostream>
#include <sstream>

using namespace boost::property_tree;
using std::cout;
using std::endl;
using std::setw;
using std::string;
using std::vector;
using std::ofstream;

OptGen::OptGen (const char* lang) : language (lang)
{
    loadFromXML ();
}

OptGen::~OptGen ()
{
   std::vector<OptElement*>::iterator it ;
   for ( it = optlist.begin(); it != optlist.end(); ++it )
   {
      delete *it ;
   }
   optlist.clear() ;
}

void OptGen::run ()
{
    genHeaderC() ;
    genHeaderCpp() ;
    genCatalogSample() ;
    genCoordSample() ;
    genDataSample() ;
	getStandAloneSample() ;
}

void OptGen::loadFromXML ()
{
    ptree pt;
    try
    {
        read_xml (OPTXMLSRC, pt);
    }
    catch ( std::exception &e )
    {
        cout<<"Can not read xml file, not exist or wrong directory for OptGen: " << e.what() <<endl;
        exit(0);
    }

    try
    {
        BOOST_FOREACH (ptree::value_type &v, pt.get_child (OPTLISTTAG))
        {
            OptElement *newele = new OptElement() ;
            if ( !newele )
            {
               cout<<"Failed to allocate memory for OptElement"<<endl ;
               exit(0) ;
            }
            try
            {
               newele->nametag = v.second.get<string>(NAMETAG) ;
               newele->longtag = v.second.get<string>(LONGTAG) ;
            }
            catch ( std::exception &e )
            {
               cout << "Name and Long tags are required: " << e.what() << endl ;
               continue ;
            }
            try
            {
               newele->shorttag = v.second.get<string>(SHORTTAG).c_str()[0] ;
            }
            catch ( std::exception & )
            {}

            try
            {
               newele->desctag = v.second.get_child(DESCRIPTIONTAG
                                        ).get<string>(language) ;
            }
            catch ( std::exception & )
            {}

            try
            {
               newele->defttag = v.second.get<string>(DEFAULTTAG) ;
            }
            catch ( std::exception & )
            {}

            try
            {
               newele->catatag = v.second.get<string>(CATALOGTAG) ;
            }
            catch ( std::exception & )
            {}

            try
            {
               newele->cordtag = v.second.get<string>(COORDTAG) ;
            }
            catch ( std::exception & )
            {}

			try
			{
			   newele->standtag= v.second.get<string>(STANDTAG) ;
			}
			catch ( std::exception & )
			{}
						

            try
            {
               newele->datatag = v.second.get<string>(DATATAG) ;
            }
            catch ( std::exception & )
            {}

            try
            {
               newele->typetag = v.second.get<string>(TYPETAG) ;
               if ( newele->typetag == NONETYPE )
                  newele->typetag = "" ;
            }
            catch ( std::exception & )
            {
               newele->typetag = "string" ;
            }

            try
            {
               ossStrToBoolean ( v.second.get<string>(HIDDENTAG).c_str(),
                                 &newele->hiddentag ) ;
            }
            catch ( std::exception & )
            {
               newele->hiddentag = FALSE ;
            }
            optlist.push_back (newele);
        }
    }
    catch ( std::exception&)
    {
        cout<<"XML format error, unknown node name or description language, please check!"<<endl;
        exit(0);
    }
}


void OptGen::genHeaderC ()
{
   ofstream fout(HEADERPATHC) ;
    if ( fout == NULL )
    {
        cout<<"can not open file: "<<HEADERPATHC<<endl;
        exit(0);
    }
    string comment =
        "/*    Copyright 2012 SequoiaDB Inc.\n"
        " *\n"
        " *    Licensed under the Apache License, Version 2.0 (the \"License\");\n"
        " *    you may not use this file except in compliance with the License.\n"
        " *    You may obtain a copy of the License at\n"
        " *\n"
        " *    http://www.apache.org/licenses/LICENSE-2.0\n"
        " *\n"
        " *    Unless required by applicable law or agreed to in writing, software\n"
        " *    distributed under the License is distributed on an \"AS IS\" BASIS,\n"
        " *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"
        " *    See the License for the specific language governing permissions and\n"
        " *    limitations under the License.\n"
        " */\n"
        "/*    Copyright (C) 2011-2014 SequoiaDB Ltd.\n"
        " *    This program is free software: you can redistribute it and/or modify\n"
        " *    it under the term of the GNU Affero General Public License, version 3,\n"
        " *    as published by the Free Software Foundation.\n"
        " *\n"
        " *    This program is distributed in the hope that it will be useful,\n"
        " *    but WITHOUT ANY WARRANTY; without even the implied warrenty of\n"
        " *    MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
        " *    GNU Affero General Public License for more details.\n"
        " *\n"
        " *    You should have received a copy of the GNU Affero General Public License\n"
        " *    along with this program. If not, see <http://www.gnu.org/license/>.\n"
        " */\n";
    fout<<std::left<<comment<<endl;
    comment = "\n// This Header File is automatically generated, you MUST NOT modify this file anyway!\n"
              "// On the contrary, you can modify the xml file \"sequoiadb/misc/autogen/optlist.xml\" if necessary!\n";
    fout<<comment<<endl;

    fout<<"#ifndef PMDOPTIONS_H_"<<endl ;
    fout<<"#define PMDOPTIONS_H_"<<endl ;

    for (int i = 0; i < optlist.size(); ++i)
    {
        OptElement *optEle = optlist[i] ;
        fout<<"#define "<<setw(64)<<optEle->nametag<<"\""<<optEle->longtag<<"\""<<endl ;
    }
    fout<<"#endif"<<endl ;
    fout.close() ;
}

void OptGen::genHeaderCpp ()
{
    ofstream fout(HEADERPATHCPP);
    if ( fout == NULL )
    {
        cout<<"can not open file: "<<HEADERPATHCPP<<endl;
        exit(0);
    }
    string comment =
        "/*    Copyright 2012 SequoiaDB Inc.\n"
        " *\n"
        " *    Licensed under the Apache License, Version 2.0 (the \"License\");\n"
        " *    you may not use this file except in compliance with the License.\n"
        " *    You may obtain a copy of the License at\n"
        " *\n"
        " *    http://www.apache.org/licenses/LICENSE-2.0\n"
        " *\n"
        " *    Unless required by applicable law or agreed to in writing, software\n"
        " *    distributed under the License is distributed on an \"AS IS\" BASIS,\n"
        " *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"
        " *    See the License for the specific language governing permissions and\n"
        " *    limitations under the License.\n"
        " */\n"
        "/*    Copyright (C) 2011-2014 SequoiaDB Ltd.\n"
        " *    This program is free software: you can redistribute it and/or modify\n"
        " *    it under the term of the GNU Affero General Public License, version 3,\n"
        " *    as published by the Free Software Foundation.\n"
        " *\n"
        " *    This program is distributed in the hope that it will be useful,\n"
        " *    but WITHOUT ANY WARRANTY; without even the implied warrenty of\n"
        " *    MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
        " *    GNU Affero General Public License for more details.\n"
        " *\n"
        " *    You should have received a copy of the GNU Affero General Public License\n"
        " *    along with this program. If not, see <http://www.gnu.org/license/>.\n"
        " */\n";
    fout<<std::left<<comment<<endl;
    comment = "\n// This Header File is automatically generated, you MUST NOT modify this file anyway!\n"
              "// On the contrary, you can modify the xml file \"sequoiadb/misc/autogen/optlist.xml\" if necessary!\n";
    fout<<comment<<endl;

    fout<<"#ifndef PMDOPTIONS_HPP_"<<endl
        <<"#define PMDOPTIONS_HPP_"<<endl<<endl ;

    fout <<"#include \""HEADERNAME".h\"" << endl ;
    fout<<"#define PMD_COMMANDS_OPTIONS \\"<<endl ;
    for (int i = 0; i < optlist.size(); ++i)
    {
        OptElement *optEle = optlist[i] ;
        if ( !optEle->hiddentag )
        {
           if ( optEle->shorttag != 0 )
              fout << "        ( PMD_COMMANDS_STRING ("<<optEle->nametag<<", \","<<optEle->shorttag<<"\"), " ;
           else
              fout << "        ( "<<optEle->nametag<<", " ;
           if ( optEle->typetag.length() != 0 )
              fout << "boost::program_options::value<"<<optEle->typetag<<">(), " ;
           fout << "\""<<optEle->desctag<<"\" ) \\"<<endl ;
        }
    }
    fout<<endl;

    fout<<"#define PMD_HIDDEN_COMMANDS_OPTIONS \\"<<endl ;
    for (int i = 0; i < optlist.size(); ++i)
    {
        OptElement *optEle = optlist[i] ;
        if ( optEle->hiddentag )
        {
           if ( optEle->shorttag != 0 )
              fout << "        ( PMD_COMMANDS_STRING ("<<optEle->nametag<<", \","<<optEle->shorttag<<"\"), " ;
           else
              fout << "        ( "<<optEle->nametag<<", " ;
           if ( optEle->typetag.length() != 0 )
              fout << "boost::program_options::value<"<<optEle->typetag<<">(), " ;
           fout << "\""<<optEle->desctag<<"\" ) \\"<<endl ;
        }
    }
    fout<<endl;
    fout<<"#endif /* PMDOPTIONS_HPP_ */";

    fout.close();
}

void OptGen::genSampleHeader ( ofstream &fout )
{
    string comment =
        "# SequoiaDB configuration\n" ;
    fout<<std::left<<comment<<endl;
}

void OptGen::genConfPair ( ofstream &fout, string key, string value1,
                           string value2, string desc )
{
   if ( desc.length() != 0 )
   {
      fout << "# " << desc << endl ;
   }
   if ( value1.length() != 0 )
   {
      fout << key << "=" << value1 << endl ;
   }
   else if ( value2.length() != 0 )
   {
      fout << key << "=" << value2 << endl ;
   }
   else
   {
      fout << "# " << key << "=" << endl ;
   }
}

void OptGen::genCatalogSample()
{
    ofstream fout(CATALOGSAMPLEPATH);
    if ( fout == NULL )
    {
        cout<<"can not open file: "<<CATALOGSAMPLEPATH<<endl;
        exit(0);
    }
    genSampleHeader ( fout ) ;
    for (int i = 0; i < optlist.size(); ++i)
    {
        OptElement *optEle = optlist[i] ;
        if ( !optEle->hiddentag && optEle->typetag.length() != 0 )
        {
           genConfPair ( fout, optEle->longtag, optEle->catatag,
                         optEle->defttag, optEle->desctag ) ;
           fout << endl ;
        }
    }
    fout.close() ;
}


void OptGen::getStandAloneSample()
{
    ofstream fout(STANDALONESAMPLEPATH);
	if ( fout == NULL )
    {
        cout<<"can not open file: "<<STANDALONESAMPLEPATH<<endl;
        exit(0);
    }
	genSampleHeader ( fout ) ;

    for (int i = 0; i < optlist.size(); ++i)
    {
        OptElement *optEle = optlist[i] ;
        if ( !optEle->hiddentag && optEle->typetag.length() != 0 )
        {
           genConfPair ( fout, optEle->longtag, optEle->standtag,
                         optEle->defttag, optEle->desctag ) ;
           fout << endl ;
        }
    }
    fout.close() ;


}


void OptGen::genCoordSample()
{
    ofstream fout(COORDSAMPLEPATH);
    if ( fout == NULL )
    {
        cout<<"can not open file: "<<COORDSAMPLEPATH<<endl;
        exit(0);
    }
    genSampleHeader ( fout ) ;
    for (int i = 0; i < optlist.size(); ++i)
    {
        OptElement *optEle = optlist[i] ;
        if ( !optEle->hiddentag && optEle->typetag.length() != 0 )
        {
           genConfPair ( fout, optEle->longtag, optEle->cordtag,
                         optEle->defttag, optEle->desctag ) ;
           fout << endl ;
        }
    }
    fout.close () ;
}

void OptGen::genDataSample()
{
    ofstream fout(DATASAMPLEPATH);
    if ( fout == NULL )
    {
        cout<<"can not open file: "<<DATASAMPLEPATH<<endl;
        exit(0);
    }
    genSampleHeader ( fout ) ;
    for (int i = 0; i < optlist.size(); ++i)
    {
        OptElement *optEle = optlist[i] ;
        if ( !optEle->hiddentag && optEle->typetag.length() != 0 )
        {
           genConfPair ( fout, optEle->longtag, optEle->datatag,
                         optEle->defttag, optEle->desctag ) ;
           fout << endl ;
        }
    }
    fout.close () ;
}
