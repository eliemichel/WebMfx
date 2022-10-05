#ifndef _parameterSuite_h_
#define _parameterSuite_h_

/*****************************************************************************/
/* Parameter Suite */

#include <ofxCore.h>
#include <ofxParam.h>

OfxStatus paramDefine(OfxParamSetHandle paramSet,
                      const char *paramType,
                      const char *name,
                      OfxPropertySetHandle *propertySet);

extern const OfxParameterSuiteV1 parameterSuiteV1;

#endif // _parameterSuite_h_
