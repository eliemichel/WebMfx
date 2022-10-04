// Copyright 2011 The Emscripten Authors.  All rights reserved.
// Emscripten is available under two separate licenses, the MIT license and the
// University of Illinois/NCSA Open Source License.  Both these licenses can be
// found in the LICENSE file.

#include "openmfx-sdk/c/common/common.h" // for MFX_CHECK and MFX_ENSURE

#include <ofxCore.h>
#include <ofxMeshEffect.h>
#include <ofxProperty.h>
#include <ofxParam.h>

#include <stdio.h>
#include <SDL/SDL.h>
#include <dlfcn.h>
#include <assert.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

/*****************************************************************************/
/* Data Structures (and their ctor/dtor/copy) */

typedef enum OfxPropertySetType {
  PROPSET_UNKNOWN,
  PROPSET_INPUT,
  PROPSET_PARAM,
  PROPSET_MESH,
  PROPSET_ATTRIBUTE,
} OfxPropertySetType;

typedef struct OfxPropertySetStruct {
  OfxPropertySetType type;
} OfxPropertySetStruct;

typedef struct OfxParamPropertySet {
  OfxPropertySetStruct *header;
  char label[256];
} OfxParamPropertySet;

typedef struct OfxParamStruct {
  int is_valid;
  char name[64];
  char type[64];
  OfxParamPropertySet properties;
} OfxParamStruct;

typedef struct OfxParamSetStruct {
  OfxParamStruct entries[16];
} OfxParamSetStruct;

typedef struct OfxMeshPropertySet {
  OfxPropertySetStruct *header;
  int point_count;
  int corner_count;
  int face_count;
} OfxMeshPropertySet;

typedef struct OfxMeshAttributePropertySet {
  OfxPropertySetStruct *header;
  int is_valid;
  char name[64];
  char attachment[64];
  int component_count;
  char type[64];
  char semantic[64];
  char *data;
  size_t byte_stride;
  int is_owner;
} OfxMeshAttributePropertySet;

typedef struct OfxMeshStruct {
  OfxMeshAttributePropertySet attributes[32];
  OfxMeshPropertySet properties;
} OfxMeshStruct;

typedef struct OfxMeshInputPropertySet {
  OfxPropertySetStruct *header;
  char label[256];
} OfxMeshInputPropertySet;

typedef struct OfxMeshInputStruct {
  int is_valid;
  char name[256];
  OfxMeshStruct mesh;
  OfxMeshInputPropertySet properties;
} OfxMeshInputStruct;

typedef struct OfxMeshEffectStruct {
  int is_valid;
  OfxMeshInputStruct inputs[16];
  OfxParamSetStruct parameters;
} OfxMeshEffectStruct;

void meshInputPropertySetCopy(OfxMeshInputPropertySet *dst, const OfxMeshInputPropertySet *src) {
  strncpy(dst->label, src->label, 256);
}

void propertySetCopy(OfxPropertySetHandle dst, const OfxPropertySetHandle src) {
  assert(dst->type == src->type);
  switch (src->type) {
    case PROPSET_INPUT:
      meshInputPropertySetCopy((OfxMeshInputPropertySet*)dst, (const OfxMeshInputPropertySet*)src);
      break;
    case PROPSET_UNKNOWN:
    default:
      break;
  }
}

void propertySetInit(OfxPropertySetHandle propertySet, OfxPropertySetType type) {
  propertySet->type = type;
}

void attributeInit(OfxMeshAttributePropertySet *attrib) {
  propertySetInit((OfxPropertySetHandle)attrib, PROPSET_ATTRIBUTE);
  attrib->is_valid = 0;
  attrib->data = NULL;
  attrib->is_owner = 1;
}

