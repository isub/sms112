#ifndef _SMS112_CURL_H_
#define _SMS112_CURL_H_

#define SMS112_CURL_UAGENT        "SMS to SIP emergency assistance"

#include <string>

int sms112_curl_init();
void sms112_curl_fini();

int sms112_curl_perf_request( const char *p_pszURL, std::string *p_pstrBody, const char *p_pszContentType, std::string &p_strResult );

#endif /* _SMS112_CURL_H_ */
