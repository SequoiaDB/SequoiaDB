#include <fcntl.h>
#include <mongoc.h>

//#include "mongoc-cursor-private.h"
//#include "mock-server.h"
//#include "mongoc-client-private.h"
#include "mongoc-tests.h"
#include "TestSuite.h"

#include "test-libmongoc.h"

static char *gTestUri;
static char *gTestUriWithBadPassword;
extern TestSuite suite;


#define MONGOD_VERSION_HEX(a, b, c) ((a << 16) | (b << 8) | (c))


#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "client-test"


#ifdef _WIN32
static void
usleep (int64_t usec)
{
    HANDLE timer;
    LARGE_INTEGER ft;

    ft.QuadPart = -(10 * usec);

    timer = CreateWaitableTimer(NULL, true, NULL);
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
}
#endif


static mongoc_collection_t *
get_test_collection (mongoc_client_t *client,
                     const char      *name)
{
   mongoc_collection_t *ret;
   char *str;

   str = gen_collection_name (name);
   ret = mongoc_client_get_collection (client, "test", str);
   bson_free (str);

   return ret;
}


static char *
gen_test_user (void)
{
   return bson_strdup_printf ("testuser_%u_%u",
                              (unsigned)time(NULL),
                              (unsigned)gettestpid());
}


static char *
gen_good_uri (const char *username)
{
   return bson_strdup_printf("mongodb://%s:testpass@%s/test",
                             username,
                             MONGOC_TEST_HOST);
}


static void
test_mongoc_client_authenticate (void)
{
   mongoc_collection_t *collection;
   mongoc_database_t *database;
   mongoc_client_t *client;
   mongoc_cursor_t *cursor;
   const bson_t *doc;
   bson_error_t error;
   char *username;
   char *uri;
   bool r;
   bson_t q;

   username = gen_test_user ();
   uri = gen_good_uri (username);

   /*
    * Add a user to the test database.
    */
   client = mongoc_client_new(gTestUri);
   database = mongoc_client_get_database(client, "test");
   mongoc_database_remove_user (database, username, &error);
   r = mongoc_database_add_user(database, username, "testpass", NULL, NULL, &error);
   ASSERT_CMPINT(r, ==, 1);
   mongoc_database_destroy(database);
   mongoc_client_destroy(client);

   /*
    * Try authenticating with that user.
    */
   bson_init(&q);
   client = mongoc_client_new(uri);
   collection = mongoc_client_get_collection(client, "test", "test");
   cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 1, 0,
                                   &q, NULL, NULL);
   r = mongoc_cursor_next(cursor, &doc);
   if (!r) {
      r = mongoc_cursor_error(cursor, &error);
      if (r) {
         MONGOC_ERROR("Authentication failure: \"%s\"", error.message);
      }
      ASSERT_CMPINT(r, ==, false);
   }
   mongoc_cursor_destroy(cursor);

   /*
    * Remove all test users.
    */
   database = mongoc_client_get_database (client, "test");
   //r = mongoc_database_remove_all_users (database, &error);
   r = mongoc_database_remove_user(database, username, &error);
   ASSERT_CMPINT(r, ==, true);
   mongoc_database_destroy (database);

   mongoc_collection_destroy(collection);
   mongoc_client_destroy(client);

   bson_free (username);
   bson_free (uri);
}

static bool createUser(mongoc_client_t **client, char *username)
{
   bool r ;
   mongoc_database_t *database;
   bson_error_t error;
   *client = mongoc_client_new(gTestUri);
   database = mongoc_client_get_database(*client, "test");
   mongoc_database_remove_user (database, username, &error);
   r = mongoc_database_add_user(database, username, "testpass", NULL, NULL, &error);
   
   mongoc_database_destroy(database);
   return r;
}

static bool dropUser(mongoc_client_t *client, char *username)
{
   bool r ;
   mongoc_database_t *database;
   bson_error_t error;
   database = mongoc_client_get_database(client, "test");
   r = mongoc_database_remove_user(database, username, &error);
   mongoc_database_destroy(database);
   mongoc_client_destroy(client);
   return r;
}

