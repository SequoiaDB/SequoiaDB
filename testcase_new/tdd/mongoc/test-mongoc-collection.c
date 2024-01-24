#include <bcon.h>
#include <mongoc.h>
#include <mongoc-collection.h>

#include "TestSuite.h"

#include "test-libmongoc.h"
#include "mongoc-tests.h"

static char *gTestUri;
extern TestSuite suite;

static mongoc_database_t *
get_test_database (mongoc_client_t *client)
{
   return mongoc_client_get_database (client, "test");
}


static mongoc_collection_t *
get_test_collection (mongoc_client_t *client,
                     const char      *prefix)
{
   mongoc_collection_t *ret;
   char *str;

   str = gen_collection_name (prefix);
   ret = mongoc_client_get_collection (client, "test", str);
   bson_free (str);

   return ret;
}

void
test_insert (void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_context_t *context;
   bson_error_t error;
   bool r;
   bson_oid_t oid;
   unsigned i;
   bson_t b;
   bson_t q;

   client = mongoc_client_new(gTestUri);
   ASSERT_CMPPTR (NULL, !=, client);

   //collection = get_test_collection (client, "test_insert");
   collection = setUp ("test_insert", client);
   context = bson_context_new(BSON_CONTEXT_NONE);
   ASSERT_CMPPTR (NULL, !=, context);

   for (i = 0; i < 10; i++) {
      bson_init(&b);
      bson_oid_init(&oid, context);
      bson_append_oid(&b, "_id", 3, &oid);
      bson_append_utf8(&b, "hello", 5, "/world", 5);
      r = mongoc_collection_insert(collection, MONGOC_INSERT_NONE, &b, NULL,
                                   &error);
      if (!r) {
         MONGOC_WARNING("%s\n", error.message);
      }
      ASSERT_CMPINT (r, ==, true);
      bson_destroy(&b);
   }


   bson_init(&q);
   bson_append_utf8(&q, "hello", 5, "/world", 5);
   ASSERT_CMPINT(check_records(collection, q, true), ==, 10);
   bson_destroy(&q);
   bson_init (&b);
   BSON_APPEND_INT32 (&b, "$hello", 1);
   r = mongoc_collection_insert (collection, MONGOC_INSERT_NONE, &b, NULL,
                                 &error);
   ASSERT_CMPINT (r, ==, false);
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_BSON);
   ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_BSON_INVALID);

   tearDown (collection);
   bson_context_destroy(context);
   mongoc_client_destroy(client);
}

static void
test_insert_bulk (void)
{
   mongoc_collection_t *collection;
   mongoc_database_t *database;
   mongoc_client_t *client;
   bson_context_t *context;
   bson_error_t error;
   bool r;
   bson_oid_t oid;
   unsigned i;
   bson_t q;
   bson_t b[10];
   bson_t *bptr[10];
   int64_t count = 0;
   mongoc_cursor_t *cursor;
   const bson_t *doc;

   client = mongoc_client_new(gTestUri);
   ASSERT_CMPPTR(NULL, !=, client);

   database = get_test_database (client);
   ASSERT_CMPPTR (database, !=, NULL);

   //collection = get_test_collection (client, "test_insert_bulk");
   collection = setUp("test_insert_bulk", client);
   ASSERT_CMPPTR(collection, !=, NULL);

   //mongoc_collection_drop(collection, &error);

   context = bson_context_new(BSON_CONTEXT_NONE);
   ASSERT_CMPPTR(context, !=, NULL);

   bson_init(&q);
   bson_append_int32(&q, "n", -1, 0);

   for (i = 0; i < 10; i++) {
      bson_init(&b[i]);
      bson_oid_init(&oid, context);
      bson_append_oid(&b[i], "_id", -1, &oid);
      bson_append_int32(&b[i], "n", -1, i % 2);
      bptr[i] = &b[i];
   }

   BEGIN_IGNORE_DEPRECATIONS;
   r = mongoc_collection_insert_bulk (collection, MONGOC_INSERT_NONE,
                                      (const bson_t **)bptr, 10, NULL, &error);
   END_IGNORE_DEPRECATIONS;

   if (!r) {
      MONGOC_WARNING("%s\n", error.message);
   }
   ASSERT_CMPINT (r, ==, true);

   cursor = mongoc_collection_find (collection, MONGOC_QUERY_NONE, 0, 0, 0, &q, NULL, NULL);
   ASSERT_CMPPTR(cursor, !=, NULL);

   while(mongoc_cursor_next(cursor, &doc))
   {
       ++count;
   }
   mongoc_cursor_destroy(cursor); 
   
   //count = mongoc_collection_count (collection, MONGOC_QUERY_NONE, &q, 0, 0, NULL, &error);
   ASSERT_CMPINT ((int)count, ==, 5);

   for (i = 8; i < 10; i++) {
      bson_destroy(&b[i]);
      bson_init(&b[i]);
      bson_oid_init(&oid, context);
      bson_append_oid(&b[i], "_id", -1, &oid);
      bson_append_int32(&b[i], "n", -1, i % 2);
      bptr[i] = &b[i];
   }

   BEGIN_IGNORE_DEPRECATIONS;
   r = mongoc_collection_insert_bulk (collection, MONGOC_INSERT_NONE,
                                      (const bson_t **)bptr, 10, NULL, &error);
   END_IGNORE_DEPRECATIONS;

   ASSERT_CMPINT (r, ==, false);
   ASSERT_CMPINT (error.code, ==, 11000);
   //ASSERT_CMPINT (error.code, ==, -38);
   cursor = mongoc_collection_find (collection, MONGOC_QUERY_NONE, 0, 0, 0, &q, NULL, NULL);
   ASSERT_CMPPTR(cursor, !=, NULL);
   count = 0;
   while(mongoc_cursor_next(cursor, &doc))
   {
       ++count;
   }
   mongoc_cursor_destroy(cursor); 
   //count = mongoc_collection_count (collection, MONGOC_QUERY_NONE, &q, 0, 0, NULL, &error);

   /*
    * MongoDB <2.6 and 2.6 will return different values for this. This is a
    * primary reason that mongoc_collection_insert_bulk() is deprecated.
    * Instead, you should use the new bulk api which will hide the differences
    * for you.  However, since the new bulk API is slower on 2.4 when write
    * concern is needed for inserts, we will support this for a while, albeit
    * deprecated.
    */
//   if (client->cluster.nodes[0].max_wire_version == 0) {
//      ASSERT_CMPINT ((int)count, ==, 6);
//   } else {
      ASSERT_CMPINT ((int)count, ==, 5);
//   }

   BEGIN_IGNORE_DEPRECATIONS;
   r = mongoc_collection_insert_bulk (collection, MONGOC_INSERT_CONTINUE_ON_ERROR,
                                      (const bson_t **)bptr, 10, NULL, &error);
   END_IGNORE_DEPRECATIONS;
   //ASSERT_CMPINT (r, ==, false);
   //ASSERT_CMPINT (error.code, ==, 11000);
   //ASSERT_CMPINT (error.code, ==, -38);
   cursor = mongoc_collection_find (collection, MONGOC_QUERY_NONE, 0, 0, 0, &q, NULL, NULL);
   ASSERT_CMPPTR(cursor, !=, NULL);
   count = 0;
   while(mongoc_cursor_next(cursor, &doc))
   {
       ++count;
   }
   mongoc_cursor_destroy(cursor); 
   //count = mongoc_collection_count (collection, MONGOC_QUERY_NONE, &q, 0, 0, NULL, &error);
   ASSERT_CMPINT ((int)count, ==, 6);

   /* test validate */
   for (i = 0; i < 10; i++) {
      bson_destroy (&b[i]);
      bson_init (&b[i]);
      BSON_APPEND_INT32 (&b[i], "$invalid_dollar_prefixed_name", i);
      bptr[i] = &b[i];
   }
   BEGIN_IGNORE_DEPRECATIONS;
   r = mongoc_collection_insert_bulk (collection, MONGOC_INSERT_NONE,
                                      (const bson_t **)bptr, 10, NULL, &error);
   END_IGNORE_DEPRECATIONS;
   ASSERT_CMPINT (r, ==, false);
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_BSON);
   ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_BSON_INVALID);

   bson_destroy(&q);
   for (i = 0; i < 10; i++) {
      bson_destroy(&b[i]);
   }

   //r = mongoc_collection_drop (collection, &error);
   //ASSERT_CMPINT (r, ==, true);

   //mongoc_collection_destroy(collection);
   tearDown(collection) ;
   mongoc_database_destroy(database);
   bson_context_destroy(context);
   mongoc_client_destroy(client);
}

