#include "sms112.h"
#ifdef DEBUG
  #include "sms112_geodata.h"
  #include "sms112_sip.h"
  #include <string>
#endif

#include <sys/time.h>
#include <signal.h>
#include <poll.h>
#include <errno.h>

static volatile int g_iStop;
static          pthread_mutex_t g_mutexSMPPWait;

void * do_smpp_ping( void *p_vParam )
{
  timespec soTS;
  int iSock = *reinterpret_cast<int*>( p_vParam );
  int iFnRes;

  if ( 0 == pthread_mutex_init( &g_mutexSMPPWait, NULL ) ) {
    pthread_mutex_lock( &g_mutexSMPPWait );
  } else {
    pthread_exit( NULL );
  }

  sms112_make_timespec_timeout( soTS, g_psoConf->m_uiSMPPEnqLinkSpan, 0 );

  while ( 0 == g_iStop ) {
    if ( ETIMEDOUT == ( iFnRes = pthread_mutex_timedlock( &g_mutexSMPPWait, &soTS ) ) ) {
    } else {
      break;
    }
    sms112_smpp_enq_link( iSock );
    soTS.tv_sec += g_psoConf->m_uiSMPPEnqLinkSpan;
  }

  pthread_exit( NULL );
}

void * msg_receiver( void *p_vParam )
{
  int iSock = *reinterpret_cast<int*>( p_vParam );
  struct pollfd soPollFd;
  int iFnRes;

  /* ловим изменение состояния сокета */
  while ( 0 == g_iStop ) {
    soPollFd.fd = iSock;
    soPollFd.events = POLLIN;
    soPollFd.revents = 0;
    poll( &soPollFd, 1, 500 );
    switch ( soPollFd.revents ) {
      case POLLIN:
        sms112_smpp_handler( iSock );
        break;
    }
  }

  pthread_exit( NULL );
}

void sig_oper( int p_iSig )
{
  UTL_LOG_N( g_coLog, "signal caught: %d", p_iSig );

  switch ( p_iSig ) {
    case SIGTERM:
    case SIGINT:
      g_iStop = 1;
      pthread_mutex_unlock( &g_mutexSMPPWait );
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
      sms112_sip_send_req( "79506656062", "помощь нужна ващета" );
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
  } else {
    iRetVal = errno;
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
