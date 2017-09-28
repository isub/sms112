#include "sms112.h"
#include "sms112_sip.h"
#include <errno.h>
#include <string>

#include "utils/ipconnector/ipconnector.h"
#include "sms112_geodata.h"

int sms112_sip_init()
{
  int iRetVal = 0;
  return iRetVal;
}

void sms112_sip_fini()
{
}

/* функция формирования заголовка запроса */
static int sms112_sip_make_hdr( const char *p_pszCaller, std::string &p_strReq );

/* функция формирования тела запроса */
static int sms112_sip_make_body( std::string &p_strBody, std::string &p_strGeoData, const char *p_pszText );

int sms112_sip_send_req( const char *p_pszCaller, const char *p_pszText )
{
  int iRetVal = 0;
  int iFnRes;
  CIPConnector coIPConn;
  std::string strGeoData;
  std::string strRequest;
  std::string strBody;

  iFnRes = sms112_geodata_get( p_pszCaller, strGeoData );
  if ( 0 == iFnRes ) {
    UTL_LOG_D( g_coLog, "geodata is received" );
  } else {
    UTL_LOG_E( g_coLog, "get geodata: failed!!! error code: %d", iFnRes );
    return -1;
  }

  /* задаем таймаут для операций с сокетом (0.1 сек.) */
  coIPConn.SetTimeout( 100 );

  /* подключаемся к удаленному серверу */
  coIPConn.Connect( g_psoConf->m_pszSIPSrvAdr, g_psoConf->m_usSIPSrvPort, IPPROTO_UDP );

  /* формируем основную часть заголовка */
  if ( 0 == sms112_sip_make_hdr( p_pszCaller, strRequest ) ) {
  } else {
    iRetVal = -1;
    goto cleanup_and_exit;
  }

  /* формируем тело запроса */
  if ( 0 == sms112_sip_make_body( strBody, strGeoData, p_pszText ) ) {
  } else {
    iRetVal = -1;
    goto cleanup_and_exit;
  }

  /* дописываем длину контента и присоединяем тело запроса */
  strRequest += "Content-Length: ";
  strRequest += std::to_string( strBody.length() );
  strRequest += "\n\n";
  strRequest += strBody;

#ifdef DEBUG
  g_coLog.Dump2( "sip request:\n", strRequest.c_str() );
#endif

  /* отправляем запрос */
  iFnRes = coIPConn.Send( strRequest.data(), strRequest.length() );
  if ( 0 == iFnRes ) {
    UTL_LOG_N( g_coLog, "send sip request: ok" );
  } else {
    UTL_LOG_E( g_coLog, "send sip request: failed!!! error code: %d", iFnRes );
    goto cleanup_and_exit;
  }

  /* ждем ответ */
  char mcBuf[ 4096 ];
  iFnRes = coIPConn.Recv( mcBuf, sizeof( mcBuf ) );
  if ( 0 < iFnRes ) {
#ifdef DEBUG
    mcBuf[ iFnRes >= sizeof( mcBuf ) ? sizeof( mcBuf ) - 1 : iFnRes ] = '\0';
    g_coLog.Dump2( "sip response recived:\n", mcBuf );
#endif
  } else {
    UTL_LOG_E( g_coLog, "receive sip response: failed!!! error code: %d\n", iFnRes );
  }

  cleanup_and_exit:

  sms112_send_report(p_pszCaller, p_pszText, 0 < iFnRes ? "OK" : "Error: there is no response from the SIP server", strGeoData.c_str());

  /* завершаем соединение */
  coIPConn.DisConnect();

  return iRetVal;
}