OfxStatus attributeAlloc(OfxMeshAttributePropertySet *attrib, OfxMeshPropertySet *props)
{
  int element_count;
  if (0 == strcmp(attrib->attachment, kOfxMeshAttribPoint)) {
    element_count = props->point_count;
  } else if (0 == strcmp(attrib->attachment, kOfxMeshAttribCorner)) {
    element_count = props->corner_count;
  } else if (0 == strcmp(attrib->attachment, kOfxMeshAttribFace)) {
    element_count = props->face_count;
  } else {
    element_count = 1;
  }

  size_t component_size = 0;
  if (0 == strcmp(attrib->type, kOfxMeshAttribTypeFloat)) {
    component_size = sizeof(float);
  } else if (0 == strcmp(attrib->type, kOfxMeshAttribTypeInt)) {
    component_size = sizeof(int);
  } else if (0 == strcmp(attrib->type, kOfxMeshAttribTypeUByte)) {
    component_size = sizeof(unsigned char);
  } else {
    assert(0);
  }

  attrib->byte_stride = attrib->component_count * component_size;
  attrib->data = malloc(element_count * attrib->byte_stride);
  if (NULL == attrib->data) {
    return kOfxStatErrMemory;
  }

  printf("[host] attributeAlloc -> %p\n", attrib->data);

  return kOfxStatOK;
}

void attributeDestroy(OfxMeshAttributePropertySet *attrib) {
  attrib->is_valid = 0;
  if (attrib->is_owner && NULL != attrib->data) {
    printf("[host] attributeDestroy(data %p)\n", attrib->data);
    free(attrib->data);
    attrib->data = NULL;
  }
}

void parameterSetInit(OfxParamSetHandle parameterSet) {
  for (int i = 0 ; i < 16 ; ++i) {
    parameterSet->entries[1].is_valid = 0;
  }
}

void paramInit(OfxParamHandle param) {
  param->is_valid = 1;
  propertySetInit((OfxPropertySetHandle)&param->properties, PROPSET_PARAM);
}

void meshInit(OfxMeshHandle mesh) {
  propertySetInit((OfxPropertySetHandle)&mesh->properties, PROPSET_MESH);
  for (int i = 0 ; i < 32 ; ++i) {
    attributeInit(&mesh->attributes[i]);
  }
}

void meshDestroy(OfxMeshHandle mesh) {
  for (int i = 0 ; i < 32 && mesh->attributes[i].is_valid ; ++i) {
    attributeDestroy(&mesh->attributes[i]);
  }
}

void meshInputInit(OfxMeshInputHandle input) {
  input->is_valid = 1;
  input->name[0] = '\0';
  meshInit(&input->mesh);
  propertySetInit((OfxPropertySetHandle)&input->properties, PROPSET_INPUT);
}

void meshInputCopy(OfxMeshInputHandle dst, const OfxMeshInputHandle src) {
  meshInputInit(dst);
  dst->is_valid = src->is_valid;
  strncpy(dst->name, src->name, 256);
  meshInputPropertySetCopy(&dst->properties, &src->properties);
}

void meshInputDestroy(OfxMeshInputHandle input) {
  meshDestroy(&input->mesh);
  input->is_valid = 0;
}

void meshEffectInit(OfxMeshEffectHandle meshEffect) {
  parameterSetInit(&meshEffect->parameters);
  for (int i = 0 ; i < 16 ; ++i) {
    meshEffect->inputs[1].is_valid = 0;
  }
  meshEffect->is_valid = 1;
}

void meshEffectDestroy(OfxMeshEffectHandle meshEffect) {
  for (int i = 0 ; i < 16 ; ++i) {
    meshInputDestroy(&meshEffect->inputs[i]);
  }
  meshEffect->is_valid = 0;
}

void meshEffectCopy(OfxMeshEffectHandle dst, const OfxMeshEffectHandle src) {
  if (dst->is_valid) {
    meshEffectDestroy(dst);
  }
  dst->is_valid = src->is_valid;
  for (int input_index = 0; src->inputs[input_index].is_valid; ++input_index) {
    meshInputCopy(&dst->inputs[input_index], &src->inputs[input_index]);
  }
}

/*****************************************************************************/
/* Mesh Effect Suite */

OfxStatus defaultAttributesDefine(OfxMeshHandle mesh);

OfxStatus getParamSet(OfxMeshEffectHandle meshEffect,
                      OfxParamSetHandle *paramSet)
{
  printf("[host] getParamSet(meshEffect %p)\n", meshEffect);
  *paramSet = &meshEffect->parameters;
  return kOfxStatOK;
}

