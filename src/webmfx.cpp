/**
 * This defines the core binding between the OpenMfx host (WebAssembly) and
 * JavaScript client code.
 */

// OpenMfx SDK
extern "C" {
#include <host/types.h>
#include <host/meshEffectSuite.h>
#include <host/propertySuite.h>
#include <host/parameterSuite.h>
#include <host/host.h>
#include <common/common.h> // for MFX_CHECK and MFX_ENSURE
}

// OpenMfx API
#include <ofxCore.h>
#include <ofxMeshEffect.h>
#include <ofxProperty.h>
#include <ofxParam.h>

// Other includes
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <cstdio>
#include <SDL/SDL.h>
#include <dlfcn.h>
#include <cassert>
#include <cstring>

#include <vector>

#define MOVE_ONLY(ClassName) \
  ClassName(const ClassName&) = delete; \
  ClassName& operator=(const ClassName&) = delete; \
  ClassName(ClassName&&) = default; \
  ClassName& operator=(ClassName&&) = default;

//--------------------------------------------------------

OfxHost* getGlobalHost() {
  static OfxHost host;
  host.fetchSuite = &fetchSuite;
  return &host;
}

//--------------------------------------------------------

/**
 * Wrapper around descriptor's parameters to expose them to JS
 */
class Parameter {
public:
  Parameter(const OfxParamStruct *parameter = nullptr);
  MOVE_ONLY(Parameter)

  const char* identifier() const;

private:
  const OfxParamStruct * m_parameter;
};

Parameter::Parameter(const OfxParamStruct *parameter)
  : m_parameter(parameter)
{}

const char* Parameter::identifier() const {
  return m_parameter->name;
}

//--------------------------------------------------------

class Attribute {
public:
  Attribute(OfxMeshAttributePropertySet *attribute = nullptr);
  MOVE_ONLY(Attribute)

  char* attachment() const;
  char* identifier() const;
  int componentCount() const;
  char* type() const;
  void* data() const;

private:
  OfxMeshAttributePropertySet *m_attribute;
};

Attribute::Attribute(OfxMeshAttributePropertySet *attribute)
  : m_attribute(attribute)
{}

char* Attribute::attachment() const {
  return m_attribute->attachment;
}

char* Attribute::identifier() const {
  return m_attribute->name;
}

int Attribute::componentCount() const {
  return m_attribute->component_count;
}

char* Attribute::type() const {
  return m_attribute->type;
}


void* Attribute::data() const {
  return m_attribute ? m_attribute->data : nullptr;
}

//--------------------------------------------------------

class Input {
public:
  Input(const OfxMeshInputStruct* input = nullptr);
  MOVE_ONLY(Input)

  const char* identifier() const;
  const char* label() const;

private:
  const OfxMeshInputStruct* m_input;
};

Input::Input(const OfxMeshInputStruct* input)
  : m_input(input)
{}

const char* Input::identifier() const {
  return m_input->name;
}

const char* Input::label() const {
  return m_input->properties.label;
}

//--------------------------------------------------------

class Mesh {
public:
  Mesh(OfxMeshStruct *mesh = nullptr);
  MOVE_ONLY(Mesh)

  bool isValid() const;
  int pointCount() const;
  int cornerCount() const;
  int faceCount() const;
  int constantFaceSize() const;
  int attributeCount() const;
  Attribute getAttribute(const char *attachment, const char *identifier) const;
  Attribute getAttributeByIndex(int attributeIndex) const;

  /**
   * Load a mesh from a file, in which case this object points to a newly
   * allocated mesh that must be freed by calling unload().
   * If a file was already loaded, it is unloaded automatically.
   */
  OfxStatus loadObj(const char* filename);
  /**
   * unload MUST be called when the mesh has been previously loaded from a mesh
   * and MUST NOT be called otherwise.
   */
  OfxStatus unload();

  OfxMeshStruct* raw() const { return m_mesh; }

private:
  OfxMeshStruct *m_mesh;
  bool m_loaded; // tells whether the mesh has been allocated when loading it from a file
};

Mesh::Mesh(OfxMeshStruct *mesh)
  : m_mesh(mesh)
  , m_loaded(false)
{}

