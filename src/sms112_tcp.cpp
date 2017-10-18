#include "sms112.h"
#include "sms112_tcp.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <string.h>
#include <unistd.h>
#include <fcntl.h>

static volatile bool g_bIsConnected;
static int           g_iSock = -1;

static void sms112_tcp_set_connected( bool p_bIsConnected );

int sms112_tcp_connect()
{
  int iRetVal = 0;
  int iReuseAddr = 1;

  /* создаем сокет */
  g_iSock = socket( AF_INET, SOCK_STREAM, 0 );
  if ( -1 != g_iSock ) {
  } else {
    UTL_LOG_E( g_coLog, "establish tcp-connection to SMPP-server: failed!!! error code: %d; descr: %s", iRetVal, strerror( errno ) );
    return errno;
  }

  /* подключаемся к smpp-серверу */
  sockaddr_in soAddr;

  memset( &soAddr, 0, sizeof( soAddr ) );
  soAddr.sin_family = AF_INET;
  soAddr.sin_port = htons( g_psoConf->m_usSMPPPort );
  soAddr.sin_addr.s_addr = inet_addr( g_psoConf->m_pszSMPPServer );
  iRetVal = connect( g_iSock, reinterpret_cast<sockaddr*>( &soAddr ), sizeof( soAddr ) );
  if ( 0 == iRetVal ) {
  } else {
    UTL_LOG_E( g_coLog, "establish tcp-connection to SMPP-server: failed!!! error code: %d; descr: %s", iRetVal, strerror( errno ) );
    iRetVal = errno;
  }

  /* задаем режим работы сокета */
  iRetVal = fcntl( g_iSock, F_SETFL, O_NONBLOCK );
  if ( 0 == iRetVal ) {
    UTL_LOG_N( g_coLog, "establish tcp-connection to SMPP-server: ok" );
  } else {
    UTL_LOG_E( g_coLog, "establish tcp-connection to SMPP-server: failed!!! error code: %d; descr: %s", iRetVal, strerror( errno ) );
    return errno;
  }

  sms112_tcp_set_connected( true );

  return iRetVal;
}

void sms112_tcp_disconn()
{
  if ( -1 != g_iSock ) {
    shutdown( g_iSock, SHUT_RD | SHUT_WR );
    if ( 0 == close( g_iSock ) ) {
      sms112_tcp_set_connected( false );
      g_iSock = -1;
      UTL_LOG_N( g_coLog, "close tcp-connection: ok" );
    } else {
      UTL_LOG_E( g_coLog, "close tcp-connection: failed!!! descr: %s", strerror( errno ) );
    }
  }
}

int sms112_tcp_send( uint8_t *p_pBuf, uint32_t p_uiDataSize )
{
  int iRetVal = 0;
  int iFnRes;

  iFnRes = send( g_iSock, p_pBuf, p_uiDataSize, 0 );
  if ( iFnRes == p_uiDataSize ) {
#ifdef DEBUG
    sms112_smpp_dump_buf( p_pBuf, p_uiDataSize, "SEND BUFFER:\n" );
#endif
  } else {
    iRetVal = errno;
    if ( ECONNRESET == iRetVal ) {
      sms112_tcp_set_connected( false );
    }
    UTL_LOG_E( g_coLog, "sms112_tcp_send: failed!!! error code: %u; descr: %s", iRetVal, strerror( iRetVal ) );
  }

  if ( 0 == iRetVal ) {
    UTL_LOG_D( g_coLog, "sms112_tcp_send: ok" );
  }

  return iRetVal;
}

int sms112_tcp_recv( uint8_t *p_pBuf, uint32_t p_uiDataSize )
{
  int iRetVal = 0;
  int iFnRes;

  iFnRes = recv( g_iSock, p_pBuf, p_uiDataSize, 0 );
  if ( iFnRes == p_uiDataSize ) {
#ifdef DEBUG
    sms112_smpp_dump_buf( p_pBuf, p_uiDataSize, "RECV BUFFER:\n" );
#endif
  } else {
    iRetVal = -1;
  }

  return iRetVal;
}

int sms112_tcp_peek( uint8_t *p_pBuf, uint32_t p_uiDataSize )
{
  int iRetVal = 0;
  int iFnRes;

  iFnRes = recv( g_iSock, p_pBuf, p_uiDataSize, MSG_PEEK );
  if ( iFnRes == p_uiDataSize ) {
  } else {
    iRetVal = -1;
  }

  return iRetVal;
}

static void sms112_tcp_set_connected( bool p_bIsConnected )
{
  g_bIsConnected = p_bIsConnected;
}

bool sms112_tcp_is_connected()
{
  UTL_LOG_D( g_coLog, "tcp status: %s", g_bIsConnected ? "connected" : "disconnected" );

  return g_bIsConnected;
}

int sms112_tcp_get_sock()
{
  return g_iSock;
}
