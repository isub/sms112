#include "sms112.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int sms112_tcp_connect( int &p_iSock )
{
  int iRetVal = 0;
  int iReuseAddr = 1;

  /* создаем сокет */
  p_iSock = socket( AF_INET, SOCK_STREAM, 0 );
  if ( -1 != p_iSock ) {
  } else {
    return errno;
  }

  /* подключаемся к smpp-серверу */
  sockaddr_in soAddr;

  memset( &soAddr, 0, sizeof( soAddr ) );
  soAddr.sin_family = AF_INET;
  soAddr.sin_port = htons( g_psoConf->m_usSMPPPort );
  soAddr.sin_addr.s_addr = inet_addr( g_psoConf->m_pszSMPPServer );
  iRetVal = connect( p_iSock, reinterpret_cast<sockaddr*>( &soAddr ), sizeof( soAddr ) );
  if ( 0 == iRetVal ) {
  } else {
    iRetVal = errno;
  }

  /* задаем режим работы сокета */
  iRetVal = fcntl( p_iSock, F_SETFL, O_NONBLOCK );
  if ( 0 == iRetVal ) {
  } else {
    return errno;
  }

  return iRetVal;
}

void sms112_tcp_disconn( int &p_iSock )
{
  if ( -1 != p_iSock ) {
    shutdown( p_iSock, SHUT_RD | SHUT_WR );
    close( p_iSock );
  }
}

int sms112_tcp_send( int p_iSock, uint8_t *p_pBuf, uint32_t p_uiDataSize )
{
  int iRetVal = 0;
  int iFnRes;

  iFnRes = send( p_iSock, p_pBuf, p_uiDataSize, 0 );
  if ( iFnRes == p_uiDataSize ) {
    sms112_smpp_dump_buf( p_pBuf, p_uiDataSize, "SEND BUFFER:\n" );
  } else {
    return -1;
  }

  return iRetVal;
}

int sms112_tcp_recv( int p_iSock, uint8_t *p_pBuf, uint32_t p_uiDataSize )
{
  int iRetVal = 0;
  int iFnRes;

  iFnRes = recv( p_iSock, p_pBuf, p_uiDataSize, 0 );
  if ( iFnRes == p_uiDataSize ) {
    sms112_smpp_dump_buf( p_pBuf, p_uiDataSize, "RECV BUFFER:\n" );
  } else {
    iRetVal = -1;
  }

  return iRetVal;
}

int sms112_tcp_peek( int p_iSock, uint8_t *p_pBuf, uint32_t p_uiDataSize )
{
  int iRetVal = 0;
  int iFnRes;

  iFnRes = recv( p_iSock, p_pBuf, p_uiDataSize, MSG_PEEK );
  if ( iFnRes == p_uiDataSize ) {
  } else {
    iRetVal = -1;
  }

  return iRetVal;
}