static void
test_mongoc_client_authenticate_failure (void)
{
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   mongoc_client_t *initclient;
   mongoc_client_t *client;
   const bson_t *doc;
   bson_error_t error;
   bool r;
   bson_t q;
   char *username;
   bson_t empty = BSON_INITIALIZER;

   username = gen_test_user ();
   r = createUser( &initclient, username);
   if (r == false) return ;
   /*
    * Try authenticating with that user.
    */
   bson_init(&q);
   client = mongoc_client_new(gTestUriWithBadPassword);
   collection = mongoc_client_get_collection(client, "test", "test");
   cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 1, 0,
                                   &q, NULL, NULL);
   r = mongoc_cursor_next(cursor, &doc);
   ASSERT_CMPINT(r, ==, false);
   r = mongoc_cursor_error(cursor, &error);
   ASSERT_CMPINT(r, ==, true);
   ASSERT_CMPINT(error.domain, ==, MONGOC_ERROR_CLIENT);
   ASSERT_CMPINT(error.code, ==, MONGOC_ERROR_CLIENT_AUTHENTICATE);
   mongoc_cursor_destroy(cursor);

   /*
    * Try various commands while in the failed state to ensure we get the
    * same sort of errors.
    */
   r = mongoc_collection_insert (collection, 0, &empty, NULL, &error);
   ASSERT_CMPINT (r, ==, false);
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_CLIENT);
   ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_CLIENT_AUTHENTICATE);

   /*
    * Try various commands while in the failed state to ensure we get the
    * same sort of errors.
    */
   r = mongoc_collection_update (collection, 0, &q, &empty, NULL, &error);
   ASSERT_CMPINT (r, ==, false);
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_CLIENT);
   ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_CLIENT_AUTHENTICATE);

   mongoc_collection_destroy(collection);
   mongoc_client_destroy(client);
   r = dropUser(initclient,username);
   ASSERT_CMPINT (r, ==, true);
}

#if 0
static void
test_wire_version (void)
{
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   mongoc_client_t *client;
   mock_server_t *server;
   uint16_t port;
   const bson_t *doc;
   bson_error_t error;
   bool r;
   bson_t q = BSON_INITIALIZER;
   char *uristr;

   port = 20000 + (rand () % 1000);

   server = mock_server_new ("127.0.0.1", port, NULL, NULL);
   mock_server_set_wire_version (server, 10, 11);
   mock_server_run_in_thread (server);

   usleep (5000);

   uristr = bson_strdup_printf ("mongodb://127.0.0.1:%hu/", port);
   client = mongoc_client_new (uristr);

   collection = mongoc_client_get_collection (client, "test", "test");

   cursor = mongoc_collection_find (collection,
                                    MONGOC_QUERY_NONE,
                                    0,
                                    1,
                                    0,
                                    &q,
                                    NULL,
                                    NULL);

   r = mongoc_cursor_next (cursor, &doc);
   ASSERT_CMPINT (r, ==, false);

   r = mongoc_cursor_error (cursor, &error);
   ASSERT_CMPINT(r, ==, true);

   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_PROTOCOL);
   ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION);

   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mock_server_quit (server, 0);
   mongoc_client_destroy (client);
   bson_free (uristr);
}


