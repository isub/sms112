#include "sms112.h"
#include "sms112_sip.h"

#include <errno.h>
#include <string>
#include <stdio.h>
#include <pjsip.h>
#include <pjlib-util.h>
#include <pjlib.h>
#include <pthread.h>
#include <sys/time.h>

#include <string>
#include <map>

static pj_caching_pool     g_soCachPool;
static pjsip_endpoint     *g_psoEndPoint;
static pj_pool_t          *g_ptPool;
static pjsip_transport    *g_psoTrans;
static pjsip_auth_clt_sess g_soAuthSess;
static pj_bool_t           g_bQuitFlag;
static pjsip_user_agent   *g_psoUserAgent;
static pj_thread_t        *g_threadHandler;

static int worker_thread( void *arg );
static pj_bool_t on_rx_response( pjsip_rx_data *p_psoRxData );
static void on_tsx_state( pjsip_transaction *p_pTrans, pjsip_event *p_pEvent );
static int init_auth();
static void sip_make_body( pj_str_t *p_strBody, const char *p_pszText, const char *p_pszGeoData );
static void send_request( pjsip_dialog *p_psoDialog, const pjsip_method *p_pMethod, const char *p_pszCaller, const char *p_pszText, const char* p_pszGeodata );
static void send_ack( pjsip_dialog *p_psoDialog, pjsip_rx_data *p_psoRxData );

struct SSync {
  pthread_mutex_t *m_ptMutex;
  int m_iResult;
  SSync( pthread_mutex_t *p_ptMutex ) : m_ptMutex( p_ptMutex ), m_iResult( -1 ) { }
};
static std::map<std::string, SSync> g_mapReqLock;
static void sync_init( pj_str_t *p_pstrCallId );
static int  sync_lock( pj_str_t *p_pstrCallId );
static void sync_ulck( pj_str_t *p_pstrCallId, int p_iResult );
static void sync_del( pj_str_t *p_pstrCallId );

static pjsip_module g_soModApp = {
  NULL, NULL,                            /* prev, next.      */
  { const_cast<char*>( "mod-app" ), 7 }, /* Name.            */
  -1,                                    /* Id               */
  PJSIP_MOD_PRIORITY_APPLICATION,        /* Priority         */
  NULL,                                  /* load()           */
  NULL,                                  /* start()          */
  NULL,                                  /* stop()           */
  NULL,                                  /* unload()         */
  NULL,                                  /* on_rx_request()  */
  &on_rx_response,                       /* on_rx_response() */
  NULL,                                  /* on_tx_request.   */
  NULL,                                  /* on_tx_response() */
  &on_tsx_state                          /* on_tsx_state()   */
};

int sms112_sip_init()
{
  int iRetVal = 0;

  pj_status_t tStatus;

  /* инициализация библиотеки */
  tStatus = pj_init();
  PJ_ASSERT_RETURN( tStatus == PJ_SUCCESS, 1 );

  /* инициализация утилит */
  tStatus = pjlib_util_init();
  PJ_ASSERT_RETURN( tStatus == PJ_SUCCESS, 1 );

  /* Must create a pool factory before we can allocate any memory. */
  pj_caching_pool_init( &g_soCachPool, &pj_pool_factory_default_policy, 0 );

  /* Create the endpoint: */
  tStatus = pjsip_endpt_create( &g_soCachPool.factory, "sipstateless", &g_psoEndPoint );
  PJ_ASSERT_RETURN( tStatus == PJ_SUCCESS, 1 );

  {
    pj_sockaddr_in soAddr;
    pj_str_t strSIPLocalAddr;

    strSIPLocalAddr = pj_str( const_cast<char*>( g_psoConf->m_pszSIPLocalAddr ) );
    pj_inet_aton( &strSIPLocalAddr, &soAddr.sin_addr );

    soAddr.sin_family = pj_AF_INET();
    if ( 0 != pj_inet_aton( &strSIPLocalAddr, &soAddr.sin_addr ) ) {
    } else {
      return -1;
    }
    soAddr.sin_port = pj_htons( g_psoConf->m_usSIPLocalPort );

    tStatus = pjsip_udp_transport_start( g_psoEndPoint, &soAddr, NULL, 1, &g_psoTrans );
    if ( tStatus != PJ_SUCCESS ) {
      PJ_LOG( 3, ( __FILE__, "Error starting UDP transport (port in use?)" ) );
      return 1;
    }
  }

  tStatus = pjsip_tsx_layer_init_module( g_psoEndPoint );
  pj_assert( tStatus == PJ_SUCCESS );

  tStatus = pjsip_ua_init_module( g_psoEndPoint, NULL );
  pj_assert( tStatus == PJ_SUCCESS );

  /*
   * Register our module to receive incoming requests.
   */
  tStatus = pjsip_endpt_register_module( g_psoEndPoint, &g_soModApp );
  PJ_ASSERT_RETURN( tStatus == PJ_SUCCESS, 1 );

  g_ptPool = pjsip_endpt_create_pool( g_psoEndPoint, "", 4096, 4096 );

  tStatus = pj_thread_create( g_ptPool, "", &worker_thread, NULL, 0, 0, &g_threadHandler );
  PJ_ASSERT_RETURN( tStatus == PJ_SUCCESS, 1 );

  g_psoUserAgent = pjsip_ua_instance();

  iRetVal = init_auth();

  return iRetVal;
}

