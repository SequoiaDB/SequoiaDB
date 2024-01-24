#include "generateInterface.hpp"
#include <algorithm>

static const char *_langList[] = {
   LANG_LIST
} ;

#define LANG_LIST_SIZE (sizeof(_langList)/sizeof(const char*))

/* generateRunner */
generateRunner::generateRunner() : _lang( LANG_EN )
{
}

generateRunner::~generateRunner()
{
   vector<pair<int, generateBase*> >::iterator it ;

   for( it = _generatorList.begin(); it != _generatorList.end(); ++it )
   {
      generateBase *gen = (*it).second ;

      if ( gen )
      {
         delete gen ;
      }

      gen = NULL ;
   }

   _generatorList.clear() ;
}

int generateRunner::init()
{
   for ( unsigned int i = 0; i < LANG_LIST_SIZE; ++i )
   {
      if ( getCmdOptions()->getLang() == _langList[i] )
      {
         _lang = _langList[i] ;
      }
   }

   return 0 ;
}

bool _sortPriority( const pair<int, generateBase*> a,
                    const pair<int, generateBase*> b )
{
   return a.first < b.first ;
}

int generateRunner::run()
{
   int rc = 0 ;
   vector<pair<int, generateBase*> >::iterator it ;

   std::sort( _generatorList.begin(), _generatorList.end(), _sortPriority ) ;

   printLog( PD_INFO ) << "Compile file start, task = " << _generatorList.size()
                       << endl ;

   for( it = _generatorList.begin(); it != _generatorList.end(); ++it )
   {
      int id = 0 ;
      generateBase *gen = (*it).second ;

      gen->setLang( _lang ) ;

      rc = gen->init() ;
      if ( rc )
      {
         printLog( PD_ERROR ) << "Error: Failed to init generator"
                              << ", name = " << gen->name() << endl ;
         goto error ;
      }

      while ( gen->hasNext() )
      {
         string outputPath ;
         fileOutStream fout ;

         rc = gen->outputFile( id, fout, outputPath ) ;
         if ( rc )
         {
            printLog( PD_ERROR ) << "Failed to output file by generator: "
                                 << gen->name() << endl ;
            goto error ;
         }

         if ( getCmdOptions()->getForce() )
         {
            fout.setReplace() ;
         }

         rc = fout.close( outputPath.c_str() ) ;
         if ( -1 == rc )
         {
            printLog( PD_INFO ) << "  * Skip file:     name= " ;
         }
         else
         {
            printLog( PD_INFO ) << "  * Generate file: name= " ;
         }

         printLog( PD_INFO ) << std::left << std::setw( 20 ) << gen->name() ;
         printLog( PD_INFO ) << ", file= "
                             << utilGetRealPath2( outputPath.c_str() )
                             << endl ;
         rc = 0 ;
         ++id ;
      }
   }

   printLog( PD_SERVER ) << "Compile success" << endl ;

done:
   return rc ;
error:
   printLog( PD_SERVER ) << "Compile failed" << endl ;
   goto done ;
}

void generateRunner::_register( int priority, generateBase *generator )
{
   if ( NULL == generator )
   {
      goto done ;
   }

   _generatorList.push_back( make_pair( priority, generator ) ) ;

done:
   return ;
}

generateRunner* getGenerateRunner()
{
   static generateRunner runner ;
   return &runner ;
}

/* generateBase */
generateBase::generateBase() : _lang( LANG_EN )
{
   utilGetCWD2( _rootPath ) ;
}

generateBase::~generateBase()
{
}

const char *generateBase::_getLang( int n )
{
   return _langList[n] ;
}

int generateBase::_langListSize()
{
   return LANG_LIST_SIZE ;
}

void generateBase::setLang( const char *lang )
{
   _lang = lang ;
}

int generateBase::init()
{
   return 0 ;
}

/* _generatorAssit */
_generatorAssit::_generatorAssit( GENERATOR_NEW_FUNC pFunc, int priority )
{
   if ( pFunc )
   {
      generateBase *pCommand = (*pFunc)() ;
      if ( pCommand )
      {
         getGenerateRunner()->_register( priority, pCommand ) ;
      }
   }
}

_generatorAssit::~_generatorAssit ()
{
}