static int sms112_sip_make_hdr( const char *p_pszCaller, std::string &p_strReq )
{
  int iRetVal = 0;
  static unsigned int uiTrans = 1;
  static unsigned int uiReqId = 1;
  static unsigned int uiTag = 1;
  static unsigned long long ullCallId = 1;
  static unsigned long long ullCSeq = 1;

  /* формируем строку запроса */
  p_strReq = "INVITE sip:112@";
  p_strReq += g_psoConf->m_pszSIPSrvAdr;
  p_strReq += ":";
  p_strReq += std::to_string( g_psoConf->m_usSIPSrvPort );
  p_strReq += " SIP/2.0\n";

  /* формируем заголовок Via */
  p_strReq += "Via: SIP/2.0/";
  p_strReq += "UDP";
  p_strReq += " ";
  p_strReq += g_psoConf->m_pszLocalSIPAddr;
  p_strReq += ";branch=z9hG4bK-";
  p_strReq += std::to_string( uiReqId++ );
  p_strReq += "\n";

  /* формируем заголовок Max-Forwards */
  p_strReq += "Max-Forwards: 70\n";

  /* формируем заголовок To */
  p_strReq += "To: \"112\" <sip:112@";
  p_strReq += g_psoConf->m_pszSIPSrvAdr;
  p_strReq += ">\n";

  /* формируем заголовок From */
  p_strReq += "From: \"";
  p_strReq += p_pszCaller;
  p_strReq += "\" <sip:";
  p_strReq += p_pszCaller;
  p_strReq += "@";
  p_strReq += g_psoConf->m_pszLocalSIPAddr;
  p_strReq += ">;tag=";
  p_strReq += std::to_string( uiTag++ ) + "\n";

  /* формируем заголовок Call-ID */
  p_strReq += "Call-ID: ";
  p_strReq += std::to_string( ullCallId++ ) + "@";
  p_strReq += g_psoConf->m_pszLocalSIPAddr;
  p_strReq += "\n";

  /* формируем заголовок Geolocation */
  p_strReq += "Geolocation: <cid:TMT@";
  p_strReq += g_psoConf->m_pszLocalSIPAddr;
  p_strReq += ">\n";

  /* формируем заголовок Geolocation-Routing */
  p_strReq += "Geolocation-Routing: no\n";

  /* формируем заголовок Accept */
  p_strReq += "Accept: application/sdp, application/pidf+xml\n";

  /* формируем заголовок CSeq */
  p_strReq += "CSeq: ";
  p_strReq += std::to_string( ullCSeq ++ ) + " INVITE\n";

  /* формируем заголовок Contact */
  p_strReq += "Contact: <sip:";
  p_strReq += p_pszCaller;
  p_strReq += "@";
  p_strReq += g_psoConf->m_pszLocalSIPAddr;
  p_strReq += ">\n";

  /* формируем заголовок Content-Type */
  p_strReq += "Content-Type: multipart/mixed; boundary=";
  p_strReq += g_psoConf->m_pszBoundary;
  p_strReq += "\n";

  /* формируем заголовок P-Visited-Network-ID */
  p_strReq += "P-Visited-Network-ID: ";
  p_strReq += g_psoConf->m_pszVisitedNetworkID;
  p_strReq += "\n";

  return iRetVal;
}

static int sms112_sip_make_body( std::string &p_strBody, std::string &p_strGeoData, const char *p_pszText )
{
  int iRetVal = 0;

  p_strBody += "--";
  p_strBody += g_psoConf->m_pszBoundary;
  p_strBody += "\n";

  p_strBody += "Content-Type: application/sdp\n\n";

  p_strBody += "(v): 0\n";
  p_strBody += "(o): itscc 0 0 IN IP4 0.0.0.0\n";
  p_strBody += "(s): its\n";
  p_strBody += "(c): IN IP4 0.0.0.0\n";
  p_strBody += "(t): 0 0\n";
  p_strBody += "(m): audio 1234 RTP/AVP 8\n";
  p_strBody += "(a): inactive\n";

  p_strBody += "--";
  p_strBody += g_psoConf->m_pszBoundary;
  p_strBody += "\n";

  p_strBody += "Content-Type: application/pidf+xml\n";
  p_strBody += "Content-ID: target123@172.16.78.4\n\n";

  p_strBody += "<?xml version = \"1.0\" encoding = \"UTF-8\"?>\n";
  p_strBody += p_strGeoData;

  p_strBody += "\n--";
  p_strBody += g_psoConf->m_pszBoundary;
  p_strBody += "\n";

  p_strBody += "Content-Type: text/plain; charset=UTF-8\n";
  p_strBody += "Content-Transfer-Encoding: 8bit\n";
  p_strBody += "Content-Disposition: form-data; name=\"part\"\n\n";

  p_strBody += p_pszText;

  p_strBody += "\n--";
  p_strBody += g_psoConf->m_pszBoundary;
  p_strBody += "--\n";

  return iRetVal;
}