void sms112_sip_fini()
{
  g_bQuitFlag = true;
  pj_thread_join( g_threadHandler );
}

/* Worker thread */
static int worker_thread( void *arg )
{
  PJ_UNUSED_ARG( arg );

  while ( ! g_bQuitFlag ) {
    pj_time_val soTimeOut = { 0, 500 };
    pjsip_endpt_handle_events( g_psoEndPoint, &soTimeOut );
  }

  return 0;
}

static pj_bool_t on_rx_response( pjsip_rx_data *p_psoRxData )
{
  pj_bool_t bRetVal = PJ_FALSE;
  pjsip_dialog *psoDialog;
  pjsip_method_e eSIPMethod;
  int iStatusCode;

  psoDialog = pjsip_rdata_get_dlg( p_psoRxData );
  if ( NULL != psoDialog ) {
    pjsip_transaction *psoTrans = pjsip_rdata_get_tsx( p_psoRxData );

    if ( NULL != psoTrans ) {
      eSIPMethod = psoTrans->method.id;
      iStatusCode = psoTrans->status_code;
    } else {
      eSIPMethod = p_psoRxData->msg_info.cseq->method.id;
      iStatusCode = p_psoRxData->msg_info.msg->line.status.code;
    }
    switch ( eSIPMethod ) {
      case PJSIP_INVITE_METHOD:
        UTL_LOG_D( g_coLog, "PJSIP_INVITE_METHOD" );
        if ( iStatusCode / 100 == 2 ) {
          send_ack( psoDialog, p_psoRxData );
          sync_ulck( &psoDialog->call_id->id, 0 );
          bRetVal = PJ_TRUE;
        } else if ( 401 == iStatusCode || 407 == iStatusCode ) {
        } else if ( 300 <= iStatusCode ) {
          pjsip_dlg_dec_session( psoDialog, &g_soModApp );
        }
        break;
      case PJSIP_CANCEL_METHOD:
        UTL_LOG_D( g_coLog, "SIP METHOD: PJSIP_CANCEL_METHOD" );
        break;
      case PJSIP_ACK_METHOD:
        UTL_LOG_D( g_coLog, "SIP METHOD: PJSIP_ACK_METHOD" );
        break;
      case PJSIP_BYE_METHOD:
        UTL_LOG_D( g_coLog, "SIP METHOD: PJSIP_BYE_METHOD" );
        break;
      case PJSIP_REGISTER_METHOD:
        UTL_LOG_D( g_coLog, "SIP METHOD: PJSIP_REGISTER_METHOD" );
        break;
      case PJSIP_OPTIONS_METHOD:
        UTL_LOG_D( g_coLog, "SIP METHOD: PJSIP_OPTIONS_METHOD" );
        break;
      case PJSIP_OTHER_METHOD:
        UTL_LOG_D( g_coLog, "SIP METHOD: PJSIP_OTHER_METHOD" );
        break;
      default:
        UTL_LOG_D( g_coLog, "SIP METHOD: PJSIP_UNDEFINED_METHOD" );
    }
  } else {
    /* non Dialog message */
    bRetVal = PJ_FALSE;
  }

  return bRetVal;
}

