#include <ofxCore.h>
#include <ofxMeshEffect.h>
#include <ofxProperty.h>
#include <ofxParam.h>

#include <string.h>

const OfxMeshEffectSuiteV1 *meshEffectSuite;
const OfxPropertySuiteV1 *propertySuite;
const OfxParameterSuiteV1 *parameterSuite;

static void setHost(OfxHost *host) {
	meshEffectSuite = host->fetchSuite(
	    host->host, // host properties, might be useful to the host's internals
	    kOfxMeshEffectSuite, // name of the suite we want to fetch
	    1 // version of the suite
	);
	propertySuite = host->fetchSuite(host->host, kOfxPropertySuite, 1);
	parameterSuite = host->fetchSuite(host->host, kOfxParameterSuite, 1);
}

static OfxStatus load() {
	if (NULL == meshEffectSuite ||
		NULL == propertySuite ||
		NULL == parameterSuite) {
        return kOfxStatErrMissingHostFeature;
    }
    return kOfxStatOK;
}

static OfxStatus unload() {
    return kOfxStatOK;
}

static OfxStatus describe(OfxMeshEffectHandle descriptor) {
	OfxMeshInputHandle output;
	OfxPropertySetHandle outputProperties;
	meshEffectSuite->inputDefine(descriptor, kOfxMeshMainOutput, &output, &outputProperties);
	propertySuite->propSetString(outputProperties, kOfxPropLabel, 0, "Output");

    OfxParamSetHandle parameters;
    meshEffectSuite->getParamSet(descriptor, &parameters);
    parameterSuite->paramDefine(parameters, kOfxParamTypeDouble, "width", NULL);
    parameterSuite->paramDefine(parameters, kOfxParamTypeDouble, "height", NULL);
    parameterSuite->paramDefine(parameters, kOfxParamTypeDouble, "depth", NULL);
    return kOfxStatOK;
}

static OfxStatus createInstance(OfxMeshEffectHandle instance) {
    return kOfxStatOK;
}

static OfxStatus destroyInstance(OfxMeshEffectHandle instance) {
    return kOfxStatReplyDefault;
}

static OfxStatus cook(OfxMeshEffectHandle instance) {
    OfxMeshInputHandle output;
    meshEffectSuite->inputGetHandle(instance, kOfxMeshMainOutput, &output, NULL);

    OfxMeshHandle output_mesh;
    OfxPropertySetHandle output_mesh_props;
    OfxTime time = 0.0;
    meshEffectSuite->inputGetMesh(output, time, &output_mesh, &output_mesh_props);

    int output_point_count = 8;
    int output_face_count = 6;
    int output_corner_count = 4 * output_face_count;
    propertySuite->propSetInt(output_mesh_props, kOfxMeshPropPointCount, 0, output_point_count);
    propertySuite->propSetInt(output_mesh_props, kOfxMeshPropCornerCount, 0, output_corner_count);
    propertySuite->propSetInt(output_mesh_props, kOfxMeshPropFaceCount, 0, output_face_count);

    meshEffectSuite->meshAlloc(output_mesh);

    OfxPropertySetHandle
        output_point_position_attrib,
        output_corner_point_attrib,
        output_face_size_attrib;
    meshEffectSuite->meshGetAttribute(output_mesh, kOfxMeshAttribPoint,
                                      kOfxMeshAttribPointPosition, &output_point_position_attrib);
    meshEffectSuite->meshGetAttribute(output_mesh, kOfxMeshAttribCorner,
                                      kOfxMeshAttribCornerPoint, &output_corner_point_attrib);
    meshEffectSuite->meshGetAttribute(output_mesh, kOfxMeshAttribFace,
                                      kOfxMeshAttribFaceSize, &output_face_size_attrib);

    meshEffectSuite->inputReleaseMesh(output_mesh);
    return kOfxStatReplyDefault;
}

static OfxStatus mainEntry(const char *action,
                           const void *handle,
                           OfxPropertySetHandle inArgs,
                           OfxPropertySetHandle outArgs) {
    if (0 == strcmp(action, kOfxActionLoad)) {
        return load();
    }
    if (0 == strcmp(action, kOfxActionUnload)) {
        return unload();
    }
    if (0 == strcmp(action, kOfxActionDescribe)) {
        return describe((OfxMeshEffectHandle)handle);
    }
    if (0 == strcmp(action, kOfxActionCreateInstance)) {
        return createInstance((OfxMeshEffectHandle)handle);
    }
    if (0 == strcmp(action, kOfxActionDestroyInstance)) {
        return destroyInstance((OfxMeshEffectHandle)handle);
    }
    if (0 == strcmp(action, kOfxMeshEffectActionCook)) {
        return cook((OfxMeshEffectHandle)handle);
    }
    return kOfxStatReplyDefault; // this means "unhandled action"
}



OfxExport int OfxGetNumberOfPlugins(void) {
    return 1;
}

OfxExport OfxPlugin *OfxGetPlugin(int nth) {
    static OfxPlugin plugin = {
        /* pluginApi */          kOfxMeshEffectPluginApi,
        /* apiVersion */         kOfxMeshEffectPluginApiVersion,
        /* pluginIdentifier */   "Box",
        /* pluginVersionMajor */ 1,
        /* pluginVersionMinor */ 0,
        /* setHost */            setHost,
        /* mainEntry */          mainEntry
    };
    return &plugin;
}