static void
read_prefs_handler (mock_server_t   *server,
                    mongoc_stream_t *stream,
                    mongoc_rpc_t    *rpc,
                    void            *user_data)
{
   bool *success = user_data;
   int32_t len;
   bson_iter_t iter;
   bson_iter_t child;
   bson_iter_t child2;
   bson_iter_t child3;
   bson_t b;
   bson_t reply = BSON_INITIALIZER;
   int r;

   if (rpc->header.opcode == MONGOC_OPCODE_QUERY) {
      memcpy (&len, rpc->query.query, 4);
      len = BSON_UINT32_FROM_LE (len);

      r = bson_init_static (&b, rpc->query.query, len);
      ASSERT_CMPINT(r, ==, true);

      r = bson_iter_init_find (&iter, &b, "$query");
      ASSERT_CMPINT(r, ==, true);
      ASSERT_CMPINT (BSON_ITER_HOLDS_DOCUMENT (&iter), ==, true);

      r = bson_iter_init_find (&iter, &b, "$readPreference");
      ASSERT_CMPINT(r, ==, true);
      ASSERT_CMPINT (BSON_ITER_HOLDS_DOCUMENT (&iter), ==, true);

      r = bson_iter_recurse (&iter, &child);
      ASSERT_CMPINT(r, ==, true);

      r = bson_iter_next (&child);
      ASSERT_CMPINT(r, ==, true);
      ASSERT_CMPINT (BSON_ITER_HOLDS_UTF8 (&child), ==, true);
      ASSERT_CMPINT (strcmp ("mode", bson_iter_key (&child)), ==, 0);
      ASSERT_CMPINT (strcmp ("secondaryPreferred", bson_iter_utf8 (&child, NULL)), ==, 0);

      r = bson_iter_next (&child);
      ASSERT_CMPINT(r, ==, true);
      ASSERT_CMPINT (BSON_ITER_HOLDS_ARRAY (&child), ==, true);

      r = bson_iter_recurse (&child, &child2);
      ASSERT_CMPINT(r, ==, true);

      r = bson_iter_next (&child2);
      ASSERT_CMPINT(r, ==, true);
      ASSERT_CMPINT (BSON_ITER_HOLDS_DOCUMENT (&child2), ==, true);

      r = bson_iter_recurse (&child2, &child3);
      ASSERT_CMPINT(r, ==, true);

      r = bson_iter_next (&child3);
      ASSERT_CMPINT(r, ==, true);
      ASSERT_CMPINT (BSON_ITER_HOLDS_UTF8 (&child3), ==, true);
      ASSERT_CMPINT (strcmp ("dc", bson_iter_key (&child3)), ==, 0);
      ASSERT_CMPINT (strcmp ("ny", bson_iter_utf8 (&child3, NULL)),==, 0);
      r = bson_iter_next (&child3);
      ASSERT_CMPINT (r, ==, false);

      r = bson_iter_next (&child2);
      ASSERT_CMPINT(r, ==, true);

      r = bson_iter_recurse (&child2, &child3);
      ASSERT_CMPINT(r, ==, true);

      r = bson_iter_next (&child3);
      ASSERT_CMPINT (r, ==, false);

      mock_server_reply_simple (server, stream, rpc, MONGOC_REPLY_NONE, &reply);

      *success = true;
   }
}


static void
test_mongoc_client_read_prefs (void)
{
   mongoc_collection_t *collection;
   mongoc_read_prefs_t *read_prefs;
   mongoc_cursor_t *cursor;
   mongoc_client_t *client;
   mock_server_t *server;
   uint16_t port;
   const bson_t *doc;
   bson_error_t error;
   bool success = false;
   bson_t b = BSON_INITIALIZER;
   bson_t q = BSON_INITIALIZER;
   char *uristr;

   port = 20000 + (rand () % 1000);

   server = mock_server_new ("127.0.0.1", port, read_prefs_handler, &success);
   mock_server_run_in_thread (server);

   usleep (5000);

   uristr = bson_strdup_printf ("mongodb://127.0.0.1:%hu/", port);
   client = mongoc_client_new (uristr);

   if (!_mongoc_client_warm_up (client, &error)) {
      ASSERT_CMPINT (false, !=, false);
   }

   collection = mongoc_client_get_collection (client, "test", "test");

   bson_append_utf8 (&b, "dc", 2, "ny", 2);

   read_prefs = mongoc_read_prefs_new (MONGOC_READ_SECONDARY_PREFERRED);
   mongoc_read_prefs_add_tag (read_prefs, &b);
   mongoc_read_prefs_add_tag (read_prefs, NULL);
   mongoc_collection_set_read_prefs (collection, read_prefs);

   cursor = mongoc_collection_find (collection,
                                    MONGOC_QUERY_NONE,
                                    0,
                                    1,
                                    0,
                                    &q,
                                    NULL,
                                    read_prefs);

   mongoc_cursor_next (cursor, &doc);

   usleep (50000);

   ASSERT_CMPINT (success, ==, true);

   mongoc_read_prefs_destroy (read_prefs);
   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_quit (server, 0);
   bson_destroy (&b);
   bson_free (uristr);
}
#endif

