#ifndef SDB_CONN_PTR__H
#define SDB_CONN_PTR__H

#include <atomic_class.h>

class sdb_conn ;
class sdb_conn_auto_ptr ;

class sdb_conn_ref_ptr
{
public:

   friend class sdb_conn_auto_ptr ;

protected:

   sdb_conn_ref_ptr( sdb_conn *connection ) ;

   virtual ~sdb_conn_ref_ptr() ;

protected:
   sdb_conn                         *sdb_connection ;
   Atomic_int32                     ref ; 
} ;

class sdb_conn_auto_ptr
{
public:

   sdb_conn_auto_ptr() ;

   virtual ~sdb_conn_auto_ptr() ;

   sdb_conn_auto_ptr( sdb_conn *connection ) ;

   sdb_conn_auto_ptr( const sdb_conn_auto_ptr &other ) ;

   sdb_conn_auto_ptr & operator = ( sdb_conn_auto_ptr &other ) ;

   sdb_conn& operator *() ;

   sdb_conn* operator ->() ;

   int ref() ;

   void clear() ;

private:
   sdb_conn_ref_ptr                 *ref_ptr ;
} ;

#endif