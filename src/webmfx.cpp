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
#include <stdio.h>
#include <SDL/SDL.h>
#include <dlfcn.h>
#include <assert.h>

#include <vector>

class NonCopyable {
public:
  NonCopyable() {}
  NonCopyable(const NonCopyable&) = delete;
  NonCopyable& operator=(const NonCopyable&) = delete;
  NonCopyable(NonCopyable&&) = default;
  NonCopyable& operator=(NonCopyable&&) = default;
};

class EffectLibrary : public NonCopyable {
public:
  ~EffectLibrary();
  bool load(const char* plugin_filename);
  void unload();
  int getEffectCount();

private:
  void* m_handle;
  std::vector<OfxPlugin*> m_effects;
};

#include <binding.cpp>

EffectLibrary::~EffectLibrary() {
  unload();
}

bool EffectLibrary::load(const char* plugin_filename) {
  if (nullptr != m_handle) {
    unload();
  }

  m_handle = dlopen(plugin_filename, RTLD_LAZY);

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
  m_effects.resize(plugin_count);
  for (int i = 0 ; i < plugin_count ; ++i) {
    m_effects[i] = OfxGetPlugin(i);
    printf(" - %s\n", m_effects[i]->pluginIdentifier);
  }

  return true;
}

void EffectLibrary::unload() {
  if (nullptr != m_handle) {
    int status = dlclose(m_handle);
    m_handle = nullptr;

    if (status != 0) {
      printf("Error while closing plugin: %s (status=%d)\n", dlerror(), status);
    }
  }
}

int EffectLibrary::getEffectCount() {
  return static_cast<int>(m_effects.size());
}

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
