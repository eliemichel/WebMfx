#include "openmfx-sdk/c/common/common.h" // for MFX_ENSURE
#include "openmfx-sdk/c/plugin/attribute.h" // for MfxAttributeProperties and mfxPullAttributeProperties
#include "openmfx-sdk/c/plugin/mesh.h" // for MfxMeshProperties and mfxPullMeshProperties

#include <ofxCore.h>
#include <ofxMeshEffect.h>
#include <ofxProperty.h>
#include <ofxParam.h>

#include <string.h>
#include <stdio.h>
#include <math.h>

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
    OfxMeshInputHandle input;
    OfxPropertySetHandle inputProperties;
    meshEffectSuite->inputDefine(descriptor, kOfxMeshMainInput, &input, &inputProperties);
    propertySuite->propSetString(inputProperties, kOfxPropLabel, 0, "Input");

	OfxMeshInputHandle output;
	OfxPropertySetHandle outputProperties;
	meshEffectSuite->inputDefine(descriptor, kOfxMeshMainOutput, &output, &outputProperties);
	propertySuite->propSetString(outputProperties, kOfxPropLabel, 0, "Output");
    return kOfxStatOK;
}

static OfxStatus createInstance(OfxMeshEffectHandle instance) {
    return kOfxStatOK;
}

static OfxStatus destroyInstance(OfxMeshEffectHandle instance) {
    return kOfxStatReplyDefault;
}

static void cross_product(const float a[3], const float b[3], float out[3]) {
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

static void copy_v3(const float a[3], float out[3]) {
    out[0] = a[0];
    out[1] = a[1];
    out[2] = a[2];
}

static void add_v3(float inout[3], const float a[3]) {
    inout[0] += a[0];
    inout[1] += a[1];
    inout[2] += a[2];
}

static void sub_v3_v3(const float a[3], const float b[3], float out[3]) {
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1];
    out[2] = a[2] - b[2];
}

static float dot_product(const float a[3], const float b[3]) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static void normalize_v3(float inout[3]) {
    float normalizer = 1.0f / sqrt(dot_product(inout, inout));
    inout[0] *= normalizer;
    inout[1] *= normalizer;
    inout[2] *= normalizer;
}

