#ifndef _SMS112_SIP_H_
#define _SMS112_SIP_H_

#include <netinet/in.h>

/* инициализация */
int sms112_sip_init();
void sms112_sip_fini();

/* передача сообщения по sip */
int sms112_sip_send_req( const char *p_pszCaller, const char *p_pszText );

#endif /* _SMS112_SIP_H_ */
