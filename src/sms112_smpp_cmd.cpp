#include "sms112.h"
#include "sms112_sip.h"

extern "C" {
#include "smpp34.h"
#include "smpp34_structs.h"
#include "smpp34_params.h"
}

#include <stdlib.h>
#include <string.h>

int sms112_smpp_connect( int p_iSock )
{
  int iRetVal = 0;
  int iFnRes;
  bind_transceiver_t      req;
  bind_transceiver_resp_t res;
  memset( &req, 0, sizeof( req ) );
  memset( &res, 0, sizeof( res ) );

  /* Init PDU ***********************************************************/
  req.command_length = 0;
  req.command_id = BIND_TRANSCEIVER;
  req.command_status = ESME_ROK;
  req.sequence_number = sms112_smpp_get_seq_num();
  snprintf( reinterpret_cast<char*>( req.system_id ), sizeof( req.system_id ), "%s", g_psoConf->m_pszESMESysId );
  snprintf( reinterpret_cast<char*>( req.password ), sizeof( req.password ), "%s", g_psoConf->m_pszESMEPaswd );
  snprintf( reinterpret_cast<char*>( req.system_type ), sizeof( req.system_type ), "%s", g_psoConf->m_pszESMEType );
  req.interface_version = SMPP_VERSION;

  uint8_t muiLocalBuffer[ 1024 ];
  int iDataLen;

  /* отправляем запрос */
  memset( muiLocalBuffer, 0, sizeof( muiLocalBuffer ) );
  iFnRes = smpp34_pack2( muiLocalBuffer, sizeof( muiLocalBuffer ), &iDataLen, reinterpret_cast<void*>( &req ) );
  if ( 0 == iFnRes ) {
  } else {
    return -1;
  }

  SReqData soReqData;

  /* отправляем данные запроса и ждем ответ */
  iFnRes = sms112_smpp_send( p_iSock, muiLocalBuffer, iDataLen, req.sequence_number, soReqData );
  if ( 0 == iFnRes ) {
  } else {
    return -1;
  }

  /* ответ получен асинхронно */
  iFnRes = smpp34_unpack2( (void*)&res, soReqData.m_pBuf, soReqData.m_iDataLen );
  if ( iFnRes == 0 ) {
  } else {
    return -1;
  };

  destroy_tlv( res.tlv );

  if ( res.command_id == BIND_TRANSCEIVER_RESP && res.command_status == ESME_ROK ) {
  } else {
    return -1;
  };

  return iRetVal;
}

int sms112_smpp_disconn( int p_iSock )
{
  int iRetVal = 0;
  int iFnRes;
  unbind_t      req;
  unbind_resp_t res;
  memset( &req, 0, sizeof( req ) );
  memset( &res, 0, sizeof( res ) );

  /* Init PDU ***********************************************************/
  req.command_length = 0;
  req.command_id = UNBIND;
  req.command_status = ESME_ROK;
  req.sequence_number = sms112_smpp_get_seq_num();

  uint8_t muiLocalBuffer[ 1024 ];
  int iDataLen;

  /* отправляем запрос */
  memset( muiLocalBuffer, 0, sizeof( muiLocalBuffer ) );
  iFnRes = smpp34_pack2( muiLocalBuffer, sizeof( muiLocalBuffer ), &iDataLen, reinterpret_cast<void*>( &req ) );
  if ( 0 == iFnRes ) {
  } else {
    return -1;
  }

  SReqData soReqData;

  iFnRes = sms112_smpp_send( p_iSock, muiLocalBuffer, iDataLen, req.sequence_number, soReqData );
  if ( 0 == iFnRes ) {
  } else {
    return -1;
  }

  iFnRes = smpp34_unpack2( (void*)&res, soReqData.m_pBuf, soReqData.m_iDataLen );
  if ( 0 == iFnRes ) {
  } else {
    return -1;
  };

  if ( res.command_id == UNBIND_RESP && res.command_status == ESME_ROK ) {
  } else {
    return -1;
  };

  return iRetVal;
}