static void
test_save (void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_context_t *context;
   bson_error_t error;
   bool r;
   bson_oid_t oid;
   unsigned i;
   bson_t b;

   client = mongoc_client_new(gTestUri);
   ASSERT_CMPPTR(NULL, !=, client);


   collection = setUp ("test_save",client);
   ASSERT_CMPPTR(collection, !=, NULL);


   context = bson_context_new(BSON_CONTEXT_NONE);
   ASSERT_CMPPTR(context, !=, NULL);

   for (i = 0; i < 10; i++) {
      bson_init(&b);
      bson_oid_init(&oid, context);
      bson_append_oid(&b, "_id", 3, &oid);
      bson_append_utf8(&b, "hello", 5, "/world", 5);
      r = mongoc_collection_save(collection, &b, NULL, &error);
      if (!r) {
         MONGOC_WARNING("%s\n", error.message);
      }
      ASSERT_CMPINT (r, ==, true);
      bson_destroy(&b);
   }

   bson_destroy (&b);

   tearDown (collection);
   bson_context_destroy(context);
   mongoc_client_destroy(client);
}

static void
test_regex (void)
{
   mongoc_collection_t *collection;
   mongoc_write_concern_t *wr;
   mongoc_client_t *client;
   bson_error_t error = { 0 };
   int64_t count;
   bson_t q = BSON_INITIALIZER;
   bson_t *doc;
   bool r;

   client = mongoc_client_new (gTestUri);
   ASSERT_CMPPTR(NULL, !=, client);

   collection = setUp ("test_regex", client);
   ASSERT_CMPPTR(collection, !=, NULL);

   wr = mongoc_write_concern_new ();
   mongoc_write_concern_set_journal (wr, true);

   doc = BCON_NEW ("hello", "/world");
   r = mongoc_collection_insert (collection, MONGOC_INSERT_NONE, doc, wr, &error);
   ASSERT_CMPINT (r, ==, true);

   BSON_APPEND_REGEX (&q, "hello", "^/wo", "i");

   count = mongoc_collection_count (collection,
                                    MONGOC_QUERY_NONE,
                                    &q,
                                    0,
                                    0,
                                    NULL,
                                    &error);

   ASSERT_CMPINT ((int)count, ==, 1);
   ASSERT_CMPINT(error.domain, ==, 0);


   mongoc_write_concern_destroy (wr);

   tearDown (collection);
   bson_destroy (&q);
   bson_destroy (doc);
   mongoc_client_destroy (client);
}


static void
test_update (void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_context_t *context;
   bson_error_t error;
   bool r;
   bson_oid_t oid;
   unsigned i;
   bson_t b;
   bson_t q;
   bson_t u;
   bson_t set;

   client = mongoc_client_new(gTestUri);
   ASSERT_CMPPTR(NULL, !=, client);

   //collection = get_test_collection (client, "test_update");
   //ASSERT_CMPPTR(collection, !=, NULL);
   collection = setUp("test_update", client);

   context = bson_context_new(BSON_CONTEXT_NONE);
   ASSERT_CMPPTR (context, !=, NULL);

   for (i = 0; i < 10; i++) {
      bson_init(&b);
      bson_oid_init(&oid, context);
      bson_append_oid(&b, "_id", 3, &oid);
      bson_append_utf8(&b, "utf8", 4, "utf8 string", 11);
      bson_append_int32(&b, "int32", 5, 1234);
      bson_append_int64(&b, "int64", 5, 12345678);
      bson_append_bool(&b, "bool", 4, 1);

      r = mongoc_collection_insert(collection, MONGOC_INSERT_NONE, &b, NULL, &error);
      if (!r) {
         MONGOC_WARNING("%s\n", error.message);
      }
      ASSERT_CMPINT (r, ==, true);

      bson_init(&q);
      bson_append_oid(&q, "_id", 3, &oid);

      bson_init(&u);
      bson_append_document_begin(&u, "$set", 4, &set);
      bson_append_utf8(&set, "utf8", 4, "updated", 7);
      bson_append_document_end(&u, &set);

      r = mongoc_collection_update(collection, MONGOC_UPDATE_NONE, &q, &u, NULL, &error);
      if (!r) {
         MONGOC_WARNING("%s\n", error.message);
      }
      ASSERT_CMPINT (r, ==, true);

      bson_destroy(&b);
      bson_destroy(&q);
      bson_destroy(&u);
   }

   bson_init(&q);
   bson_append_utf8(&q, "utf8", 4, "updated", 7);
   ASSERT_CMPINT(check_records(collection, q, true), ==, 10);
   bson_destroy(&q);

   bson_init(&q);
   bson_init(&u);
   BSON_APPEND_INT32 (&u, "abcd", 1);
   BSON_APPEND_INT32 (&u, "$hi", 1);
   r = mongoc_collection_update(collection, MONGOC_UPDATE_NONE, &q, &u, NULL, &error);
   ASSERT_CMPINT (r, ==, false);
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_BSON);
   ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_BSON_INVALID);
   bson_destroy(&q);
   bson_destroy(&u);

   bson_init(&q);
   bson_init(&u);
   BSON_APPEND_INT32 (&u, "a.b.c.d", 1);
   r = mongoc_collection_update(collection, MONGOC_UPDATE_NONE, &q, &u, NULL, &error);
   ASSERT_CMPINT (r, ==, false);
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_BSON);
   ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_BSON_INVALID);
   bson_destroy(&q);
   bson_destroy(&u);

   tearDown(collection);
   bson_context_destroy(context);
   mongoc_client_destroy(client);
}


