#ifndef _SMS112_JSON_H_
#define _SMS112_JSON_H_

#include <string>

struct SParamDescr {
  const char *m_pszName; /* Имя атрибута */
  const int   m_iType; /* тип данных атрибута (в текущей реализации 0 - значение без кавычек, 1 - значение заключено в кавычки) */
};

void sms112_make_json( const char *p_mpszParam[], int p_iParamCnt, const SParamDescr p_msoParamDesc[ ], int p_iDescCnt, std::string &p_strOut );

#endif /* _SMS112_JSON_H_ */
