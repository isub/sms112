#include "sms112.h"

#ifdef DEBUG
  #include "sms112_geodata.h"
  #include "sms112_sip.h"
  #include <string>
#endif

#include <signal.h>

static void sig_oper( int p_iSig )
{
  UTL_LOG_N( g_coLog, "signal caught: %d", p_iSig );

  switch ( p_iSig ) {
    case SIGTERM:
    case SIGINT:
      stop_smpp_receiver_thread();
      break;
#ifdef DEBUG
    case SIGUSR1:
    {
      std::string str;
      if ( 0 == sms112_geodata_get( "79506656062", str ) ) {
        UTL_LOG_D( g_coLog, "geodata is received" );
      } else {
        UTL_LOG_D( g_coLog, "geodata is not received" );
      }
    }
    break;
    case SIGUSR2:
    {
      std::string str;
      if ( 0 == sms112_geodata_get( "79506656062", str ) ) {
        UTL_LOG_D( g_coLog, "geodata is received" );
      } else {
        UTL_LOG_D( g_coLog, "geodata is not received" );
      }
      sms112_sip_send_req( "79506656062", "помощь нужна ващета", str.c_str() );
    }
    break;
#endif
  }
}

int sms112_reg_sig()
{
  int iRetVal = 0;

  if ( SIG_ERR != signal( SIGTERM, sig_oper ) ) {
  } else {
    iRetVal = errno;
    return iRetVal;
  }

  if ( SIG_ERR != signal( SIGINT, sig_oper ) ) {
    UTL_LOG_N( g_coLog, "register signal handler: ok" );
  } else {
    iRetVal = errno;
    UTL_LOG_E( g_coLog, "register signal handler: failed!!! error code: %d", iRetVal );
    return iRetVal;
  }

#ifdef DEBUG
  if ( SIG_ERR != signal( SIGUSR1, sig_oper ) ) {
  } else {
    iRetVal = errno;
    return iRetVal;
  }

  if ( SIG_ERR != signal( SIGUSR2, sig_oper ) ) {
  } else {
    iRetVal = errno;
    return iRetVal;
  }
#endif

  return iRetVal;
}