OfxStatus inputDefine(OfxMeshEffectHandle meshEffect,
                      const char *name,
                      OfxMeshInputHandle *inputHandle,
                      OfxPropertySetHandle *propertySet)
{
  printf("[host] inputDefine(meshEffect %p, %s)\n", meshEffect, name);
  if (meshEffect->is_valid != 1) {
    return kOfxStatErrBadHandle;
  }

  int input_index = 0;
  while (meshEffect->inputs[input_index].is_valid) ++input_index;
  if (input_index == 16) return kOfxStatErrMemory;
  OfxMeshInputHandle new_input = &meshEffect->inputs[input_index];

  meshInputInit(new_input);
  strncpy(new_input->name, name, 256);

  *inputHandle = new_input;
  *propertySet = (OfxPropertySetHandle)&new_input->properties;
  return kOfxStatOK;
}

OfxStatus inputGetHandle(OfxMeshEffectHandle meshEffect,
                         const char *name,
                         OfxMeshInputHandle *inputHandle,
                         OfxPropertySetHandle *propertySet)
{
  printf("[host] inputGetHandle(meshEffect %p, %s)\n", meshEffect, name);
  if (meshEffect->is_valid != 1) {
    return kOfxStatErrBadHandle;
  }

  for (int i = 0 ; i < 16 && meshEffect->inputs[i].is_valid ; ++i) {
    OfxMeshInputHandle input = &meshEffect->inputs[i];
    if (0 == strncmp(name, input->name, 256)) {
      *inputHandle = input;
      if (NULL != propertySet) {
        *propertySet = (OfxPropertySetHandle)&input->properties;
      }
      return kOfxStatOK;
    }
  }

  return kOfxStatErrBadIndex;
}

OfxStatus inputGetMesh(OfxMeshInputHandle input,
                       OfxTime time,
                       OfxMeshHandle *meshHandle,
                       OfxPropertySetHandle *propertySet)
{
  printf("[host] inputGetMesh(input %p, %f)\n", input, time);
  if (input->is_valid != 1) {
    return kOfxStatErrBadHandle;
  }
  if (time != 0.0) {
    return kOfxStatErrMissingHostFeature;
  }

  if (0 == strncmp(input->name, kOfxMeshMainOutput, 256)) {
    MFX_ENSURE(defaultAttributesDefine(&input->mesh));
  }

  *meshHandle = &input->mesh;
  if (NULL != propertySet) {
    *propertySet = (OfxPropertySetHandle)&input->mesh.properties;
  }

  return kOfxStatOK;
}

OfxStatus inputReleaseMesh(OfxMeshHandle meshHandle)
{
  printf("[host] inputReleaseMesh(mesh %p)\n", meshHandle);
  return kOfxStatOK;
}

OfxStatus meshGetAttribute(OfxMeshHandle meshHandle,
                           const char *attachment,
                           const char *name,
                           OfxPropertySetHandle *attributeHandle)
{
  printf("[host] meshGetAttribute(mesh %p, %s, %s)\n", meshHandle, attachment, name);
  for (int i = 0 ; i < 32 && meshHandle->attributes[i].is_valid ; ++i) {
    OfxMeshAttributePropertySet* attribute = &meshHandle->attributes[i];
    if (0 == strncmp(attachment, attribute->attachment, 64) &&
        0 == strncmp(name, attribute->name, 64))
    {
      *attributeHandle = (OfxPropertySetHandle)attribute;
      return kOfxStatOK;
    }
  }

  return kOfxStatErrBadIndex;
}

OfxStatus attributeDefine(OfxMeshHandle meshHandle,
                          const char *attachment,
                          const char *name,
                          int componentCount,
                          const char *type,
                          const char *semantic,
                          OfxPropertySetHandle *attributeHandle)
{
  printf("[host] attributeDefine(mesh %p, %s, %s, %d, %s, %s)\n", meshHandle, attachment, name, componentCount, type, semantic);

  // Check for duplicates
  OfxPropertySetHandle existingAttribute = NULL;
  if (kOfxStatErrBadIndex != meshGetAttribute(meshHandle, attachment, name, &existingAttribute)) {
    return kOfxStatErrExists;
  }

  int attribute_index = 0;
  while (meshHandle->attributes[attribute_index].is_valid) ++attribute_index;
  if (attribute_index == 32) return kOfxStatErrMemory;
  OfxMeshAttributePropertySet* attribute = &meshHandle->attributes[attribute_index];

  attribute->is_valid = 1;
  strncpy(attribute->attachment, attachment, 64);
  strncpy(attribute->name, name, 64);
  attribute->component_count = componentCount;
  strncpy(attribute->type, type, 64);
  strncpy(attribute->semantic, semantic, 64);

  *attributeHandle = (OfxPropertySetHandle)attribute;
  return kOfxStatOK;
}