static void
test_mongoc_client_command (void)
{
   mongoc_client_t *client;
   mongoc_cursor_t *cursor;
   const bson_t *doc;
   bool r;
   bson_iter_t iter;
   bson_t cmd = BSON_INITIALIZER;

   client = mongoc_client_new (gTestUri);
   ASSERT_CMPPTR (client, !=, NULL);

   bson_append_int32 (&cmd, "ping", 4, 1);

   cursor = mongoc_client_command (client, "admin", MONGOC_QUERY_NONE, 0, 1, 0, &cmd, NULL, NULL);
   //assert (!cursor->redir_primary);

   r = mongoc_cursor_next (cursor, &doc);
   ASSERT_CMPINT(r, ==, true);
   ASSERT_CMPPTR (doc, !=, NULL);
   if (!( bson_iter_init_find(&iter, doc, "ok") &&
        (BSON_TYPE_INT32 == bson_iter_type(&iter))&&
        (1==bson_iter_int32 (&iter))))
   {
      MONGOC_ERROR("%s", (char*)bson_as_json(doc,0));
      ASSERT(false);
   }

   r = mongoc_cursor_next (cursor, &doc);
   ASSERT_CMPINT (r, ==, false);
   ASSERT_CMPPTR (doc, ==, NULL);

   mongoc_cursor_destroy (cursor);
   mongoc_client_destroy (client);
   bson_destroy (&cmd);
}