static void on_tsx_state( pjsip_transaction *p_pTrans, pjsip_event *p_pEvent )
{
  pj_assert( p_pEvent->type == PJSIP_EVENT_TSX_STATE );
  pj_status_t tStatus;

  int iCode = p_pEvent->body.tsx_state.tsx->status_code;

#ifdef DEBUG
  {
    const char *pszState;
    switch ( p_pTrans->state ) {
      case PJSIP_TSX_STATE_NULL:
        pszState = "PJSIP_TSX_STATE_NULL";
        break;
      case PJSIP_TSX_STATE_CALLING:
        pszState = "PJSIP_TSX_STATE_CALLING";
        break;
      case PJSIP_TSX_STATE_TRYING:
        pszState = "PJSIP_TSX_STATE_TRYING";
        break;
      case PJSIP_TSX_STATE_PROCEEDING:
        pszState = "PJSIP_TSX_STATE_PROCEEDING";
        break;
      case PJSIP_TSX_STATE_COMPLETED:
        pszState = "PJSIP_TSX_STATE_COMPLETED";
        break;
      case PJSIP_TSX_STATE_CONFIRMED:
        pszState = "PJSIP_TSX_STATE_CONFIRMED";
        break;
      case PJSIP_TSX_STATE_TERMINATED:
        pszState = "PJSIP_TSX_STATE_TERMINATED";
        break;
      case PJSIP_TSX_STATE_DESTROYED:
        pszState = "PJSIP_TSX_STATE_DESTROYED";
        break;
      case PJSIP_TSX_STATE_MAX:
        pszState = "PJSIP_TSX_STATE_MAX";
        break;
      default:
        pszState = "PJSIP_TSX_STATE_UNDEFINED";
        break;
    }
    UTL_LOG_D( g_coLog, "transaction state: code: '%d'; name: '%s'; event code: '%d'", p_pTrans->state, pszState, iCode );
  }
#endif

  if ( PJSIP_TSX_STATE_TERMINATED == p_pTrans->state ) {
  } else if ( 401 == iCode || 407 == iCode ) {
    pjsip_tx_data *new_request;
    tStatus = pjsip_auth_clt_reinit_req( &g_soAuthSess, p_pEvent->body.tsx_state.src.rdata, p_pEvent->body.tsx_state.tsx->last_tx, &new_request );

    if ( tStatus == PJ_SUCCESS ){
      tStatus = pjsip_endpt_send_request( g_psoEndPoint, new_request, -1, NULL, NULL );
    } else {
      UTL_LOG_E( g_coLog, "Authentication failed!!!" );
    }
  }
}

static int init_auth()
{
  pj_status_t tStatus;

  tStatus = pjsip_auth_clt_init( &g_soAuthSess, g_psoEndPoint, g_ptPool, 0 );
  pj_assert( tStatus == PJ_SUCCESS );

  pjsip_cred_info soCredInfo;
  memset( &soCredInfo, 0, sizeof( soCredInfo ) );
  soCredInfo.realm = pj_str( const_cast<char*>( g_psoConf->m_pszSIPAuthRealm ) );
  soCredInfo.scheme = pj_str( const_cast<char*>( g_psoConf->m_pszSIPAuthScheme ) );
  soCredInfo.username = pj_str( const_cast<char*>( g_psoConf->m_pszSIPAuthUserName ) );
  soCredInfo.data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
  soCredInfo.data = pj_str( const_cast<char*>( g_psoConf->m_pszSIPAuthPassword ) );
  tStatus = pjsip_auth_clt_set_credentials( &g_soAuthSess, 1, &soCredInfo );
  pj_assert( tStatus == PJ_SUCCESS );

  return 0;
}

