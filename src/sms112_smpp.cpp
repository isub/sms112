#include "sms112.h"

extern "C" {
#include "smpp34.h"
#include "smpp34_structs.h"
#include "smpp34_params.h"
}

#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/time.h>

static volatile uint32_t g_uiSecNum;

int sms112_smpp_send( int p_iSock, uint8_t *p_pmBuf, int p_iDataLen, uint32_t p_uiSeqNum, SReqData &p_soReqData )
{
  int iFnRes;

  iFnRes = sms112_req_stor_add( p_uiSeqNum, p_soReqData );
  if ( 0 == iFnRes ) {
  } else {
    return iFnRes;
  }

  iFnRes = sms112_tcp_send( p_iSock, p_pmBuf, p_iDataLen );
  if ( 0 == iFnRes ) {
  } else {
    return iFnRes;
  }

  timespec soTS;

  sms112_make_timespec_timeout( soTS, 1, 0 );

  pthread_mutex_timedlock( &p_soReqData.m_mutexWaitResp, &soTS );
  if ( p_soReqData.m_iDataLen != 0 ) {
    return 0;
  } else {
    return -1;
  }
}

struct SPDUHdr {
  uint32_t command_length;
  uint32_t command_id;
  uint32_t command_status;
  uint32_t sequence_number;
};

int sms112_oper_resp( SPDUHdr &p_soHdr, int p_iSock )
{
  int iRetVal = 0;
  int iFnRes;
  SReqData *psoReqData;

  iFnRes = sms112_req_stor_get_and_rem( p_soHdr.sequence_number, psoReqData );
  if ( 0 == iFnRes ) {
    /* выдел¤ем пам¤ть дл¤ хранени¤ запроса */
    psoReqData->m_pBuf = reinterpret_cast<uint8_t*>( malloc( p_soHdr.command_length ) );

    iFnRes = sms112_tcp_recv( p_iSock, psoReqData->m_pBuf, p_soHdr.command_length );
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

int sms112_smpp_oper_req( SPDUHdr &p_soHdr, int p_iSock )
{
  int iRetVal = 0;
  int iFnRes;
  uint8_t muiLocalBuffer[ 1024 ];
  int iDataLen;

  if ( p_soHdr.command_length > sizeof( muiLocalBuffer ) ) {
    return EINVAL;
  }
  iFnRes = sms112_tcp_recv( p_iSock, muiLocalBuffer, p_soHdr.command_length );
  if ( 0 == iFnRes ) {
  } else {
    return EINVAL;
  }

  switch ( p_soHdr.command_id ) {
    case DELIVER_SM:
      iDataLen = p_soHdr.command_length;
      iFnRes = sms112_smpp_deliver_sm_oper( muiLocalBuffer, iDataLen, sizeof(muiLocalBuffer), p_soHdr.sequence_number );
      break; /* DELIVER_SM */
    default:
      iFnRes = -1;
      break;
  }

  if ( 0 == iFnRes ) {
  } else {
    return -1;
  }

  iFnRes = sms112_tcp_send( p_iSock, muiLocalBuffer, iDataLen );
  if ( iFnRes == 0 ) {
  } else {
    return -1;
  };

  return iRetVal;
}

int sms112_smpp_handler( int p_iSock )
{
  int iRetVal = 0;
  SPDUHdr soHdr;
  int iFnRes;

  /* получаем ответ */
  iFnRes = sms112_tcp_peek( p_iSock, reinterpret_cast<uint8_t*>(&soHdr), sizeof( soHdr ) );
  if ( 0 == iFnRes ) {
  } else {
    return -1;
  };
  soHdr.command_length = ntohl( soHdr.command_length );
  soHdr.command_id = ntohl( soHdr.command_id );
  soHdr.sequence_number = ntohl( soHdr.sequence_number );

  if ( 0x80000000 & soHdr.command_id ) {
    /* ответ */
    iRetVal = sms112_oper_resp( soHdr, p_iSock );
  } else {
    /* запрос */
    iRetVal = sms112_smpp_oper_req( soHdr, p_iSock );
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