#if 0
static void
test_mongoc_client_command_secondary (void)
{
   mongoc_client_t *client;
   mongoc_cursor_t *cursor;
   mongoc_read_prefs_t *read_prefs;
   bson_t cmd = BSON_INITIALIZER;

   client = mongoc_client_new (gTestUri);
   ASSERT_CMPPTR (client, !=, NULL)

   BSON_APPEND_INT32 (&cmd, "invalid_command_here", 1);

   read_prefs = mongoc_read_prefs_new (MONGOC_READ_PRIMARY_PREFERRED);

   suppress_one_message ();
   cursor = mongoc_client_command (client, "admin", MONGOC_QUERY_NONE, 0, 1, 0, &cmd, NULL, read_prefs);

   mongoc_read_prefs_destroy (read_prefs);

   /* ensure we detected this must go to primary */
   //assert (cursor->redir_primary);

   mongoc_cursor_destroy (cursor);
   mongoc_client_destroy (client);
   bson_destroy (&cmd);
}
#endif
static void
test_mongoc_client_preselect (void)
{
   mongoc_client_t *client;
   bson_error_t error;
   uint32_t node;

   client = mongoc_client_new (gTestUri);
   ASSERT_CMPPTR (client, !=, NULL);

   node = _mongoc_client_preselect (client, MONGOC_OPCODE_INSERT,
                                    NULL, NULL, &error);
   ASSERT_CMPINT (node, >, 0);

   mongoc_client_destroy (client);
}
#if 0
static void
test_exhaust_cursor (void)
{
   mongoc_write_concern_t *wr;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   mongoc_cursor_t *cursor2;
   mongoc_stream_t *stream;
   mongoc_cluster_node_t *node;
   const bson_t *doc;
   bson_t q;
   bson_t b[10];
   bson_t *bptr[10];
   int i;
   bool r;
   bson_error_t error;
   bson_oid_t oid;

   client = mongoc_client_new (gTestUri);
   ASSERT_CMPPTR (client, !=, NULL)

   collection = get_test_collection (client, "test_exhaust_cursor");
   ASSERT_CMPPTR (collection, !=, NULL);

   mongoc_collection_drop(collection, &error);

   wr = mongoc_write_concern_new ();
   mongoc_write_concern_set_journal (wr, true);

   /* bulk insert some records to work on */
   {
      bson_init(&q);

      for (i = 0; i < 10; i++) {
         bson_init(&b[i]);
         bson_oid_init(&oid, NULL);
         bson_append_oid(&b[i], "_id", -1, &oid);
         bson_append_int32(&b[i], "n", -1, i % 2);
         bptr[i] = &b[i];
      }

      BEGIN_IGNORE_DEPRECATIONS;
      r = mongoc_collection_insert_bulk (collection, MONGOC_INSERT_NONE,
                                         (const bson_t **)bptr, 10, wr, &error);
      END_IGNORE_DEPRECATIONS;

      if (!r) {
         MONGOC_WARNING("Insert bulk failure: %s\n", error.message);
      }
      ASSERT_CMPINT(r, ==, true);
   }

   /* create a couple of cursors */
   {
      cursor = mongoc_collection_find (collection, MONGOC_QUERY_EXHAUST, 0, 0, 0, &q,
                                       NULL, NULL);

      cursor2 = mongoc_collection_find (collection, MONGOC_QUERY_NONE, 0, 0, 0, &q,
                                        NULL, NULL);
   }

   /* Read from the exhaust cursor, ensure that we're in exhaust where we
    * should be and ensure that an early destroy properly causes a disconnect
    * */
   {
      r = mongoc_cursor_next (cursor, &doc);
      ASSERT_CMPINT(r, ==, true);
      ASSERT_CMPPTR(doc,!=, NULL);
      ASSERT_CMPINT (cursor->in_exhaust, ==, true);
      ASSERT_CMPINT (client->in_exhaust, ==, true);
      node = &client->cluster.nodes[cursor->hint - 1];
      stream = node->stream;

      mongoc_cursor_destroy (cursor);
      /* make sure a disconnect happened */
      ASSERT_CMPPTR (stream, !=, node->stream);
      ASSERT_CMPINT (client->in_exhaust, ==, false);
   }

   /* Grab a new exhaust cursor, then verify that reading from that cursor
    * (putting the client into exhaust), breaks a mid-stream read from a
    * regular cursor */
   {
      cursor = mongoc_collection_find (collection, MONGOC_QUERY_EXHAUST, 0, 0, 0, &q,
                                       NULL, NULL);

      for (i = 0; i < 5; i++) {
         r = mongoc_cursor_next (cursor2, &doc);
         ASSERT_CMPINT(r, ==, true);
         ASSERT_CMPPTR(doc,!=, NULL);
      }

      r = mongoc_cursor_next (cursor, &doc);
      ASSERT_CMPINT(r, ==, true);
      ASSERT_CMPPTR(doc,!=, NULL);

      doc = NULL;
      r = mongoc_cursor_next (cursor2, &doc);
      ASSERT_CMPINT (r, ==, false);
      ASSERT_CMPPTR (doc, ==, NULL);

      mongoc_cursor_error(cursor2, &error);
      ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_CLIENT);
      ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_CLIENT_IN_EXHAUST);

      mongoc_cursor_destroy (cursor2);
   }

   /* make sure writes fail as well */
   {
      BEGIN_IGNORE_DEPRECATIONS;
      r = mongoc_collection_insert_bulk (collection, MONGOC_INSERT_NONE,
                                         (const bson_t **)bptr, 10, wr, &error);
      END_IGNORE_DEPRECATIONS;

      ASSERT_CMPINT (r, ==, false);
      ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_CLIENT);
      ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_CLIENT_IN_EXHAUST);
   }

   /* we're still in exhaust.
    *
    * 1. check that we can create a new cursor, as long as we don't read from it
    * 2. fully exhaust the exhaust cursor
    * 3. make sure that we don't disconnect at destroy
    * 4. make sure we can read the cursor we made during the exhuast
    */
   {
      cursor2 = mongoc_collection_find (collection, MONGOC_QUERY_NONE, 0, 0, 0, &q,
                                        NULL, NULL);

      node = &client->cluster.nodes[cursor->hint - 1];
      stream = node->stream;

      for (i = 1; i < 10; i++) {
         r = mongoc_cursor_next (cursor, &doc);
         ASSERT_CMPINT(r, ==, true);
         ASSERT_CMPPTR(doc,!=, NULL);
      }

      r = mongoc_cursor_next (cursor, &doc);
      ASSERT_CMPINT (r, ==, false);
      ASSERT_CMPPTR (doc, ==, NULL);


      mongoc_cursor_destroy (cursor);

      ASSERT_CMPPTR (stream, ==, node->stream);

      r = mongoc_cursor_next (cursor2, &doc);
      ASSERT_CMPINT(r, ==, true);
      ASSERT_CMPPTR(doc,!=, NULL);
   }

   bson_destroy(&q);
   for (i = 0; i < 10; i++) {
      bson_destroy(&b[i]);
   }

   r = mongoc_collection_drop (collection, &error);
   ASSERT_CMPINT(r, ==, true);

   mongoc_write_concern_destroy (wr);
   mongoc_cursor_destroy (cursor2);
   mongoc_collection_destroy(collection);
   mongoc_client_destroy (client);
}
#endif