static void
test_remove (void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_context_t *context;
   bson_error_t error;
   bool r;
   bson_oid_t oid;
   bson_t b;
   int i;
   bson_t q = BSON_INITIALIZER;

   client = mongoc_client_new(gTestUri);
   ASSERT_CMPPTR(NULL, !=, client);

   //collection = get_test_collection (client, "test_remove");
   //ASSERT_CMPPTR(collection, !=, NULL);
   collection = setUp("test_remove", client);

   context = bson_context_new(BSON_CONTEXT_NONE);
   ASSERT_CMPPTR (context,!=, NULL);

   for (i = 0; i < 100; i++) {
      bson_init(&b);
      bson_oid_init(&oid, context);
      bson_append_oid(&b, "_id", 3, &oid);
      bson_append_utf8(&b, "hello", 5, "world", 5);
      r = mongoc_collection_insert(collection, MONGOC_INSERT_NONE, &b, NULL,
                                   &error);
      if (!r) {
         MONGOC_WARNING("%s\n", error.message);
      }
      ASSERT_CMPINT (r, ==, true);
      bson_destroy(&b);

      bson_init(&b);
      bson_append_oid(&b, "_id", 3, &oid);
      r = mongoc_collection_remove(collection, MONGOC_REMOVE_NONE, &b, NULL,
                                   &error);
      if (!r) {
         MONGOC_WARNING("%s\n", error.message);
      }
      ASSERT_CMPINT (r, ==, true);
      bson_destroy(&b);
   }

   ASSERT_CMPINT(check_records(collection, q, true), ==, 0);
   tearDown(collection);
   bson_context_destroy(context);
   mongoc_client_destroy(client);
}

static void
test_index (void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   mongoc_index_opt_t opt;
   bson_error_t error;
   bool r;
   bson_t keys;
   const bson_t *indexinfo = NULL;
   bson_iter_t idx_infos_iter;
   bson_iter_t idx_arr_iter;
   mongoc_cursor_t *cursor;

   mongoc_index_opt_init(&opt);

   client = mongoc_client_new(gTestUri);
   ASSERT_CMPPTR(NULL, !=, client);


   //collection = get_test_collection (client, "test_index");
   //ASSERT_CMPPTR(collection, !=, NULL);
   collection = setUp ("test_index", client);

   bson_init(&keys);
   bson_append_int32(&keys, "hello", -1, 1);
   r = mongoc_collection_create_index(collection, &keys, &opt, &error);
   ASSERT_CMPINT (r, ==, true);


  // r = mongoc_collection_create_index(collection, &keys, &opt, &error);
  // ASSERT_CMPINT (r, ==, true);

   r = mongoc_collection_drop_index(collection, "hello_1", &error);
   ASSERT_CMPINT (r, ==, true);

   bson_destroy(&keys);

   tearDown (collection);
   mongoc_client_destroy(client);
}

static void
test_index_compound (void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   mongoc_index_opt_t opt;
   bson_error_t error;
   bool r;
   bson_t keys;

   mongoc_index_opt_init(&opt);

   client = mongoc_client_new(gTestUri);
   ASSERT_CMPPTR(NULL, !=, client);


   //collection = get_test_collection (client, "test_index_compound");
   //ASSERT_CMPPTR(collection, !=, NULL);
   collection = setUp ("test_index_compound", client);

   bson_init(&keys);
   bson_append_int32(&keys, "hello", -1, 1);
   bson_append_int32(&keys, "world", -1, -1);
   r = mongoc_collection_create_index(collection, &keys, &opt, &error);
   ASSERT_CMPINT (r, ==, true);

   //r = mongoc_collection_create_index(collection, &keys, &opt, &error);
   //ASSERT_CMPINT (r, ==, true);

   r = mongoc_collection_drop_index(collection, "hello_1_world_-1", &error);
   ASSERT_CMPINT (r, ==, true);

   bson_destroy(&keys);

   tearDown(collection);
   mongoc_client_destroy(client);
}
#if 0
static void
test_index_geo (void)
{
   mongoc_collection_t *collection;
   mongoc_database_t *database;
   mongoc_client_t *client;
   mongoc_index_opt_t opt;
   mongoc_index_opt_geo_t geo_opt;
   bson_error_t error;
   bool r;
   bson_t keys;

   mongoc_index_opt_init(&opt);
   mongoc_index_opt_geo_init(&geo_opt);

   client = mongoc_client_new(gTestUri);
   ASSERT_CMPPTR(NULL, !=, client);

   database = get_test_database (client);
   ASSERT_CMPPTR (database, !=, NULL);

   collection = get_test_collection (client, "test_geo_index");
   ASSERT_CMPPTR(collection, !=, NULL);

   /* Create a basic 2d index */
   bson_init(&keys);
   BSON_APPEND_UTF8(&keys, "location", "2d");
   r = mongoc_collection_create_index(collection, &keys, &opt, &error);
   ASSERT_CMPINT (r, ==, true);

   r = mongoc_collection_drop_index(collection, "location_2d", &error);
   ASSERT_CMPINT (r, ==, true);

   /* Create a 2d index with bells and whistles */
   bson_init(&keys);
   BSON_APPEND_UTF8(&keys, "location", "2d");

   geo_opt.twod_location_min = -123;
   geo_opt.twod_location_max = +123;
   geo_opt.twod_bits_precision = 30;
   opt.geo_options = &geo_opt;

   if (client->cluster.nodes [0].max_wire_version > 0) {
      r = mongoc_collection_create_index(collection, &keys, &opt, &error);
         ASSERT_CMPINT (r, ==, true);

      r = mongoc_collection_drop_index(collection, "location_2d", &error);
      ASSERT_CMPINT (r, ==, true);
   }

   /* Create a Haystack index */
   bson_init(&keys);
   BSON_APPEND_UTF8(&keys, "location", "geoHaystack");
   BSON_APPEND_INT32(&keys, "category", 1);

   mongoc_index_opt_geo_init(&geo_opt);
   geo_opt.haystack_bucket_size = 5;
   opt.geo_options = &geo_opt;

   if (client->cluster.nodes [0].max_wire_version > 0) {
      r = mongoc_collection_create_index(collection, &keys, &opt, &error);
      ASSERT_CMPINT (r, ==, true);

      r = mongoc_collection_drop_index(collection, "location_geoHaystack_category_1", &error);
      ASSERT_CMPINT (r, ==, true);
   }

   mongoc_collection_destroy(collection);
   mongoc_database_destroy(database);
   mongoc_client_destroy(client);
}
#endif
static char *
storage_engine (mongoc_client_t *client)
{
   bson_iter_t iter;
   bson_error_t error;
   bson_t cmd = BSON_INITIALIZER;
   bson_t reply;
   bool r;

   /* NOTE: this default will change eventually */
   char *engine = bson_strdup("mmapv1");

   BSON_APPEND_INT32 (&cmd, "getCmdLineOpts", 1);
   r = mongoc_client_command_simple(client, "admin", &cmd, NULL, &reply, &error);
   ASSERT_CMPINT (r, ==, true);

   if (bson_iter_init_find (&iter, &reply, "parsed.storage.engine")) {
      engine = bson_strdup(bson_iter_utf8(&iter, NULL));
   }

   bson_destroy (&reply);
   bson_destroy (&cmd);

   return engine;
}

