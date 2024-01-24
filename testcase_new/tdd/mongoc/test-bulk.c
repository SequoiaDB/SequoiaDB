#include <bcon.h>
#include <mongoc.h>

#include "TestSuite.h"

#include "test-libmongoc.h"
#include "mongoc-tests.h"

static char *gTestUri;
extern TestSuite suite;

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

static void
cleanup_globals (void)
{
   bson_free (gTestUri);
}

static void
test_bulk (void)
{
   mongoc_bulk_operation_t *bulk;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_error_t error;
   bson_iter_t iter;
   bson_t reply;
   bson_t child;
   bson_t del;
   bson_t up;
   bson_t doc = BSON_INITIALIZER;
   bson_t q   = BSON_INITIALIZER;
   bool r;

   client = mongoc_client_new (gTestUri);
   ASSERT_CMPPTR (client, !=, NULL);

   //collection = get_test_collection (client, "test_bulk");
   //ASSERT_CMPPTR (collection, !=, NULL);
   collection = setUp("test_bulk", client);

   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   ASSERT_CMPPTR (bulk, !=, NULL);

   mongoc_bulk_operation_insert (bulk, &doc);
   mongoc_bulk_operation_insert (bulk, &doc);
   mongoc_bulk_operation_insert (bulk, &doc);
   mongoc_bulk_operation_insert (bulk, &doc);

   bson_init (&up);
   bson_append_document_begin (&up, "$set", -1, &child);
   bson_append_int32 (&child, "hello", -1, 123);
   bson_append_document_end (&up, &child);
   mongoc_bulk_operation_update (bulk, &doc, &up, false);
   bson_destroy (&up);

   //bson_init (&del);
   //BSON_APPEND_INT32 (&del, "hello", 123);
   //mongoc_bulk_operation_remove (bulk, &del);
   //bson_destroy (&del);
   r = mongoc_bulk_operation_execute (bulk, &reply, &error);
   ASSERT_CMPINT(r, !=, false);

   BSON_APPEND_INT32(&q, "hello", 123);
   int recordnum = check_records(collection, q, true);
   ASSERT_CMPINT(recordnum, ==, 4);

   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "nInserted"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_INT32 (&iter), ==, true);
#if 0
   ASSERT_CMPINT (bson_iter_int32 (&iter), ==, 4);
#endif
   /*
    * This may be omitted if we talked to a (<= 2.4.x) node, or a mongos
    * talked to a (<= 2.4.x) node.
    */
   if (bson_iter_init_find (&iter, &reply, "nModified")) {
      ASSERT_CMPINT (BSON_ITER_HOLDS_INT32 (&iter), ==, true);
#if 0
      ASSERT_CMPINT (bson_iter_int32 (&iter), ==, 4);
#endif
   }

//   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "nRemoved"), ==, true);
//   ASSERT_CMPINT (BSON_ITER_HOLDS_INT32 (&iter), ==, true);
//   ASSERT_CMPINT (4, ==, bson_iter_int32 (&iter));

   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "nMatched"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_INT32 (&iter), ==, true);
#if 0
   ASSERT_CMPINT (4, ==, bson_iter_int32 (&iter));
#endif

   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "nUpserted"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_INT32 (&iter), ==, true);
   ASSERT_CMPINT (bson_iter_int32 (&iter), ==, false);

   mongoc_bulk_operation_destroy (bulk);
   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);

   bson_init (&del);
   BSON_APPEND_INT32 (&del, "hello", 123);
   mongoc_bulk_operation_remove (bulk, &del);
   bson_destroy (&del);

   r = mongoc_bulk_operation_execute (bulk, &reply, &error);
   ASSERT_CMPINT (r, ==, true);
   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "nRemoved"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_INT32 (&iter), ==, true);
#if 0
   ASSERT_CMPINT (4, ==, bson_iter_int32 (&iter));
