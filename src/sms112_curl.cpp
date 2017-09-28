#include "sms112_curl.h"
#include "utils/log/log.h"

#include <curl/curl.h>
#include <errno.h>

extern CLog g_coLog;

static size_t sms112_curl_write_result( char *p_pcData, size_t p_stSize, size_t p_stNMemb, void *p_pvUserData )
{
  if ( NULL != p_pvUserData ) {
  } else {
    return static_cast<size_t>( -1 );
  }

  size_t stDataSize = p_stSize * p_stNMemb;
  std::string *pstrResult = reinterpret_cast<std::string*>( p_pvUserData );

  pstrResult->append( p_pcData, stDataSize );

  return stDataSize;
}

int sms112_curl_perf_request( const char *p_pszURL, std::string *p_pstrBody, const char *p_pszContentType, std::string &p_strResult )
{
  int iRetVal = 0;
  CURL *psoCURL = NULL;
  CURLcode resCode;
  char mcErrBuf[ CURL_ERROR_SIZE ];
  curl_slist *psoHdrList = NULL;

  do {
    /* инициализация сессии */
    psoCURL = curl_easy_init();
    if ( NULL != psoCURL ) {
    } else {
      iRetVal = ENOMEM;
      break;
    }

    /* задаем буффер для записи описания ошибки */
    resCode = curl_easy_setopt( psoCURL, CURLOPT_ERRORBUFFER, mcErrBuf );
    if ( CURLE_OK == resCode ) {
    } else {
      iRetVal = resCode;
      break;
    }

    /* задаем URL */
    resCode = curl_easy_setopt( psoCURL, CURLOPT_URL, p_pszURL );
    if ( CURLE_OK == resCode ) {
    } else {
      iRetVal = resCode;
      break;
    }

    /* задаем user agent */
    resCode = curl_easy_setopt( psoCURL, CURLOPT_USERAGENT, SMS112_CURL_UAGENT );
    if ( CURLE_OK == resCode ) {
    } else {
      iRetVal = resCode;
      break;
    }

    /* задаем тип запроса */
    resCode = curl_easy_setopt( psoCURL, CURLOPT_HTTPGET, static_cast<long>( 1 ) );
    if ( CURLE_OK == resCode ) {
    } else {
      iRetVal = resCode;
      break;
    }

    /* задаем указатель на буфер для записи результата */
    resCode = curl_easy_setopt( psoCURL, CURLOPT_WRITEDATA, reinterpret_cast<void*>( &p_strResult ) );
    if ( CURLE_OK == resCode ) {
    } else {
      iRetVal = resCode;
      break;
    }

    /* задаем указатель на функцию записи результата */
    resCode = curl_easy_setopt( psoCURL, CURLOPT_WRITEFUNCTION, reinterpret_cast<void*>( sms112_curl_write_result ) );
    if ( CURLE_OK == resCode ) {
    } else {
      iRetVal = resCode;
      break;
    }

    /* просим не проверять сертификат пира */
    resCode = curl_easy_setopt( psoCURL, CURLOPT_SSL_VERIFYPEER, static_cast<long>( 0 ) );
    if ( CURLE_OK == resCode ) {
    } else {
      iRetVal = resCode;
      break;
    }

    /* просим не проверять соответсвие имени хоста сертификата */
    resCode = curl_easy_setopt( psoCURL, CURLOPT_SSL_VERIFYHOST, static_cast<long>( 0 ) );
    if ( CURLE_OK == resCode ) {
    } else {
      iRetVal = resCode;
      break;
    }

    /* добавляем тело запроса (по необходиости) и задаем тип запроса POST */
    if ( NULL != p_pstrBody ) {
      /* задаем тип запроса POST */
      resCode = curl_easy_setopt( psoCURL, CURLOPT_POST, static_cast<long>( 1 ) );
      if ( CURLE_OK == resCode ) {
      } else {
        iRetVal = resCode;
        break;
      }

      /* задаем заголовок Content-Type */
      if ( NULL != p_pszContentType ) {
        psoHdrList = curl_slist_append( psoHdrList, p_pszContentType );

        resCode = curl_easy_setopt( psoCURL, CURLOPT_HTTPHEADER, psoHdrList );
        if ( CURLE_OK == resCode ) {
        } else {
          iRetVal = resCode;
          break;
        }
      }

      /* задаем длину тела запроса POST */
      resCode = curl_easy_setopt( psoCURL, CURLOPT_POSTFIELDSIZE, static_cast<long>( p_pstrBody->length() ) );
      if ( CURLE_OK == resCode ) {
      } else {
        iRetVal = resCode;
        break;
      }

      /* задаем указатель тело запроса POST */
      resCode = curl_easy_setopt( psoCURL, CURLOPT_POSTFIELDS, p_pstrBody->data() );
      if ( CURLE_OK == resCode ) {
      } else {
        iRetVal = resCode;
        break;
      }
    }

    /* выполняем запрос */
    resCode = curl_easy_perform( psoCURL );
    if ( CURLE_OK == resCode ) {
#ifdef DEBUG
      g_coLog.Dump2( "request:\n", p_pszURL );
      g_coLog.Dump2( "response:\n", p_strResult.c_str() );
#endif
    } else {
      UTL_LOG_D( g_coLog, "error occurred: %d; descr: %s", resCode, mcErrBuf );
      iRetVal = resCode;
      break;
    }
  } while ( 0 );

  if ( NULL != psoCURL ) {
    curl_easy_cleanup( psoCURL );
  }
  if ( NULL != psoHdrList ) {
    curl_slist_free_all( psoHdrList );
  }

  return iRetVal;
}

int sms112_curl_init()
{
  CURLcode resCode;

  resCode = curl_global_init( CURL_GLOBAL_SSL );

  return resCode;
}

void sms112_curl_fini()
{
  curl_global_cleanup();
}