static void
test_server_status (void)
{
   mongoc_client_t *client;
   bson_error_t error;
   bson_iter_t iter;
   bson_t reply;
   bool r;

   client = mongoc_client_new (gTestUri);
   ASSERT_CMPPTR (client, !=, NULL);

   r = mongoc_client_get_server_status (client, NULL, &reply, &error);
   ASSERT_CMPINT(r, ==, true);

   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "host"), ==, true);
   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "version"), ==, true);
   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "ok"), ==, true);

   bson_destroy (&reply);

   mongoc_client_destroy (client);
}


static void
test_mongoc_client_ipv6 (void)
{
   mongoc_client_t *client;
   bson_error_t error;
   bson_iter_t iter;
   bson_t reply;
   bool r;

   client = mongoc_client_new ("mongodb://[::1]/");
   ASSERT_CMPPTR (client, !=, NULL);

   r = mongoc_client_get_server_status (client, NULL, &reply, &error);
   ASSERT_CMPINT(r, ==, true);

   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "host"), ==, true);
   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "version"), ==, true);
   ASSERT_CMPINT (bson_iter_init_find (&iter, &reply, "ok"), ==, true);

   bson_destroy (&reply);

   mongoc_client_destroy (client);
}


static void
cleanup_globals (void)
{
   bson_free(gTestUri);
   bson_free(gTestUriWithBadPassword);
}


void
test_client_install (TestSuite *suite)
{
   bool local;

   gTestUri = bson_strdup_printf("mongodb://%s/", MONGOC_TEST_HOST);
   gTestUriWithBadPassword = bson_strdup_printf("mongodb://baduser:badpass@%s/test", MONGOC_TEST_HOST);

   local = !getenv ("MONGOC_DISABLE_MOCK_SERVER");

   if (!local) {
      //TestSuite_Add (suite, "Client_wire_version", test_wire_version);
      //TestSuite_Add (suite, "Client_read_prefs", test_mongoc_client_read_prefs);
   }
   if (getenv ("MONGOC_CHECK_IPV6")) {
      /* try to validate ipv6 too */
      TestSuite_Add (suite, "Client_ipv6", test_mongoc_client_ipv6);
   }
   TestSuite_Add (suite, "Client_authenticate", test_mongoc_client_authenticate);
   TestSuite_Add (suite, "Client_authenticate_failure", test_mongoc_client_authenticate_failure);
   TestSuite_Add (suite, "Client_command", test_mongoc_client_command);
   //TestSuite_Add (suite, "Client_command_secondary", test_mongoc_client_command_secondary);
   TestSuite_Add (suite, "Client_preselect", test_mongoc_client_preselect);
   //TestSuite_Add (suite, "Client_exhaust_cursor", test_exhaust_cursor);
   //TestSuite_Add (suite, "Client_server_status", test_server_status);

   atexit (cleanup_globals);
}
