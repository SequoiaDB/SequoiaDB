#ifndef SDB_CL_PTR__H
#define SDB_CL_PTR__H

class sdb_cl_auto_ptr ;
class sdb_cl ;
class sdb_cl_ref_ptr
{
public:

   friend class sdb_cl_auto_ptr ;

protected:

   sdb_cl_ref_ptr( sdb_cl *collection ) ;

   virtual ~sdb_cl_ref_ptr() ;

protected:
   sdb_cl                           *sdb_collection ;
   Atomic_int32                     ref ; 
} ;

class sdb_cl_auto_ptr
{
public:

   sdb_cl_auto_ptr() ;

   virtual ~sdb_cl_auto_ptr() ;

   sdb_cl_auto_ptr( sdb_cl *collection ) ;

   sdb_cl_auto_ptr( const sdb_cl_auto_ptr &other ) ;

   sdb_cl_auto_ptr & operator = ( sdb_cl_auto_ptr &other ) ;

   sdb_cl& operator *() ;

   sdb_cl* operator ->() ;

   int ref() ;

   void clear() ;

private:
   sdb_cl_ref_ptr                   *ref_ptr ;
} ;
#endif