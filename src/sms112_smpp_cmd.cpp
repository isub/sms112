#include "sms112.h"
#include "sms112_sip.h"
#include "sms112_smpp.h"

#include "sms112_geodata.h"

extern "C" {
#include "smpp34.h"
#include "smpp34_structs.h"
#include "smpp34_params.h"
}

#include <stdlib.h>
#include <string.h>

int sms112_smpp_connect()
{
  int iRetVal = 0;
  int iFnRes;
  bind_transceiver_t      req;

  /* Init PDU ***********************************************************/
  memset( &req, 0, sizeof( req ) );
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
    iRetVal = iFnRes;
    goto exit;
  }

  /* отправляем данные запроса и ждем ответ */
  {
    bind_transceiver_resp_t res;
    memset( &res, 0, sizeof( res ) );
    SReqData soReqData;

    iFnRes = sms112_smpp_send( muiLocalBuffer, iDataLen, req.sequence_number, soReqData );
    if ( 0 == iFnRes ) {
    } else {
      iRetVal = iFnRes;
      goto exit;
    }

    /* ответ получен асинхронно */
    iFnRes = smpp34_unpack2( (void*)&res, soReqData.m_pBuf, soReqData.m_iDataLen );
    if ( iFnRes == 0 ) {
    } else {
      iRetVal = iFnRes;
      goto exit;
    }

    destroy_tlv( res.tlv );

    if ( res.command_id == BIND_TRANSCEIVER_RESP && res.command_status == ESME_ROK ) {
    } else {
      iRetVal = EINVAL;
      goto exit;
    }
  }

  exit:

  if ( 0 == iRetVal ) {
    sms112_smpp_set_connected( true );
    UTL_LOG_N( g_coLog, "open smpp connection: ok" );
  } else {
    UTL_LOG_E( g_coLog, "open smpp connection: failed!!! error code: %d; descr: %s", iRetVal, strerror( iRetVal ) );
  }

  return iRetVal;
}

int sms112_smpp_disconn()
{
  int iRetVal = 0;
  int iFnRes;
  unbind_t      req;

  sms112_smpp_set_connected( false );

  /* Init PDU ***********************************************************/
  memset( &req, 0, sizeof( req ) );
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
    iRetVal = iFnRes;
    goto exit;
  }

  {
    unbind_resp_t res;
    memset( &res, 0, sizeof( res ) );
    SReqData soReqData;

    iFnRes = sms112_smpp_send( muiLocalBuffer, iDataLen, req.sequence_number, soReqData );
    if ( 0 == iFnRes ) {
    } else {
      iRetVal = iFnRes;
      goto exit;
    }

    iFnRes = smpp34_unpack2( (void*)&res, soReqData.m_pBuf, soReqData.m_iDataLen );
    if ( 0 == iFnRes ) {
    } else {
      iRetVal = iFnRes;
      goto exit;
    }

    if ( res.command_id == UNBIND_RESP && res.command_status == ESME_ROK ) {
    } else {
      iRetVal = EINVAL;
      goto exit;
    }
  }

  exit:

  if ( 0 == iRetVal ) {
    UTL_LOG_N( g_coLog, "close SMPP connection: ok" );
  } else {
    UTL_LOG_E( g_coLog, "close SMPP connection: failed!!! error code: %d; descr: %s", iRetVal, strerror( iRetVal ) );
  }

  return iRetVal;
}

int sms112_smpp_enq_link()
{
  int iRetVal = 0;
  int iFnRes;
  unbind_t      req;

  /* Init PDU ***********************************************************/
  memset( &req, 0, sizeof( req ) );
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
    iRetVal = iFnRes;
    goto exit;
  }

  {
    unbind_resp_t res;
    memset( &res, 0, sizeof( res ) );
    SReqData soReqData;

    iFnRes = sms112_smpp_send( muiLocalBuffer, iDataLen, req.sequence_number, soReqData );
    if ( 0 == iFnRes ) {
    } else {
      iRetVal = iFnRes;
      goto exit;
    }

    iFnRes = smpp34_unpack2( (void*)&res, soReqData.m_pBuf, soReqData.m_iDataLen );
    if ( iFnRes != 0 ) {
      iRetVal = iFnRes;
      goto exit;
    };

    if ( res.command_id == ENQUIRE_LINK_RESP && res.command_status == ESME_ROK ) {
    } else {
      iRetVal = EINVAL;
      goto exit;
    }
  }

  exit:

  if ( 0 == iRetVal ) {
    UTL_LOG_N( g_coLog, "operate ENQUIRE_LINK-request: ok" );
  } else {
    UTL_LOG_E( g_coLog, "operate ENQUIRE_LINK-request: failed!!! error code: %d; descr: %s", iRetVal, strerror( iRetVal ) );
  }

  return iRetVal;
}

