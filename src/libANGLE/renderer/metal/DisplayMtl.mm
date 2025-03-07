//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// DisplayMtl.mm: Metal implementation of DisplayImpl

#include "libANGLE/renderer/metal/DisplayMtl.h"

#include "common/system_utils.h"
#include "gpu_info_util/SystemInfo.h"
#include "libANGLE/Context.h"
#include "libANGLE/Display.h"
#include "libANGLE/Surface.h"
#include "libANGLE/renderer/driver_utils.h"
#include "libANGLE/renderer/glslang_wrapper_utils.h"
#include "libANGLE/renderer/metal/ContextMtl.h"
#include "libANGLE/renderer/metal/DeviceMtl.h"
#include "libANGLE/renderer/metal/IOSurfaceSurfaceMtl.h"
#include "libANGLE/renderer/metal/ImageMtl.h"
#include "libANGLE/renderer/metal/SurfaceMtl.h"
#include "libANGLE/renderer/metal/SyncMtl.h"
#include "libANGLE/renderer/metal/mtl_common.h"
#include "libANGLE/renderer/metal/shaders/mtl_default_shaders_src_autogen.inc"
#include "libANGLE/trace.h"
#include "platform/Platform.h"

#include "EGL/eglext.h"

#if defined(ANGLE_PLATFORM_MACOS) || defined(ANGLE_PLATFORM_MACCATALYST)
constexpr char kANGLEPreferredDeviceEnv[] = "ANGLE_PREFERRED_DEVICE";
#endif

namespace rx
{

static EGLint GetDepthSize(GLint internalformat)
{
    switch (internalformat)
    {
        case GL_STENCIL_INDEX8:
            return 0;
        case GL_DEPTH_COMPONENT16:
            return 16;
        case GL_DEPTH_COMPONENT24:
            return 24;
        case GL_DEPTH_COMPONENT32_OES:
            return 32;
        case GL_DEPTH_COMPONENT32F:
            return 32;
        case GL_DEPTH24_STENCIL8:
            return 24;
        case GL_DEPTH32F_STENCIL8:
            return 32;
        default:
            //    UNREACHABLE(internalformat);
            return 0;
    }
}

static EGLint GetStencilSize(GLint internalformat)
{
    switch (internalformat)
    {
        case GL_STENCIL_INDEX8:
            return 8;
        case GL_DEPTH_COMPONENT16:
            return 0;
        case GL_DEPTH_COMPONENT24:
            return 0;
        case GL_DEPTH_COMPONENT32_OES:
            return 0;
        case GL_DEPTH_COMPONENT32F:
            return 0;
        case GL_DEPTH24_STENCIL8:
            return 8;
        case GL_DEPTH32F_STENCIL8:
            return 8;
        default:
            //    UNREACHABLE(internalformat);
            return 0;
    }
}

bool IsMetalDisplayAvailable()
{
    // We only support macos 10.13+ and 11 for now. Since they are requirements for Metal 2.0.
#if TARGET_OS_SIMULATOR
    if (ANGLE_APPLE_AVAILABLE_XCI(10.13, 13.0, 13))
#else
    if (ANGLE_APPLE_AVAILABLE_XCI(10.13, 13.0, 11))
#endif
    {
        return true;
    }
    return false;
}

DisplayImpl *CreateMetalDisplay(const egl::DisplayState &state)
{
    return new DisplayMtl(state);
}

struct DefaultShaderAsyncInfoMtl
{
    mtl::AutoObjCPtr<id<MTLLibrary>> defaultShaders;
    mtl::AutoObjCPtr<NSError *> defaultShadersCompileError;

    // Synchronization primitives for compiling default shaders in back-ground
    std::condition_variable cv;
    std::mutex lock;