OfxStatus meshAlloc(OfxMeshHandle meshHandle)
{
  printf("[host] meshAlloc(mesh %p)\n", meshHandle);
  OfxMeshPropertySet *props = &meshHandle->properties;
  for (int i = 0 ; i < 32 && meshHandle->attributes[i].is_valid ; ++i) {
    MFX_ENSURE(attributeAlloc(&meshHandle->attributes[i], props));
  }
  return kOfxStatOK;
}

OfxStatus defaultAttributesDefine(OfxMeshHandle mesh)
{
  OfxPropertySetHandle attrib = NULL;
  MFX_ENSURE(attributeDefine(mesh, kOfxMeshAttribPoint, kOfxMeshAttribPointPosition, 3, kOfxMeshAttribTypeFloat, NULL, &attrib));
  MFX_ENSURE(attributeDefine(mesh, kOfxMeshAttribCorner, kOfxMeshAttribCornerPoint, 1, kOfxMeshAttribTypeInt, NULL, &attrib));
  MFX_ENSURE(attributeDefine(mesh, kOfxMeshAttribFace, kOfxMeshAttribFaceSize, 1, kOfxMeshAttribTypeInt, NULL, &attrib));
  return kOfxStatOK;
}

static const OfxMeshEffectSuiteV1 meshEffectSuiteV1 = {
  NULL, // OfxStatus (*getPropertySet)(OfxMeshEffectHandle meshEffect,
  getParamSet, // OfxStatus (*getParamSet)(OfxMeshEffectHandle meshEffect,
  inputDefine, // OfxStatus (*inputDefine)(OfxMeshEffectHandle meshEffect,
  inputGetHandle, // OfxStatus (*inputGetHandle)(OfxMeshEffectHandle meshEffect,
  NULL, // OfxStatus (*inputGetPropertySet)(OfxMeshInputHandle input,
  NULL, // OfxStatus (*inputRequestAttribute)(OfxMeshInputHandle input,
  inputGetMesh, // OfxStatus (*inputGetMesh)(OfxMeshInputHandle input,
  inputReleaseMesh, // OfxStatus (*inputReleaseMesh)(OfxMeshHandle meshHandle);
  attributeDefine, // OfxStatus(*attributeDefine)(OfxMeshHandle meshHandle,
  meshGetAttribute, // OfxStatus(*meshGetAttribute)(OfxMeshHandle meshHandle,
  NULL, // OfxStatus (*meshGetPropertySet)(OfxMeshHandle mesh,
  meshAlloc, // OfxStatus (*meshAlloc)(OfxMeshHandle meshHandle);
  NULL, // int (*abort)(OfxMeshEffectHandle meshEffect);
};

/*****************************************************************************/
/* Property Suite */

OfxStatus propSetPointer(OfxPropertySetHandle properties,
                         const char *property,
                         int index,
                         void *value)
{
  printf("[host] propSetPointer(properties %p, %s, %d, %p)\n", properties, property, index, value);

  switch (properties->type) {
    case PROPSET_ATTRIBUTE:
    {
      OfxMeshAttributePropertySet *attrib_props = (OfxMeshAttributePropertySet*)properties;
      if (0 == strcmp(property, kOfxMeshAttribPropData)) {
        if (index != 0) {
          return kOfxStatErrBadIndex;
        }
        attrib_props->data = (char*)value;
        return kOfxStatOK;
      }
      return kOfxStatErrBadHandle;
    }
    case PROPSET_UNKNOWN:
    default:
      return kOfxStatErrBadHandle;
  }

  return kOfxStatOK;
}

OfxStatus propGetPointer(OfxPropertySetHandle properties,
                         const char *property,
                         int index,
                         void **value)
{
  printf("[host] propGetPointer(properties %p, %s, %d)\n", properties, property, index);

  switch (properties->type) {
    case PROPSET_ATTRIBUTE:
    {
      OfxMeshAttributePropertySet *attrib_props = (OfxMeshAttributePropertySet*)properties;
      if (0 == strcmp(property, kOfxMeshAttribPropData)) {
        if (index != 0) {
          return kOfxStatErrBadIndex;
        }
        *value = (void*)attrib_props->data;
        return kOfxStatOK;
      }
      return kOfxStatErrBadHandle;
    }
    case PROPSET_UNKNOWN:
    default:
      return kOfxStatErrBadHandle;
  }

  return kOfxStatOK;
}

