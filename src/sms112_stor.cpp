#include "sms112.h"

#include <pthread.h>
#include <map>
#include <stdlib.h>

static std::map<uint32_t, SReqData*> g_mapReq;

int sms112_req_stor_add( uint32_t p_uiSeqNum, SReqData &p_soReqData )
{
  int iRetVal = 0;
  std::pair<std::map<uint32_t, SReqData*>::iterator, bool> pairResult;

  pairResult = g_mapReq.insert( std::pair<uint32_t, SReqData*>( p_uiSeqNum, &p_soReqData ) );
  if ( pairResult.second ) {
  } else {
    iRetVal = EEXIST;
  }

  return iRetVal;
}

int sms112_req_stor_get_and_rem( uint32_t p_uiSeqNum, SReqData *&p_psoReqData )
{
  int iRetVal = 0;
  std::map<uint32_t, SReqData*>::iterator iter;

  iter = g_mapReq.find( p_uiSeqNum );
  if ( iter != g_mapReq.end() ) {
    p_psoReqData = iter->second;
    g_mapReq.erase( iter );
  } else {
    iRetVal = ENOENT;
  }

  return iRetVal;
}

SReqData::SReqData() : m_pBuf( NULL ), m_iDataLen( 0 )
{
  pthread_mutex_init( &m_mutexWaitResp, NULL );
  pthread_mutex_lock( &m_mutexWaitResp );
}

SReqData::~SReqData()
{
  if ( NULL != m_pBuf ) {
    free( m_pBuf );
    m_iDataLen = 0;
  }
  pthread_mutex_destroy( &m_mutexWaitResp );
}
