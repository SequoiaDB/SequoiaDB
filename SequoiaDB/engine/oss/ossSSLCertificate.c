/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = ossSSLCertificate.c

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          30/1/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifdef SDB_SSL

#include "ossSSLWrapper.h"
#include "openssl/rsa.h"
#include "openssl/evp.h"
#include "openssl/x509.h"
#include <string.h>

SDB_EXTERN_C_START

static INT32 _ossSSLNewRSA(RSA** rsa)
{
   RSA* r = NULL;
   BIGNUM* bne = NULL;
   const INT32 bits = 2048;
   const UINT32 e = RSA_F4;
   INT32 ret;

   SSL_ASSERT(NULL != rsa);

   bne = BN_new();
   if (NULL == bne)
   {
      goto error;
   }

   ret = BN_set_word(bne, e);
   if (!ret)
   {
      goto error;
   }

   r = RSA_new();
   if (NULL == r)
   {
      goto error;
   }

   ret = RSA_generate_key_ex(r, bits, bne, NULL);
   if (!ret)
   {
      goto error;
   }

   *rsa = r;
   ret = SSL_OK;

done:
   if (NULL != bne)
   {
      BN_free(bne);
   }
   return ret;
error:
   if (NULL != r)
   {
      RSA_free(r);
   }
   ret = SSL_ERROR;
   goto done;
}

INT32 ossSSLNewKey(SSLKey** key)
{
   RSA* rsa = NULL;
   EVP_PKEY* pk = NULL;
   INT32 ret;

   SSL_ASSERT(NULL != key);

   ret = _ossSSLNewRSA(&rsa);
   if (SSL_OK != ret)
   {
      goto error;
   }

   pk = EVP_PKEY_new();
   if (NULL == pk)
   {
      goto error;
   }

   ret = EVP_PKEY_assign_RSA(pk, rsa);
   if (!ret)
   {
      goto error;
   }

   *key = pk;
   ret = SSL_OK;

done:
   return ret;
error:
   if (NULL != rsa)
   {
      RSA_free(rsa);
   }
   if (NULL != pk)
   {
      EVP_PKEY_free(pk);
   }
   ret = SSL_ERROR;
   goto done;
}

void ossSSLFreeKey(SSLKey** key)
{
   if (NULL != *key)
   {
      EVP_PKEY_free(*key);
      *key = NULL;
   }
}

static INT32 _ossSSLNewSubjectName(X509_NAME** n,
                                          const CHAR* countryName,
                                          const CHAR* stateOrProvinceName,
                                          const CHAR* organizationName,
                                          const CHAR* organizationalUnitName,
                                          const CHAR* commonName)
{
   X509_NAME* name = NULL;
   X509_NAME_ENTRY* entry = NULL;
   INT32 ret;

   SSL_ASSERT(NULL != n);
   SSL_ASSERT(NULL != countryName);
   SSL_ASSERT(NULL != stateOrProvinceName);
   SSL_ASSERT(NULL != organizationName);
   SSL_ASSERT(NULL != organizationalUnitName);
   SSL_ASSERT(NULL != commonName);

   name = X509_NAME_new();
   if (NULL == name)
   {
      goto error;
   }

   entry = X509_NAME_ENTRY_create_by_NID(&entry, NID_countryName,
      V_ASN1_UTF8STRING, (unsigned char*)countryName, (int)strlen(countryName));
   if (NULL == entry)
   {
      goto error;
   }
   ret = X509_NAME_add_entry(name, entry, 0, -1);
   if (!ret)
   {
      goto error;
   }

   entry = X509_NAME_ENTRY_create_by_NID(&entry, NID_stateOrProvinceName,
      V_ASN1_UTF8STRING, (unsigned char*)stateOrProvinceName, (int)strlen(stateOrProvinceName));
   if (NULL == entry)
   {
      goto error;
   }
   ret = X509_NAME_add_entry(name, entry, 1, -1);
   if (!ret)
   {
      goto error;
   }

   entry = X509_NAME_ENTRY_create_by_NID(&entry, NID_organizationName,
      V_ASN1_UTF8STRING, (unsigned char*)organizationName, (int)strlen(organizationName));
   if (NULL == entry)
   {
      goto error;
   }
   ret = X509_NAME_add_entry(name, entry, 2, -1);
   if (!ret)
   {
      goto error;
   }

   entry = X509_NAME_ENTRY_create_by_NID(&entry, NID_organizationalUnitName,
      V_ASN1_UTF8STRING, (unsigned char*)organizationalUnitName, (int)strlen(organizationalUnitName));
   if (NULL == entry)
   {
      goto error;
   }
   ret = X509_NAME_add_entry(name, entry, 3, -1);
   if (!ret)
   {
      goto error;
   }

   entry = X509_NAME_ENTRY_create_by_NID(&entry, NID_commonName,
      V_ASN1_UTF8STRING, (unsigned char*)commonName, (int)strlen(commonName));
   if (NULL == entry)
   {
      goto error;
   }
   ret = X509_NAME_add_entry(name, entry, 4, -1);
   if (!ret)
   {
      goto error;
   }

   *n = name;
   ret = SSL_OK;

done:
   if (NULL != entry)
   {
      X509_NAME_ENTRY_free(entry);
   }
   return ret;
error:
   if (NULL != name)
   {
      X509_NAME_free(name);
   }
   ret = SSL_ERROR;
   goto done;
}

INT32 ossSSLNewCertificate(SSLCertificate** c, SSLKey* key)
{
   X509* cert = NULL;
   X509_NAME* name = NULL;
   INT32 ret;

   SSL_ASSERT(NULL != c);
   SSL_ASSERT(NULL != key);

   cert = X509_new();
   if (NULL == cert)
   {
      goto error;
   }

   ret = X509_set_version(cert, 2);
   if (!ret)
   {
      goto error;
   }

   ret = ASN1_INTEGER_set(X509_get_serialNumber(cert), 0);
   if (!ret)
   {
      goto error;
   }

   if (NULL == X509_gmtime_adj(X509_get_notBefore(cert), 0))
   {
      goto error;
   }

   /* 10 years */
   if (NULL == X509_gmtime_adj(X509_get_notAfter(cert), 60 * 60 * 24 * 365 * 10))
   {
      goto error;
   }

   ret = _ossSSLNewSubjectName(&name,
      "China", "Guangdong", "SequoiaDB", "SequoiaDB", "SequoiaDB Server");
   if (SSL_OK != ret)
   {
      goto error;
   }

   ret = X509_set_subject_name(cert, name);
   if (!ret)
   {
      goto error;
   }

   ret = X509_set_issuer_name(cert, name);
   if (!ret)
   {
      goto error;
   }

   ret = X509_set_pubkey(cert, key);
   if (!ret)
   {
      goto error;
   }

   ret = X509_sign(cert, key, EVP_sha1());
   if (!ret)
   {
      goto error;
   }

   *c = cert;
   ret = SSL_OK;

done:
   if (NULL != name)
   {
      X509_NAME_free(name);
   }
   return ret;
error:
   if (NULL != cert)
   {
      X509_free(cert);
   }
   ret = SSL_ERROR;
   goto done;
}

void ossSSLFreeCertificate(SSLCertificate** cert)
{
   if (NULL != *cert)
   {
      X509_free(*cert);
      *cert = NULL;
   }
}

SDB_EXTERN_C_END

#endif /* SDB_SSL */