int sms112_sip_send_req( const char *p_pszCaller, const char *p_pszText, const char *p_pszGeodata )
{
  int iRetVal = 0;
  int iFnRes;
  char mcLocal[ 1024 ] = { '\0' };
  char mcRemote[ 1024 ] = { '\0' };
  pj_str_t strLocal;
  pj_str_t strRemote;
  pj_status_t tStatus;
  pj_thread_desc tDesc;
  pj_thread_t *threadThis;

  pj_bzero( tDesc, sizeof( tDesc ) );

  tStatus = pj_thread_register( __FILE__, tDesc, &threadThis );
  pj_assert( tStatus == PJ_SUCCESS );

  do {
    if ( NULL != g_psoTrans && NULL != g_psoTrans->local_name.host.ptr ) {
      iFnRes = snprintf( mcLocal, sizeof( mcLocal ), "\"%s\" <sip:%s@%s>", p_pszCaller, p_pszCaller, g_psoTrans->local_name.host.ptr );
    } else {
      iFnRes = snprintf( mcLocal, sizeof( mcLocal ), "\"%s\" <sip:%s>", p_pszCaller, p_pszCaller );
    }
    if ( iFnRes > 0 && iFnRes < sizeof( mcLocal ) ) {
    } else {
      iRetVal = -1;
      break;
    }
    pj_strset2( &strLocal, mcLocal );

    if ( g_psoConf->m_usSIPSrvPort == 5060 || g_psoConf->m_usSIPSrvPort == 0 ) {
      iFnRes = snprintf( mcRemote, sizeof( mcRemote ), "\"%s\" <sip:%s@%s>", g_psoConf->m_pszSIPTo, g_psoConf->m_pszSIPTo, g_psoConf->m_pszSIPSrvAdr );
    } else {
      iFnRes = snprintf( mcRemote, sizeof( mcRemote ), "\"%s\" <sip:%s@%s:%u>", g_psoConf->m_pszSIPTo, g_psoConf->m_pszSIPTo, g_psoConf->m_pszSIPSrvAdr, g_psoConf->m_usSIPSrvPort );
    }
    if ( iFnRes > 0 && iFnRes < sizeof( mcRemote ) ) {
    } else {
      iRetVal = -1;
      break;
    }

    pjsip_dialog *psoDialog = NULL;

    pj_strset2( &strRemote, mcRemote );
    tStatus = pjsip_dlg_create_uac( g_psoUserAgent, &strLocal, &strLocal, &strRemote, &strRemote, &psoDialog );
    pj_assert( tStatus == PJ_SUCCESS );

    pjsip_dlg_inc_lock( psoDialog );

    tStatus = pjsip_dlg_add_usage( psoDialog, &g_soModApp, NULL );
    pj_assert( tStatus == PJ_SUCCESS );

    pjsip_dlg_inc_session( psoDialog, &g_soModApp );

    sync_init( &psoDialog->call_id->id );

    send_request( psoDialog, &pjsip_invite_method, p_pszCaller, p_pszText, p_pszGeodata );
    pjsip_dlg_dec_lock( psoDialog );

    iRetVal = sync_lock( &psoDialog->call_id->id );
    sync_del( &psoDialog->call_id->id );
  } while ( 0 );

  return iRetVal;
}

static void sip_make_body( pj_str_t *p_strBody, const char *p_pszText, const char *p_pszGeoData )
{
  pj_strcat2( p_strBody, "--" );
  pj_strcat2( p_strBody, g_psoConf->m_pszBoundary );
  pj_strcat2( p_strBody, "\r\n" );

  pj_strcat2( p_strBody, "Content-Type: application/sdp\r\n\r\n" );

  pj_strcat2( p_strBody, "(v): 0\r\n" );
  pj_strcat2( p_strBody, "(o): itscc 0 0 IN IP4 0.0.0.0\r\n" );
  pj_strcat2( p_strBody, "(s): its\r\n" );
  pj_strcat2( p_strBody, "(c): IN IP4 0.0.0.0\r\n" );
  pj_strcat2( p_strBody, "(t): 0 0\r\n" );
  pj_strcat2( p_strBody, "(m): audio 1234 RTP/AVP 8\r\n" );
  pj_strcat2( p_strBody, "(a): inactive\r\n" );

  pj_strcat2( p_strBody, "--" );
  pj_strcat2( p_strBody, g_psoConf->m_pszBoundary );
  pj_strcat2( p_strBody, "\r\n" );

  pj_strcat2( p_strBody, "Content-Type: application/pidf+xml\r\n" );
  pj_strcat2( p_strBody, "Content-ID: target123@172.16.78.4\r\n\r\n" );

  pj_strcat2( p_strBody, "<?xml version = \"1.0\" encoding = \"UTF-8\"?>\r\n" );
  pj_strcat2( p_strBody, p_pszGeoData );

  pj_strcat2( p_strBody, "\r\n--" );
  pj_strcat2( p_strBody, g_psoConf->m_pszBoundary );
  pj_strcat2( p_strBody, "\r\n" );

  pj_strcat2( p_strBody, "Content-Type: text/plain; charset=UTF-8\r\n" );
  pj_strcat2( p_strBody, "Content-Transfer-Encoding: 8bit\r\n" );
  pj_strcat2( p_strBody, "Content-Disposition: form-data; name=\"part\"\r\n\r\n" );

  pj_strcat2( p_strBody, p_pszText );

  pj_strcat2( p_strBody, "\r\n--" );
  pj_strcat2( p_strBody, g_psoConf->m_pszBoundary );
  pj_strcat2( p_strBody, "--\r\n" );
}