OfxStatus propSetString(OfxPropertySetHandle properties,
                        const char *property,
                        int index,
                        const char *value)
{
  printf("[host] propSetString(properties %p, %s, %d, %s)\n", properties, property, index, value);

  switch (properties->type) {
    case PROPSET_INPUT:
    {
      OfxMeshInputPropertySet *input_props = (OfxMeshInputPropertySet*)properties;
      if (0 == strcmp(property, kOfxPropLabel)) {
        if (index != 0) {
          return kOfxStatErrBadIndex;
        }
        strncpy(input_props->label, value, 256);
        return kOfxStatOK;
      }
      return kOfxStatErrBadHandle;
    }
    case PROPSET_ATTRIBUTE:
    {
      OfxMeshAttributePropertySet *attrib_props = (OfxMeshAttributePropertySet*)properties;
      if (0 == strcmp(property, kOfxMeshAttribPropType)) {
        if (index != 0) {
          return kOfxStatErrBadIndex;
        }
        strncpy(attrib_props->type, value, 64);
        return kOfxStatOK;
      }
      else if (0 == strcmp(property, kOfxMeshAttribPropSemantic)) {
        if (index != 0) {
          return kOfxStatErrBadIndex;
        }
        strncpy(attrib_props->semantic, value, 64);
        return kOfxStatOK;
      }
      return kOfxStatErrBadHandle;
    }
    case PROPSET_UNKNOWN:
    default:
      return kOfxStatErrBadHandle;
  }

  return kOfxStatOK;
}

OfxStatus propGetString(OfxPropertySetHandle properties,
                        const char *property,
                        int index,
                        char **value)
{
  printf("[host] propSetString(properties %p, %s, %d)\n", properties, property, index);

  switch (properties->type) {
    case PROPSET_INPUT:
    {
      OfxMeshInputPropertySet *input_props = (OfxMeshInputPropertySet*)properties;
      if (0 == strcmp(property, kOfxPropLabel)) {
        if (index != 0) {
          return kOfxStatErrBadIndex;
        }
        *value = input_props->label;
        return kOfxStatOK;
      }
      return kOfxStatErrBadHandle;
    }
    case PROPSET_ATTRIBUTE:
    {
      OfxMeshAttributePropertySet *attrib_props = (OfxMeshAttributePropertySet*)properties;
      if (0 == strcmp(property, kOfxMeshAttribPropType)) {
        if (index != 0) {
          return kOfxStatErrBadIndex;
        }
        *value = attrib_props->type;
        return kOfxStatOK;
      }
      else if (0 == strcmp(property, kOfxMeshAttribPropSemantic)) {
        if (index != 0) {
          return kOfxStatErrBadIndex;
        }
        *value = attrib_props->semantic;
        return kOfxStatOK;
      }
      return kOfxStatErrBadHandle;
    }
    case PROPSET_UNKNOWN:
    default:
      return kOfxStatErrBadHandle;
  }

  return kOfxStatOK;
}