static void
test_index_storage (void)
{
   mongoc_collection_t *collection = NULL;
   mongoc_client_t *client = NULL;
   mongoc_index_opt_t opt;
   mongoc_index_opt_wt_t wt_opt;
   bson_error_t error;
   bool r;
   bson_t keys;
   char *engine = NULL;

   client = mongoc_client_new (gTestUri);
   ASSERT_CMPPTR(NULL, !=, client);

   /* Skip unless we are on WT */
   engine = storage_engine(client);
   if (strcmp("wiredTiger", engine) != 0) {
      goto cleanup;
   }

   mongoc_index_opt_init (&opt);
   mongoc_index_opt_wt_init (&wt_opt);


   //collection = get_test_collection (client, "test_storage_index");
   //ASSERT_CMPPTR(collection, !=, NULL);
   collection = setUp("test_storage_index", client);

   /* Create a simple index */
   bson_init (&keys);
   bson_append_int32 (&keys, "hello", -1, 1);

   /* Add storage option to the index */
   wt_opt.base.type = MONGOC_INDEX_STORAGE_OPT_WIREDTIGER;
   wt_opt.config_str = "block_compressor=zlib";

   opt.storage_options = (mongoc_index_opt_storage_t *)&wt_opt;

   r = mongoc_collection_create_index (collection, &keys, &opt, &error);
   ASSERT_CMPINT (r, ==, true);

 cleanup:
   if (engine) bson_free (engine);
   if (collection) tearDown (collection);
   if (client) mongoc_client_destroy (client);
}

static void
test_count (void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_error_t error;
   int64_t count;
   bson_t b;


   client = mongoc_client_new(gTestUri);
   ASSERT_CMPPTR(NULL, !=, client);

   //collection = mongoc_client_get_collection(client, "test", "test");
   collection = setUp ("test", client);

   bson_init(&b);
   count = mongoc_collection_count(collection, MONGOC_QUERY_NONE, &b,
                                   0, 0, NULL, &error);
   bson_destroy(&b);

   if (count == -1) {
      MONGOC_WARNING("%s\n", error.message);
   }
   ASSERT_CMPINT ((int)count, ==, 0);

   tearDown (collection);
   mongoc_client_destroy(client);
}


static void
test_count_with_opts (void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_error_t error;
   int64_t count;
   bson_t b;
   bson_t opts;

   client = mongoc_client_new (gTestUri);
   ASSERT_CMPPTR(NULL, !=, client);

   collection = setUp ("test", client);

   bson_init (&opts);

   BSON_APPEND_UTF8 (&opts, "hint", "_id_");

   bson_init (&b);
   count = mongoc_collection_count_with_opts (collection, MONGOC_QUERY_NONE, &b,
                                              0, 0, &opts, NULL, &error);
   bson_destroy (&b);
   bson_destroy (&opts);

   if (count == -1) {
      MONGOC_WARNING ("%s\n", error.message);
   }

   ASSERT_CMPINT ((int)count, !=, -1);

   tearDown (collection);
   mongoc_client_destroy (client);
}


static void
test_drop (void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_error_t error;
   bson_t *doc;
   bool r;

   client = mongoc_client_new(gTestUri);
   ASSERT_CMPPTR(NULL, !=, client);

   collection = setUp ("test_drop", client);
   ASSERT_CMPPTR(collection, !=, NULL);

   doc = BCON_NEW("hello", "world");
   r = mongoc_collection_insert(collection, MONGOC_INSERT_NONE, doc, NULL, &error);
   bson_destroy (doc);
   ASSERT_CMPINT (r, ==, true);

   r = mongoc_collection_drop(collection, &error);
   ASSERT_CMPINT (r, ==, true);

   r = mongoc_collection_drop(collection, &error);
   ASSERT_CMPINT (r, ==, false);
   ASSERT_CMPINT (error.code, ==, -23);

   mongoc_collection_destroy(collection);
   mongoc_client_destroy(client);
}


static void
test_aggregate (void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   mongoc_cursor_t *cursor;
   const bson_t *doc;
   bson_error_t error;
   bool did_alternate = false;
   bool r;
   bson_t opts;
   bson_t *pipeline;
   bson_t *b;
   bson_iter_t iter;
   int i;

   client = mongoc_client_new(gTestUri);
   ASSERT_CMPPTR(NULL, !=, client);

   //collection = get_test_collection (client, "test_aggregate");
   //ASSERT_CMPPTR(collection, !=, NULL);
   collection = setUp ("test_aggregate", client);

   pipeline = BCON_NEW ("pipeline", "[", "{", "$match", "{", "hello", BCON_UTF8 ("world"), "}", "}", "]");
   b = BCON_NEW ("hello", BCON_UTF8 ("world"));

again:
   //mongoc_collection_drop(collection, &error);

   for (i = 0; i < 2; i++) {
      r = mongoc_collection_insert(collection, MONGOC_INSERT_NONE, b, NULL, &error);
      ASSERT_CMPINT (r, ==, true);
   }

   for (i = 0; i < 2; i++) {
      if (i % 2 == 0) {
         cursor = mongoc_collection_aggregate(collection, MONGOC_QUERY_NONE, pipeline, NULL, NULL);
         ASSERT_CMPPTR(NULL, !=, cursor);
      } else {
         bson_init (&opts);
         BSON_APPEND_INT32 (&opts, "batchSize", 10);
         BSON_APPEND_BOOL (&opts, "allowDiskUse", true);

         cursor = mongoc_collection_aggregate(collection, MONGOC_QUERY_NONE, pipeline, &opts, NULL);
         ASSERT_CMPPTR(NULL, !=, cursor);

         bson_destroy (&opts);
      }

      for (i = 0; i < 2; i++) {
         /*
          * This can fail if we are connecting to a 2.0 MongoDB instance.
          */
         r = mongoc_cursor_next(cursor, &doc);
         if (mongoc_cursor_error(cursor, &error)) {
            if ((error.domain == MONGOC_ERROR_QUERY) &&
                (error.code == MONGOC_ERROR_QUERY_COMMAND_NOT_FOUND)) {
               mongoc_cursor_destroy (cursor);
               break;
            }
            MONGOC_WARNING("[%d.%d] %s", error.domain, error.code, error.message);
         }

         ASSERT_CMPINT (r, ==, true);
         ASSERT_CMPPTR (doc, !=, NULL);

         ASSERT_CMPINT (bson_iter_init_find (&iter, doc, "hello") &&
                 BSON_ITER_HOLDS_UTF8 (&iter), ==, true);
      }

      r = mongoc_cursor_next(cursor, &doc);
      if (mongoc_cursor_error(cursor, &error)) {
         MONGOC_WARNING("%s", error.message);
      }
      ASSERT_CMPINT (r, ==, false);
      ASSERT_CMPPTR (doc, ==, NULL);

      mongoc_cursor_destroy(cursor);
   }

   if (!did_alternate) {
      did_alternate = true;
      bson_destroy (pipeline);
      pipeline = BCON_NEW ("0", "{", "$match", "{", "hello", BCON_UTF8 ("world"), "}", "}");
      goto again;
   }

   //r = mongoc_collection_drop(collection, &error);
   //ASSERT_CMPINT (r, ==, true);

   //mongoc_collection_destroy(collection);
   tearDown (collection);
   mongoc_client_destroy(client);
   bson_destroy(b);
   bson_destroy(pipeline);
}


