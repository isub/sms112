#include "sms112_json.h"

struct SReplSym {
  char m_cWhat;
  std::string m_strBy;
};

static void sms112_escape_spec_sym( std::string &p_strValue )
{
  SReplSym msoRepl[ ] = {
    { '"',  "\\\"" },
    { '/',  "\\/"  },
    { '\\', "\\\\" },
    { '\f', "\\f"  },
    { '\n', "\\n"  },
    { '\r', "\\r"  },
    { '\t', "\\t"  }
  };

  for ( size_t st = 0; st < p_strValue.length(); ++st ) {
    for ( int i = 0; i < sizeof( msoRepl ) / sizeof( *msoRepl ); ++i ) {
      if ( p_strValue[ st ] == msoRepl[ i ].m_cWhat ) {
        p_strValue.replace( st, 1, msoRepl[ i ].m_strBy );
        st += msoRepl[ i ].m_strBy.length() - 1;
        break;
      }
    }
  }
}

void sms112_make_json( const char *p_mpszParam[], int p_iParamCnt, const SParamDescr p_msoParamDesc[ ], int p_iDescCnt, std::string &p_strOut )
{
  int iFirst = 1;
  std::string strAttrValue;

  p_strOut = '{';
  for ( int i = 0; i < p_iDescCnt && i < p_iParamCnt; ++i ) {
    if ( 0 == iFirst ) {
      p_strOut += ',';
    } else {
      iFirst = 0;
    }
    p_strOut += '"';
    p_strOut += p_msoParamDesc[ i ].m_pszName;
    p_strOut += "\":";
    if ( 0 != p_msoParamDesc[ i ].m_iType ) {
      p_strOut += '"';
    }
    strAttrValue = p_mpszParam[ i ];
    sms112_escape_spec_sym( strAttrValue );
    p_strOut += strAttrValue;
    if ( 0 != p_msoParamDesc[ i ].m_iType ) {
      p_strOut += '"';
    }
  }
  p_strOut += '}';
}
