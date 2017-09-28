#include "sms112.h"

#include <iconv.h>

int sms112_iconv( const char *p_pszSrcCod, const char *p_pszDscCod, const char *p_pszSrcStr, size_t p_stSrcLen, char **p_ppDstStr, size_t *p_pstDstLen, int p_iIncomplete, int p_iNonreversible )
{
  int iRetVal = 0;

  iconv_t tIConv;
  size_t stStrLen = p_stSrcLen;
  size_t stInBufLeft = stStrLen;
  size_t stConverted;
  size_t stOutBufSize = stStrLen * 4;
  size_t stOutBufLeft = stOutBufSize;
  char *pszConvStopIn = const_cast<char*>( p_pszSrcStr );
  char *pszOutBuf = new char[ stOutBufSize ];
  char *pszConStopOut;

  tIConv = iconv_open( p_pszDscCod, p_pszSrcCod );
  if ( tIConv != reinterpret_cast<iconv_t>( -1 ) ) {
  } else {
    iRetVal = -1;
    goto cleanup_and_exit;
  }

  pszConStopOut = pszOutBuf;
  stConverted = iconv( tIConv, &pszConvStopIn, &stInBufLeft, &pszConStopOut, &stOutBufLeft );
  if ( stConverted != static_cast<size_t>( -1 ) ) {
    /* копируем успешно сконвертированные символы */
    *p_pstDstLen = stOutBufSize - stOutBufLeft;
    *p_ppDstStr = pszOutBuf;
    /* если строка сконвертировалась полностью */
    if ( 0 == stInBufLeft ) {
    } else if ( 0 == p_iIncomplete ) {
      /* если неполная конвертация не допускается */
      iRetVal = -1;
      goto cleanup_and_exit;
    } else {
      /* если неполная конвертация допускается */
    }
    if ( 0 == stConverted ) {
      /* строка сконвертирована корректно */
    } else if ( 0 == p_iNonreversible ) {
      /* если необратимая конвертация не допускается */
      iRetVal = -1;
      goto cleanup_and_exit;
    } else {
      /* если необратимая конвертация не допускается */
    }
  } else {
    iRetVal = -1;
    goto cleanup_and_exit;
  }

  cleanup_and_exit:
  if ( tIConv != (iconv_t)-1 ) {
    iconv_close( tIConv );
  }

  return iRetVal;
}