bool Mesh::isValid() const {
  return m_mesh != nullptr;
}

int Mesh::pointCount() const {
  return m_mesh->properties.point_count;
}

int Mesh::cornerCount() const {
  return m_mesh->properties.corner_count;
}

int Mesh::faceCount() const {
  return m_mesh->properties.face_count;
}

int Mesh::constantFaceSize() const {
  return m_mesh->properties.constant_face_size;
}

int Mesh::attributeCount() const {
  int count = 0;
  for (; count < 32 && m_mesh->attributes[count].is_valid ; ++count) {}
  return count;
}

Attribute Mesh::getAttribute(const char *attachment, const char *identifier) const {
  OfxMeshAttributePropertySet *attrib;
  OfxStatus status = meshGetAttribute(m_mesh, attachment, identifier, (OfxPropertySetHandle*)&attrib);
  if (kOfxStatOK == status) {
    return Attribute(attrib);
  } else {
    printf("Error: could not find the attribute %s for attachment %s!\n", identifier, attachment);
    return Attribute();
  }
}

Attribute Mesh::getAttributeByIndex(int attributeIndex) const {
  OfxMeshAttributePropertySet *attrib;
  OfxStatus status = meshGetAttributeByIndex(m_mesh, attributeIndex, (OfxPropertySetHandle*)&attrib);
  if (kOfxStatOK == status) {
    return Attribute(attrib);
  } else {
    printf("Error: could not find the attribute #%d!\n", attributeIndex);
    return Attribute();
  }
}

OfxStatus Mesh::loadObj(const char* filename) {
  if (m_loaded) unload();

  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string err;
  bool triangulate = false;
  const char *mtl_basedir = NULL;
  bool status = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename, mtl_basedir, triangulate);
  if (!status) {
    printf("Error: could not load OBJ file: %s\n", err.c_str());
    return kOfxStatErrFatal;
  }

  m_mesh = new OfxMeshStruct();
  meshInit(m_mesh);

  int& pointCount = m_mesh->properties.point_count;
  int& cornerCount = m_mesh->properties.corner_count;
  int& faceCount = m_mesh->properties.face_count;
  m_mesh->properties.constant_face_size = -1;

  pointCount = (int)attrib.vertices.size();

  cornerCount = 0;
  faceCount = 0;
  for (const auto& sh : shapes) {
    cornerCount += (int)sh.mesh.indices.size();
    faceCount += (int)sh.mesh.num_face_vertices.size();
  }

  // 1. Point Position
  OfxMeshAttributePropertySet *pointPositionAttrib;
  attributeDefine(m_mesh,
                  kOfxMeshAttribPoint,
                  kOfxMeshAttribPointPosition,
                  3,
                  kOfxMeshAttribTypeFloat,
                  nullptr,
                  (OfxPropertySetHandle*)&pointPositionAttrib);
  
  size_t buffsize = attrib.vertices.size() * sizeof(float);
  pointPositionAttrib->data = new char[buffsize];
  memcpy(pointPositionAttrib->data, attrib.vertices.data(), buffsize);
  pointPositionAttrib->byte_stride = 3 * sizeof(float);
  pointPositionAttrib->is_owner = true;

  // 2. Corner Point
  OfxMeshAttributePropertySet *cornerPointAttrib;
  attributeDefine(m_mesh,
                  kOfxMeshAttribCorner,
                  kOfxMeshAttribCornerPoint,
                  1,
                  kOfxMeshAttribTypeInt,
                  nullptr,
                  (OfxPropertySetHandle*)&cornerPointAttrib);
  
  size_t indexSize = sizeof(tinyobj::index_t);
  cornerPointAttrib->data = new char[cornerCount * indexSize];
  cornerPointAttrib->byte_stride = indexSize;
  cornerPointAttrib->is_owner = true;
  size_t offset = 0;
  for (const auto& sh : shapes) {
    memcpy(cornerPointAttrib->data + offset * indexSize, sh.mesh.indices.data(), sh.mesh.indices.size() * indexSize);
    offset += sh.mesh.indices.size();
  }

  // 3. Face Size
  OfxMeshAttributePropertySet *faceSizeAttrib;
  attributeDefine(m_mesh,
                  kOfxMeshAttribFace,
                  kOfxMeshAttribFaceSize,
                  1,
                  kOfxMeshAttribTypeInt,
                  nullptr,
                  (OfxPropertySetHandle*)&faceSizeAttrib);
  
  faceSizeAttrib->data = new char[faceCount * sizeof(int)];
  faceSizeAttrib->byte_stride = sizeof(int);
  faceSizeAttrib->is_owner = true;
  offset = 0;
  for (const auto& sh : shapes) {
    for (unsigned char faceSize : sh.mesh.num_face_vertices) {
      int *ptr = (int*)(faceSizeAttrib->data + offset * sizeof(int));
      *ptr = (int)faceSize; // we convert because at the moment the backend does not support uint8 but in theory it should not be needed
      ++offset;
    }
  }

  m_loaded = true;
  return kOfxStatOK;
}

