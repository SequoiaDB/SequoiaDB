#ifndef UTIL_SHARED_PTR_HELPER__
#define UTIL_SHARED_PTR_HELPER__

#include <boost/shared_ptr.hpp>

namespace engine
{
   template < typename T > struct SHARED_TYPE_LESS
   {
      typedef boost::shared_ptr< T > SHARED_TYPE;
      bool operator()( const SHARED_TYPE &lhs, const SHARED_TYPE &rhs ) const
      {
         if ( lhs.get() == rhs.get() )
         {
            return false;
         }
         return ( *lhs ) < ( *rhs );
      }
   };

   template < typename T > class SHARED_TYPE_EQUAL
   {
      typedef boost::shared_ptr< T > SHARED_TYPE;

   public:
      SHARED_TYPE_EQUAL( const SHARED_TYPE &outer ) : _outer( outer ) {}
      bool operator()( const SHARED_TYPE &value ) const
      {
         return *value == *_outer;
      }

   private:
      SHARED_TYPE _outer;
   };
} // namespace engine

#endif