int sms112_smpp_enq_link( int p_iSock )
{
  int iRetVal = 0;
  int iFnRes;
  unbind_t      req;
  unbind_resp_t res;
  memset( &req, 0, sizeof( req ) );
  memset( &res, 0, sizeof( res ) );

  /* Init PDU ***********************************************************/
  req.command_length = 0;
  req.command_id = ENQUIRE_LINK;
  req.command_status = ESME_ROK;
  req.sequence_number = sms112_smpp_get_seq_num();

  uint8_t muiLocalBuffer[ 1024 ];
  int iDataLen;

  /* отправляем запрос */
  memset( muiLocalBuffer, 0, sizeof( muiLocalBuffer ) );
  iFnRes = smpp34_pack2( muiLocalBuffer, sizeof( muiLocalBuffer ), &iDataLen, reinterpret_cast<void*>( &req ) );
  if ( 0 == iFnRes ) {
  } else {
    return -1;
  }

  SReqData soReqData;

  iFnRes = sms112_smpp_send( p_iSock, muiLocalBuffer, iDataLen, req.sequence_number, soReqData );
  if ( 0 == iFnRes ) {
  } else {
    return -1;
  }

  iFnRes = smpp34_unpack2( (void*)&res, soReqData.m_pBuf, soReqData.m_iDataLen );
  if ( iFnRes != 0 ) {
    return -1;
  };

  if ( res.command_id == ENQUIRE_LINK_RESP && res.command_status == ESME_ROK ) {
  } else {
    return -1;
  }

  return iRetVal;
}

int sms112_smpp_deliver_sm_oper( uint8_t *p_puiBuf, int &p_iDataLen, size_t p_stBufSize, uint8_t p_uiSeqNum )
{
  int iRetVal = 0;
  int iFnRes;
  deliver_sm_t req;

  iFnRes = smpp34_unpack2( reinterpret_cast<void*>( &req ), p_puiBuf, p_iDataLen );
  if ( 0 == iFnRes ) {
  } else {
    return -1;
  }

#ifdef DEBUG
  UTL_LOG_D( g_coLog, "source_addr_ton:     %d", req.source_addr_ton );
  UTL_LOG_D( g_coLog, "source_addr_npi:     %d", req.source_addr_npi );
  UTL_LOG_D( g_coLog, "source_addr:         %s", req.source_addr );
  UTL_LOG_D( g_coLog, "registered_delivery: %d", req.registered_delivery );
  UTL_LOG_D( g_coLog, "data_coding:         %d", req.data_coding );
  UTL_LOG_D( g_coLog, "sm_length:           %d", req.sm_length );
#endif

  const char *pszSrcCoding;
  const char *pszSrcStr;
  size_t stSrcLen;
  const char *pszField;
  char *pszPrintData = NULL;
  size_t stLen;

  if ( 0 != req.sm_length ) {
    pszSrcStr = reinterpret_cast<char*>( req.short_message );
    stSrcLen = req.sm_length;
    pszField = "short_message:       ";
  } else {
    tlv_t *tlv = req.tlv;

    while ( NULL != tlv ) {
      if ( tlv->tag == 0x0424 ) {
        pszSrcStr = reinterpret_cast<char*>( tlv->value.octet );
        stSrcLen = tlv->length;
        pszField = "message_payload:     ";
        break;
      }
      tlv = tlv->next;
    }
    if ( NULL == tlv ) {
      goto cleanup_and_exit;
    }
  }

  switch ( req.data_coding ) {
    case 8:
      pszSrcCoding = "UCS-2BE";
      iFnRes = sms112_iconv( pszSrcCoding, "UTF-8", pszSrcStr, stSrcLen, &pszPrintData, &stLen, 0, 0 );
      if ( 0 == iFnRes ) {
        pszPrintData[ stLen ] = '\0';
#ifdef DEBUG
        g_coLog.Dump2( pszField, pszPrintData );
#endif
      }
      break;
    case 0: /* GSM-7 */
      pszPrintData = new char[ req.sm_length + 1 ];
      memcpy( pszPrintData, pszSrcStr, req.sm_length );
      pszPrintData[ req.sm_length ] = '\0';
#ifdef DEBUG
      g_coLog.Dump2( pszField, pszPrintData );
#endif
      break;
    default:
      return EINVAL;
  }

  sms112_sip_send_req( reinterpret_cast<char*>( req.source_addr ), pszPrintData );

  deliver_sm_resp_t res;

  res.command_length = 0;
  res.command_id = DELIVER_SM_RESP;
  res.command_status = ESME_ROK;
  res.sequence_number = req.sequence_number;

  memset( p_puiBuf, 0, p_stBufSize );
  iFnRes = smpp34_pack2( p_puiBuf, p_stBufSize, &p_iDataLen, reinterpret_cast<void*>( &res ) );
  if ( 0 == iFnRes ) {
  } else {
    goto cleanup_and_exit;
  }

  cleanup_and_exit:
  if ( NULL != pszPrintData ) {
    delete [] pszPrintData;
  }

  return iRetVal;
}