OfxStatus Mesh::unload() {
  if (!m_loaded) return kOfxStatErrBadHandle;
  meshDestroy(m_mesh);
  delete m_mesh;
  m_mesh = nullptr;
  m_loaded = false;
  return kOfxStatOK;
}

//--------------------------------------------------------

class EffectDescriptor;

class EffectInstance {
public:
  EffectInstance(const EffectDescriptor& descriptor);
  ~EffectInstance();
  MOVE_ONLY(EffectInstance)

  OfxStatus setParameter(const char* identifier, double value);
  OfxStatus cook();

  // Warning: the mesh buffers must remain valid until cook() is called
  OfxStatus setInputMesh(const char *identifier, const Mesh *mesh);
  // Warning: the mesh is no longer valid after calling cook() again
  Mesh getOutputMesh();

private:
  OfxParamStruct* findParameter(const char* identifier);

private:
  const OfxPlugin* m_plugin = nullptr;
  const OfxMeshEffectStruct* m_descriptor;
  OfxMeshEffectStruct m_instance;
};

//--------------------------------------------------------

class EffectDescriptor {
public:
  EffectDescriptor();
  ~EffectDescriptor();
  MOVE_ONLY(EffectDescriptor)

  void setPlugin(OfxPlugin* plugin);
  const char* identifier() const;
  // Load and build descriptor
  OfxStatus load();
  OfxStatus unload();

  int getParameterCount() const;
  Parameter getParameter(int parameterIndex) const;

  int getInputCount() const;
  Input getInput(int inputIndex) const;

  EffectInstance *instantiate() const;

  // For EffectInstance only
  const OfxPlugin* plugin() const { return m_plugin; }
  const OfxMeshEffectStruct* raw() const { return &m_descriptor; }

private:
  const OfxPlugin* m_plugin = nullptr;
  OfxMeshEffectStruct m_descriptor;
  bool m_loaded = false;
};

//--------------------------------------------------------

EffectInstance::EffectInstance(const EffectDescriptor& descriptor)
  : m_descriptor(descriptor.raw())
  , m_plugin(descriptor.plugin())
{
  meshEffectCopy(&m_instance, m_descriptor);
}

EffectInstance::~EffectInstance() {
  meshEffectDestroy(&m_instance);
}

OfxStatus EffectInstance::setParameter(const char* identifier, double value) {
  OfxParamStruct *param = findParameter(identifier);
  if (nullptr == param) return kOfxStatErrBadHandle;
  param->values[0].as_double = value;
  return kOfxStatOK;
}

OfxStatus EffectInstance::cook() {
  // Clear previous output
  for (int i = 0 ; i < 16 && m_instance.inputs[i].is_valid ; ++i) {
    OfxMeshInputStruct *input = &m_instance.inputs[i];
    if (0 == strcmp(input->name, kOfxMeshMainOutput)) {
      meshDestroy(&input->mesh);
      meshInit(&input->mesh);
    }
  }

  MFX_ENSURE(m_plugin->mainEntry(kOfxMeshEffectActionCook, &m_instance, NULL, NULL));
  return kOfxStatOK;
}