/* Send request */
static void send_request( pjsip_dialog *p_psoDialog, const pjsip_method *p_pMethod, const char *p_pszCaller, const char *p_pszText, const char* p_pszGeodata )
{
  pjsip_tx_data     *pTData;
  pj_str_t strBody = pj_str( const_cast<char*>( "" ) );
  pj_status_t        tStatus;
  pj_thread_t       *threadThis;

  tStatus = pjsip_dlg_create_request( p_psoDialog, p_pMethod, -1, &pTData );
  pj_assert( tStatus == PJ_SUCCESS );

  /* задаем заголовок Accept */
  {
    pjsip_accept_hdr *psoHdr;
    psoHdr = pjsip_accept_hdr_create( g_ptPool );
    psoHdr->count = 2;
    psoHdr->values[ 0 ] = pj_str( const_cast<char*>( "application/sdp" ) );
    psoHdr->values[ 1 ] = pj_str( const_cast<char*>( "application/pidf+xml" ) );
    pjsip_msg_add_hdr( pTData->msg, reinterpret_cast<pjsip_hdr*>( psoHdr ) );
  }

  char mcGeolocatoin[ 1024 ] = { '\0' };
  /* задаем заголовок Geolocation */
  {
    pj_str_t strValue;
    pjsip_generic_array_hdr *psoHdr;

    const pj_str_t strHdrName = pj_str( const_cast<char*>( "Geolocation" ) );
    psoHdr = pjsip_generic_array_hdr_create( g_ptPool, &strHdrName );
    psoHdr->count = 1;
    pj_strset2( &strValue, mcGeolocatoin );
    pj_strcat2( &strValue, "<cid:" );
    pj_strcat2( &strValue, g_psoConf->m_pszGeolocationCId );
    pj_strcat2( &strValue, "@172.27.40.129>" );
    psoHdr->values[ 0 ] = strValue;
    pjsip_msg_add_hdr( pTData->msg, reinterpret_cast<pjsip_hdr*>( psoHdr ) );
  }

  /* задаем заголовок Geolocation-Routing */
  {
    pjsip_generic_array_hdr *psoHdr;
    const pj_str_t strHdrName = pj_str( const_cast<char*>( "Geolocation-Routing" ) );
    psoHdr = pjsip_generic_array_hdr_create( g_ptPool, &strHdrName );
    psoHdr->count = 1;
    psoHdr->values[ 0 ] = pj_str( const_cast<char*>( "no" ) );
    pjsip_msg_add_hdr( pTData->msg, reinterpret_cast<pjsip_hdr*>( psoHdr ) );
  }

  /* задаем заголовок P-Visited-Network-ID */
  {
    pjsip_generic_array_hdr *psoHdr;
    const pj_str_t strHdrName = pj_str( const_cast<char*>( "P-Visited-Network-ID" ) );
    psoHdr = pjsip_generic_array_hdr_create( g_ptPool, &strHdrName );
    psoHdr->count = 1;
    psoHdr->values[ 0 ] = pj_str( const_cast<char*>( g_psoConf->m_pszVisitedNetworkID ) );
    pjsip_msg_add_hdr( pTData->msg, reinterpret_cast<pjsip_hdr*>( psoHdr ) );
  }

  /* формируем тело запроса */
  char mcBody[ 4096 ] = { '\0' };
  pj_strset2( &strBody, mcBody );
  sip_make_body( &strBody, p_pszText, p_pszGeodata );

  char mcContentType[ 1024 ] = { '\0' };
  pjsip_msg_body *pBody;
  pj_str_t strContentType = pj_str( const_cast<char*>( "multipart" ) );
  pj_str_t strContSubType;

  pj_strset2( &strContSubType, mcContentType );
  pj_strcat2( &strContSubType, "mixed; boundary=" );
  pj_strcat2( &strContSubType, g_psoConf->m_pszBoundary );

  strBody.slen = pj_ansi_strlen( strBody.ptr );
  pBody = pjsip_msg_body_create( pTData->pool, &strContentType, &strContSubType, &strBody );
  pTData->msg->body = pBody;

  tStatus = pjsip_dlg_send_request( p_psoDialog, pTData, -1, NULL );
  pj_assert( tStatus == PJ_SUCCESS );
}

