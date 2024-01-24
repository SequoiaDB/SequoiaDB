#include <mongoc.h>

#include "TestSuite.h"
extern TestSuite suite; 

static void
test_mongoc_client_pool_basic (void)
{
   mongoc_client_pool_t *pool;
   mongoc_client_t *client;
   mongoc_uri_t *uri;

   uri = mongoc_uri_new("mongodb://127.0.0.1?maxpoolsize=1&minpoolsize=1");
   pool = mongoc_client_pool_new(uri);
   client = mongoc_client_pool_pop(pool);
   ASSERT_CMPPTR(client, !=, NULL);
   mongoc_client_pool_push(pool, client);
   mongoc_uri_destroy(uri);
   mongoc_client_pool_destroy(pool);
}


static void
test_mongoc_client_pool_try_pop (void)
{
   mongoc_client_pool_t *pool;
   mongoc_client_t *client;
   mongoc_uri_t *uri;

   uri = mongoc_uri_new("mongodb://127.0.0.1?maxpoolsize=1&minpoolsize=1");
   pool = mongoc_client_pool_new(uri);
   client = mongoc_client_pool_pop(pool);
   ASSERT_CMPPTR(client, !=, NULL);
   ASSERT_CMPPTR(mongoc_client_pool_try_pop(pool), ==, NULL);
   mongoc_client_pool_push(pool, client);
   mongoc_uri_destroy(uri);
   mongoc_client_pool_destroy(pool);
}


void
test_client_pool_install (TestSuite *suite)
{
   TestSuite_Add (suite, "ClientPool_basic", test_mongoc_client_pool_basic);
   TestSuite_Add (suite, "ClientPool_try_pop", test_mongoc_client_pool_try_pop);
}
