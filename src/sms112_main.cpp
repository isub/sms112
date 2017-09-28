#include "sms112.h"
#include "sms112_curl.h"
#include "sms112_json.h"
#include "sms112_sip.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

static SConf g_soConf;
SConf *g_psoConf;

CLog g_coLog;

int main( int p_iArgC, char *p_mpszArgV[ ] )
{
  int iRetVal = 0;
  int iSMPPSock;
  int iFnRes;
  pthread_t tThreadPing;
  pthread_t tThreadRecvr;
  const char *pszConfFile = NULL;

  if ( 2 == p_iArgC) {
    pszConfFile = p_mpszArgV[ 1 ];
  } else {
    pszConfFile = "/usr/local/etc/sms112/sms112.conf";
  }

  /* загрузка конфигураци */
  iFnRes = sms112_load_conf( pszConfFile );
  if ( 0 == iFnRes ) {
  } else {
    return iRetVal;
  }

  /* инициализация логгера */
  iFnRes = g_coLog.Init( g_psoConf->m_pszLogFileMask );
  if ( 0 == iFnRes ) {
  } else {
    return iRetVal;
  }

  iFnRes = sms112_curl_init();
  if ( 0 == iFnRes ) {
  } else {
    return iRetVal;
  }

  iFnRes = sms112_sip_init();
  if ( 0 == iFnRes ) {
  } else {
    return iRetVal;
  }

  iFnRes = sms112_reg_sig();
  if ( 0 == iFnRes ) {
    UTL_LOG_N( g_coLog, "register signal handler: ok" );
  } else {
    UTL_LOG_E( g_coLog, "register signal handler: failed!!! error code: %d", iFnRes );
  }

  iFnRes = sms112_tcp_connect(iSMPPSock);
  if(0 == iFnRes) {
    UTL_LOG_N( g_coLog, "establish tcp-connection to SMPP-server: ok" );
  } else {
    UTL_LOG_N( g_coLog, "establish tcp-connection to SMPP-server: failed!!! error code: %d", iFnRes );
    return iRetVal;
  }

  iFnRes = pthread_create(&tThreadRecvr, NULL, msg_receiver, &iSMPPSock);
  if(0 == iFnRes) {
    UTL_LOG_N( g_coLog, "create smpp listener thread: ok" );
  } else {
    UTL_LOG_E( g_coLog, "create smpp listener thread: failed!!! error code: %d", iFnRes );
  }

  iFnRes = sms112_smpp_connect( iSMPPSock );
  if ( 0 == iFnRes ) {
    UTL_LOG_N( g_coLog, "establish SMPP connection: ok" );
  } else {
    UTL_LOG_E( g_coLog, "establish SMPP connection: failed!!! error code: %d", iFnRes );
    iRetVal = iFnRes;
    goto cleanup_and_exit;
  }

  iFnRes = pthread_create(&tThreadPing, NULL, do_smpp_ping, &iSMPPSock);
  if ( 0 == iFnRes ) {
    UTL_LOG_N( g_coLog, "create ENQUIRE_LINK thread: ok" );
    pthread_join( tThreadPing, NULL );
  } else {
    UTL_LOG_N( g_coLog, "create ENQUIRE_LINK thread: failed!!! error code: %d", iFnRes );
  }

  disconnect_and_clean:
  iFnRes = sms112_smpp_disconn(iSMPPSock);
  if(0 == iFnRes) {
    UTL_LOG_N( g_coLog, "close SMPP connection: ok");
  } else {
    UTL_LOG_E( g_coLog, "close SMPP connection: failed!!! error code: %d", iFnRes );
    iRetVal = iFnRes;
    goto cleanup_and_exit;
  }

  cleanup_and_exit:
  sms112_tcp_disconn(iSMPPSock);

  sms112_sip_fini();

  sms112_curl_fini();

  g_coLog.Flush();

  return 0;
}

int sms112_send_report( const char *p_pszPhoneNumber, const char *p_pszMessage, const char *p_pszResultString, const char *p_pszResultXML )
{
  int iRetVal = 0;
  int iFnRes;
  char mcTimeStamp[ 32 ];
  time_t tTime;
  tm soTM;

  if ( static_cast<time_t>( -1 ) != time( &tTime ) ) {
  } else {
    return -1;
  }

  if ( NULL != localtime_r( &tTime, &soTM ) ) {
  } else {
    return -1;
  }

  iFnRes = strftime( mcTimeStamp, sizeof( mcTimeStamp ), "%Y-%m-%dT%H:%M:%S", &soTM );
  if ( iFnRes < sizeof( mcTimeStamp ) ) {
  } else {
    return -1;
  }

  const char *mpszParam[ ] = { "SMS", mcTimeStamp, p_pszPhoneNumber, p_pszMessage, p_pszResultString, p_pszResultXML };

  const SParamDescr msoParam[ ] = {
    { "EventType",    1 },
    { "EventDate",    1 },
    { "PhoneNumber",  1 },
    { "TextMessage",  1 },
    { "ResultString", 1 },
    { "ResultXML",    1 }
  };

  std::string strReq;
  std::string strResp;

  sms112_make_json( mpszParam, sizeof(mpszParam)/sizeof(*mpszParam), msoParam, sizeof( msoParam ) / sizeof( *msoParam ), strReq );

  sms112_curl_perf_request( g_psoConf->m_pszReportURL, &strReq, "Content-Type: application/json", strResp );

  return 0;
}