static void
test_validate (void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_iter_t iter;
   bson_error_t error;
   bson_t doc = BSON_INITIALIZER;
   bson_t opts = BSON_INITIALIZER;
   bson_t reply;
   bool r;

   client = mongoc_client_new (gTestUri);
   ASSERT_CMPPTR(NULL, !=, client);

   //collection = get_test_collection (client, "test_validate");
   //ASSERT_CMPPTR(collection, !=, NULL);
   collection = setUp ("test_validate",client);

   r = mongoc_collection_insert(collection, MONGOC_INSERT_NONE, &doc, NULL, &error);
   ASSERT_CMPINT(r, ==, true);

   BSON_APPEND_BOOL (&opts, "full", true);

   r = mongoc_collection_validate (collection, &opts, &reply, &error);
   ASSERT_CMPINT(r, ==, true);

   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "ns"), ==, true);
   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "valid"), ==, true);

   bson_destroy (&reply);

   bson_reinit (&opts);
   BSON_APPEND_UTF8 (&opts, "full", "bad_value");

   r = mongoc_collection_validate (collection, &opts, &reply, &error);
   ASSERT_CMPINT(r, ==, false);
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_BSON);
   ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_BSON_INVALID);

  // r = mongoc_collection_drop (collection, &error);
  // ASSERT_CMPINT(r, ==, true)

   tearDown (collection);
   mongoc_client_destroy (client);
   bson_destroy (&doc);
   bson_destroy (&opts);
}


static void
test_rename (void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_error_t error;
   bson_t doc = BSON_INITIALIZER;
   bool r;

   client = mongoc_client_new (gTestUri);
   ASSERT_CMPPTR(NULL, !=, client);

   //collection = get_test_collection (client, "test_rename");
   //ASSERT_CMPPTR(collection, !=, NULL);
   collection = setUp ("test_rename", client);

   r = mongoc_collection_insert (collection, MONGOC_INSERT_NONE, &doc, NULL, &error);
   ASSERT_CMPINT(r, ==, true);

   r = mongoc_collection_rename (collection, "test", "test_rename_2", false, &error);
   ASSERT_CMPINT(r, ==, true);

   tearDown (collection);
   mongoc_client_destroy (client);
   bson_destroy (&doc);
}


static void
test_stats (void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_error_t error;
   bson_iter_t iter;
   bson_t stats;
   bson_t doc = BSON_INITIALIZER;
   bool r;

   client = mongoc_client_new (gTestUri);
   ASSERT_CMPPTR(NULL, !=, client);

   //collection = get_test_collection (client, "test_stats");
   //ASSERT_CMPPTR(collection, !=, NULL);
   collection = setUp ("test_stats", client);

   r = mongoc_collection_insert (collection, MONGOC_INSERT_NONE, &doc, NULL, &error);
   ASSERT_CMPINT(r, ==, true);

   r = mongoc_collection_stats (collection, NULL, &stats, &error);
   ASSERT_CMPINT(r, ==, true);

   ASSERT_CMPINT (bson_iter_init_find (&iter, &stats, "ns"), ==, true);

   ASSERT_CMPINT (bson_iter_init_find (&iter, &stats, "count"), ==, true);
   ASSERT_CMPINT ((int)bson_iter_as_int64 (&iter), >=, 1);

   bson_destroy (&stats);


   tearDown (collection);
   mongoc_client_destroy (client);
   bson_destroy (&doc);
}


static void
test_find_and_modify (void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_error_t error;
   bson_iter_t iter;
   bson_iter_t citer;
   bson_t *update;
   bson_t doc = BSON_INITIALIZER;
   bson_t reply;
   bool r;

   client = mongoc_client_new (gTestUri);
   ASSERT_CMPPTR(NULL, !=, client);

  // collection = get_test_collection (client, "test_find_and_modify");
  // ASSERT_CMPPTR(collection, !=, NULL);
   collection = setUp ("test_find_and_modify", client);
   BSON_APPEND_INT32 (&doc, "superduper", 77889);

   r = mongoc_collection_insert (collection, MONGOC_INSERT_NONE, &doc, NULL, &error);
   ASSERT_CMPINT(r, ==, true);

   update = BCON_NEW ("$set", "{",
                         "superduper", BCON_INT32 (1234),
                      "}");

   r = mongoc_collection_find_and_modify (collection,
                                          &doc,
                                          NULL,
                                          update,
                                          NULL,
                                          false,
                                          false,
                                          true,
                                          &reply,
                                          &error);
   ASSERT_CMPINT(r, ==, true);

   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "value"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_DOCUMENT (&iter), ==, true);
   ASSERT_CMPINT (bson_iter_recurse (&iter, &citer), ==, true);
   ASSERT_CMPINT (bson_iter_find (&citer, "superduper"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_INT32 (&citer), ==, true);
   ASSERT_CMPINT (bson_iter_int32 (&citer), ==, 1234);

   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "lastErrorObject"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_DOCUMENT (&iter), ==, true);
   ASSERT_CMPINT (bson_iter_recurse (&iter, &citer), ==, true);
   ASSERT_CMPINT (bson_iter_find (&citer, "updatedExisting"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_BOOL (&citer), ==, true);
   ASSERT_CMPINT (bson_iter_bool (&citer), ==, true);

   bson_destroy (&reply);
   bson_destroy (update);


   tearDown (collection);
   mongoc_client_destroy (client);
   bson_destroy (&doc);
}