OfxStatus EffectInstance::setInputMesh(const char *identifier, const Mesh *mesh) {
  for (int i = 0 ; i < 16 && m_instance.inputs[i].is_valid ; ++i) {
    OfxMeshInputStruct *input = &m_instance.inputs[i];
    if (0 == strcmp(input->name, identifier)) {
      meshShallowCopy(&input->mesh, mesh->raw());
      return kOfxStatOK;
    }
  }
  return kOfxStatErrBadHandle;
}

Mesh EffectInstance::getOutputMesh() {
  for (int i = 0 ; i < 16 && m_instance.inputs[i].is_valid ; ++i) {
    OfxMeshInputStruct *input = &m_instance.inputs[i];
    if (0 == strcmp(input->name, kOfxMeshMainOutput)) {
      return Mesh(&input->mesh);
    }
  }
  return Mesh();
}

OfxParamStruct* EffectInstance::findParameter(const char* identifier) {
  auto& parameters = m_instance.parameters.entries;
  for (int i = 0 ; i < 16 && parameters[i].is_valid ; ++i) {
    OfxParamStruct* param = &parameters[i];
    if (0 == strncmp(identifier, param->name, 64)) {
      return param;
    }
  }
  return nullptr;
}

//--------------------------------------------------------

EffectDescriptor::EffectDescriptor() {}

EffectDescriptor::~EffectDescriptor() {
  unload();
}

void EffectDescriptor::setPlugin(OfxPlugin* plugin) {
  m_plugin = plugin;
}

const char* EffectDescriptor::identifier() const {
  return m_plugin->pluginIdentifier;
}

OfxStatus EffectDescriptor::load() {
  assert(m_plugin);
  m_plugin->setHost(getGlobalHost());
  MFX_ENSURE(m_plugin->mainEntry(kOfxActionLoad, NULL, NULL, NULL));
  meshEffectInit(&m_descriptor);
  MFX_ENSURE(m_plugin->mainEntry(kOfxActionDescribe, &m_descriptor, NULL, NULL));
  m_loaded = true;
  return kOfxStatOK;
}

OfxStatus EffectDescriptor::unload() {
  if (!m_loaded) return kOfxStatOK;
  meshEffectDestroy(&m_descriptor);
  MFX_ENSURE(m_plugin->mainEntry(kOfxActionUnload, NULL, NULL, NULL));
  m_loaded = false;
  return kOfxStatOK;
}

int EffectDescriptor::getParameterCount() const {
  assert(m_loaded);
  int count = 0;
  for (int i = 0 ; i < 16 && m_descriptor.parameters.entries[i].is_valid ; ++i) {
    ++count;
  }
  return count;
}

Parameter EffectDescriptor::getParameter(int parameterIndex) const {
  return Parameter(&m_descriptor.parameters.entries[parameterIndex]);
}

int EffectDescriptor::getInputCount() const {
  assert(m_loaded);
  int count = 0;
  for (int i = 0 ; i < 16 && m_descriptor.inputs[i].is_valid ; ++i) {
    ++count;
  }
  return count;
}

Input EffectDescriptor::getInput(int inputIndex) const {
  return Input(&m_descriptor.inputs[inputIndex]);
}

EffectInstance* EffectDescriptor::instantiate() const {
  return new EffectInstance(*this);
}

//--------------------------------------------------------

class EffectLibrary {
public:
  EffectLibrary();
  ~EffectLibrary();
  MOVE_ONLY(EffectLibrary)

  bool load(const char* pluginFilename);
  void unload();
  int getEffectCount() const;
  const EffectDescriptor* getEffectDescriptor(int effectIndex) const;

private:
  void* m_handle;
  std::vector<EffectDescriptor> m_effectDescriptors;
};

#include <binding.cpp>

EffectLibrary::EffectLibrary() {}

EffectLibrary::~EffectLibrary() {
  unload();
}