int sms112_load_conf( const char *p_pszConf )
{
  if ( NULL != p_pszConf ) {
  } else {
    return EINVAL;
  }

  int iRetVal = 0;
  int iFnRes;
  CConfig coConf;
  std::string strParamValue;

  g_psoConf = &g_soConf;

  iFnRes = coConf.LoadConf( p_pszConf );
  if ( 0 == iFnRes ) {
    UTL_LOG_N( g_coLog, "open configuration file: ok" );
  } else {
    UTL_LOG_E( g_coLog, "open configuration file: failed!!! error code: %d", iFnRes );
    return iFnRes;
  }

  iFnRes = coConf.GetParamValue( "SMPPServer", strParamValue );
  if ( 0 == iFnRes ) {
    g_soConf.m_pszSMPPServer = strdup( strParamValue.c_str() );
  } else {
    return EINVAL;
  }

  iFnRes = coConf.GetParamValue( "SMPPPort", strParamValue );
  if ( 0 == iFnRes ) {
    g_soConf.m_usSMPPPort = static_cast<uint16_t>( std::stoul( strParamValue ) );
  } else {
    return EINVAL;
  }

  iFnRes = coConf.GetParamValue( "SMPPEnqLinkSpan", strParamValue );
  if ( 0 == iFnRes ) {
    g_soConf.m_uiSMPPEnqLinkSpan = std::stoul( strParamValue );
  } else {
    return EINVAL;
  }

  iFnRes = coConf.GetParamValue( "ESMESysId", strParamValue );
  if ( 0 == iFnRes ) {
    g_soConf.m_pszESMESysId = strdup( strParamValue.c_str() );
  } else {
    return EINVAL;
  }

  iFnRes = coConf.GetParamValue( "ESMEPaswd", strParamValue );
  if ( 0 == iFnRes ) {
    g_soConf.m_pszESMEPaswd = strdup( strParamValue.c_str() );
  } else {
    return EINVAL;
  }

  iFnRes = coConf.GetParamValue( "ESMEType", strParamValue );
  if ( 0 == iFnRes ) {
    g_soConf.m_pszESMEType = strdup( strParamValue.c_str() );
  } else {
    return EINVAL;
  }

  iFnRes = coConf.GetParamValue( "ReportURL", strParamValue );
  if ( 0 == iFnRes ) {
    g_soConf.m_pszReportURL = strdup( strParamValue.c_str() );
  } else {
    return EINVAL;
  }

  iFnRes = coConf.GetParamValue( "SIPSrvAdr", strParamValue );
  if ( 0 == iFnRes ) {
    g_soConf.m_pszSIPSrvAdr = strdup( strParamValue.c_str() );
  } else {
    return EINVAL;
  }

  iFnRes = coConf.GetParamValue( "SIPSrvPort", strParamValue );
  if ( 0 == iFnRes ) {
    g_soConf.m_usSIPSrvPort = static_cast<uint16_t>( std::stoul( strParamValue ) );
  } else {
    return EINVAL;
  }

  iFnRes = coConf.GetParamValue( "SIPUsrAgn", strParamValue );
  if ( 0 == iFnRes ) {
    g_soConf.m_pszSIPUsrAgn = strdup( strParamValue.c_str() );
  } else {
    return EINVAL;
  }

  iFnRes = coConf.GetParamValue( "Boundary", strParamValue );
  if ( 0 == iFnRes ) {
    g_soConf.m_pszBoundary = strdup( strParamValue.c_str() );
  } else {
    return EINVAL;
  }

  iFnRes = coConf.GetParamValue( "LocalSIPAddr", strParamValue );
  if ( 0 == iFnRes ) {
    g_soConf.m_pszLocalSIPAddr = strdup( strParamValue.c_str() );
  } else {
    return EINVAL;
  }

  iFnRes = coConf.GetParamValue( "VisitedNetworkID", strParamValue );
  if ( 0 == iFnRes ) {
    g_soConf.m_pszVisitedNetworkID = strdup( strParamValue.c_str() );
  } else {
    return EINVAL;
  }

  iFnRes = coConf.GetParamValue( "GeolocationCId", strParamValue );
  if ( 0 == iFnRes ) {
    g_soConf.m_pszGeolocationCId = strdup( strParamValue.c_str() );
  } else {
    return EINVAL;
  }

  iFnRes = coConf.GetParamValue( "GeolocationURLTemplate", strParamValue );
  if ( 0 == iFnRes ) {
    g_soConf.m_pszGeolocationURLTemplate = strdup( strParamValue.c_str() );
  } else {
    return EINVAL;
  }

  iFnRes = coConf.GetParamValue( "LogFileMask", strParamValue );
  if ( 0 == iFnRes ) {
    g_soConf.m_pszLogFileMask = strdup( strParamValue.c_str() );
  } else {
    return EINVAL;
  }

  return iRetVal;
}

int sms112_make_timespec_timeout( timespec &p_soTimeSpec, uint32_t p_uiSec, uint32_t p_uiAddUSec )
{
  timeval soTimeVal;

  if ( 0 == gettimeofday( &soTimeVal, NULL ) ) {
  } else {
    return -1;
  }
  p_soTimeSpec.tv_sec = soTimeVal.tv_sec;
  p_soTimeSpec.tv_sec += p_uiSec;
  if ( ( soTimeVal.tv_usec + p_uiAddUSec ) < USEC_PER_SEC ) {
    p_soTimeSpec.tv_nsec = ( soTimeVal.tv_usec + p_uiAddUSec ) * NSEC_PER_USEC;
  } else {
    ++p_soTimeSpec.tv_sec;
    p_soTimeSpec.tv_nsec = ( soTimeVal.tv_usec - USEC_PER_SEC + p_uiAddUSec ) * NSEC_PER_USEC;
  }

  return 0;
}
