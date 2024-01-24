/*
 * Copyright 2013 MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MONGOC_CLIENT_PRIVATE_H
#define MONGOC_CLIENT_PRIVATE_H

#if !defined (MONGOC_I_AM_A_DRIVER) && !defined (MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-buffer-private.h"
#include "mongoc-client.h"
#include "mongoc-cluster-private.h"
#include "mongoc-config.h"
#include "mongoc-host-list.h"
#include "mongoc-read-prefs.h"
#include "mongoc-rpc-private.h"
#include "mongoc-opcode.h"
#ifdef MONGOC_ENABLE_SSL
#include "mongoc-ssl.h"
#endif
#include "mongoc-stream.h"
#include "mongoc-topology-private.h"
#include "mongoc-write-concern.h"


BSON_BEGIN_DECLS


struct _mongoc_client_t
{
   uint32_t                   request_id;
   mongoc_list_t             *conns;
   mongoc_uri_t              *uri;
   mongoc_cluster_t           cluster;
   bool                       in_exhaust;

   mongoc_stream_initiator_t  initiator;
   void                      *initiator_data;

#ifdef MONGOC_ENABLE_SSL
   mongoc_ssl_opt_t           ssl_opts;
   char                      *pem_subject;
#endif

   mongoc_topology_t             *topology;

   mongoc_read_prefs_t       *read_prefs;
   mongoc_write_concern_t    *write_concern;
};


mongoc_client_t *
_mongoc_client_new_from_uri (const mongoc_uri_t *uri,
                             mongoc_topology_t      *topology);

mongoc_stream_t *
mongoc_client_default_stream_initiator (const mongoc_uri_t       *uri,
                                        const mongoc_host_list_t *host,
                                        void                     *user_data,
                                        bson_error_t             *error);

mongoc_stream_t *
_mongoc_client_create_stream (mongoc_client_t          *client,
                              const mongoc_host_list_t *host,
                              bson_error_t             *error);

bool
_mongoc_client_recv (mongoc_client_t *client,
                     mongoc_rpc_t    *rpc,
                     mongoc_buffer_t *buffer,
                     uint32_t         server_id,
                     bson_error_t    *error);

bool
_mongoc_client_recv_gle (mongoc_client_t *client,
                         uint32_t         hint,
                         bson_t         **gle_doc,
                         bson_error_t    *error);

uint32_t
_mongoc_client_preselect (mongoc_client_t              *client,
                          mongoc_opcode_t               opcode,
                          const mongoc_write_concern_t *write_concern,
                          const mongoc_read_prefs_t    *read_prefs,
                          bson_error_t                 *error);

void
_mongoc_topology_background_thread_start (mongoc_topology_t *topology);

void
_mongoc_topology_background_thread_stop (mongoc_topology_t *topology);

mongoc_server_description_t *
_mongoc_client_get_server_description (mongoc_client_t *client,
                                       uint32_t         server_id);

bool
_mongoc_client_command_simple_with_hint (mongoc_client_t           *client,
                                         const char                *db_name,
                                         const bson_t              *command,
                                         const mongoc_read_prefs_t *read_prefs,
                                         bool                       is_write_command,
                                         bson_t                    *reply,
                                         uint32_t                   hint,
                                         bson_error_t              *error);
void
_mongoc_client_kill_cursor              (mongoc_client_t *client,
                                         uint32_t         server_id,
                                         int64_t          cursor_id);

BSON_END_DECLS


#endif /* MONGOC_CLIENT_PRIVATE_H */