static void
test_large_return (void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   mongoc_cursor_t *cursor;
   bson_error_t error;
   const bson_t *doc = NULL;
   bson_oid_t oid;
   bson_t insert_doc = BSON_INITIALIZER;
   bson_t query = BSON_INITIALIZER;
   size_t len;
   char *str;
   bool r;
   bson_iter_t iter;

   client = mongoc_client_new (gTestUri);
   ASSERT_CMPPTR(NULL, !=, client);
   mongoc_client_set_read_prefs(client,mongoc_read_prefs_new(MONGOC_READ_PRIMARY));
   //collection = get_test_collection (client, "test_large_return");
   //ASSERT_CMPPTR(collection, !=, NULL);
   collection = setUp ("test_large_return", client);

   len = 1024 * 1024 * 4;
   str = bson_malloc (len);
   memset (str, (int)' ', len);
   str [len - 1] = '\0';

   bson_oid_init (&oid, NULL);
   BSON_APPEND_OID (&insert_doc, "_id", &oid);
   BSON_APPEND_UTF8 (&insert_doc, "big", str);

   r = mongoc_collection_insert (collection, MONGOC_INSERT_NONE, &insert_doc, NULL, &error);
   ASSERT_CMPINT(r, ==, true);

   bson_destroy (&insert_doc);

   BSON_APPEND_OID (&query, "_id", &oid);

   cursor = mongoc_collection_find (collection, MONGOC_QUERY_NONE, 0, 0, 0, &query, NULL, NULL);
   ASSERT_CMPPTR (cursor, !=, NULL);
   bson_destroy (&query);

   r = mongoc_cursor_next (cursor, &doc);
   ASSERT_CMPINT(r, ==, true);
   ASSERT_CMPPTR (doc, !=, NULL);
   if (!( bson_iter_init(&iter, doc)  &&
         bson_iter_find(&iter, "big")&&
         (BSON_TYPE_UTF8 == bson_iter_type(&iter)) ))
   {
      MONGOC_ERROR("record not exist big field" );
      ASSERT_CMPINT(false, !=, false);
   }


   r = mongoc_cursor_next (cursor, &doc);
   ASSERT_CMPINT(r, ==, false);

   mongoc_cursor_destroy (cursor);

   //r = mongoc_collection_drop (collection, &error);
   //if (!r) fprintf (stderr, "ERROR: %s\n", error.message);
   //ASSERT_CMPINT(r, ==, true)

   tearDown (collection);
   mongoc_client_destroy (client);
   bson_free (str);
}


static void
test_many_return (void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   mongoc_cursor_t *cursor;
   bson_error_t error;
   const bson_t *doc = NULL;
   bson_oid_t oid;
   bson_t query = BSON_INITIALIZER;
   bson_t **docs;
   bool r;
   int i;

   client = mongoc_client_new (gTestUri);
   mongoc_client_set_read_prefs(client,mongoc_read_prefs_new(MONGOC_READ_SECONDARY));
   ASSERT_CMPPTR(NULL, !=, client);

   //collection = get_test_collection (client, "test_many_return");
   //ASSERT_CMPPTR(collection, !=, NULL);
   collection = setUp ("test_many_return", client);

   docs = bson_malloc (sizeof(bson_t*) * 5000);

   for (i = 0; i < 5000; i++) {
      docs [i] = bson_new ();
      bson_oid_init (&oid, NULL);
      BSON_APPEND_OID (docs [i], "_id", &oid);
   }

BEGIN_IGNORE_DEPRECATIONS;

   r = mongoc_collection_insert_bulk (collection, MONGOC_INSERT_NONE, (const bson_t **)docs, 5000, NULL, &error);

END_IGNORE_DEPRECATIONS;

   ASSERT_CMPINT(r, ==, true);

   for (i = 0; i < 5000; i++) {
      bson_destroy (docs [i]);
   }

   bson_free (docs);

   ASSERT_CMPINT(check_records(collection, query, true), ==, 5000);
   //cursor = mongoc_collection_find (collection, MONGOC_QUERY_NONE, 0, 0, 6000, &query, NULL, NULL);
   //ASSERT_CMPPTR (cursor, !=, NULL);
   bson_destroy (&query);

   //i = 0;

   //while (mongoc_cursor_next (cursor, &doc)) {
   //   ASSERT_CMPPTR (doc, !=, NULL);
   //   i++;
  // }

  // ASSERT_CMPINT (i, ==, 5000);

  // r = mongoc_cursor_next (cursor, &doc);
  // ASSERT_CMPINT(r, ==, false);

  // mongoc_cursor_destroy (cursor);

   tearDown (collection);
   mongoc_client_destroy (client);
}


static void
test_command_fq (void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   mongoc_cursor_t *cursor;
   const bson_t *doc = NULL;
   bson_t *cmd;
   bool r;

   client = mongoc_client_new (gTestUri);
   ASSERT_CMPPTR(NULL, !=, client);


  // collection = get_test_collection (client, "$cmd.sys.inprog");
   //ASSERT_CMPPTR(collection, !=, NULL);
   collection = setUp ("$cmd.sys.inprog", client);

   cmd = BCON_NEW ("query", "{", "}");

   cursor = mongoc_collection_command (collection, MONGOC_QUERY_NONE, 0, 1, 0, cmd, NULL, NULL);
   r = mongoc_cursor_next (cursor, &doc);
   ASSERT_CMPINT(r, ==, true);

   r = mongoc_cursor_next (cursor, &doc);
   ASSERT_CMPINT(r, ==, false);

   mongoc_cursor_destroy (cursor);
   bson_destroy (cmd);
   tearDown (collection);
   mongoc_client_destroy (client);
}

