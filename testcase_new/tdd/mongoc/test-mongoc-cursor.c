#include <mongoc.h>
//#include <mongoc-cursor-private.h>

#include "TestSuite.h"
#include "test-libmongoc.h"

extern TestSuite suite;
static void
test_get_host (void)
{
   const mongoc_host_list_t *hosts;
   mongoc_collection_t *collection;
   mongoc_host_list_t host;
   mongoc_client_t *client;
   mongoc_cursor_t *cursor;
   mongoc_uri_t *uri;
   const bson_t *doc;
   bson_error_t error;
   bool r;
   bson_t q = BSON_INITIALIZER;
   char *uristr;
   char *csfullname;

   uristr = bson_strdup_printf("mongodb://%s/", MONGOC_TEST_HOST);
   uri = mongoc_uri_new(uristr);
   bson_free(uristr);

   hosts = mongoc_uri_get_hosts(uri);
   
   client = mongoc_client_new_from_uri(uri);
   collection = setUp ("test", client);
   cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 1, 1,
                                   &q, NULL, NULL);
   r = mongoc_cursor_next(cursor, &doc);
   if (!r && mongoc_cursor_error(cursor, &error)) {
      MONGOC_ERROR("empty cl query. %s", error.message);
      abort();
   }

   ASSERT_CMPINT(r, ==, false);
   ASSERT_CMPINT( bson_empty0(doc), ==, true);
   ASSERT_CMPPTR (doc, ==, mongoc_cursor_current (cursor));

   mongoc_cursor_get_host(cursor, &host);
   ASSERT_CMPSTR (host.host, hosts->host);
   ASSERT_CMPSTR (host.host_and_port, hosts->host_and_port);
   ASSERT_CMPINT (host.port, ==, hosts->port);
   ASSERT_CMPINT (host.family, ==, hosts->family);

   mongoc_uri_destroy(uri);
   tearDown(collection);
   mongoc_client_destroy (client);
   mongoc_cursor_destroy (cursor);
}

static void
test_clone (void)
{
   mongoc_cursor_t *clone;
   mongoc_cursor_t *cursor;
   mongoc_client_t *client;
   const bson_t *doc1;
   const bson_t *doc2;
   bson_error_t error;
   mongoc_uri_t *uri;
   bson_iter_t iter;
   bool r;
   bson_t q = BSON_INITIALIZER;
   char *uristr;
   mongoc_collection_t *col;
   mongoc_database_t *database; 
   char *csfullname;

   uristr = bson_strdup_printf("mongodb://%s/", MONGOC_TEST_HOST);
   uri = mongoc_uri_new(uristr);
   bson_free(uristr);

   client = mongoc_client_new_from_uri(uri);
   ASSERT_CMPPTR(client, !=, NULL);

   {
      /*
       * Ensure test.test has a document.
       */

      //mongoc_collection_t *col;

      //database = mongoc_client_get_database (client, "test");
      //ASSERT_CMPPTR(database, !=, NULL);

      //char *str;
      //str = gen_collection_name ("test");      
      //col = create_collection(str, database);
      col = setUp("test", client);
      //col = mongoc_client_get_collection (client, "test", "test");
      r = mongoc_collection_insert (col, MONGOC_INSERT_NONE, &q, NULL, &error);
      ASSERT_CMPINT (r, ==, true);

      //mongoc_collection_destroy (col);
   }

   cursor = mongoc_collection_find(col, MONGOC_QUERY_NONE, 0, 1, 1,
                                   &q, NULL, NULL);
   ASSERT_CMPPTR(cursor, !=, NULL);

   r = mongoc_cursor_next(cursor, &doc1);
   if (!r || mongoc_cursor_error(cursor, &error)) {
      MONGOC_ERROR("%s", error.message);
      abort();
   }
   ASSERT_CMPPTR (doc1, !=, NULL);
   if (!( bson_iter_init(&iter, doc1)  &&
        bson_iter_find(&iter, "_id") &&
        (BSON_TYPE_OID == bson_iter_type(&iter)) ))
   {
      MONGOC_ERROR("find record is %s", (char*)bson_as_json(doc1,0));
      ASSERT_CMPINT(false, !=, false);
   }

   clone = mongoc_cursor_clone(cursor);
   ASSERT_CMPPTR(cursor, !=, NULL);

   r = mongoc_cursor_next(clone, &doc2);
   if (!r || mongoc_cursor_error(clone, &error)) {
      MONGOC_ERROR("%s", error.message);
      abort();
   }
   ASSERT_CMPPTR (doc2, !=, NULL);
   ASSERT_CMPINT(bson_compare(doc1,doc2), ==, 0 ); 

   tearDown(col);
   mongoc_cursor_destroy(cursor);
   mongoc_cursor_destroy(clone);
  // mongoc_collection_destroy(col);
   
   mongoc_client_destroy(client);
   mongoc_uri_destroy(uri);
}


static void
test_invalid_query (void)
{
   mongoc_client_t *client;
   mongoc_cursor_t *cursor;
   mongoc_uri_t *uri;
   bson_error_t error;
   const bson_t *doc = NULL;
   bson_t *q;
   bool r;
   char *uristr;
   mongoc_collection_t *col;
   mongoc_database_t *database;

   uristr = bson_strdup_printf("mongodb://%s/", MONGOC_TEST_HOST);
   uri = mongoc_uri_new(uristr);
   bson_free(uristr);

   client = mongoc_client_new_from_uri (uri);
   ASSERT_CMPPTR (client, !=, NULL);

   database = mongoc_client_get_database (client, "test");
   ASSERT_CMPPTR(database, !=, NULL);

   char *str;
   str = gen_collection_name ("test");
   col = create_collection(str, database);
   ASSERT_CMPPTR(col, !=, NULL);
   free(str);
   q = BCON_NEW ("foo", BCON_INT32 (1), "$orderby", "{", "}");

   cursor = mongoc_collection_find (col, MONGOC_QUERY_NONE, 0, 1, 1,
                                     q, NULL, NULL);
   r = mongoc_cursor_next (cursor, &doc);
   ASSERT_CMPINT(r, ==, false);
   mongoc_cursor_error (cursor, &error);
   ASSERT_CMPPTR (strstr (error.message, "$query"), !=, NULL);
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_CURSOR);
   ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_CURSOR_INVALID_CURSOR);
   ASSERT_CMPPTR (doc, ==, NULL);

   bson_destroy (q);
   mongoc_collection_drop(col, &error);
   mongoc_database_destroy (database);
   mongoc_collection_destroy (col);
   mongoc_cursor_destroy (cursor);
   mongoc_client_destroy (client);
   mongoc_uri_destroy(uri);
}


void
test_cursor_install (TestSuite *suite)
{
   TestSuite_Add (suite, "Cursor_get_host", test_get_host);
   TestSuite_Add (suite, "Cursor_clone", test_clone);
   TestSuite_Add (suite, "Cursor_invalid_query", test_invalid_query);
}