    bool compiled = false;
};

DisplayMtl::DisplayMtl(const egl::DisplayState &state)
    : DisplayImpl(state), mStateCache(mFeatures), mUtils(this)
{}

DisplayMtl::~DisplayMtl() {}

egl::Error DisplayMtl::initialize(egl::Display *display)
{
    ASSERT(IsMetalDisplayAvailable());

    angle::Result result = initializeImpl(display);
    if (result != angle::Result::Continue)
    {
        return egl::EglNotInitialized();
    }
    return egl::NoError();
}

angle::Result DisplayMtl::initializeImpl(egl::Display *display)
{
    ANGLE_MTL_OBJC_SCOPE
    {
        mMetalDevice =
            [getMetalDeviceMatchingAttribute(display->getAttributeMap()) ANGLE_MTL_AUTORELEASE];
        // If we can't create a device, fail initialization.
        if (!mMetalDevice.get())
        {
            return angle::Result::Stop;
        }

        mMetalDeviceVendorId = mtl::GetDeviceVendorId(mMetalDevice);

        mCmdQueue.set([[mMetalDevice.get() newCommandQueue] ANGLE_MTL_AUTORELEASE]);

        mCapsInitialized = false;
#if ANGLE_ENABLE_METAL_SPIRV
        ANGLE_TRACE_EVENT0("gpu.angle,startup", "GlslangWarmup");
        sh::InitializeGlslang();
#endif

        if (!mState.featuresAllDisabled)
        {
            initializeFeatures();
        }

        ANGLE_TRY(mFormatTable.initialize(this));
        ANGLE_TRY(initializeShaderLibrary());

        return mUtils.initialize();
    }
}

void DisplayMtl::terminate()
{
    mUtils.onDestroy();
    mCmdQueue.reset();
    mDefaultShadersAsyncInfo = nullptr;
    mMetalDevice             = nil;
#if ANGLE_MTL_EVENT_AVAILABLE
    mSharedEventListener = nil;
#endif
    mCapsInitialized = false;

    mMetalDeviceVendorId = 0;
#if ANGLE_ENABLE_METAL_SPIRV
    sh::FinalizeGlslang();
#endif
}

bool DisplayMtl::testDeviceLost()
{
    return false;
}

egl::Error DisplayMtl::restoreLostDevice(const egl::Display *display)
{
    return egl::NoError();
}

std::string DisplayMtl::getRendererDescription()
{
    ANGLE_MTL_OBJC_SCOPE
    {
        std::string desc = "ANGLE Metal Renderer";

        if (mMetalDevice)
        {
            desc += ": ";
            desc += mMetalDevice.get().name.UTF8String;
        }

        return desc;
    }
}

std::string DisplayMtl::getVendorString()
{
    return GetVendorString(mMetalDeviceVendorId);
}

std::string DisplayMtl::getVersionString()
{
    ANGLE_MTL_OBJC_SCOPE
    {
        NSProcessInfo *procInfo = [NSProcessInfo processInfo];
        return procInfo.operatingSystemVersionString.UTF8String;
    }
}

DeviceImpl *DisplayMtl::createDevice()
{
    return new DeviceMtl();
}

id<MTLDevice> DisplayMtl::getMetalDeviceMatchingAttribute(const egl::AttributeMap &attribs)
{
#if defined(ANGLE_PLATFORM_MACOS) || defined(ANGLE_PLATFORM_MACCATALYST)
    const std::string anglePreferredDevice = angle::GetEnvironmentVar(kANGLEPreferredDeviceEnv);
    NSArray<id<MTLDevice>> *deviceList     = MTLCopyAllDevices();
    if (anglePreferredDevice != "")
    {
        for (id<MTLDevice> device in deviceList)
        {
            if ([device.name.lowercaseString
                    containsString:[NSString stringWithUTF8String:anglePreferredDevice.c_str()]
                                       .lowercaseString])
            {
                NSLog(@"Using Metal Device: %@", [device name]);
                return device;
            }
        }
    }

    // TODO(anglebug.com/6143): reintroduce Apple's GPU selection code
    // under a run-time check; don't perform it on macOS versions
    // earlier than 10.15. Respecify EGL_ANGLE_power_preference to
    // allow it to be used at display creation time rather than
    // context creation time.
#endif
    // If we can't find anything, or are on a platform that doesn't support power options, create a
    // default device.
    return MTLCreateSystemDefaultDevice();
}

egl::Error DisplayMtl::waitClient(const gl::Context *context)
{
    auto contextMtl      = GetImplAs<ContextMtl>(context);
    angle::Result result = contextMtl->finishCommandBuffer();

    if (result != angle::Result::Continue)
    {
        return egl::EglBadAccess();
    }
    return egl::NoError();
}

egl::Error DisplayMtl::waitNative(const gl::Context *context, EGLint engine)
{
    UNIMPLEMENTED();
    return egl::NoError();
}

SurfaceImpl *DisplayMtl::createWindowSurface(const egl::SurfaceState &state,
                                             EGLNativeWindowType window,
                                             const egl::AttributeMap &attribs)
{
    return new WindowSurfaceMtl(this, state, window, attribs);
}

SurfaceImpl *DisplayMtl::createPbufferSurface(const egl::SurfaceState &state,
                                              const egl::AttributeMap &attribs)
{
    return new PBufferSurfaceMtl(this, state, attribs);
}

SurfaceImpl *DisplayMtl::createPbufferFromClientBuffer(const egl::SurfaceState &state,
                                                       EGLenum buftype,
                                                       EGLClientBuffer clientBuffer,
                                                       const egl::AttributeMap &attribs)
{
    switch (buftype)
    {
        case EGL_IOSURFACE_ANGLE:
            return new IOSurfaceSurfaceMtl(this, state, clientBuffer, attribs);
        default:
            UNREACHABLE();
    }
    return nullptr;
}

SurfaceImpl *DisplayMtl::createPixmapSurface(const egl::SurfaceState &state,
                                             NativePixmapType nativePixmap,
                                             const egl::AttributeMap &attribs)
{
    UNIMPLEMENTED();
    return static_cast<SurfaceImpl *>(0);
}

ImageImpl *DisplayMtl::createImage(const egl::ImageState &state,
                                   const gl::Context *context,
                                   EGLenum target,
                                   const egl::AttributeMap &attribs)
{
    return new ImageMtl(state, context);
}

rx::ContextImpl *DisplayMtl::createContext(const gl::State &state,
                                           gl::ErrorSet *errorSet,
                                           const egl::Config *configuration,
                                           const gl::Context *shareContext,
                                           const egl::AttributeMap &attribs)
{
    return new ContextMtl(state, errorSet, this);
}

StreamProducerImpl *DisplayMtl::createStreamProducerD3DTexture(
    egl::Stream::ConsumerType consumerType,
    const egl::AttributeMap &attribs)
{
    UNIMPLEMENTED();
    return nullptr;
}

ShareGroupImpl *DisplayMtl::createShareGroup()
{
    return new ShareGroupMtl();
}

ExternalImageSiblingImpl *DisplayMtl::createExternalImageSibling(const gl::Context *context,
                                                                 EGLenum target,
                                                                 EGLClientBuffer buffer,
                                                                 const egl::AttributeMap &attribs)
{
    switch (target)
    {
        case EGL_METAL_TEXTURE_ANGLE:
            return new TextureImageSiblingMtl(buffer);

        default:
            UNREACHABLE();
            return nullptr;
    }
}

gl::Version DisplayMtl::getMaxSupportedESVersion() const
{
    // NOTE(hqle): Supports GLES 3.0 on iOS GPU Family 4+ for now.
#if TARGET_OS_SIMULATOR  // Simulator should be able to support ES3, despite not supporting iOS GPU
                         // Family 4 in its entirety.
    return gl::Version(3, 0);
#else
    if (supportsEitherGPUFamily(4, 1))
    {
        return mtl::kMaxSupportedGLVersion;
    }
    return gl::Version(2, 0);
#endif
}

gl::Version DisplayMtl::getMaxConformantESVersion() const
{
    return std::min(getMaxSupportedESVersion(), gl::Version(2, 0));
}

EGLSyncImpl *DisplayMtl::createSync(const egl::AttributeMap &attribs)
{
    return new EGLSyncMtl(attribs);
}

egl::Error DisplayMtl::makeCurrent(egl::Display *display,
                                   egl::Surface *drawSurface,
                                   egl::Surface *readSurface,
                                   gl::Context *context)
{
    if (!context)
    {
        return egl::NoError();
    }

    return egl::NoError();
}

void DisplayMtl::generateExtensions(egl::DisplayExtensions *outExtensions) const
{
    outExtensions->flexibleSurfaceCompatibility = true;
    outExtensions->iosurfaceClientBuffer        = true;
    outExtensions->surfacelessContext           = true;
    outExtensions->displayTextureShareGroup     = true;
    outExtensions->displaySemaphoreShareGroup   = true;
    outExtensions->mtlTextureClientBuffer       = true;

    if (mFeatures.hasEvents.enabled)
    {
        // MTLSharedEvent is only available since Metal 2.1
        outExtensions->fenceSync = true;
        outExtensions->waitSync  = true;
    }

    // Note that robust resource initialization is not yet implemented. We only expose
    // this extension so that ANGLE can be initialized in Chrome. WebGL will fail to use
    // this extension (anglebug.com/4929)
    outExtensions->robustResourceInitialization = true;
    outExtensions->powerPreference              = true;

    // EGL_KHR_image
    outExtensions->image     = true;
    outExtensions->imageBase = true;
}

void DisplayMtl::generateCaps(egl::Caps *outCaps) const {}

void DisplayMtl::populateFeatureList(angle::FeatureList *features)
{
    mFeatures.populateFeatureList(features);
}

egl::ConfigSet DisplayMtl::generateConfigs()
{
    // NOTE(hqle): generate more config permutations
    egl::ConfigSet configs;

    const gl::Version &maxVersion = getMaxSupportedESVersion();
    ASSERT(maxVersion >= gl::Version(2, 0));
    bool supportsES3 = maxVersion >= gl::Version(3, 0);

    egl::Config config;

    // Native stuff
    config.nativeVisualID   = 0;
    config.nativeVisualType = 0;
    config.nativeRenderable = EGL_TRUE;

    config.colorBufferType = EGL_RGB_BUFFER;
    config.luminanceSize   = 0;
    config.alphaMaskSize   = 0;

    config.transparentType = EGL_NONE;

    // Pbuffer
    config.bindToTextureTarget = EGL_TEXTURE_2D;
    config.maxPBufferWidth     = 4096;
    config.maxPBufferHeight    = 4096;
    config.maxPBufferPixels    = 4096 * 4096;

    // Caveat
    config.configCaveat = EGL_NONE;

    // Misc
    config.sampleBuffers     = 0;
    config.samples           = 0;
    config.level             = 0;
    config.bindToTextureRGB  = EGL_FALSE;
    config.bindToTextureRGBA = EGL_TRUE;

    config.surfaceType = EGL_WINDOW_BIT | EGL_PBUFFER_BIT;

#if TARGET_OS_OSX || TARGET_OS_MACCATALYST
    config.minSwapInterval = 0;
    config.maxSwapInterval = 1;
#else
    config.minSwapInterval = 1;
    config.maxSwapInterval = 1;
#endif

    config.renderTargetFormat = GL_RGBA8;
    config.depthStencilFormat = GL_DEPTH24_STENCIL8;

    config.conformant     = EGL_OPENGL_ES2_BIT | (supportsES3 ? EGL_OPENGL_ES3_BIT_KHR : 0);
    config.renderableType = config.conformant;

    config.matchNativePixmap = EGL_NONE;

    config.colorComponentType = EGL_COLOR_COMPONENT_TYPE_FIXED_EXT;

    constexpr int samplesSupported[] = {0, 4};

    for (int samples : samplesSupported)
    {
        config.samples       = samples;
        config.sampleBuffers = (samples == 0) ? 0 : 1;

        // Buffer sizes
        config.redSize    = 8;
        config.greenSize  = 8;
        config.blueSize   = 8;
        config.alphaSize  = 8;
        config.bufferSize = config.redSize + config.greenSize + config.blueSize + config.alphaSize;

        // With DS
        config.depthSize   = 24;
        config.stencilSize = 8;

        configs.add(config);

        // With D
        config.depthSize   = 24;
        config.stencilSize = 0;
        configs.add(config);

        // With S
        config.depthSize   = 0;
        config.stencilSize = 8;
        configs.add(config);

        // Tests like dEQP-GLES2.functional.depth_range.* assume EGL_DEPTH_SIZE is properly set even
        // if renderConfig attributes are set to glu::RenderConfig::DONT_CARE
        config.depthSize   = GetDepthSize(config.depthStencilFormat);
        config.stencilSize = GetStencilSize(config.depthStencilFormat);
        configs.add(config);
    }

    return configs;
}

bool DisplayMtl::isValidNativeWindow(EGLNativeWindowType window) const
{
    ANGLE_MTL_OBJC_SCOPE
    {
        NSObject *layer = (__bridge NSObject *)(window);
        return [layer isKindOfClass:[CALayer class]];
    }
}

egl::Error DisplayMtl::validateClientBuffer(const egl::Config *configuration,
                                            EGLenum buftype,
                                            EGLClientBuffer clientBuffer,
                                            const egl::AttributeMap &attribs) const
{
    switch (buftype)
    {
        case EGL_IOSURFACE_ANGLE:
            if (!IOSurfaceSurfaceMtl::ValidateAttributes(clientBuffer, attribs))
            {
                return egl::EglBadAttribute();
            }
            break;
        default:
            UNREACHABLE();
            return egl::EglBadAttribute();
    }
    return egl::NoError();
}

egl::Error DisplayMtl::validateImageClientBuffer(const gl::Context *context,
                                                 EGLenum target,
                                                 EGLClientBuffer clientBuffer,
                                                 const egl::AttributeMap &attribs) const
{
    switch (target)
    {
        case EGL_METAL_TEXTURE_ANGLE:
            if (!TextureImageSiblingMtl::ValidateClientBuffer(this, clientBuffer))
            {
                return egl::EglBadAttribute();
            }
            break;
        default:
            UNREACHABLE();
            return egl::EglBadAttribute();
    }
    return egl::NoError();
}

gl::Caps DisplayMtl::getNativeCaps() const
{
    ensureCapsInitialized();
    return mNativeCaps;
}
const gl::TextureCapsMap &DisplayMtl::getNativeTextureCaps() const
{
    ensureCapsInitialized();
    return mNativeTextureCaps;
}
const gl::Extensions &DisplayMtl::getNativeExtensions() const
{
    ensureCapsInitialized();
    return mNativeExtensions;
}

const gl::Limitations &DisplayMtl::getNativeLimitations() const
{
    ensureCapsInitialized();
    return mNativeLimitations;
}

void DisplayMtl::ensureCapsInitialized() const
{
    if (mCapsInitialized)
    {
        return;
    }

    mCapsInitialized = true;

    // Reset
    mNativeCaps = gl::Caps();

    // Fill extension and texture caps
    initializeExtensions();
    initializeTextureCaps();

    // https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf
    mNativeCaps.maxElementIndex  = std::numeric_limits<GLuint>::max() - 1;
    mNativeCaps.max3DTextureSize = 2048;
#if TARGET_OS_OSX || TARGET_OS_MACCATALYST
    mNativeCaps.max2DTextureSize = 16384;
    // On macOS exclude [[position]] from maxVaryingVectors.
    mNativeCaps.maxVaryingVectors         = 31 - 1;
    mNativeCaps.maxVertexOutputComponents = mNativeCaps.maxFragmentInputComponents = 124 - 4;
#else
    if (supportsAppleGPUFamily(3))
    {
        mNativeCaps.max2DTextureSize          = 16384;
        mNativeCaps.maxVertexOutputComponents = mNativeCaps.maxFragmentInputComponents = 124;
        mNativeCaps.maxVaryingVectors = mNativeCaps.maxVertexOutputComponents / 4;
    }
    else
    {
        mNativeCaps.max2DTextureSize          = 8192;
        mNativeCaps.maxVertexOutputComponents = mNativeCaps.maxFragmentInputComponents = 60;
        mNativeCaps.maxVaryingVectors = mNativeCaps.maxVertexOutputComponents / 4;
    }
#endif

    mNativeCaps.maxArrayTextureLayers = 2048;
    mNativeCaps.maxLODBias            = 2.0;  // default GLES3 limit
    mNativeCaps.maxCubeMapTextureSize = mNativeCaps.max2DTextureSize;
    mNativeCaps.maxRenderbufferSize   = mNativeCaps.max2DTextureSize;
    mNativeCaps.minAliasedPointSize   = 1;
    // NOTE(hqle): Metal has some problems drawing big point size even though
    // Metal-Feature-Set-Tables.pdf says that max supported point size is 511. We limit it to 64
    // for now. http://anglebug.com/4816
    mNativeCaps.maxAliasedPointSize = 64;

    mNativeCaps.minAliasedLineWidth = 1.0f;
    mNativeCaps.maxAliasedLineWidth = 1.0f;

    mNativeCaps.maxDrawBuffers       = mtl::kMaxRenderTargets;
    mNativeCaps.maxFramebufferWidth  = mNativeCaps.max2DTextureSize;
    mNativeCaps.maxFramebufferHeight = mNativeCaps.max2DTextureSize;
    mNativeCaps.maxColorAttachments  = mtl::kMaxRenderTargets;
    mNativeCaps.maxViewportWidth     = mNativeCaps.max2DTextureSize;
    mNativeCaps.maxViewportHeight    = mNativeCaps.max2DTextureSize;

    // MSAA
    mNativeCaps.maxSamples             = mFormatTable.getMaxSamples();
    mNativeCaps.maxSampleMaskWords     = 0;
    mNativeCaps.maxColorTextureSamples = mNativeCaps.maxSamples;
    mNativeCaps.maxDepthTextureSamples = mNativeCaps.maxSamples;
    mNativeCaps.maxIntegerSamples      = 1;

    mNativeCaps.maxVertexAttributes           = mtl::kMaxVertexAttribs;
    mNativeCaps.maxVertexAttribBindings       = mtl::kMaxVertexAttribs;
    mNativeCaps.maxVertexAttribRelativeOffset = std::numeric_limits<GLint>::max();
    mNativeCaps.maxVertexAttribStride         = std::numeric_limits<GLint>::max();

    // glGet() use signed integer as parameter so we have to use GLint's max here, not GLuint.
    mNativeCaps.maxElementsIndices  = std::numeric_limits<GLint>::max();
    mNativeCaps.maxElementsVertices = std::numeric_limits<GLint>::max();

    // Looks like all floats are IEEE according to the docs here:
    mNativeCaps.vertexHighpFloat.setIEEEFloat();
    mNativeCaps.vertexMediumpFloat.setIEEEFloat();
    mNativeCaps.vertexLowpFloat.setIEEEFloat();
    mNativeCaps.fragmentHighpFloat.setIEEEFloat();
    mNativeCaps.fragmentMediumpFloat.setIEEEFloat();
    mNativeCaps.fragmentLowpFloat.setIEEEFloat();

    mNativeCaps.vertexHighpInt.setTwosComplementInt(32);
    mNativeCaps.vertexMediumpInt.setTwosComplementInt(32);
    mNativeCaps.vertexLowpInt.setTwosComplementInt(32);
    mNativeCaps.fragmentHighpInt.setTwosComplementInt(32);
    mNativeCaps.fragmentMediumpInt.setTwosComplementInt(32);
    mNativeCaps.fragmentLowpInt.setTwosComplementInt(32);

    GLuint maxDefaultUniformVectors = mtl::kDefaultUniformsMaxSize / (sizeof(GLfloat) * 4);

    const GLuint maxDefaultUniformComponents = maxDefaultUniformVectors * 4;

    // Uniforms are implemented using a uniform buffer, so the max number of uniforms we can
    // support is the max buffer range divided by the size of a single uniform (4X float).
    mNativeCaps.maxVertexUniformVectors                              = maxDefaultUniformVectors;
    mNativeCaps.maxShaderUniformComponents[gl::ShaderType::Vertex]   = maxDefaultUniformComponents;
    mNativeCaps.maxFragmentUniformVectors                            = maxDefaultUniformVectors;
    mNativeCaps.maxShaderUniformComponents[gl::ShaderType::Fragment] = maxDefaultUniformComponents;

    mNativeCaps.maxShaderUniformBlocks[gl::ShaderType::Vertex]   = mtl::kMaxShaderUBOs;
    mNativeCaps.maxShaderUniformBlocks[gl::ShaderType::Fragment] = mtl::kMaxShaderUBOs;
    mNativeCaps.maxCombinedUniformBlocks                         = mtl::kMaxGLUBOBindings;

    // Note that we currently implement textures as combined image+samplers, so the limit is
    // the minimum of supported samplers and sampled images.
    mNativeCaps.maxCombinedTextureImageUnits                         = mtl::kMaxGLSamplerBindings;
    mNativeCaps.maxShaderTextureImageUnits[gl::ShaderType::Fragment] = mtl::kMaxShaderSamplers;
    mNativeCaps.maxShaderTextureImageUnits[gl::ShaderType::Vertex]   = mtl::kMaxShaderSamplers;

    // No info from Metal given, use default GLES3 spec values:
    mNativeCaps.minProgramTexelOffset = -8;
    mNativeCaps.maxProgramTexelOffset = 7;

    // NOTE(hqle): support storage buffer.
    const uint32_t maxPerStageStorageBuffers                     = 0;
    mNativeCaps.maxShaderStorageBlocks[gl::ShaderType::Vertex]   = maxPerStageStorageBuffers;
    mNativeCaps.maxShaderStorageBlocks[gl::ShaderType::Fragment] = maxPerStageStorageBuffers;
    mNativeCaps.maxCombinedShaderStorageBlocks                   = maxPerStageStorageBuffers;

    // Fill in additional limits for UBOs and SSBOs.
    mNativeCaps.maxUniformBufferBindings = mNativeCaps.maxCombinedUniformBlocks;
    mNativeCaps.maxUniformBlockSize      = mtl::kMaxUBOSize;  // Default according to GLES 3.0 spec.
    mNativeCaps.uniformBufferOffsetAlignment = 1;

    mNativeCaps.maxShaderStorageBufferBindings     = 0;
    mNativeCaps.maxShaderStorageBlockSize          = 0;
    mNativeCaps.shaderStorageBufferOffsetAlignment = 0;

    // UBO plus default uniform limits
    const uint32_t maxCombinedUniformComponents =
        maxDefaultUniformComponents + mtl::kMaxUBOSize * mtl::kMaxShaderUBOs / 4;
    for (gl::ShaderType shaderType : gl::kAllGraphicsShaderTypes)
    {
        mNativeCaps.maxCombinedShaderUniformComponents[shaderType] = maxCombinedUniformComponents;
    }

    mNativeCaps.maxCombinedShaderOutputResources = 0;

    mNativeCaps.maxTransformFeedbackInterleavedComponents =
        gl::IMPLEMENTATION_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS;
    mNativeCaps.maxTransformFeedbackSeparateAttributes =
        gl::IMPLEMENTATION_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS;
    mNativeCaps.maxTransformFeedbackSeparateComponents =
        gl::IMPLEMENTATION_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS;

    // GL_OES_get_program_binary
    mNativeCaps.programBinaryFormats.push_back(GL_PROGRAM_BINARY_ANGLE);

    // GL_APPLE_clip_distance
    mNativeCaps.maxClipDistances = 8;

    // Metal doesn't support GL_TEXTURE_COMPARE_MODE=GL_NONE for shadow samplers
    mNativeLimitations.noShadowSamplerCompareModeNone = true;

    // Apple platforms require PVRTC1 textures to be squares.
    mNativeLimitations.squarePvrtc1 = true;

    // Direct-to-metal constants:
    mNativeCaps.driverUniformsBindingIndex    = mtl::kDriverUniformsBindingIndex;
    mNativeCaps.defaultUniformsBindingIndex   = mtl::kDefaultUniformsBindingIndex;
    mNativeCaps.UBOArgumentBufferBindingIndex = mtl::kUBOArgumentBufferBindingIndex;
}

void DisplayMtl::initializeExtensions() const
{
    // Reset
    mNativeExtensions = gl::Extensions();

    // Enable this for simple buffer readback testing, but some functionality is missing.
    // NOTE(hqle): Support full mapBufferRange extension.
    mNativeExtensions.mapBufferOES           = true;
    mNativeExtensions.mapBufferRange         = true;
    mNativeExtensions.textureStorage         = true;
    mNativeExtensions.drawBuffers            = true;
    mNativeExtensions.drawBuffersIndexedEXT  = true;
    mNativeExtensions.drawBuffersIndexedOES  = true;
    mNativeExtensions.fragDepth              = true;
    mNativeExtensions.framebufferBlitANGLE   = true;
    mNativeExtensions.framebufferMultisample = true;
    mNativeExtensions.copyTexture            = true;
    mNativeExtensions.copyCompressedTexture  = false;

    // EXT_debug_marker is not implemented yet, but the entry points must be exposed for the
    // Metal backend to be used in Chrome (http://anglebug.com/4946)
    mNativeExtensions.debugMarker = true;

    mNativeExtensions.robustness             = true;
    mNativeExtensions.textureBorderClampOES  = false;  // not implemented yet
    mNativeExtensions.translatedShaderSource = true;
    mNativeExtensions.discardFramebuffer     = true;

    // TODO(anglebug.com/5505): figure out why WebGL drawing buffer
    // creation fails on macOS when the Metal backend advertises the
    // EXT_multisampled_render_to_texture extension.
#if !defined(ANGLE_PLATFORM_MACOS)
    // EXT_multisampled_render_to_texture
    if (mFeatures.allowMultisampleStoreAndResolve.enabled &&
        mFeatures.hasDepthAutoResolve.enabled && mFeatures.hasStencilAutoResolve.enabled)
    {
        mNativeExtensions.multisampledRenderToTexture = true;
    }
#endif

    // Enable EXT_blend_minmax
    mNativeExtensions.blendMinMax = true;

    mNativeExtensions.eglImageOES         = true;
    mNativeExtensions.eglImageExternalOES = false;
    // NOTE(hqle): Support GL_OES_EGL_image_external_essl3.
    mNativeExtensions.eglImageExternalEssl3OES = false;

    mNativeExtensions.memoryObject   = false;
    mNativeExtensions.memoryObjectFd = false;

    mNativeExtensions.semaphore   = false;
    mNativeExtensions.semaphoreFd = false;

    mNativeExtensions.instancedArraysANGLE = mFeatures.hasBaseVertexInstancedDraw.enabled;
    mNativeExtensions.instancedArraysEXT   = mNativeExtensions.instancedArraysANGLE;

    mNativeExtensions.robustBufferAccessBehavior = false;

    mNativeExtensions.eglSyncOES = false;

    mNativeExtensions.occlusionQueryBoolean = true;

    mNativeExtensions.disjointTimerQuery          = false;
    mNativeExtensions.queryCounterBitsTimeElapsed = false;
    mNativeExtensions.queryCounterBitsTimestamp   = false;

    mNativeExtensions.textureFilterAnisotropic = true;
    mNativeExtensions.maxTextureAnisotropy     = 16;

    mNativeExtensions.textureNPOTOES = true;

    mNativeExtensions.texture3DOES = true;

    mNativeExtensions.standardDerivativesOES = true;

    mNativeExtensions.elementIndexUintOES = true;

    // GL_OES_get_program_binary
    mNativeExtensions.getProgramBinaryOES = true;

    // GL_APPLE_clip_distance
    mNativeExtensions.clipDistanceAPPLE = true;

    // GL_NV_pixel_buffer_object
    mNativeExtensions.pixelBufferObjectNV = true;

    if (mFeatures.hasEvents.enabled)
    {
        // MTLSharedEvent is only available since Metal 2.1

        // GL_NV_fence
        mNativeExtensions.fenceNV = true;

        // GL_OES_EGL_sync
        mNativeExtensions.eglSyncOES = true;
    }
}

void DisplayMtl::initializeTextureCaps() const
{
    mNativeTextureCaps.clear();

    mFormatTable.generateTextureCaps(this, &mNativeTextureCaps,
                                     &mNativeCaps.compressedTextureFormats);

    // Re-verify texture extensions.
    mNativeExtensions.setTextureExtensionSupport(mNativeTextureCaps);

    // When ETC2/EAC formats are natively supported, enable ANGLE-specific extension string to
    // expose them to WebGL. In other case, mark potentially-available ETC1 extension as
    // emulated.
    if (supportsAppleGPUFamily(1) && gl::DetermineCompressedTextureETCSupport(mNativeTextureCaps))
    {
        mNativeExtensions.compressedTextureETC = true;
    }
    else
    {
        mNativeLimitations.emulatedEtc1 = true;
    }

    // Enable ASTC sliced 3D, requires MTLGPUFamilyApple3
    if (supportsAppleGPUFamily(3) && mNativeExtensions.textureCompressionASTCLDRKHR)
    {
        mNativeExtensions.textureCompressionSliced3dASTCKHR = true;
    }

    // Enable ASTC HDR, requires MTLGPUFamilyApple6
    if (supportsAppleGPUFamily(6) && mNativeExtensions.textureCompressionASTCLDRKHR)
    {
        mNativeExtensions.textureCompressionASTCHDRKHR = true;
    }

    // Disable all depth buffer and stencil buffer readback extensions until we need them
    mNativeExtensions.readDepthNV         = false;
    mNativeExtensions.readStencilNV       = false;
    mNativeExtensions.depthBufferFloat2NV = false;
    mNativeExtensions.textureCompressionASTCLDRKHR &= supportsAppleGPUFamily(2);
}

void DisplayMtl::initializeLimitations()
{
    mNativeLimitations.noVertexAttributeAliasing = true;
}

void DisplayMtl::initializeFeatures()
{
    bool isMetal2_1 = false;
    bool isMetal2_2 = false;
    if (ANGLE_APPLE_AVAILABLE_XCI(10.14, 13.0, 12.0))
    {
        isMetal2_1 = true;
    }

    if (ANGLE_APPLE_AVAILABLE_XCI(10.15, 13.0, 13.0))
    {
        isMetal2_2 = true;
    }

    bool isOSX       = TARGET_OS_OSX;
    bool isCatalyst  = TARGET_OS_MACCATALYST;
    bool isSimulator = TARGET_OS_SIMULATOR;
    bool isARM       = ANGLE_MTL_ARM;

    ANGLE_FEATURE_CONDITION((&mFeatures), allowGenMultipleMipsPerPass, true);
    ANGLE_FEATURE_CONDITION((&mFeatures), forceBufferGPUStorage, false);

    ANGLE_FEATURE_CONDITION((&mFeatures), hasDepthTextureFiltering,
                            (isOSX || isCatalyst) && !isARM);
    ANGLE_FEATURE_CONDITION((&mFeatures), hasExplicitMemBarrier,
                            isMetal2_1 && (isOSX || isCatalyst) && !isARM);
    ANGLE_FEATURE_CONDITION((&mFeatures), hasDepthAutoResolve, supportsEitherGPUFamily(3, 2));
    ANGLE_FEATURE_CONDITION((&mFeatures), hasStencilAutoResolve, supportsEitherGPUFamily(5, 2));
    ANGLE_FEATURE_CONDITION((&mFeatures), allowMultisampleStoreAndResolve,
                            supportsEitherGPUFamily(3, 1));

    ANGLE_FEATURE_CONDITION((&mFeatures), allowRuntimeSamplerCompareMode,
                            supportsEitherGPUFamily(3, 1));
    ANGLE_FEATURE_CONDITION((&mFeatures), allowSamplerCompareGradient,
                            supportsEitherGPUFamily(3, 1));
    ANGLE_FEATURE_CONDITION((&mFeatures), allowSamplerCompareLod, supportsEitherGPUFamily(3, 1));

    // http://anglebug.com/4919
    // Stencil blit shader is not compiled on Intel & NVIDIA, need investigation.
    ANGLE_FEATURE_CONDITION((&mFeatures), hasStencilOutput,
                            isMetal2_1 && !isIntel() && !isNVIDIA());

    ANGLE_FEATURE_CONDITION((&mFeatures), hasTextureSwizzle,
                            isMetal2_2 && supportsEitherGPUFamily(3, 2) && !isSimulator);

    // http://crbug.com/1136673
    // Fence sync is flaky on Nvidia
    ANGLE_FEATURE_CONDITION((&mFeatures), hasEvents, isMetal2_1 && !isNVIDIA());

    ANGLE_FEATURE_CONDITION((&mFeatures), hasCheapRenderPass, (isOSX || isCatalyst) && !isARM);

    // http://anglebug.com/5235
    // D24S8 is unreliable on AMD.
    ANGLE_FEATURE_CONDITION((&mFeatures), forceD24S8AsUnsupported, isAMD());

    // Base Vertex drawing is only supported since GPU family 3.
    ANGLE_FEATURE_CONDITION((&mFeatures), hasBaseVertexInstancedDraw,
                            isOSX || isCatalyst || supportsAppleGPUFamily(3));

    ANGLE_FEATURE_CONDITION((&mFeatures), hasNonUniformDispatch,
                            isOSX || isCatalyst || supportsAppleGPUFamily(4));

    ANGLE_FEATURE_CONDITION((&mFeatures), allowSeparatedDepthStencilBuffers,
                            !isOSX && !isCatalyst && !isSimulator);
    ANGLE_FEATURE_CONDITION((&mFeatures), rewriteRowMajorMatrices, true);
    ANGLE_FEATURE_CONDITION((&mFeatures), emulateTransformFeedback, true);

    ANGLE_FEATURE_CONDITION((&mFeatures), forceNonCSBaseMipmapGeneration, isIntel());

    angle::PlatformMethods *platform = ANGLEPlatformCurrent();
    platform->overrideFeaturesMtl(platform, &mFeatures);

    ApplyFeatureOverrides(&mFeatures, getState());
}

angle::Result DisplayMtl::initializeShaderLibrary()
{
#ifdef ANGLE_METAL_XCODE_BUILDS_SHADERS
    mDefaultShadersAsyncInfo.reset(new DefaultShaderAsyncInfoMtl);

    NSString *path = [NSBundle bundleWithIdentifier:@"com.apple.WebKit"].bundlePath;
    NSError *error = nullptr;
    mDefaultShadersAsyncInfo->defaultShaders =
        [getMetalDevice() newDefaultLibraryWithBundle:[NSBundle bundleWithPath:path] error:&error];

    if (error && !mDefaultShadersAsyncInfo->defaultShaders)
    {
        ANGLE_MTL_OBJC_SCOPE
        {
            ERR() << "Internal error: newDefaultLibraryWithBundle failed. "
                  << error.localizedDescription.UTF8String;
        }
        mDefaultShadersAsyncInfo->defaultShadersCompileError = std::move(error);
        return angle::Result::Stop;
    }
    mDefaultShadersAsyncInfo->compiled = true;

#else
    mDefaultShadersAsyncInfo.reset(new DefaultShaderAsyncInfoMtl);

    // Create references to async info struct since it might be released in terminate(), but the
    // callback might still not be fired yet.
    std::shared_ptr<DefaultShaderAsyncInfoMtl> asyncRef = mDefaultShadersAsyncInfo;

    // Compile the default shaders asynchronously
    ANGLE_MTL_OBJC_SCOPE
    {
        auto nsSource = [[NSString alloc] initWithBytesNoCopy:gDefaultMetallibSrc
                                                       length:sizeof(gDefaultMetallibSrc)
                                                     encoding:NSUTF8StringEncoding
                                                 freeWhenDone:NO];
        auto options  = [[[MTLCompileOptions alloc] init] ANGLE_MTL_AUTORELEASE];
        [getMetalDevice() newLibraryWithSource:nsSource
                                       options:options
                             completionHandler:^(id<MTLLibrary> library, NSError *error) {
                               std::unique_lock<std::mutex> lg(asyncRef->lock);

                               asyncRef->defaultShaders             = std::move(library);
                               asyncRef->defaultShadersCompileError = std::move(error);

                               asyncRef->compiled = true;
                               asyncRef->cv.notify_one();
                             }];

        [nsSource ANGLE_MTL_AUTORELEASE];
    }
#endif
    return angle::Result::Continue;
}

id<MTLLibrary> DisplayMtl::getDefaultShadersLib()
{
    std::unique_lock<std::mutex> lg(mDefaultShadersAsyncInfo->lock);
    if (!mDefaultShadersAsyncInfo->compiled)
    {
        // Wait for async compilation
        mDefaultShadersAsyncInfo->cv.wait(lg,
                                          [this] { return mDefaultShadersAsyncInfo->compiled; });
    }

    if (mDefaultShadersAsyncInfo->defaultShadersCompileError &&
        !mDefaultShadersAsyncInfo->defaultShaders)
    {
        ANGLE_MTL_OBJC_SCOPE
        {
            ERR() << "Internal error: "
                  << mDefaultShadersAsyncInfo->defaultShadersCompileError.get()
                         .localizedDescription.UTF8String;
        }
        // This is not supposed to happen
        UNREACHABLE();
    }

    return mDefaultShadersAsyncInfo->defaultShaders;
}

bool DisplayMtl::supportsAppleGPUFamily(uint8_t iOSFamily) const
{
#if TARGET_OS_MACCATALYST
    return false;
#else
#    if (__IPHONE_OS_VERSION_MAX_ALLOWED >= 130000) || (__TV_OS_VERSION_MAX_ALLOWED >= 130000) || \
        (__MAC_OS_X_VERSION_MAX_ALLOWED >= 101500)
    // If device supports [MTLDevice supportsFamily:], then use it.
    if (ANGLE_APPLE_AVAILABLE_XI(10.15, 13.0))
    {
        MTLGPUFamily family;
        switch (iOSFamily)
        {
            case 1:
                family = MTLGPUFamilyApple1;
                break;
            case 2:
                family = MTLGPUFamilyApple2;
                break;
            case 3:
                family = MTLGPUFamilyApple3;
                break;
            case 4:
                family = MTLGPUFamilyApple4;
                break;
            case 5:
                family = MTLGPUFamilyApple5;
                break;
#        if TARGET_OS_IOS || (TARGET_OS_OSX && (__MAC_OS_X_VERSION_MAX_ALLOWED >= 101600))
            case 6:
                family = MTLGPUFamilyApple6;
                break;
#        endif
#        if (TARGET_OS_IOS && (__IPHONE_OS_VERSION_MAX_ALLOWED >= 140000)) || \
            (TARGET_OS_OSX && (__MAC_OS_X_VERSION_MAX_ALLOWED >= 101600))
            case 7:
                family = MTLGPUFamilyApple7;
                break;
#        endif
            default:
                return false;
        }
        return [getMetalDevice() supportsFamily:family];
    }  // Metal 2.2
#    endif  // __IPHONE_OS_VERSION_MAX_ALLOWED

    // If device doesn't support [MTLDevice supportsFamily:], then use
    // [MTLDevice supportsFeatureSet:].
#    if TARGET_OS_IOS || TARGET_OS_TV
    MTLFeatureSet featureSet;
#    endif
    switch (iOSFamily)
    {
#    if TARGET_OS_IOS
        case 1:
            featureSet = MTLFeatureSet_iOS_GPUFamily1_v1;
            break;
        case 2:
            featureSet = MTLFeatureSet_iOS_GPUFamily2_v1;
            break;
        case 3:
            featureSet = MTLFeatureSet_iOS_GPUFamily3_v1;
            break;
        case 4:
            featureSet = MTLFeatureSet_iOS_GPUFamily4_v1;
            break;
#        if __IPHONE_OS_VERSION_MAX_ALLOWED >= 120000
        case 5:
            featureSet = MTLFeatureSet_iOS_GPUFamily5_v1;
            break;
#        endif  // __IPHONE_OS_VERSION_MAX_ALLOWED
#    elif TARGET_OS_TV
        case 1:
        case 2:
            featureSet = MTLFeatureSet_tvOS_GPUFamily1_v1;
            break;
        case 3:
            featureSet = MTLFeatureSet_tvOS_GPUFamily2_v1;
            break;
#    endif  // TARGET_OS_IOS
        default:
            return false;
    }

#    if TARGET_OS_IOS || TARGET_OS_TV
    return [getMetalDevice() supportsFeatureSet:featureSet];
#    endif
#endif  // TARGET_OS_MACCATALYST
}

bool DisplayMtl::supportsMacGPUFamily(uint8_t macFamily) const
{
#if TARGET_OS_OSX || TARGET_OS_MACCATALYST
#    if defined(__MAC_10_15)
    // If device supports [MTLDevice supportsFamily:], then use it.
    if (ANGLE_APPLE_AVAILABLE_XC(10.15, 13.0))
    {
        MTLGPUFamily family;

        switch (macFamily)
        {
#        if TARGET_OS_MACCATALYST
            case 1:
                family = MTLGPUFamilyMacCatalyst1;
                break;
            case 2:
                family = MTLGPUFamilyMacCatalyst2;
                break;
#        else   // TARGET_OS_MACCATALYST
            case 1:
                family = MTLGPUFamilyMac1;
                break;
            case 2:
                family = MTLGPUFamilyMac2;
                break;
#        endif  // TARGET_OS_MACCATALYST
            default:
                return false;
        }

        return [getMetalDevice() supportsFamily:family];
    }  // Metal 2.2
#    endif

    // If device doesn't support [MTLDevice supportsFamily:], then use
    // [MTLDevice supportsFeatureSet:].
#    if TARGET_OS_MACCATALYST
    UNREACHABLE();
    return false;
#    else
    MTLFeatureSet featureSet;
    switch (macFamily)
    {
        case 1:
            featureSet = MTLFeatureSet_macOS_GPUFamily1_v1;
            break;
#        if defined(__MAC_10_14)
        case 2:
            featureSet = MTLFeatureSet_macOS_GPUFamily2_v1;
            break;
#        endif
        default:
            return false;
    }
    return [getMetalDevice() supportsFeatureSet:featureSet];
#    endif  // TARGET_OS_MACCATALYST
#else       // #if TARGET_OS_OSX || TARGET_OS_MACCATALYST

    return false;

#endif
}

bool DisplayMtl::supportsEitherGPUFamily(uint8_t iOSFamily, uint8_t macFamily) const
{
    return supportsAppleGPUFamily(iOSFamily) || supportsMacGPUFamily(macFamily);
}

bool DisplayMtl::isAMD() const
{
    return angle::IsAMD(mMetalDeviceVendorId);
}

bool DisplayMtl::isIntel() const
{
    return angle::IsIntel(mMetalDeviceVendorId);
}

bool DisplayMtl::isNVIDIA() const
{
    return angle::IsNVIDIA(mMetalDeviceVendorId);
}

#if ANGLE_MTL_EVENT_AVAILABLE
mtl::AutoObjCObj<MTLSharedEventListener> DisplayMtl::getOrCreateSharedEventListener()
{
    if (!mSharedEventListener)
    {
        ANGLE_MTL_OBJC_SCOPE
        {
            mSharedEventListener = [[[MTLSharedEventListener alloc] init] ANGLE_MTL_AUTORELEASE];
        }
    }
    return mSharedEventListener;
}
#endif

}  // namespace rx