OfxStatus propSetInt(OfxPropertySetHandle properties,
                     const char *property,
                     int index,
                     int value)
{
  printf("[host] propSetInt(properties %p, %s, %d, %d)\n", properties, property, index, value);

  switch (properties->type) {
    case PROPSET_MESH:
    {
      OfxMeshPropertySet *mesh_props = (OfxMeshPropertySet*)properties;
      if (0 == strcmp(property, kOfxMeshPropPointCount)) {
        if (index != 0) {
          return kOfxStatErrBadIndex;
        }
        mesh_props->point_count = value;
        return kOfxStatOK;
      }
      else if (0 == strcmp(property, kOfxMeshPropCornerCount)) {
        if (index != 0) {
          return kOfxStatErrBadIndex;
        }
        mesh_props->corner_count = value;
        return kOfxStatOK;
      }
      else if (0 == strcmp(property, kOfxMeshPropFaceCount)) {
        if (index != 0) {
          return kOfxStatErrBadIndex;
        }
        mesh_props->face_count = value;
        return kOfxStatOK;
      }
      return kOfxStatErrBadHandle;
    }
    case PROPSET_ATTRIBUTE:
    {
      OfxMeshAttributePropertySet *attrib_props = (OfxMeshAttributePropertySet*)properties;
      if (0 == strcmp(property, kOfxMeshAttribPropComponentCount)) {
        if (index != 0) {
          return kOfxStatErrBadIndex;
        }
        attrib_props->component_count = value;
        return kOfxStatOK;
      }
      else if (0 == strcmp(property, kOfxMeshAttribPropStride)) {
        if (index != 0) {
          return kOfxStatErrBadIndex;
        }
        attrib_props->byte_stride = (size_t)value;
        return kOfxStatOK;
      }
      else if (0 == strcmp(property, kOfxMeshAttribPropIsOwner)) {
        if (index != 0) {
          return kOfxStatErrBadIndex;
        }
        attrib_props->is_owner = value;
        return kOfxStatOK;
      }
      return kOfxStatErrBadHandle;
    }
    case PROPSET_UNKNOWN:
    default:
      return kOfxStatErrBadHandle;
  }

  return kOfxStatOK;
}

OfxStatus propGetInt(OfxPropertySetHandle properties,
                     const char *property,
                     int index,
                     int *value)
{
  printf("[host] propGetInt(properties %p, %s, %d)\n", properties, property, index);

  switch (properties->type) {
    case PROPSET_MESH:
    {
      OfxMeshPropertySet *mesh_props = (OfxMeshPropertySet*)properties;
      if (0 == strcmp(property, kOfxMeshPropPointCount)) {
        if (index != 0) {
          return kOfxStatErrBadIndex;
        }
        *value = mesh_props->point_count;
        return kOfxStatOK;
      }
      else if (0 == strcmp(property, kOfxMeshPropCornerCount)) {
        if (index != 0) {
          return kOfxStatErrBadIndex;
        }
        *value = mesh_props->corner_count;
        return kOfxStatOK;
      }
      else if (0 == strcmp(property, kOfxMeshPropFaceCount)) {
        if (index != 0) {
          return kOfxStatErrBadIndex;
        }
        *value = mesh_props->face_count;
        return kOfxStatOK;
      }
      return kOfxStatErrBadHandle;
    }
    case PROPSET_ATTRIBUTE:
    {
      OfxMeshAttributePropertySet *attrib_props = (OfxMeshAttributePropertySet*)properties;
      if (0 == strcmp(property, kOfxMeshAttribPropComponentCount)) {
        if (index != 0) {
          return kOfxStatErrBadIndex;
        }
        *value = attrib_props->component_count;
        return kOfxStatOK;
      }
      else if (0 == strcmp(property, kOfxMeshAttribPropStride)) {
        if (index != 0) {
          return kOfxStatErrBadIndex;
        }
        *value = (int)attrib_props->byte_stride;
        return kOfxStatOK;
      }
      else if (0 == strcmp(property, kOfxMeshAttribPropIsOwner)) {
        if (index != 0) {
          return kOfxStatErrBadIndex;
        }
        *value = attrib_props->is_owner;
        return kOfxStatOK;
      }
      return kOfxStatErrBadHandle;
    }
    case PROPSET_UNKNOWN:
    default:
      return kOfxStatErrBadHandle;
  }

  return kOfxStatOK;
}

