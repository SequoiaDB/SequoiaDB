#ifndef GENERATE_INTERFACE_HPP
#define GENERATE_INTERFACE_HPP

#include "config.h"
#include "options.hpp"
#include "util.h"
#include "fileStream.hpp"
#include <vector>
#include <iostream>

using namespace std ;

class generateBase ;
class _generatorAssit ;

class generateRunner
{
friend class _generatorAssit ;

public:
   generateRunner() ;
   ~generateRunner() ;
   int init() ;
   int run() ;

private:
   void _register( int priority, generateBase *generator ) ;

private:
   const char *_lang ;
   vector<pair<int, generateBase*> > _generatorList ;
} ;

generateRunner* getGenerateRunner() ;

#define DECLARE_GENERATOR_AUTO_REGISTER()       \
   public:                                      \
      static generateBase* newThis() ;          \

#define IMPLEMENT_GENERATOR_AUTO_REGISTER(theClass, priority)        \
   generateBase* theClass::newThis()                                 \
   {                                                                 \
      return new theClass() ;                                        \
   }                                                                 \
   _generatorAssit theClass##Assit( theClass::newThis, priority ) ;  \

class generateBase
{
public:
   generateBase() ;
   virtual ~generateBase() ;
   void setLang( const char *lang ) ;
   virtual int init() ;
   virtual bool hasNext() = 0 ;

   /*
      [in]  id:   output file id
      [in]  fout: output file streambuf
      [out] outputPath: output file path
   */
   virtual int outputFile( int id, fileOutStream &fout,
                           string &outputPath ) = 0;

   virtual const char* name() = 0 ;

protected:
   const char *_getLang( int n ) ;
   int _langListSize() ;

protected:
   const char* _lang ;
   string      _rootPath ;
} ;

typedef generateBase* (*GENERATOR_NEW_FUNC)() ;

class _generatorAssit
{
public:
   _generatorAssit( GENERATOR_NEW_FUNC, int priority ) ;
   virtual ~_generatorAssit() ;
} ;

#endif