#endif
   recordnum = check_records(collection, q, true);
   ASSERT_CMPINT(recordnum, ==, 0);
   bson_destroy(&q);

   bson_destroy (&reply);

   //r = mongoc_collection_drop (collection, &error);
   //ASSERT_CMPINT (r, ==, true);
   mongoc_bulk_operation_destroy (bulk);
   tearDown(collection);
   mongoc_client_destroy (client);
   bson_destroy (&doc);
}


static void
test_update_upserted (void)
{
   mongoc_bulk_operation_t *bulk;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_error_t error;
   bson_iter_t iter;
   bson_iter_t ar;
   bson_iter_t citer;
   bson_t reply;
   bson_t *sel;
   bson_t *doc;
   bson_t q;
   bool r;

   client = mongoc_client_new (gTestUri);
   ASSERT_CMPPTR (client, !=, NULL);

   //collection = get_test_collection (client, "test_update_upserted");
   //ASSERT_CMPPTR (collection, !=, NULL);
   collection = setUp("test_update_upserted",client);

   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   ASSERT_CMPPTR (bulk, !=, NULL);

   sel = BCON_NEW ("abcd", BCON_INT32 (1234));
   doc = BCON_NEW ("$set", "{", "hello", "there", "}");

   mongoc_bulk_operation_update (bulk, sel, doc, true);

   r = mongoc_bulk_operation_execute (bulk, &reply, &error);
   ASSERT_CMPINT (r, ==, true);

   bson_init(&q);
   BSON_APPEND_UTF8(&q,"hello","there");
   ASSERT_CMPINT(check_records(collection, q,true), ==, 1);
   bson_destroy(&q);
   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "nUpserted"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_INT32 (&iter), ==, true);
#if 0
   ASSERT_CMPINT (bson_iter_int32 (&iter), ==, 1);
#endif

   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "nMatched"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_INT32 (&iter), ==, true);
#if 0
   ASSERT_CMPINT (bson_iter_int32 (&iter), ==, 0);
#endif

   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "nRemoved"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_INT32 (&iter), ==, true);
#if 0
   ASSERT_CMPINT (bson_iter_int32 (&iter), ==, 0);
#endif

   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "nInserted"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_INT32 (&iter), ==, true);
#if 0
   ASSERT_CMPINT (bson_iter_int32 (&iter), ==, 0);
#endif

   if (bson_iter_init_find (&iter, &reply, "nModified")) {
      ASSERT_CMPINT (BSON_ITER_HOLDS_INT32 (&iter), ==, true);
#if 0
      ASSERT_CMPINT (bson_iter_int32 (&iter), ==, 0);
#endif
   }

#if 0
   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "upserted"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_ARRAY (&iter), ==, true);
   ASSERT_CMPINT (bson_iter_recurse (&iter, &ar), ==, true);
   ASSERT_CMPINT (bson_iter_next (&ar), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_DOCUMENT (&ar), ==, true);
   ASSERT_CMPINT (bson_iter_recurse (&ar, &citer), ==, true);
   ASSERT_CMPINT (bson_iter_next (&citer), ==, true);
   ASSERT_CMPINT (BSON_ITER_IS_KEY (&citer, "index"), ==, true);
   ASSERT_CMPINT (bson_iter_next (&citer), ==, true);
   ASSERT_CMPINT (BSON_ITER_IS_KEY (&citer, "_id"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_OID (&citer), ==, true);
   ASSERT_CMPINT (bson_iter_next (&citer), ==, false);
   ASSERT_CMPINT (bson_iter_next (&ar), ==, false);
#endif
   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "writeErrors"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_ARRAY (&iter), ==, true);
   ASSERT_CMPINT (bson_iter_recurse (&iter, &ar), ==, true);
   ASSERT_CMPINT (bson_iter_next (&ar), ==, false);

   bson_destroy (&reply);

   //r = mongoc_collection_drop (collection, &error);
   //ASSERT_CMPINT (r, ==, true);

   mongoc_bulk_operation_destroy (bulk);
   tearDown(collection);
   mongoc_client_destroy (client);

   bson_destroy (doc);
   bson_destroy (sel);
}