static const OfxPropertySuiteV1 propertySuiteV1 = {
  propSetPointer, // OfxStatus (*propSetPointer)(OfxPropertySetHandle properties, const char *property, int index, void *value);
  propSetString, // OfxStatus (*propSetString) (OfxPropertySetHandle properties, const char *property, int index, const char *value);
  NULL, // OfxStatus (*propSetDouble) (OfxPropertySetHandle properties, const char *property, int index, double value);
  propSetInt, // OfxStatus (*propSetInt)    (OfxPropertySetHandle properties, const char *property, int index, int value);
  NULL, // OfxStatus (*propSetPointerN)(OfxPropertySetHandle properties, const char *property, int count, void *const*value);
  NULL, // OfxStatus (*propSetStringN) (OfxPropertySetHandle properties, const char *property, int count, const char *const*value);
  NULL, // OfxStatus (*propSetDoubleN) (OfxPropertySetHandle properties, const char *property, int count, const double *value);
  NULL, // OfxStatus (*propSetIntN)    (OfxPropertySetHandle properties, const char *property, int count, const int *value);
  propGetPointer, // OfxStatus (*propGetPointer)(OfxPropertySetHandle properties, const char *property, int index, void **value);
  propGetString, // OfxStatus (*propGetString) (OfxPropertySetHandle properties, const char *property, int index, char **value);
  NULL, // OfxStatus (*propGetDouble) (OfxPropertySetHandle properties, const char *property, int index, double *value);
  propGetInt, // OfxStatus (*propGetInt)    (OfxPropertySetHandle properties, const char *property, int index, int *value);
  NULL, // OfxStatus (*propGetPointerN)(OfxPropertySetHandle properties, const char *property, int count, void **value);
  NULL, // OfxStatus (*propGetStringN) (OfxPropertySetHandle properties, const char *property, int count, char **value);
  NULL, // OfxStatus (*propGetDoubleN) (OfxPropertySetHandle properties, const char *property, int count, double *value);
  NULL, // OfxStatus (*propGetIntN)    (OfxPropertySetHandle properties, const char *property, int count, int *value);
  NULL, // OfxStatus (*propReset)    (OfxPropertySetHandle properties, const char *property);
  NULL, // OfxStatus (*propGetDimension)  (OfxPropertySetHandle properties, const char *property, int *count);
};

/*****************************************************************************/
/* Parameter Suite */

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