static void
test_get_index_info (void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   mongoc_index_opt_t opt1;
   mongoc_index_opt_t opt2;
   bson_error_t error = { 0 };
   mongoc_cursor_t *cursor;
   const bson_t *indexinfo;
   bson_t indexkey1;
   bson_t indexkey2;
   bson_t dummy = BSON_INITIALIZER;
   bson_iter_t idx_spec_iter;
   bson_iter_t idx_spec_iter_copy;
   bson_iter_t child;
   bool r;
   const char *cur_idx_name;
   char *idx1_name = NULL;
   char *idx2_name = NULL;
   const char *id_idx_name = "$id";
   int num_idxs = 0;

   client = mongoc_client_new (gTestUri);
   ASSERT_CMPPTR(NULL, !=, client);

   //collection = get_test_collection (client, "test_get_index_info");
   //ASSERT_CMPPTR(collection, !=, NULL);
   //collection = setUp ("test_get_index_info", client);

   /*
    * Try it on a collection that doesn't exist.
    */
   //cursor = mongoc_collection_find_indexes (collection, &error);

   //ASSERT_CMPPTR(NULL, !=, cursor);
   //ASSERT_CMPINT(error.domain, ==, 0);
   //ASSERT_CMPINT (error.code, ==, 0);

   //r = mongoc_cursor_next( cursor, &indexinfo );

   //ASSERT_CMPINT (mongoc_cursor_next( cursor, &indexinfo ), ==, false);


   //mongoc_cursor_destroy (cursor);

   collection = setUp ("test_get_index_info", client);
   /* insert a dummy document so that the collection actually exists */
   r = mongoc_collection_insert (collection, MONGOC_INSERT_NONE, &dummy, NULL,
                                 &error);
   ASSERT_CMPINT (r, ==, true);

   /* Try it on a collection with no secondary indexes.
    * We should just get back the index on _id.
    */
   cursor = mongoc_collection_find_indexes (collection, &error);
   ASSERT_CMPPTR(NULL, !=, cursor);
   ASSERT_CMPINT(error.domain, ==, 0);
   ASSERT_CMPINT (error.code, ==, 0);

   while (mongoc_cursor_next (cursor, &indexinfo)) {
      if (bson_iter_init (&idx_spec_iter, indexinfo) &&
          bson_iter_find (&idx_spec_iter, "name") &&
          BSON_ITER_HOLDS_UTF8 (&idx_spec_iter) &&
          (cur_idx_name = bson_iter_utf8 (&idx_spec_iter, NULL))) {
         ASSERT_CMPINT (0, ==, strcmp (cur_idx_name, id_idx_name));
         ++num_idxs;
      } else {
         MONGOC_ERROR("index is: %s", (char*)bson_as_json(indexinfo, 0));
         ASSERT_CMPINT(false, !=, false);
      }
   }

   ASSERT_CMPINT (1, ==, num_idxs);

   mongoc_cursor_destroy (cursor);

   num_idxs = 0;
   indexinfo = NULL;

   bson_init (&indexkey1);
   BSON_APPEND_INT32 (&indexkey1, "rasberry", 1);
   idx1_name = mongoc_collection_keys_to_index_string (&indexkey1);
   mongoc_index_opt_init (&opt1);
   opt1.background = true;
   r = mongoc_collection_create_index (collection, &indexkey1, &opt1, &error);
   ASSERT_CMPINT (r, ==, true);

   bson_init (&indexkey2);
   BSON_APPEND_INT32 (&indexkey2, "snozzberry", 1);
   idx2_name = mongoc_collection_keys_to_index_string (&indexkey2);
   mongoc_index_opt_init (&opt2);
   opt2.unique = true;
   r = mongoc_collection_create_index (collection, &indexkey2, &opt2, &error);
   ASSERT_CMPINT (r, ==, true);

   /*
    * Now we try again after creating two indexes.
    */
   cursor = mongoc_collection_find_indexes (collection, &error);
   ASSERT_CMPPTR(NULL, !=, cursor);
   ASSERT_CMPINT(error.domain, ==, 0);
   ASSERT_CMPINT (error.code, ==, 0);

   while (mongoc_cursor_next (cursor, &indexinfo)) {
      if (bson_iter_init (&idx_spec_iter, indexinfo) &&
          bson_iter_find (&idx_spec_iter, "name") &&
          BSON_ITER_HOLDS_UTF8 (&idx_spec_iter) &&
          (cur_idx_name = bson_iter_utf8 (&idx_spec_iter, NULL))) {
         if (0 == strcmp (cur_idx_name, idx1_name)) {
            /* need to use the copy of the iter since idx_spec_iter may have gone
             * past the key we want */
            //ASSERT_CMPINT (bson_iter_init_find (&idx_spec_iter_copy, indexinfo, "background"), ==, true);
            //ASSERT_CMPINT (BSON_ITER_HOLDS_BOOL (&idx_spec_iter_copy), ==, true);
            //ASSERT_CMPINT (bson_iter_bool (&idx_spec_iter_copy), ==, true);
         } else if (0 == strcmp (cur_idx_name, idx2_name)) {
            ASSERT_CMPINT (bson_iter_find (&idx_spec_iter, "unique"), ==, true);
            ASSERT_CMPINT (BSON_ITER_HOLDS_BOOL (&idx_spec_iter), ==, true);
            ASSERT_CMPINT (bson_iter_bool (&idx_spec_iter), ==, true);
         } else {
            ASSERT_CMPINT (0, ==, strcmp (cur_idx_name, id_idx_name));
         }

         ++num_idxs;
      } else {
         MONGOC_ERROR("current index is: %s", (char*)bson_as_json(indexinfo, 0));
         ASSERT_CMPINT(false, !=, false);
      }
   }

   ASSERT_CMPINT (3, ==, num_idxs);

   mongoc_cursor_destroy (cursor);

   bson_free (idx1_name);
   bson_free (idx2_name);

   tearDown (collection);
   mongoc_client_destroy (client);
}

static void
test_find_one(void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_error_t error = { 0 };
   mongoc_cursor_t *cursor;
   bool r;
   bson_t b;
   bson_context_t *context;
   bson_oid_t oid;
   bson_t q = BSON_INITIALIZER;
   const bson_t *doc = NULL;
   int i;
   bson_iter_t iter;

   client = mongoc_client_new (gTestUri);
   ASSERT_CMPPTR(NULL, !=, client);

   collection = setUp("test_find_one", client);
   context = bson_context_new(BSON_CONTEXT_NONE);
   for (i = 0; i < 10; i++) {
      bson_init(&b);
      bson_oid_init(&oid, context);
      bson_append_oid(&b, "_id", 3, &oid);
      bson_append_utf8(&b, "hello", 5, "/world", 5);
      r = mongoc_collection_insert(collection, MONGOC_INSERT_NONE, &b, NULL,
                                   &error);
      if (!r) {
         MONGOC_WARNING("%s\n", error.message);
      }
      ASSERT_CMPINT (r, ==, true);
      bson_destroy(&b);
   }

   cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, -1, 0, &q, NULL, NULL);
   ASSERT_CMPPTR (cursor, !=, NULL);

   r = mongoc_cursor_next (cursor, &doc);
   ASSERT_CMPINT(r, ==, true);
   ASSERT_CMPPTR (doc, !=, NULL);
   if (!( bson_iter_init(&iter, doc)  &&
         bson_iter_find(&iter, "_id") &&
         (BSON_TYPE_OID == bson_iter_type(&iter))))
   {
      MONGOC_WARNING("find record is %s\n", (char*)bson_as_json(doc,0));
      ASSERT_CMPINT(false, !=, false);
   }

   r = mongoc_cursor_next (cursor, &doc);
   ASSERT_CMPINT(r, ==, false);

   mongoc_cursor_destroy (cursor);

   tearDown (collection);
   mongoc_client_destroy (client);
}