static void
test_index_offset (void)
{
   mongoc_bulk_operation_t *bulk;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_error_t error;
   bson_iter_t iter;
   bson_iter_t ar;
   bson_iter_t citer;
   bson_t reply;
   bson_t *sel;
   bson_t *doc;
   bson_t q;
   bool r;

   client = mongoc_client_new (gTestUri);
   ASSERT_CMPPTR (client, !=, NULL);

   //collection = get_test_collection (client, "test_index_offset");
   //ASSERT_CMPPTR (collection, !=, NULL);
   collection = setUp("test_index_offset", client);

   doc = bson_new ();
   BSON_APPEND_INT32 (doc, "abcd", 1234);
   r = mongoc_collection_insert (collection, MONGOC_INSERT_NONE, doc, NULL, &error);
   ASSERT_CMPINT (r, ==, true);
   bson_destroy (doc);

   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   ASSERT_CMPPTR (bulk, !=, NULL);

   sel = BCON_NEW ("abcd", BCON_INT32 (1234));
   doc = BCON_NEW ("$set", "{", "hello", "there", "}");

   mongoc_bulk_operation_remove_one (bulk, sel);
   mongoc_bulk_operation_update (bulk, sel, doc, true);

   r = mongoc_bulk_operation_execute (bulk, &reply, &error);
   ASSERT_CMPINT (r, ==, true);

   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "nUpserted"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_INT32 (&iter), ==, true);
#if 0
   ASSERT_CMPINT (bson_iter_int32 (&iter), ==, 1);
#endif

   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "nMatched"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_INT32 (&iter), ==, true);
#if 0
   ASSERT_CMPINT (bson_iter_int32 (&iter), ==, 0);
#endif

   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "nRemoved"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_INT32 (&iter), ==, true);
#if 0
   ASSERT_CMPINT (bson_iter_int32 (&iter), ==, 1);
#endif

   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "nInserted"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_INT32 (&iter), ==, true);
#if 0
   ASSERT_CMPINT (bson_iter_int32 (&iter), ==, 0);
#endif

   if (bson_iter_init_find (&iter, &reply, "nModified")) {
      ASSERT_CMPINT (BSON_ITER_HOLDS_INT32 (&iter), ==, true);
#if 0
      ASSERT_CMPINT (bson_iter_int32 (&iter), ==, 0);
#endif
   }
   bson_init(&q);
   BSON_APPEND_UTF8(&q,"hello","there");
   bson_append_int32(&q, "abcd", -1, 1234);
   ASSERT_CMPINT(check_records(collection, q, true), ==, 1);
   bson_destroy(&q);

#if 0
   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "upserted"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_ARRAY (&iter), ==, true);
   ASSERT_CMPINT (bson_iter_recurse (&iter, &ar), ==, true);
   ASSERT_CMPINT (bson_iter_next (&ar), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_DOCUMENT (&ar), ==, true);
   ASSERT_CMPINT (bson_iter_recurse (&ar, &citer), ==, true);
   ASSERT_CMPINT (bson_iter_next (&citer), ==, true);
   ASSERT_CMPINT (BSON_ITER_IS_KEY (&citer, "index"), ==, true);
   ASSERT_CMPINT (bson_iter_int32 (&citer), ==, 1);
   ASSERT_CMPINT (bson_iter_next (&citer), ==, true);
   ASSERT_CMPINT (BSON_ITER_IS_KEY (&citer, "_id"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_OID (&citer), ==, true);
   ASSERT_CMPINT (bson_iter_next (&citer), ==, false);
   ASSERT_CMPINT (bson_iter_next (&ar), ==, false);
#endif
   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "writeErrors"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_ARRAY (&iter), ==, true);
   ASSERT_CMPINT (bson_iter_recurse (&iter, &ar), ==, true);
   ASSERT_CMPINT (bson_iter_next (&ar), ==, false);

   bson_destroy (&reply);

   //r = mongoc_collection_drop (collection, &error);
   //ASSERT_CMPINT (r, ==, true);

   mongoc_bulk_operation_destroy (bulk);
   tearDown(collection);
   mongoc_client_destroy (client);

   bson_destroy (doc);
   bson_destroy (sel);
}