static const OfxParameterSuiteV1 parameterSuiteV1 = {
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

/*****************************************************************************/
/* Master Host */

const void* fetchSuite(OfxPropertySetHandle host, const char *suiteName, int suiteVersion) {
  printf("[host] fetchSuite(host, %s, %d)\n", suiteName, suiteVersion);
  if (0 == strcmp(suiteName, kOfxMeshEffectSuite)) {
    switch (suiteVersion) {
      case 1:
        return (void*)&meshEffectSuiteV1;
    }
  }
  if (0 == strcmp(suiteName, kOfxPropertySuite)) {
    switch (suiteVersion) {
      case 1:
        return (void*)&propertySuiteV1;
    }
  }
  if (0 == strcmp(suiteName, kOfxParameterSuite)) {
    switch (suiteVersion) {
      case 1:
        return (void*)&parameterSuiteV1;
    }
  }
  return NULL;
}

/*****************************************************************************/
/* Test */

typedef struct test_return_t {
  int point_count;
  char *point_position_data;
} test_return_t;

EMSCRIPTEN_KEEPALIVE
int test(test_return_t* ret) {
  printf("hello, world!\n");

  SDL_Init(SDL_INIT_VIDEO);
  SDL_Surface *screen = SDL_SetVideoMode(256, 256, 32, SDL_SWSURFACE);

#ifdef TEST_SDL_LOCK_OPTS
  EM_ASM("SDL.defaults.copyOnLock = false; SDL.defaults.discardOnLock = true; SDL.defaults.opaqueFrontBuffer = false;");
#endif

  if (SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
  for (int i = 0; i < 256; i++) {
    for (int j = 0; j < 256; j++) {
#ifdef TEST_SDL_LOCK_OPTS
      // Alpha behaves like in the browser, so write proper opaque pixels.
      int alpha = 255;
#else
      // To emulate native behavior with blitting to screen, alpha component is ignored. Test that it is so by outputting
      // data (and testing that it does get discarded)
      int alpha = (i+j) % 255;
#endif
      *((Uint32*)screen->pixels + i * 256 + j) = SDL_MapRGBA(screen->format, i, j, 255-i, alpha);
    }
  }
  if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
  SDL_Flip(screen); 

  printf("you should see a smoothly-colored square - no sharp lines but the square borders!\n");
  printf("and here is some text that should be HTML-friendly: amp: |&| double-quote: |\"| quote: |'| less-than, greater-than, html-like tags: |<cheez></cheez>|\nanother line.\n");

  SDL_Quit();

  printf("===============\n");

  printf("Checking plugin file\n");
  FILE *file = fopen("plugin.wasm", "rb");
  if (!file) {
    printf("cannot open file\n");
    return 1;
  } else {
    printf("can open file\n");
  }
  fclose(file);

  printf("Loading plugin\n");
  void* handle = dlopen("plugin.wasm", RTLD_LAZY);
  if (NULL == handle) {
    printf("Error while loading plugin: %s\n", dlerror());
  }

  int (*OfxGetNumberOfPlugins)(void) = dlsym(handle, "OfxGetNumberOfPlugins");
  if (NULL == OfxGetNumberOfPlugins) {
    printf("Error while loading symbol 'OfxGetNumberOfPlugins': %s\n", dlerror());
  }
  OfxPlugin*(*OfxGetPlugin)(int) = dlsym(handle, "OfxGetPlugin");
  if (NULL == OfxGetPlugin) {
    printf("Error while loading symbol 'OfxGetPlugin': %s\n", dlerror());
  }

  int plugin_count = OfxGetNumberOfPlugins();
  printf("Found %d plugins:\n", plugin_count);
  for (int i = 0 ; i < plugin_count ; ++i) {
    OfxPlugin *plugin = OfxGetPlugin(i);
    printf(" - %s\n", plugin->pluginIdentifier);
  }

  printf("Create host\n");
  OfxHost host;
  host.fetchSuite = &fetchSuite;

  printf("Loading plugin #0...\n");
  OfxPlugin *plugin = OfxGetPlugin(0);
  printf("(setHost)\n");
  plugin->setHost(&host);
  printf("(ActionLoad)\n");
  MFX_CHECK(plugin->mainEntry(kOfxActionLoad, NULL, NULL, NULL));

  OfxMeshEffectStruct descriptor;
  meshEffectInit(&descriptor);
  printf("(ActionDescribe)\n");
  MFX_CHECK(plugin->mainEntry(kOfxActionDescribe, &descriptor, NULL, NULL));

  printf("Parameters:\n");
  for (int i = 0 ; descriptor.parameters.entries[i].is_valid ; ++i) {
    const OfxParamHandle param = &descriptor.parameters.entries[i];
    printf(" - %s (%s)\n", param->name, param->type);
  }
  printf("Inputs:\n");
  for (int i = 0 ; descriptor.inputs[i].is_valid ; ++i) {
    const OfxMeshInputHandle input = &descriptor.inputs[i];
    printf(" - %s (%s)\n", input->properties.label, input->name);
  }

  OfxMeshEffectStruct instance;
  meshEffectCopy(&instance, &descriptor);
  printf("(CreateInstance)\n");
  MFX_CHECK(plugin->mainEntry(kOfxActionCreateInstance, &instance, NULL, NULL));

  printf("Setting inputs and parameters\n");
  // TODO

  printf("(Cook)\n");
  MFX_CHECK(plugin->mainEntry(kOfxMeshEffectActionCook, &instance, NULL, NULL));

  printf("Getting output mesh:\n");
  OfxMeshInputHandle main_output = NULL;
  for (int i = 0 ; i < 16 && instance.inputs[i].is_valid ; ++i) {
    OfxMeshInputHandle input = &instance.inputs[i];
    if (0 == strcmp(input->name, kOfxMeshMainOutput)) {
      main_output = input;
    }
  }
  if (NULL != main_output) {
    const OfxMeshPropertySet *props = &main_output->mesh.properties;
    printf(" - %d points\n", props->point_count);
    printf(" - %d corners\n", props->corner_count);
    printf(" - %d faces\n", props->face_count);
    ret->point_count = props->point_count;

    OfxMeshAttributePropertySet *point_position_attrib;
    OfxStatus status = meshGetAttribute(&main_output->mesh, kOfxMeshAttribPoint, kOfxMeshAttribPointPosition, (OfxPropertySetHandle*)&point_position_attrib);
    if (kOfxStatOK == status) {
      ret->point_position_data = point_position_attrib->data;
    } else {
      printf("Error: could not find a point position attribute in the output mesh!\n");
    }
  } else {
    printf("Error: could not find any output in the effect!\n");
  }

  // TODO add a mechanism to clean up data from js code
  return 0;

  meshEffectDestroy(&instance);
  meshEffectDestroy(&descriptor);

  int status = dlclose(handle);
  if (status != 0) {
    printf("Error while closing plugin: %s (status=%d)\n", dlerror(), status);
  }

  return 0;
}
