#ifndef SDB_UTIL__H
#define SDB_UTIL__H

#include <pthread.h>
#include <time.h>
#include <mysql/psi/mysql_file.h>

int sdb_parse_table_name( const char * from,
                          char *db_name, int db_name_size,
                          char *table_name, int table_name_size ) ;
class sdb_lock_time_out
{
public:

   ~sdb_lock_time_out(){}

   static sdb_lock_time_out *get_instance()
   {
      static sdb_lock_time_out _time_out ;
      return &_time_out ;
   }

   const struct timespec *get_time()
   {
      return &time_out ;
   }

private:
   sdb_lock_time_out()
   {
      clock_gettime( CLOCK_REALTIME, &time_out ) ;
      time_out.tv_sec += 3600*24*365*10 ;
   }
   sdb_lock_time_out( const sdb_lock_time_out &rh ){}
   sdb_lock_time_out & operator = ( const sdb_lock_time_out & rh)
   {
      return *this ;
   }

private:
   struct timespec                  time_out ;
};

#define SDB_LOCK_TIMEOUT sdb_lock_time_out::get_instance()->get_time()

class sdb_rw_lock_r
{
private:
   pthread_rwlock_t*      rw_mutex ;

public:
   sdb_rw_lock_r( pthread_rwlock_t *var_lock )
      :rw_mutex(NULL)
   {
      if ( var_lock )
      {
         while( TRUE )
         {
            int rc = pthread_rwlock_timedrdlock( var_lock,
                                                 SDB_LOCK_TIMEOUT ) ;
            if ( 0 == rc )
            {
               rw_mutex = var_lock ;
            }
            else if ( EDEADLK != rc )
            {
               continue ;
            }
            else
            {
               assert( FALSE ) ;
            }
            break ;
         }
      }
   }

   ~sdb_rw_lock_r()
   {
      if ( rw_mutex )
      {
         pthread_rwlock_unlock( rw_mutex ) ;
      }
   }
};

class sdb_rw_lock_w
{
private:
   pthread_rwlock_t*      rw_mutex ;

public:
   sdb_rw_lock_w( pthread_rwlock_t *var_lock )
      :rw_mutex(NULL)
   {
      if ( var_lock )
      {
         while( TRUE )
         {
            int rc = pthread_rwlock_timedwrlock( var_lock,
                                                 SDB_LOCK_TIMEOUT ) ;
            if ( 0 == rc )
            {
               rw_mutex = var_lock ;
            }
            else if ( EDEADLK != rc )
            {
               continue ;
            }
            else
            {
               assert( FALSE ) ;
            }
            break ;
         }
      }
   }

   ~sdb_rw_lock_w()
   {
      if ( rw_mutex )
      {
         pthread_rwlock_unlock( this->rw_mutex ) ;
      }
   }
};

#endif