static void
test_order(void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_error_t error = { 0 };
   mongoc_cursor_t *cursor;
   bool r;
   bson_t b;
   bson_context_t *context;
   bson_oid_t oid;
   bson_t q = BSON_INITIALIZER;
   const bson_t *doc = NULL;
   int i;
   bson_iter_t iter;
   bson_t query;
   bson_t child;

   client = mongoc_client_new (gTestUri);
   ASSERT_CMPPTR(NULL, !=, client);

   collection = setUp("test_order", client);
   for (i = 0; i < 10; i++) {
      bson_init(&b);
      bson_append_int32(&b, "_id", 3, i);
      bson_append_int32(&b, "a", 1, i);
      bson_append_utf8(&b, "hello", 5, "/world", 5);
      r = mongoc_collection_insert(collection, MONGOC_INSERT_NONE, &b, NULL,
                                   &error);
      if (!r) {
         MONGOC_WARNING("%s\n", error.message);
      }
      ASSERT_CMPINT (r, ==, true);
      bson_destroy(&b);
   }

   bson_init(&q);
   bson_append_document_begin (&q, "$orderby", -1, &child);
   bson_append_int32 (&child, "a", -1, 1);
   bson_append_document_end (&q, &child);
   bson_append_document_begin (&q, "$query", -1, &child);
   bson_append_document_end (&q, &child);

   cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 1, 0, &q, NULL, NULL);
   ASSERT_CMPPTR (cursor, !=, NULL);
   bson_destroy(&q);

   r = mongoc_cursor_next (cursor, &doc);
   ASSERT_CMPINT(r, ==, true);
   ASSERT_CMPPTR (doc, !=, NULL);
   if (!( bson_iter_init(&iter, doc)  &&
         bson_iter_find(&iter, "a") &&
         (bson_iter_int32(&iter)) == 0))
   {
      MONGOC_WARNING("sort({a:1})%s\n", (char*)bson_as_json(doc, 0));
      ASSERT_CMPINT(false, !=, false);
   }

   mongoc_cursor_destroy (cursor);

   bson_init(&q);
   bson_append_document_begin (&q, "$orderby", -1, &child);
   bson_append_int32 (&child, "a", -1, -1);
   bson_append_document_end (&q, &child);
   bson_append_document_begin (&q, "$query", -1, &child);
   bson_append_document_end (&q, &child);
   cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 1, 0, &q, NULL, NULL);
   ASSERT_CMPPTR (cursor, !=, NULL);

   r = mongoc_cursor_next (cursor, &doc);
   ASSERT_CMPINT(r, ==, true);
   ASSERT_CMPPTR (doc, !=, NULL);
   if (!( bson_iter_init(&iter, doc) &&
          bson_iter_find(&iter, "a") &&
         (bson_iter_int32(&iter)) == 9))
   {
      MONGOC_WARNING("sort({a:-1})%s\n", (char*)bson_as_json(doc, 0));
      ASSERT_CMPINT(false, !=, false);
   }
   bson_destroy(&q);
   mongoc_cursor_destroy (cursor);
   tearDown (collection);
   mongoc_client_destroy (client);
}

static void test_explain()
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_error_t error = { 0 };
   mongoc_cursor_t *cursor;
   bool r;
   bson_context_t *context;
   bson_t q = BSON_INITIALIZER;
   const bson_t *doc = NULL;
   bson_iter_t iter;
   bson_t query;
   bson_t child;

   client = mongoc_client_new (gTestUri);
   ASSERT_CMPPTR(NULL, !=, client);

   collection = setUp("test_explain", client);

   bson_init(&q);
   bson_append_document_begin (&q, "$query", -1, &child);
   bson_append_document_end (&q, &child);
   bson_append_bool(&q, "$explain", -1, true);
   cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 1, 0, &q, NULL, NULL);
   ASSERT_CMPPTR (cursor, !=, NULL);

   r = mongoc_cursor_next (cursor, &doc);
   ASSERT_CMPINT(r, ==, true);
   ASSERT_CMPPTR (doc, !=, NULL);

   if (!(bson_iter_init_find(&iter, doc, "Name") &&
       (bson_iter_type(&iter) == BSON_TYPE_UTF8)))
   {
      MONGOC_WARNING("explain():%s\n", (char*)bson_as_json(doc, 0));
      ASSERT_CMPINT(false, !=, false);
   }

    if (!(bson_iter_init_find(&iter, doc, "ScanType") &&
       (bson_iter_type(&iter) == BSON_TYPE_UTF8)))
   {
      MONGOC_WARNING("explain():%s\n", (char*)bson_as_json(doc, 0));
      ASSERT_CMPINT(false, !=, false);
   }

   bson_destroy(&q);
   mongoc_cursor_destroy (cursor);
   tearDown (collection);
   mongoc_client_destroy (client);
}

static void
cleanup_globals (void)
{
   bson_free (gTestUri);
}


void
test_collection_install (TestSuite *suite)
{
   gTestUri = bson_strdup_printf("mongodb://%s/", MONGOC_TEST_HOST);

   //TestSuite_Add (suite, "Collection_insert_bulk", test_insert_bulk);
   TestSuite_Add (suite, "Collection_insert", test_insert);
   TestSuite_Add (suite, "Collection_save", test_save);
   TestSuite_Add (suite, "Collection_index", test_index);
   TestSuite_Add (suite, "Collection_index_compound", test_index_compound);
   //TestSuite_Add (suite, "Collection_index_geo", test_index_geo);
   //TestSuite_Add (suite, "Collection_index_storage", test_index_storage);
   TestSuite_Add (suite, "Collection_regex", test_regex);
   TestSuite_Add (suite, "Collection_update", test_update);
   TestSuite_Add (suite, "Collection_remove", test_remove);
   TestSuite_Add (suite, "Collection_count", test_count);
   TestSuite_Add (suite, "Collection_count_with_opts", test_count_with_opts);
   TestSuite_Add (suite, "Collection_drop", test_drop);
   // TestSuite_Add (suite, "Collection_aggregate", test_aggregate);
   //TestSuite_Add (suite, "Collection_validate", test_validate);
   //TestSuite_Add (suite, "Collection_rename", test_rename);
   //TestSuite_Add (suite, "Collection_stats", test_stats);
   //TestSuite_Add (suite, "Collection_find_and_modify", test_find_and_modify);
   TestSuite_Add (suite, "Collection_large_return", test_large_return);
   TestSuite_Add (suite, "Collection_many_return", test_many_return);
   //TestSuite_Add (suite, "Collection_command_fully_qualified", test_command_fq);
   TestSuite_Add (suite, "Collection_get_index_info", test_get_index_info);
   TestSuite_Add (suite, "Collection_find_one", test_find_one);
   TestSuite_Add (suite, "Collection_order", test_order);
   TestSuite_Add (suite, "Collection_explain", test_explain);
   atexit (cleanup_globals);
}
