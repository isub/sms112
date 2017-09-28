#ifndef _SMS112_H_
#define _SMS112_H_

#include "utils/config/config.h"
#include "utils/log/log.h"

#include <stdint.h>
#include <errno.h>
#include <pthread.h>

struct SConf {
  const char *m_pszSMPPServer;
  uint16_t    m_usSMPPPort;
  uint32_t    m_uiSMPPEnqLinkSpan;
  const char *m_pszESMESysId;
  const char *m_pszESMEPaswd;
  const char *m_pszESMEType;
  const char *m_pszReportURL;
  const char *m_pszSIPSrvAdr;
  uint16_t    m_usSIPSrvPort;
  const char *m_pszSIPUsrAgn;
  const char *m_pszBoundary;
  const char *m_pszLocalSIPAddr;
  const char *m_pszVisitedNetworkID;
  const char *m_pszGeolocationCId;
  const char *m_pszGeolocationURLTemplate;
  const char *m_pszLogFileMask;
};

extern SConf *g_psoConf;
extern CLog g_coLog;

/* функция чтения файла конфигурации */
int sms112_load_conf( const char *p_pszConf );

/* операции по протоколу tcp */
/* подключение к серверу по протоколу tcp */
int sms112_tcp_connect( int &p_iSock );
/* завершение сессии tcp */
void sms112_tcp_disconn( int &p_iSock );
/* функция отправки данных */
int sms112_tcp_send( int p_iSock, uint8_t *p_pBuf, uint32_t p_uiDataSize );
/* функция получения данных */
int sms112_tcp_recv( int p_iSock, uint8_t *p_pBuf, uint32_t p_uiDataSize );
/* функция считывает данные из буфера не убирая их оттуда */
int sms112_tcp_peek( int p_iSock, uint8_t *p_pBuf, uint32_t p_uiDataSize );

/* хранение и обработка активных запросов */
/* функция добавления запроса в хранилище */
struct SReqData {
  pthread_mutex_t  m_mutexWaitResp;
  uint8_t         *m_pBuf;
  int              m_iDataLen;
  SReqData();
  ~SReqData();
};
int sms112_req_stor_add( uint32_t p_uiSeqNum, SReqData &p_soReqData );
/* функция получения данных из хранилища и удаления найденного элемента */
int sms112_req_stor_get_and_rem( uint32_t p_uiSeqNum, SReqData *&p_psoReqData );

/* взаимодействие с сервером */
/* функция потока, предназначенного для получения сообщений от сервера */
void * msg_receiver( void *p_vParam );
/* функция отправки smpp-сообщений */
int sms112_smpp_send( int p_iSock, uint8_t *p_pmBuf, int p_iDataLen, uint32_t p_uiSeqNum, SReqData &p_soReqData );

/* формирование команд smpp */
/* генерация sequence_number */
uint32_t sms112_smpp_get_seq_num();
/* функция открытия snmp-сессии */
int sms112_smpp_connect( int p_iSock );
/* функция закрытия snmp-сессии */
int sms112_smpp_disconn( int p_iSock );
/* функция формирования запроса проверки smpp-сессии */
int sms112_smpp_enq_link( int p_iSock );
/* функция обработки входящих сообщений */
int sms112_smpp_handler( int p_iSock );
/* функция обработки запроса DELIVER_SM */
int sms112_smpp_deliver_sm_oper( uint8_t *p_puiBuf, int &p_iDataLen, size_t p_stBufSize, uint8_t p_uiSeqNum );
/* функция вывода отладночной информации */
void sms112_smpp_dump_buf( uint8_t *p_puiBuf, uint32_t p_uiDataSize, const char *p_pszTitle );

/* вспомогательные потоки */
/* функция регистрации обработчика сигналов */
int sms112_reg_sig();
/* поток формирования и обработки запросов контроля активности smpp-сессии */
void * do_smpp_ping( void *p_vParam );

/* конвертация данных */
int sms112_iconv( const char *p_pszSrcCod, const char *p_pszDscCod, const char *p_pszSrcStr, size_t p_stSrcLen, char **p_ppDstStr, size_t *p_pstDstLen, int p_iIncomplete, int p_iNonreversible );

/* посылка отчета о выполнении операции */
int sms112_send_report( const char *p_pszPhoneNumber, const char *p_pszMessage, const char *p_pszResultString, const char *p_pszResultXML );

/* формирование таймаутов */
#define NSEC_PER_USEC   1000L     
#define USEC_PER_SEC    1000000L
#define NSEC_PER_SEC    1000000000L

int sms112_make_timespec_timeout( timespec &p_soTimeSpec, uint32_t p_uiSec, uint32_t p_uiAddUSec );

#endif /* _SMS112_H_ */