static void send_ack( pjsip_dialog *p_psoDialog, pjsip_rx_data *p_psoRxData )
{
  pjsip_tx_data *psoTxData;
  pj_status_t tStatus;

  tStatus = pjsip_dlg_create_request( p_psoDialog, &pjsip_ack_method, p_psoRxData->msg_info.cseq->cseq, &psoTxData );
  pj_assert( tStatus == PJ_SUCCESS );

  tStatus = pjsip_dlg_send_request( p_psoDialog, psoTxData, -1, NULL );
  pj_assert( tStatus == PJ_SUCCESS );
}

static void sync_init( pj_str_t *p_pstrCallId )
{
  if ( NULL != p_pstrCallId->ptr && 0 < p_pstrCallId->slen ) {
  } else {
    return;
  }

  std::string strCallId( p_pstrCallId->ptr );
  SSync soSync( new pthread_mutex_t );

  pthread_mutex_init( soSync.m_ptMutex, NULL );
  pthread_mutex_lock( soSync.m_ptMutex );

  g_mapReqLock.insert( std::pair<std::string, SSync>( strCallId, soSync ) );

  UTL_LOG_D( g_coLog, "syncroniser for '%s' is created", p_pstrCallId->ptr );
}

static int sync_lock( pj_str_t *p_pstrCallId )
{
  if ( NULL != p_pstrCallId->ptr && 0 < p_pstrCallId->slen ) {
  } else {
    return -1;
  }

  int iRetVal = -1;

  std::string strCallId( p_pstrCallId->ptr );
  std::map<std::string, SSync>::iterator iter;

  iter = g_mapReqLock.find( strCallId );
  if ( iter != g_mapReqLock.end() ) {
    timespec soTS;

    UTL_LOG_D( g_coLog, "syncroniser for '%s' is locked", p_pstrCallId->ptr );

    if ( 0 == sms112_make_timespec_timeout( soTS, g_psoConf->m_uiSIPTimeout, 0 ) ) {
      iRetVal = pthread_mutex_timedlock( iter->second.m_ptMutex, &soTS );
      if( 0 == iRetVal ) {
        iRetVal = iter->second.m_iResult;
      } else {
      }
    } else {
      iRetVal = -1;
    }
  } else {
    UTL_LOG_D( g_coLog, "syncroniser for '%s' not found", p_pstrCallId->ptr );
  }

  return iRetVal;
}

static void sync_ulck( pj_str_t *p_pstrCallId, int p_iResult )
{
  if ( NULL != p_pstrCallId->ptr && 0 < p_pstrCallId->slen ) {
  } else {
    return;
  }

  std::string strCallId( p_pstrCallId->ptr );
  std::map<std::string, SSync>::iterator iter;

  iter = g_mapReqLock.find( strCallId );
  if ( iter != g_mapReqLock.end() ) {
    iter->second.m_iResult = p_iResult;
    pthread_mutex_unlock( iter->second.m_ptMutex );
    UTL_LOG_D( g_coLog, "syncroniser for '%s' is unlocked", p_pstrCallId->ptr );
  } else {
    UTL_LOG_D( g_coLog, "syncroniser for '%s' not found", p_pstrCallId->ptr );
  }
}

static void sync_del( pj_str_t *p_pstrCallId )
{
  if ( NULL != p_pstrCallId->ptr && 0 < p_pstrCallId->slen ) {
  } else {
    return;
  }

  std::string strCallId( p_pstrCallId->ptr );
  std::map<std::string, SSync>::iterator iter;

  iter = g_mapReqLock.find( strCallId );
  if ( iter != g_mapReqLock.end() ) {
    pthread_mutex_destroy( iter->second.m_ptMutex );
    delete iter->second.m_ptMutex;
    g_mapReqLock.erase( iter );
    UTL_LOG_D( g_coLog, "syncroniser for '%s' is destroyed", p_pstrCallId->ptr );
  } else {
    UTL_LOG_D( g_coLog, "syncroniser for '%s' not found", p_pstrCallId->ptr );
  }
}