static void
test_bulk_edge_over_1000 (void)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t * bulk_op;
   mongoc_write_concern_t * wc = mongoc_write_concern_new();
   bson_iter_t iter, error_iter, index;
   bson_t doc, result;
   bson_error_t error;
   bson_t q = BSON_INITIALIZER;
   int i;

   client = mongoc_client_new (gTestUri);
   ASSERT_CMPPTR (client, !=, NULL);

   //collection = get_test_collection (client, "OVER_1000");
   //ASSERT_CMPPTR (collection, !=, NULL);
   collection = setUp("OVER_1000", client);

   mongoc_write_concern_set_w(wc, 1);

   bulk_op = mongoc_collection_create_bulk_operation(collection, false, wc);

   for (i = 0; i < 1010; i+=3) {
      bson_init(&doc);
      bson_append_int32(&doc, "_id", -1, i);

      mongoc_bulk_operation_insert(bulk_op, &doc);

      bson_destroy(&doc);
   }

   mongoc_bulk_operation_execute(bulk_op, NULL, &error);

   mongoc_bulk_operation_destroy(bulk_op);

   bulk_op = mongoc_collection_create_bulk_operation(collection, false, wc);
   for (i = 0; i < 1010; i++) {
      bson_init(&doc);
      bson_append_int32(&doc, "_id", -1, i);

      mongoc_bulk_operation_insert(bulk_op, &doc);

      bson_destroy(&doc);
   }

   mongoc_bulk_operation_execute(bulk_op, &result, &error);
   ASSERT_CMPINT( check_records(collection, q, false), ==, 1010);
   bson_iter_init_find(&iter, &result, "writeErrors");
   ASSERT_CMPINT(bson_iter_recurse(&iter, &error_iter), ==, true);
#if 0
   ASSERT_CMPINT(bson_iter_next(&error_iter), ==, true);

   for (i = 0; i < 1010; i+=3) {
      ASSERT_CMPINT(bson_iter_recurse(&error_iter, &index), ==, true);
      ASSERT_CMPINT(bson_iter_find(&index, "index"), ==, true);
      if (bson_iter_int32(&index) != i) {
          fprintf(stderr, "index should be %d, but is %d\n", i, bson_iter_int32(&index));
      }
      ASSERT_CMPINT(bson_iter_int32(&index), ==, i);
      bson_iter_next(&error_iter);
   }
#endif
   mongoc_bulk_operation_destroy(bulk_op);
   bson_destroy (&result);

   mongoc_write_concern_destroy(wc);
   tearDown(collection);
   mongoc_client_destroy(client);
}

