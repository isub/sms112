#include "sms112.h"
#include"sms112_tcp.h"
#include "sms112_smpp.h"

#include <sys/time.h>
#include <poll.h>
#include <errno.h>

static volatile int g_iStop;
static          pthread_mutex_t g_mutexSMPPWait;

void * do_smpp_ping( void *p_vParam )
{
  timespec soTS;
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
    if ( sms112_tcp_is_connected() ) {
    } else {
      goto do_tcp_connection;
    }
    if ( sms112_smpp_is_connected() ) {
    } else {
      goto do_smpp_connection;
    }
    if ( 0 == sms112_smpp_enq_link() ) {
    } else {
      /* проверка соединения с smpp-сервером завершилась неудачно */
      /* на всякий случай просим smpp-сервер завершить соединение */
      sms112_smpp_disconn();
      /* разрываем tcp-соединение */
      sms112_tcp_disconn();
      /* пытаемся создать новое подключение */
      do_tcp_connection:
      if ( 0 == sms112_tcp_connect() ) {
      } else {
        continue;
      }
      /* пытаемся установить smpp-соединение */
      do_smpp_connection:
      sms112_smpp_connect();
    }
    soTS.tv_sec += g_psoConf->m_uiSMPPEnqLinkSpan;
  }

  pthread_exit( NULL );
}

void * msg_receiver( void *p_vParam )
{
  struct pollfd soPollFd;
  int iFnRes;

  /* ловим изменение состояния сокета */
  while ( 0 == g_iStop ) {
    soPollFd.fd = sms112_tcp_get_sock();
    soPollFd.events = POLLIN;
    soPollFd.revents = 0;
    poll( &soPollFd, 1, 500 );
    switch ( soPollFd.revents ) {
      case POLLIN:
        sms112_smpp_handler();
        break;
    }
  }

  pthread_exit( NULL );
}

void stop_smpp_receiver_thread()
{
  g_iStop = 1;
  pthread_mutex_unlock( &g_mutexSMPPWait );
}