static OfxStatus cook(OfxMeshEffectHandle instance) {
    printf("DEBUG DEBUG cook\n");
    OfxMeshInputHandle input;
    MFX_ENSURE(meshEffectSuite->inputGetHandle(instance, kOfxMeshMainInput, &input, NULL));

    OfxMeshInputHandle output;
    MFX_ENSURE(meshEffectSuite->inputGetHandle(instance, kOfxMeshMainOutput, &output, NULL));

    OfxMeshHandle input_mesh;
    OfxTime time = 0.0;
    MFX_ENSURE(meshEffectSuite->inputGetMesh(input, time, &input_mesh, NULL));

    OfxMeshHandle output_mesh;
    MFX_ENSURE(meshEffectSuite->inputGetMesh(output, time, &output_mesh, NULL));

    MfxMeshProperties input_mesh_props;
    MFX_ENSURE(mfxPullMeshProperties(propertySuite, meshEffectSuite, input_mesh, &input_mesh_props));

    MfxMeshProperties output_mesh_props;
    output_mesh_props.point_count = input_mesh_props.point_count;
    output_mesh_props.corner_count = input_mesh_props.corner_count;
    output_mesh_props.face_count = input_mesh_props.face_count;
    output_mesh_props.constant_face_size = input_mesh_props.constant_face_size;
    MFX_ENSURE(mfxPushMeshProperties(propertySuite, meshEffectSuite, output_mesh, &output_mesh_props));

    // 1. Forward attributes

    OfxPropertySetHandle
        input_point_position_attrib,
        input_corner_point_attrib,
        input_face_size_attrib,
        output_point_position_attrib,
        output_corner_point_attrib,
        output_face_size_attrib;

    MFX_ENSURE(meshEffectSuite->meshGetAttribute(input_mesh,
                                                 kOfxMeshAttribPoint,
                                                 kOfxMeshAttribPointPosition,
                                                 &input_point_position_attrib));
    MFX_ENSURE(meshEffectSuite->meshGetAttribute(output_mesh,
                                                 kOfxMeshAttribPoint,
                                                 kOfxMeshAttribPointPosition,
                                                 &output_point_position_attrib));

    MfxAttributeProperties input_point_position_props;
    MFX_ENSURE(mfxPullAttributeProperties(propertySuite, input_point_position_attrib, &input_point_position_props));
    MfxAttributeProperties output_point_position_props;
    memcpy(&output_point_position_props, &input_point_position_props, sizeof(MfxAttributeProperties));
    output_point_position_props.is_owner = 0;
    printf("DEBUG DEBUG output_point_position_props.data = %p\n", output_point_position_props.data);
    MFX_ENSURE(mfxPushAttributeProperties(propertySuite, output_point_position_attrib, &output_point_position_props));

    
    MFX_ENSURE(meshEffectSuite->meshGetAttribute(input_mesh,
                                                 kOfxMeshAttribCorner,
                                                 kOfxMeshAttribCornerPoint,
                                                 &input_corner_point_attrib));
    MFX_ENSURE(meshEffectSuite->meshGetAttribute(output_mesh,
                                                 kOfxMeshAttribCorner,
                                                 kOfxMeshAttribCornerPoint,
                                                 &output_corner_point_attrib));

    MfxAttributeProperties input_corner_point_props;
    MFX_ENSURE(mfxPullAttributeProperties(propertySuite, input_corner_point_attrib, &input_corner_point_props));
    MfxAttributeProperties output_corner_point_props;
    memcpy(&output_corner_point_props, &input_corner_point_props, sizeof(MfxAttributeProperties));
    output_corner_point_props.is_owner = 0;
    MFX_ENSURE(mfxPushAttributeProperties(propertySuite, output_corner_point_attrib, &output_corner_point_props));

    MFX_ENSURE(meshEffectSuite->meshGetAttribute(input_mesh,
                                                 kOfxMeshAttribFace,
                                                 kOfxMeshAttribFaceSize,
                                                 &input_face_size_attrib));
    MFX_ENSURE(meshEffectSuite->meshGetAttribute(output_mesh,
                                                 kOfxMeshAttribFace,
                                                 kOfxMeshAttribFaceSize,
                                                 &output_face_size_attrib));

    MfxAttributeProperties input_face_size_props;
    MFX_ENSURE(mfxPullAttributeProperties(propertySuite, input_face_size_attrib, &input_face_size_props));
    MfxAttributeProperties output_face_size_props;
    memcpy(&output_face_size_props, &input_face_size_props, sizeof(MfxAttributeProperties));
    output_face_size_props.is_owner = 0;
    MFX_ENSURE(mfxPushAttributeProperties(propertySuite, output_face_size_attrib, &output_face_size_props));

    // 2. Add extra attributes

    OfxPropertySetHandle output_face_normal_attrib;
    MFX_ENSURE(meshEffectSuite->attributeDefine(output_mesh,
                              kOfxMeshAttribFace,
                              "normal",
                              3,
                              kOfxMeshAttribTypeFloat,
                              kOfxMeshAttribSemanticNormal,
                              &output_face_normal_attrib));

    // 3. Allocate attributes

    MFX_ENSURE(meshEffectSuite->meshAlloc(output_mesh));

    // 4. Fill attribute data

    MFX_ENSURE(mfxPullAttributeProperties(propertySuite, output_point_position_attrib, &output_point_position_props));
    MFX_ENSURE(mfxPullAttributeProperties(propertySuite, output_corner_point_attrib, &output_corner_point_props));
    MFX_ENSURE(mfxPullAttributeProperties(propertySuite, output_face_size_attrib, &output_face_size_props));
    if (output_mesh_props.constant_face_size > -1) {
        output_face_size_props.data = &output_mesh_props.constant_face_size;
        output_face_size_props.byte_stride = 0;
    }

    MfxAttributeProperties output_face_normal_props;
    MFX_ENSURE(mfxPullAttributeProperties(propertySuite, output_face_normal_attrib, &output_face_normal_props));

    float barycenter[3];
    float normal[3];
    float N[3];
    float A[3];
    float B[3];
    int corner = 0, size = 0;
    for (int face = 0 ; face < output_mesh_props.face_count ; ++face, corner += size) {
        size = *(int*)(output_face_size_props.data + output_face_size_props.byte_stride * face);
        float normalizer = 1.0f / size;
        float *prev_P;

        barycenter[0] = barycenter[1] = barycenter[2] = 0.0f;
        for (int i = 0 ; i < size ; ++i) {
            int point = *(int*)(output_corner_point_props.data + output_corner_point_props.byte_stride * (corner + i));
            float *P = (float*)(output_point_position_props.data + output_point_position_props.byte_stride * point);
            add_v3(barycenter, P);
            prev_P = P;
        }
        for (int k = 0 ; k < 3 ; ++k) {
            barycenter[k] *= normalizer;
        }

        normal[0] = normal[1] = normal[2] = 0.0f;
        for (int i = 0 ; i < size ; ++i) {
            int point = *(int*)(output_corner_point_props.data + output_corner_point_props.byte_stride * (corner + i));
            float *P = (float*)(output_point_position_props.data + output_point_position_props.byte_stride * point);
            sub_v3_v3(prev_P, barycenter, A);
            sub_v3_v3(P, barycenter, B);
            cross_product(A, B, N);
            normalize_v3(N);
            add_v3(normal, N);
            prev_P = P;
        }
        normalize_v3(normal);

        float *faceNormal = (float*)(output_face_normal_props.data + output_face_normal_props.byte_stride * face);
        copy_v3(normal, faceNormal);
    }

    MFX_ENSURE(meshEffectSuite->inputReleaseMesh(input_mesh));
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
        /* pluginIdentifier */   "ComputeNormals",
        /* pluginVersionMajor */ 1,
        /* pluginVersionMinor */ 0,
        /* setHost */            setHost,
        /* mainEntry */          mainEntry
        }
    };
    return plugin;
}
