#include "openmfx-sdk/c/common/common.h" // for MFX_ENSURE
#include "openmfx-sdk/c/plugin/attribute.h" // for MfxAttributeProperties and mfxPullAttributeProperties

#include <ofxCore.h>
#include <ofxMeshEffect.h>
#include <ofxProperty.h>
#include <ofxParam.h>

#include <string.h>
#include <stdio.h>

/*****************************************/

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

static OfxStatus getDoubleParameterValue(OfxParamSetHandle parameters, const char* name, double *value) {
    OfxParamHandle param;
    MFX_ENSURE(parameterSuite->paramGetHandle(parameters, name, &param, NULL));
    MFX_ENSURE(parameterSuite->paramGetValue(param, value));
    return kOfxStatOK;
}

static OfxStatus cook(OfxMeshEffectHandle instance) {
    OfxMeshInputHandle output;
    MFX_ENSURE(meshEffectSuite->inputGetHandle(instance, kOfxMeshMainOutput, &output, NULL));

    OfxMeshHandle output_mesh;
    OfxPropertySetHandle output_mesh_props;
    OfxTime time = 0.0;
    MFX_ENSURE(meshEffectSuite->inputGetMesh(output, time, &output_mesh, &output_mesh_props));

    OfxParamSetHandle parameters;
    MFX_ENSURE(meshEffectSuite->getParamSet(instance, &parameters));

    double dimensions[3];
    MFX_ENSURE(getDoubleParameterValue(parameters, "width", &dimensions[0]));
    MFX_ENSURE(getDoubleParameterValue(parameters, "height", &dimensions[1]));
    MFX_ENSURE(getDoubleParameterValue(parameters, "depth", &dimensions[2]));

    int output_point_count = 8;
    int output_face_count = 6;
    int output_corner_count = 4 * output_face_count;
    MFX_ENSURE(propertySuite->propSetInt(output_mesh_props, kOfxMeshPropPointCount, 0, output_point_count));
    MFX_ENSURE(propertySuite->propSetInt(output_mesh_props, kOfxMeshPropCornerCount, 0, output_corner_count));
    MFX_ENSURE(propertySuite->propSetInt(output_mesh_props, kOfxMeshPropFaceCount, 0, output_face_count));
    MFX_ENSURE(propertySuite->propSetInt(output_mesh_props, kOfxMeshPropConstantFaceSize, 0, 4));

    OfxPropertySetHandle
        output_point_position_attrib,
        output_corner_point_attrib;

    MFX_ENSURE(meshEffectSuite->meshGetAttribute(output_mesh,
                                                 kOfxMeshAttribPoint,
                                                 kOfxMeshAttribPointPosition,
                                                 &output_point_position_attrib));
    MFX_ENSURE(meshEffectSuite->meshGetAttribute(output_mesh,
                                                 kOfxMeshAttribCorner,
                                                 kOfxMeshAttribCornerPoint,
                                                 &output_corner_point_attrib));

    static int s_corner_point_data[] = {
        0, 2, 3, 1,
        4, 5, 7, 6,
        0, 1, 5, 4,
        1, 3, 7, 5,
        3, 2, 6, 7,
        2, 0, 4, 6
    };
    MFX_ENSURE(propertySuite->propSetInt(output_corner_point_attrib, kOfxMeshAttribPropIsOwner, 0, 0));
    MFX_ENSURE(propertySuite->propSetPointer(output_corner_point_attrib, kOfxMeshAttribPropData, 0, (void*)&s_corner_point_data));
    MFX_ENSURE(propertySuite->propSetInt(output_corner_point_attrib, kOfxMeshAttribPropStride, 0, sizeof(int)));

    MFX_ENSURE(meshEffectSuite->meshAlloc(output_mesh));

    MfxAttributeProperties output_point_position_props;
    MFX_ENSURE(mfxPullAttributeProperties(propertySuite, output_point_position_attrib, &output_point_position_props));
    for (int i = 0 ; i < output_point_count ; ++i) {
        float *P = (float*)(output_point_position_props.data + output_point_position_props.byte_stride * i);
        for (int k = 0 ; k < 3 ; ++k) {
            int sign = (i >> k) % 2;
            P[k] = (sign - 0.5f) * dimensions[k];
        }
    }

    MFX_ENSURE(meshEffectSuite->inputReleaseMesh(output_mesh));
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
    static OfxPlugin plugin[] = {
        {
        /* pluginApi */          kOfxMeshEffectPluginApi,
        /* apiVersion */         kOfxMeshEffectPluginApiVersion,
        /* pluginIdentifier */   "Box",
        /* pluginVersionMajor */ 1,
        /* pluginVersionMinor */ 0,
        /* setHost */            setHost,
        /* mainEntry */          mainEntry
        }
    };
    return plugin;
}
