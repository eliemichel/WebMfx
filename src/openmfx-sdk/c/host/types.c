#include "types.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

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
  if (NULL != attrib->data) {
    return kOfxStatErrBadHandle;
  }

  // Don't allocate face size buffer when face size is constant
  if (props->constant_face_size > -1
    && 0 == strcmp(attrib->name, kOfxMeshAttribFaceSize)
    && 0 == strcmp(attrib->attachment, kOfxMeshAttribFace))
  {
    return kOfxStatOK;
  }

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
  mesh->properties.constant_face_size = -1;
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