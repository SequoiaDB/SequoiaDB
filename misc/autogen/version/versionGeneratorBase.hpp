#ifndef RC_GENERATOR_HPP
#define RC_GENERATOR_HPP

#include "../generateInterface.hpp"

class versionGeneratorBase : public generateBase
{
public:
   versionGeneratorBase() ;
   ~versionGeneratorBase() ;
   virtual int init() ;
   virtual bool hasNext() = 0 ;
   virtual int outputFile( int id, fileOutStream &fout,
                           string &outputPath ) = 0;
   virtual const char* name() = 0 ;
} ;

#endif
