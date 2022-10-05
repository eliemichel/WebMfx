#include "parameterSuite.h"
#include "types.h"

#include <string.h>

OfxStatus paramDefine(OfxParamSetHandle paramSet,
                      const char *paramType,
                      const char *name,
                      OfxPropertySetHandle *propertySet)
{
  int param_index = 0;
  while (paramSet->entries[param_index].is_valid) ++param_index;
  if (param_index == 16) return kOfxStatErrMemory;
  OfxParamHandle param = &paramSet->entries[param_index];

  paramInit(param);
  strncpy(param->type, paramType, 64);
  strncpy(param->name, name, 64);

  if (NULL != propertySet) {
    *propertySet = (OfxPropertySetHandle)&param->properties;
  }

  return kOfxStatOK;
}

const OfxParameterSuiteV1 parameterSuiteV1 = {
  paramDefine, // OfxStatus (*paramDefine)(OfxParamSetHandle paramSet, const char *paramType, const char *name, OfxPropertySetHandle *propertySet);
  NULL, // OfxStatus (*paramGetHandle)(OfxParamSetHandle paramSet, const char *name, OfxParamHandle *param, OfxPropertySetHandle *propertySet);
  NULL, // OfxStatus (*paramSetGetPropertySet)(OfxParamSetHandle paramSet, OfxPropertySetHandle *propHandle);
  NULL, // OfxStatus (*paramGetPropertySet)(OfxParamHandle param, OfxPropertySetHandle *propHandle);
  NULL, // OfxStatus (*paramGetValue)(OfxParamHandle paramHandle, ...);
  NULL, // OfxStatus (*paramGetValueAtTime)(OfxParamHandle paramHandle, OfxTime time, ...);
  NULL, // OfxStatus (*paramGetDerivative)(OfxParamHandle paramHandle, OfxTime time, ...);
  NULL, // OfxStatus (*paramGetIntegral)(OfxParamHandle paramHandle, OfxTime time1, OfxTime time2, ...);
  NULL, // OfxStatus (*paramSetValue)(OfxParamHandle paramHandle, ...);
  NULL, // OfxStatus (*paramSetValueAtTime)(OfxParamHandle paramHandle, OfxTime time, ...);
  NULL, // OfxStatus (*paramGetNumKeys)(OfxParamHandle paramHandle, unsigned int  *numberOfKeys);
  NULL, // OfxStatus (*paramGetKeyTime)(OfxParamHandle paramHandle, unsigned int nthKey, OfxTime *time);
  NULL, // OfxStatus (*paramGetKeyIndex)(OfxParamHandle paramHandle, OfxTime time, int direction, int *index);
  NULL, // OfxStatus (*paramDeleteKey)(OfxParamHandle paramHandle, OfxTime time);
  NULL, // OfxStatus (*paramDeleteAllKeys)(OfxParamHandle paramHandle);
  NULL, // OfxStatus (*paramCopy)(OfxParamHandle paramTo, OfxParamHandle  paramFrom, OfxTime dstOffset, const OfxRangeD *frameRange);
  NULL, // OfxStatus (*paramEditBegin)(OfxParamSetHandle paramSet, const char *name); 
  NULL, // OfxStatus (*paramEditEnd)(OfxParamSetHandle paramSet);
};
