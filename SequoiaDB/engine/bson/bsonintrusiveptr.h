#ifndef BSONINTRUSIVEPTR_HPP__
#define BSONINTRUSIVEPTR_HPP__

namespace bson
{
   template<class T, typename Allocator >
   class bson_intrusive_ptr
   {
   private :
      typedef bson_intrusive_ptr this_type ;
      Allocator al ;
   public :
      typedef T element_type ;
      bson_intrusive_ptr(): px(0)
      {}
      bson_intrusive_ptr( T * p, bool add_ref = true ) : px (p)
      {
         if ( px != 0 && add_ref ) px->addRef() ;
      }
      bson_intrusive_ptr( const bson_intrusive_ptr &rhs ) : px ( rhs.px )
      {
         if ( px != 0 ) px->addRef() ;
      }

      template< typename Allocator1 >
      bson_intrusive_ptr( const bson_intrusive_ptr<T,Allocator1> &rhs ) : px(0)
      {
         if ( rhs.get() )
           if ( 0 != makeFrom( rhs->dataptr(), rhs->datasize() ) )
              msgasserted( 16000, "alloc out-of-memory", true ) ;
      }

      ~bson_intrusive_ptr ()
      {
         if ( px != 0 )
            if ( 0 == px->decRef() )
               al.Free( (void*)px ) ;
      }
      bson_intrusive_ptr & operator=( const bson_intrusive_ptr &rhs )
      {
         this_type(rhs).swap(*this);
         return *this;
      }

      template< typename Allocator1 >
      bson_intrusive_ptr& operator=( const bson_intrusive_ptr<T,Allocator1> &rhs )
      {
         if ( rhs.get() )
         {
            if ( 0 != makeFrom( rhs->dataptr(), rhs->datasize() ) )
               msgasserted( 16001, "alloc out-of-memory", true ) ;
         }
         else
         {
            reset() ;
         }
         return *this ;
      }

      bson_intrusive_ptr & operator=(T * rhs)
      {
         this_type(rhs).swap(*this);
         return *this;
      }
      void reset()
      {
         this_type().swap( *this );
      }
      void reset( T * rhs )
      {
         this_type( rhs ).swap( *this );
      }
      T * get() const
      {
         return px;
      }
      T & operator*() const
      {
         return *px;
      }
      T * operator->() const
      {
         return px;
      }
      void swap( bson_intrusive_ptr & rhs )
      {
         T * tmp = px;
         px = rhs.px;
         rhs.px = tmp;
      }
      int makeFrom( const char* rawdata, int len )
      {
         int refLen = T::refLen() ;
         if ( rawdata && refLen + len >= (int)sizeof( T ) )
         {
            T *p = (T*)al.Malloc( refLen + len ) ;
            if ( p )
            {
               p->zeroRef() ;
               memcpy( (char*)p + refLen, rawdata, len ) ;
               reset( p ) ;
            }
            else
            {
               return -1 ;
            }
         }
         else
         {
            reset() ;
         }
         return 0 ;
      }
   private:
      T * px;
   } ;
}

#endif // BSONINTRUSIVEPTR_HPP__