bool EffectLibrary::load(const char* pluginFilename) {
  if (nullptr != m_handle) {
    unload();
  }

  m_handle = dlopen(pluginFilename, RTLD_LAZY);

  if (nullptr == m_handle) {
    printf("Error while loading plugin: %s\n", dlerror());
    return false;
  }

  int (*OfxGetNumberOfPlugins)(void) = (int(*)(void))dlsym(m_handle, "OfxGetNumberOfPlugins");
  if (nullptr == OfxGetNumberOfPlugins) {
    printf("Error while loading symbol 'OfxGetNumberOfPlugins': %s\n", dlerror());
    unload();
    return false;
  }
  OfxPlugin*(*OfxGetPlugin)(int) = (OfxPlugin*(*)(int))dlsym(m_handle, "OfxGetPlugin");
  if (nullptr == OfxGetPlugin) {
    printf("Error while loading symbol 'OfxGetPlugin': %s\n", dlerror());
    unload();
    return false;
  }

  int plugin_count = OfxGetNumberOfPlugins();
  printf("Found %d plugins:\n", plugin_count);
  m_effectDescriptors.resize(plugin_count);
  for (int i = 0 ; i < plugin_count ; ++i) {
    m_effectDescriptors[i].setPlugin(OfxGetPlugin(i));
    printf(" - %s\n", m_effectDescriptors[i].identifier());
  }

  return true;
}

void EffectLibrary::unload() {
  for (auto& effect : m_effectDescriptors) {
    effect.unload();
  }

  if (nullptr != m_handle) {
    int status = dlclose(m_handle);
    m_handle = nullptr;

    if (status != 0) {
      printf("Error while closing plugin: %s (status=%d)\n", dlerror(), status);
    }
  }
}

int EffectLibrary::getEffectCount() const {
  return static_cast<int>(m_effectDescriptors.size());
}

const EffectDescriptor* EffectLibrary::getEffectDescriptor(int effectIndex) const {
  return &m_effectDescriptors[effectIndex];
}

//--------------------------------------------------------

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

/*****************************************************************************/
/* Test */

typedef struct test_return_t {
  int point_count;
  int corner_count;
  int face_count;
  int constant_face_size;
  char *point_position_data;
  char *corner_point_data;
  char *face_size_data;
} test_return_t;

EMSCRIPTEN_KEEPALIVE
extern "C" int test(test_return_t* ret) {
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

  int (*OfxGetNumberOfPlugins)(void) = (int(*)(void))dlsym(handle, "OfxGetNumberOfPlugins");
  if (NULL == OfxGetNumberOfPlugins) {
    printf("Error while loading symbol 'OfxGetNumberOfPlugins': %s\n", dlerror());
  }
  OfxPlugin*(*OfxGetPlugin)(int) = (OfxPlugin*(*)(int))dlsym(handle, "OfxGetPlugin");
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
    OfxStatus status;
    const OfxMeshPropertySet *props = &main_output->mesh.properties;
    printf(" - %d points\n", props->point_count);
    printf(" - %d corners\n", props->corner_count);
    printf(" - %d faces\n", props->face_count);
    printf(" - constant_face_size = %d\n", props->constant_face_size);
    ret->point_count = props->point_count;
    ret->corner_count = props->corner_count;
    ret->face_count = props->face_count;
    ret->constant_face_size = props->constant_face_size;

    OfxMeshAttributePropertySet *point_position_attrib;
    status = meshGetAttribute(&main_output->mesh, kOfxMeshAttribPoint, kOfxMeshAttribPointPosition, (OfxPropertySetHandle*)&point_position_attrib);
    if (kOfxStatOK == status) {
      ret->point_position_data = point_position_attrib->data;
    } else {
      printf("Error: could not find a point position attribute in the output mesh!\n");
    }

    OfxMeshAttributePropertySet *corner_point_attrib;
    status = meshGetAttribute(&main_output->mesh, kOfxMeshAttribCorner, kOfxMeshAttribCornerPoint, (OfxPropertySetHandle*)&corner_point_attrib);
    if (kOfxStatOK == status) {
      ret->corner_point_data = corner_point_attrib->data;
    } else {
      printf("Error: could not find a corner point attribute in the output mesh!\n");
    }

    if (props->constant_face_size == -1) {
      OfxMeshAttributePropertySet *face_size_attrib;
      status = meshGetAttribute(&main_output->mesh, kOfxMeshAttribFace, kOfxMeshAttribFaceSize, (OfxPropertySetHandle*)&face_size_attrib);
      if (kOfxStatOK == status) {
        ret->face_size_data = face_size_attrib->data;
      } else {
        printf("Error: could not find a face size attribute in the output mesh!\n");
      }
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
