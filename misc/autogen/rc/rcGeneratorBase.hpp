#ifndef RC_GENERATOR_HPP
#define RC_GENERATOR_HPP

#include "../generateInterface.hpp"

// xml file
#define RC_FILENAME        "rclist.xml"
#define RC_FILE_PATH       "./" RC_FILENAME
#define RC_DESC_PATH       "sequoiadb/misc/autogen/" RC_FILENAME

struct RCInfo
{
   int    value ;
   bool   reserved ;
   string name ;
   string desc_en ;
   string desc_cn ;

   RCInfo(): value( 0 ),
             reserved( false )
   {
   }

   string getDesc( string lang ) const
   {
      if( "cn" == lang )
      {
         return desc_cn ;
      }
      else if( "en" == lang )
      {
         return desc_en ;
      }
      else
      {
         return "invalid lang" ;
      }
   }
} ;

class rcGeneratorBase : public generateBase
{
public:
   rcGeneratorBase() ;
   ~rcGeneratorBase() ;
   virtual int init() ;
   virtual bool hasNext() = 0 ;
   virtual int outputFile( int id, fileOutStream &fout,
                           string &outputPath ) = 0;
   virtual const char* name() = 0 ;

protected:
   int _loadRcList() ;
   int _buildStatement( int type, string &headerDesc ) ;

protected:
   int _maxFieldWidth ;

   vector<pair<string, int> > _conslist ;
   vector<RCInfo> _rcInfoList ;
} ;

#endif