int sms112_smpp_deliver_sm_oper( uint8_t *p_puiBuf, int &p_iDataLen, size_t p_stBufSize, uint8_t p_uiSeqNum )
{
  int iRetVal = 0;
  int iFnRes;
  deliver_sm_t req;
  char *pszPrintData = NULL;

  iFnRes = smpp34_unpack2( reinterpret_cast<void*>( &req ), p_puiBuf, p_iDataLen );
  if ( 0 == iFnRes ) {
  } else {
    iRetVal = EINVAL;
    goto cleanup_and_exit;
  }

#ifdef DEBUG
  UTL_LOG_D( g_coLog, "source_addr_ton:     %d", req.source_addr_ton );
  UTL_LOG_D( g_coLog, "source_addr_npi:     %d", req.source_addr_npi );
  UTL_LOG_D( g_coLog, "source_addr:         %s", req.source_addr );
  UTL_LOG_D( g_coLog, "registered_delivery: %d", req.registered_delivery );
  UTL_LOG_D( g_coLog, "data_coding:         %d", req.data_coding );
  UTL_LOG_D( g_coLog, "sm_length:           %d", req.sm_length );
#endif

  {
    const char *pszSrcCoding;
    const char *pszSrcStr;
    size_t stSrcLen;
    const char *pszField;
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
        iRetVal = EINVAL;
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
        iRetVal = EINVAL;
        goto cleanup_and_exit;
    }

    {
      std::string strGeodata;
      const char *pszCaller = reinterpret_cast<char*>( req.source_addr );
      char mcResult[ 4096 ];

      if ( 0 == sms112_geodata_get( pszCaller, strGeodata ) ) {
        UTL_LOG_D( g_coLog, "get geodata of caller %s: ok", pszCaller );
        iFnRes = sms112_sip_send_req( pszCaller, pszPrintData, strGeodata.c_str() );
        if ( 0 == iFnRes ) {
          iFnRes = snprintf( mcResult, sizeof( mcResult ), "OK" );
          if ( iFnRes > 0 ) {
            if ( iFnRes < sizeof( mcResult ) ) {
            } else {
              mcResult[ sizeof( mcResult ) - 1 ] = '\0';
            }
          } else {
            mcResult[ 0 ] = '\0';
          }
        } else {
          iFnRes = snprintf( mcResult, sizeof( mcResult ), "Error: sip request failed: %s", strerror( iFnRes ) );
          if ( iFnRes > 0 ) {
            if ( iFnRes < sizeof( mcResult ) ) {
            } else {
              mcResult[ sizeof( mcResult ) - 1 ] = '\0';
            }
          } else {
            mcResult[ 0 ] = '\0';
          }
        }
      } else {
        iFnRes = snprintf( mcResult, sizeof( mcResult ), "Error: can not to retrieve geodata" );
        if ( iFnRes > 0 ) {
          if ( iFnRes < sizeof( mcResult ) ) {
          } else {
            mcResult[ sizeof( mcResult ) - 1 ] = '\0';
          }
        } else {
          mcResult[ 0 ] = '\0';
        }
        UTL_LOG_E( g_coLog, "get geodata of caller %s: failed!!!", pszCaller );
      }

      sms112_send_report( pszCaller, pszPrintData, mcResult, strGeodata.c_str() );
    }
  }

  {
    deliver_sm_resp_t res;

    res.command_length = 0;
    res.command_id = DELIVER_SM_RESP;
    res.command_status = ESME_ROK;
    res.sequence_number = req.sequence_number;

    memset( p_puiBuf, 0, p_stBufSize );
    iFnRes = smpp34_pack2( p_puiBuf, p_stBufSize, &p_iDataLen, reinterpret_cast<void*>( &res ) );
    if ( 0 == iFnRes ) {
    } else {
      iRetVal = iFnRes;
      goto cleanup_and_exit;
    }

    iFnRes = sms112_tcp_send( p_puiBuf, p_iDataLen );
    if ( iFnRes == 0 ) {
    } else {
      iRetVal = iFnRes;
      goto cleanup_and_exit;
    }
  }

  cleanup_and_exit:

  if ( NULL != pszPrintData ) {
    delete [] pszPrintData;
  }

  if ( 0 == iRetVal ) {
    UTL_LOG_N( g_coLog, "operate DELIVER_SM-request: ok" );
  } else {
    UTL_LOG_E( g_coLog, "operate DELIVER_SM-request: failed!!! error code: %d; descr: %s", iRetVal, strerror( iRetVal ) );
  }

  return iRetVal;
}

int sms112_smpp_unbind_oper( uint8_t *p_puiBuf, int &p_iDataLen, size_t p_stBufSize, uint8_t p_uiSeqNum )
{
  int iRetVal = 0;
  int iFnRes;
  unbind_t req;

  sms112_smpp_set_connected( false );

  iFnRes = smpp34_unpack2( reinterpret_cast<void*>( &req ), p_puiBuf, p_iDataLen );
  if ( 0 == iFnRes ) {
  } else {
    iRetVal = iFnRes;
    goto exit;
  }

  unbind_t res;

  res.command_length = 0;
  res.command_id = UNBIND_RESP;
  res.command_status = ESME_ROK;
  res.sequence_number = req.sequence_number;

  memset( p_puiBuf, 0, p_stBufSize );
  iFnRes = smpp34_pack2( p_puiBuf, p_stBufSize, &p_iDataLen, reinterpret_cast<void*>( &res ) );
  if ( 0 == iFnRes ) {
  } else {
    iRetVal = iFnRes;
    goto exit;
  }

  iFnRes = sms112_tcp_send( p_puiBuf, p_iDataLen );
  if ( iFnRes == 0 ) {
  } else {
    iRetVal = iFnRes;
    goto exit;
  }

  exit:

  if ( 0 == iRetVal ) {
    UTL_LOG_N( g_coLog, "operate UNBIND-request: ok" );
  } else {
    UTL_LOG_E( g_coLog, "operate UNBIND-request: failed!!! error code: %d; descr: %s", iRetVal, strerror( iRetVal ) );
  }

  return iRetVal;
}