static void
test_bulk_edge_case_372 (void)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_error_t error;
   bson_iter_t iter;
   bson_iter_t citer;
   bson_iter_t child;
   const char *str;
   bson_t *selector;
   bson_t *update;
   bson_t reply;
   bool r;
   int vmaj = 0;
   int vmin = 0;
   int vmic = 0;

   client = mongoc_client_new (gTestUri);
   ASSERT_CMPPTR (client, !=, NULL);

   //collection = get_test_collection (client, "CDRIVER_372");
   //ASSERT_CMPPTR (collection, !=, NULL);
   collection = setUp( "CDRIVER_372", client );

   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);
   ASSERT_CMPPTR (bulk, !=, NULL);

   selector = BCON_NEW ("_id", BCON_INT32 (0));
   update = BCON_NEW ("$set", "{", "a", BCON_INT32 (0), "}");
   mongoc_bulk_operation_update_one (bulk, selector, update, true);
   bson_destroy (selector);
   bson_destroy (update);

   selector = BCON_NEW ("a", BCON_INT32 (1));
   update = BCON_NEW ("_id", BCON_INT32 (1));
   mongoc_bulk_operation_replace_one (bulk, selector, update, true);
   bson_destroy (selector);
   bson_destroy (update);

 #if 0
   r = mongoc_client_get_server_status (client, NULL, &reply, &error);
   if (!r) fprintf (stderr, "%s\n", error.message);
   ASSERT_CMPINT (r, ==, true);

   if (bson_iter_init_find (&iter, &reply, "version") &&
       BSON_ITER_HOLDS_UTF8 (&iter) &&
       (str = bson_iter_utf8 (&iter, NULL))) {
      sscanf (str, "%d.%d.%d", &vmaj, &vmin, &vmic);
   }

   bson_destroy (&reply);

   if (vmaj >=2 || (vmaj == 2 && vmin >= 6)) {
      /* This is just here to make the counts right in all cases. */
      selector = BCON_NEW ("_id", BCON_INT32 (2));
      update = BCON_NEW ("_id", BCON_INT32 (2));
      mongoc_bulk_operation_replace_one (bulk, selector, update, true);
      bson_destroy (selector);
      bson_destroy (update);
   } else {
      /* This case is only possible in MongoDB versions before 2.6. */
      selector = BCON_NEW ("_id", BCON_INT32 (3));
      update = BCON_NEW ("_id", BCON_INT32 (2));
      mongoc_bulk_operation_replace_one (bulk, selector, update, true);
      bson_destroy (selector);
      bson_destroy (update);
   }
#endif

   r = mongoc_bulk_operation_execute (bulk, &reply, &error);
   if (!r) fprintf (stderr, "%s\n", error.message);
   ASSERT_CMPINT (r, ==, true);

#if 0
   printf ("%s\n", bson_as_json (&reply, NULL));
