#ifndef _SMS112_SIP_H_
#define _SMS112_SIP_H_

/* инициализация */
int sms112_sip_init();
void sms112_sip_fini();

/* передача сообщения по sip */
int sms112_sip_send_req( const char *p_pszCaller, const char *p_pszText, const char *p_pszGeodata );

#endif /* _SMS112_SIP_H_ */
