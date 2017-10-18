#include "sms112.h"
#include "sms112_smpp.h"

#include <string.h>

static volatile bool g_bIsConnected;

extern "C" {
#include "smpp34.h"
#include "smpp34_structs.h"
#include "smpp34_params.h"
}

#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/time.h>

static volatile uint32_t g_uiSecNum;

int sms112_smpp_send( uint8_t *p_pmBuf, int p_iDataLen, uint32_t p_uiSeqNum, SReqData &p_soReqData )
{
  int iRetVal = 0;
  int iFnRes;

  iFnRes = sms112_req_stor_add( p_uiSeqNum, p_soReqData );
  if ( 0 == iFnRes ) {
  } else {
    iRetVal = iFnRes;
    goto exit;
  }

  iFnRes = sms112_tcp_send( p_pmBuf, p_iDataLen );
  if ( 0 == iFnRes ) {
  } else {
    iRetVal = iFnRes;
    goto exit;
  }

  timespec soTS;

  sms112_make_timespec_timeout( soTS, 1, 0 );

  iFnRes = pthread_mutex_timedlock( &p_soReqData.m_mutexWaitResp, &soTS );
  if ( p_soReqData.m_iDataLen != 0 ) {
  } else {
    if ( ETIMEDOUT == iFnRes ) {
      iRetVal = ETIMEDOUT;
      goto exit;
    } else {
      iRetVal = iFnRes;
      goto exit;
    }
  }

  exit:
  if ( 0 == iRetVal ) {
    UTL_LOG_D( g_coLog, "sms112_smpp_send: ok" );
  } else {
    UTL_LOG_E( g_coLog, "sms112_smpp_send: failed!!! error code: %d; descr: %s", iFnRes, strerror( iFnRes ) );
  }

  return iRetVal;
}

struct SPDUHdr {
  uint32_t command_length;
  uint32_t command_id;
  uint32_t command_status;
  uint32_t sequence_number;
};

int sms112_oper_resp( SPDUHdr &p_soHdr )
{
  int iRetVal = 0;
  int iFnRes;
  SReqData *psoReqData;

  iFnRes = sms112_req_stor_get_and_rem( p_soHdr.sequence_number, psoReqData );
  if ( 0 == iFnRes ) {
    /* выдел¤ем пам¤ть дл¤ хранени¤ запроса */
    psoReqData->m_pBuf = reinterpret_cast<uint8_t*>( malloc( p_soHdr.command_length ) );

    iFnRes = sms112_tcp_recv( psoReqData->m_pBuf, p_soHdr.command_length );
    if ( 0 == iFnRes ) {
      psoReqData->m_iDataLen = p_soHdr.command_length;
    } else {
      iRetVal = -1;
    }
    pthread_mutex_unlock( &psoReqData->m_mutexWaitResp );
  } else {
    iRetVal = -1;
  }

  return iRetVal;
}

int sms112_smpp_oper_req( SPDUHdr &p_soHdr )
{
  int iRetVal = 0;
  int iFnRes;
  uint8_t muiLocalBuffer[ 1024 ];
  int iDataLen;

  if ( p_soHdr.command_length > sizeof( muiLocalBuffer ) ) {
    return EINVAL;
  }
  iFnRes = sms112_tcp_recv( muiLocalBuffer, p_soHdr.command_length );
  if ( 0 == iFnRes ) {
  } else {
    return EINVAL;
  }

  switch ( p_soHdr.command_id ) {
    case DELIVER_SM:
      iDataLen = p_soHdr.command_length;
      iFnRes = sms112_smpp_deliver_sm_oper( muiLocalBuffer, iDataLen, sizeof(muiLocalBuffer), p_soHdr.sequence_number );
      break; /* DELIVER_SM */
    case UNBIND:
      iDataLen = p_soHdr.command_length;
      iFnRes = sms112_smpp_unbind_oper( muiLocalBuffer, iDataLen, sizeof( muiLocalBuffer ), p_soHdr.sequence_number );
      break; /* UNBIND */
    default:
      iFnRes = -1;
      break;
  }

  return iRetVal;
}

int sms112_smpp_handler()
{
  int iRetVal = 0;
  SPDUHdr soHdr;
  int iFnRes;

  /* получаем ответ */
  iFnRes = sms112_tcp_peek( reinterpret_cast<uint8_t*>(&soHdr), sizeof( soHdr ) );
  if ( 0 == iFnRes ) {
  } else {
    return -1;
  };
  soHdr.command_length = ntohl( soHdr.command_length );
  soHdr.command_id = ntohl( soHdr.command_id );
  soHdr.sequence_number = ntohl( soHdr.sequence_number );

  if ( 0x80000000 & soHdr.command_id ) {
    /* ответ */
    iRetVal = sms112_oper_resp( soHdr );
  } else {
    /* запрос */
    iRetVal = sms112_smpp_oper_req( soHdr );
  }

  return iRetVal;
}

uint32_t sms112_smpp_get_seq_num()
{
  return ++g_uiSecNum;
}

#ifdef DEBUG
void sms112_smpp_dump_buf( uint8_t *p_puiBuf, uint32_t p_uiDataSize, const char *p_pszTitle )
{
  uint8_t muiPrintBuffer[ 65535 ];
  if ( 0 == smpp34_dumpBuf( muiPrintBuffer, sizeof( muiPrintBuffer ), p_puiBuf, p_uiDataSize ) ) {
    g_coLog.Dump2( p_pszTitle, reinterpret_cast<char*>(muiPrintBuffer) );
  }
}
#else
void sms112_smpp_dump_buf( uint8_t *p_puiBuf, uint32_t p_uiDataSize, const char *p_pszTitle ) { }
#endif

void sms112_smpp_set_connected( bool p_bIsConnected )
{
  g_bIsConnected = p_bIsConnected;
}

bool sms112_smpp_is_connected()
{
  UTL_LOG_D( g_coLog, "smpp status: %s", g_bIsConnected ? "connected" : "disconnected" );

  return g_bIsConnected;
}