#endif

   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "nMatched") &&
           BSON_ITER_HOLDS_INT32 (&iter) &&
           (0 == bson_iter_int32 (&iter)), ==, true);
   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "nUpserted") &&
           BSON_ITER_HOLDS_INT32 (&iter) &&
           (3 == bson_iter_int32 (&iter)), ==, true);
   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "nInserted") &&
           BSON_ITER_HOLDS_INT32 (&iter) &&
           (0 == bson_iter_int32 (&iter)), ==, true);
   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "nRemoved") &&
           BSON_ITER_HOLDS_INT32 (&iter) &&
           (0 == bson_iter_int32 (&iter)), ==, true);

   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "upserted") &&
           BSON_ITER_HOLDS_ARRAY (&iter) &&
           bson_iter_recurse (&iter, &citer), ==, true);

   ASSERT_CMPINT (bson_iter_next (&citer), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_DOCUMENT (&citer), ==, true);
   ASSERT_CMPINT (bson_iter_recurse (&citer, &child), ==, true);
   ASSERT_CMPINT (bson_iter_find (&child, "_id"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_INT32 (&child), ==, true);
   ASSERT_CMPINT (0, ==, bson_iter_int32 (&child));
   ASSERT_CMPINT (bson_iter_recurse (&citer, &child), ==, true);
   ASSERT_CMPINT (bson_iter_find (&child, "index"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_INT32 (&child), ==, true);
   ASSERT_CMPINT (0, ==, bson_iter_int32 (&child));

   ASSERT_CMPINT (bson_iter_next (&citer), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_DOCUMENT (&citer), ==, true);
   ASSERT_CMPINT (bson_iter_recurse (&citer, &child), ==, true);
   ASSERT_CMPINT (bson_iter_find (&child, "_id"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_INT32 (&child), ==, true);
   ASSERT_CMPINT (1, ==, bson_iter_int32 (&child));
   ASSERT_CMPINT (bson_iter_recurse (&citer, &child), ==, true);
   ASSERT_CMPINT (bson_iter_find (&child, "index"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_INT32 (&child), ==, true);
   ASSERT_CMPINT (1, ==, bson_iter_int32 (&child));

   ASSERT_CMPINT (bson_iter_next (&citer), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_DOCUMENT (&citer), ==, true);
   ASSERT_CMPINT (bson_iter_recurse (&citer, &child), ==, true);
   ASSERT_CMPINT (bson_iter_find (&child, "_id"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_INT32 (&child), ==, true);
   ASSERT_CMPINT (2, ==, bson_iter_int32 (&child));
   ASSERT_CMPINT (bson_iter_recurse (&citer, &child), ==, true);
   ASSERT_CMPINT (bson_iter_find (&child, "index"), ==, true);
   ASSERT_CMPINT (BSON_ITER_HOLDS_INT32 (&child), ==, true);
   ASSERT_CMPINT (2, ==, bson_iter_int32 (&child));

   ASSERT_CMPINT (!bson_iter_next (&citer), ==, true);

   bson_destroy (&reply);

   //mongoc_collection_drop (collection, NULL);

   mongoc_bulk_operation_destroy (bulk);
   //mongoc_collection_destroy (collection);
   tearDown (collection);
   mongoc_client_destroy (client);
}


static void
test_bulk_new (void)
{
   mongoc_bulk_operation_t *bulk;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_error_t error;
   bson_t empty = BSON_INITIALIZER;
   bson_t q = BSON_INITIALIZER;
   bool r;

   client = mongoc_client_new (gTestUri);
   ASSERT_CMPPTR (client, !=, NULL);

   //collection = get_test_collection (client, "bulk_new");
   //ASSERT_CMPPTR (collection, !=, NULL);
   collection = setUp("bulk_new", client);

   bulk = mongoc_bulk_operation_new (true);
   mongoc_bulk_operation_destroy (bulk);

   bulk = mongoc_bulk_operation_new (true);

   r = mongoc_bulk_operation_execute (bulk, NULL, &error);
   ASSERT_CMPINT (r, ==, false);
   //ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_CLIENT);
   //ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_COMMAND_INVALID_ARG);

   mongoc_bulk_operation_set_database (bulk, "fapmongo_test");
   r = mongoc_bulk_operation_execute (bulk, NULL, &error);
   ASSERT_CMPINT (r, ==, false);
   //ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_CLIENT);
   //ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_COMMAND_INVALID_ARG);

   mongoc_bulk_operation_set_collection (bulk, mongoc_collection_get_name(collection));
   r = mongoc_bulk_operation_execute (bulk, NULL, &error);
   ASSERT_CMPINT (r, ==, false);
   //ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_CLIENT);
   //ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_COMMAND_INVALID_ARG);

   mongoc_bulk_operation_set_client (bulk, client);
   r = mongoc_bulk_operation_execute (bulk, NULL, &error);
   ASSERT_CMPINT (r, ==, false);
   //ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_CLIENT);
   //ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_COMMAND_INVALID_ARG);

   mongoc_bulk_operation_insert (bulk, &empty);
   r = mongoc_bulk_operation_execute (bulk, NULL, &error);
   ASSERT_CMPINT (r, ==, true);

   mongoc_bulk_operation_destroy (bulk);

   //mongoc_collection_drop (collection, NULL);

   //mongoc_collection_destroy (collection);
   tearDown(collection);
   mongoc_client_destroy (client);
}


void
test_bulk_install (TestSuite *suite)
{
   gTestUri = bson_strdup_printf("mongodb://%s/", MONGOC_TEST_HOST);

   TestSuite_Add (suite, "BulkOperation_basic", test_bulk);
   TestSuite_Add (suite, "BulkOperation_update_upserted", test_update_upserted);
   TestSuite_Add (suite, "BulkOperation_index_offset", test_index_offset);
   //TestSuite_Add (suite, "BulkOperation_CDRIVER-372", test_bulk_edge_case_372);
   TestSuite_Add (suite, "BulkOperation_new", test_bulk_new);
   TestSuite_Add (suite, "BulkOperation_over_1000", test_bulk_edge_over_1000);

   atexit (cleanup_globals);
}
