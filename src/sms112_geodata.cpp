#include "sms112.h"
#include "sms112_geodata.h"
#include "sms112_curl.h"

#include <stdio.h>
#include <errno.h>

int sms112_geodata_get( const char *p_pszCaller, std::string &p_strGeoData )
{
  if ( NULL != p_pszCaller ) {
  } else {
    return EINVAL;
  }

  int iRetVal = 0;
  int iFnRes;
  char mcURL[ 2048 ];
  std::string strResult;

  iFnRes = snprintf( mcURL, sizeof( mcURL ), const_cast<char*>(g_psoConf->m_pszGeolocationURLTemplate), p_pszCaller );
  if ( iFnRes < sizeof( mcURL ) ) {
  } else {
    return EINVAL;
  }
  iRetVal = sms112_curl_perf_request( mcURL, NULL, NULL, strResult );
  if ( 0 == iRetVal ) {
#ifdef DEBUG
    g_coLog.Dump2( "geodata is received:\n", strResult.c_str() );
#endif
    size_t stStart;
    size_t stLen;

    stStart = strResult.find( "<presence", 0 );
    if ( stStart != std::string::npos ) {
      stLen = strResult.find( "</presence>", stStart );
      if ( stLen != std::string::npos ) {
        stLen += 11;
        stLen -= stStart;
        p_strGeoData = strResult.substr( stStart, stLen );
      } else {
        UTL_LOG_D( g_coLog, "end xml tag is not found" );
        iRetVal = -1;
      }
    } else {
#ifdef DEBUG
      UTL_LOG_D( g_coLog, "start xml tag is not found" );
#endif
      iRetVal = -1;
    }
  }

  return iRetVal;
}
