#ifndef _SMS112_JSON_H_
#define _SMS112_JSON_H_

#include <string>

struct SParamDescr {
  const char *m_pszName; /* ��� �������� */
  const int   m_iType; /* ��� ������ �������� (� ������� ���������� 0 - �������� ��� �������, 1 - �������� ��������� � �������) */
};

void sms112_make_json( const char *p_mpszParam[], int p_iParamCnt, const SParamDescr p_msoParamDesc[ ], int p_iDescCnt, std::string &p_strOut );

#endif /* _SMS112_JSON_H_ */
