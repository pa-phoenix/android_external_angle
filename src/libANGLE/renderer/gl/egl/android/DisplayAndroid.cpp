//
// Copyright (c) 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// DisplayAndroid.cpp: Android implementation of egl::Display

#include <android/native_window.h>

#include "common/debug.h"
#include "libANGLE/Display.h"
#include "libANGLE/Surface.h"
#include "libANGLE/renderer/gl/ContextGL.h"
#include "libANGLE/renderer/gl/RendererGL.h"
#include "libANGLE/renderer/gl/egl/FunctionsEGLDL.h"
#include "libANGLE/renderer/gl/egl/PbufferSurfaceEGL.h"
#include "libANGLE/renderer/gl/egl/WindowSurfaceEGL.h"
#include "libANGLE/renderer/gl/egl/android/DisplayAndroid.h"
#include "libANGLE/renderer/gl/renderergl_utils.h"

namespace
{
const char *GetEGLPath()
{
#if defined(__LP64__)
    return "/system/lib64/libEGL.so";
#else
    return "/system/lib/libEGL.so";
#endif
}
}  // namespace

namespace rx
{

DisplayAndroid::DisplayAndroid(const egl::DisplayState &state)
    : DisplayEGL(state), mDummyPbuffer(EGL_NO_SURFACE), mCurrentSurface(EGL_NO_SURFACE)
{
}

DisplayAndroid::~DisplayAndroid()
{
}

egl::Error DisplayAndroid::initialize(egl::Display *display)
{
    FunctionsEGLDL *egl = new FunctionsEGLDL();
    mEGL = egl;
    void *eglHandle     = reinterpret_cast<void *>(display->getAttributeMap().get(
        EGL_PLATFORM_ANGLE_EGL_HANDLE_ANGLE, 0));
    ANGLE_TRY(egl->initialize(display->getNativeDisplayId(), GetEGLPath(), eglHandle));

    gl::Version eglVersion(mEGL->majorVersion, mEGL->minorVersion);
    ASSERT(eglVersion >= gl::Version(1, 4));

    static_assert(EGL_OPENGL_ES3_BIT == EGL_OPENGL_ES3_BIT_KHR, "Extension define must match core");
    EGLint esBit = (eglVersion >= gl::Version(1, 5) || mEGL->hasExtension("EGL_KHR_create_context"))
                       ? EGL_OPENGL_ES3_BIT
                       : EGL_OPENGL_ES2_BIT;

    // clang-format off
    std::vector<EGLint> configAttribListBase =
    {
        EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
        // Android doesn't support pixmaps
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
        EGL_CONFIG_CAVEAT, EGL_NONE,
        EGL_CONFORMANT, esBit,
        EGL_RENDERABLE_TYPE, esBit,
    };
    // clang-format on

    if (mEGL->hasExtension("EGL_EXT_pixel_format_float"))
    {
        // Don't request floating point configs
        configAttribListBase.push_back(EGL_COLOR_COMPONENT_TYPE_EXT);
        configAttribListBase.push_back(EGL_COLOR_COMPONENT_TYPE_FIXED_EXT);
    }

    std::vector<EGLint> configAttribListWithFormat = configAttribListBase;
    // EGL1.5 spec Section 2.2 says that depth, multisample and stencil buffer depths
    // must match for contexts to be compatible.
    // Choose RGBA8888
    configAttribListWithFormat.push_back(EGL_RED_SIZE);
    configAttribListWithFormat.push_back(8);
    configAttribListWithFormat.push_back(EGL_GREEN_SIZE);
    configAttribListWithFormat.push_back(8);
    configAttribListWithFormat.push_back(EGL_BLUE_SIZE);
    configAttribListWithFormat.push_back(8);
    configAttribListWithFormat.push_back(EGL_ALPHA_SIZE);
    configAttribListWithFormat.push_back(8);
    // Choose DEPTH24_STENCIL8
    configAttribListWithFormat.push_back(EGL_DEPTH_SIZE);
    configAttribListWithFormat.push_back(24);
    configAttribListWithFormat.push_back(EGL_STENCIL_SIZE);
    configAttribListWithFormat.push_back(8);
    // Choose no multisampling
    configAttribListWithFormat.push_back(EGL_SAMPLE_BUFFERS);
    configAttribListWithFormat.push_back(0);

    // Complete the attrib lists
    configAttribListBase.push_back(EGL_NONE);
    configAttribListWithFormat.push_back(EGL_NONE);

    EGLint numConfig;
    EGLConfig configWithFormat;

    EGLBoolean success =
        mEGL->chooseConfig(configAttribListWithFormat.data(), &configWithFormat, 1, &numConfig);
    if (success == EGL_FALSE)
    {
        return egl::EglNotInitialized()
               << "eglChooseConfig failed with " << egl::Error(mEGL->getError());
    }

    // A dummy pbuffer is only needed if surfaceless contexts are not supported.
    if (!mEGL->hasExtension("EGL_KHR_surfaceless_context"))
    {
        int dummyPbufferAttribs[] = {
            EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE,
        };
        mDummyPbuffer = mEGL->createPbufferSurface(configWithFormat, dummyPbufferAttribs);
        if (mDummyPbuffer == EGL_NO_SURFACE)
        {
            return egl::EglNotInitialized()
                   << "eglCreatePbufferSurface failed with " << egl::Error(mEGL->getError());
        }
    }

    // Create mDummyPbuffer with a normal config, but create a no_config mContext, if possible
    if (mEGL->hasExtension("EGL_KHR_no_config_context"))
    {
        mConfigAttribList = configAttribListBase;
        mConfig           = EGL_NO_CONFIG_KHR;
    }
    else
    {
        mConfigAttribList = configAttribListWithFormat;
        mConfig           = configWithFormat;
    }

    ANGLE_TRY(initializeContext(display->getAttributeMap()));

    success = mEGL->makeCurrent(mDummyPbuffer, mContext);
    if (success == EGL_FALSE)
    {
        return egl::EglNotInitialized()
               << "eglMakeCurrent failed with " << egl::Error(mEGL->getError());
    }
    mCurrentSurface = mDummyPbuffer;

    std::unique_ptr<FunctionsGL> functionsGL(mEGL->makeFunctionsGL());
    functionsGL->initialize(display->getAttributeMap());

    mRenderer.reset(new RendererGL(std::move(functionsGL), display->getAttributeMap()));

    const gl::Version &maxVersion = mRenderer->getMaxSupportedESVersion();
    if (maxVersion < gl::Version(2, 0))
    {
        return egl::EglNotInitialized() << "OpenGL ES 2.0 is not supportable.";
    }

    return DisplayGL::initialize(display);
}

void DisplayAndroid::terminate()
{
    DisplayGL::terminate();

    EGLBoolean success = mEGL->makeCurrent(EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (success == EGL_FALSE)
    {
        ERR() << "eglMakeCurrent error " << egl::Error(mEGL->getError());
    }
    mCurrentSurface = EGL_NO_SURFACE;

    if (mDummyPbuffer != EGL_NO_SURFACE)
    {
        success = mEGL->destroySurface(mDummyPbuffer);
        mDummyPbuffer = EGL_NO_SURFACE;
        if (success == EGL_FALSE)
        {
            ERR() << "eglDestroySurface error " << egl::Error(mEGL->getError());
        }
    }

    mRenderer.reset();
    if (mContext != EGL_NO_CONTEXT)
    {
        success = mEGL->destroyContext(mContext);
        mContext = EGL_NO_CONTEXT;
        if (success == EGL_FALSE)
        {
            ERR() << "eglDestroyContext error " << egl::Error(mEGL->getError());
        }
    }

    egl::Error result = mEGL->terminate();
    if (result.isError())
    {
        ERR() << "eglTerminate error " << result;
    }

    SafeDelete(mEGL);
}

SurfaceImpl *DisplayAndroid::createWindowSurface(const egl::SurfaceState &state,
                                                 EGLNativeWindowType window,
                                                 const egl::AttributeMap &attribs)
{
    EGLConfig config;
    EGLint numConfig;
    EGLBoolean success;

    const EGLint configAttribList[] = {EGL_CONFIG_ID, mConfigIds[state.config->configID], EGL_NONE};
    success = mEGL->chooseConfig(configAttribList, &config, 1, &numConfig);
    ASSERT(success && numConfig == 1);

    return new WindowSurfaceEGL(state, mEGL, config, window);
}

SurfaceImpl *DisplayAndroid::createPbufferSurface(const egl::SurfaceState &state,
                                                  const egl::AttributeMap &attribs)
{
    EGLConfig config;
    EGLint numConfig;
    EGLBoolean success;

    const EGLint configAttribList[] = {EGL_CONFIG_ID, mConfigIds[state.config->configID], EGL_NONE};
    success = mEGL->chooseConfig(configAttribList, &config, 1, &numConfig);
    ASSERT(success && numConfig == 1);

    return new PbufferSurfaceEGL(state, mEGL, config);
}

SurfaceImpl *DisplayAndroid::createPbufferFromClientBuffer(const egl::SurfaceState &state,
                                                           EGLenum buftype,
                                                           EGLClientBuffer clientBuffer,
                                                           const egl::AttributeMap &attribs)
{
    UNIMPLEMENTED();
    return nullptr;
}

SurfaceImpl *DisplayAndroid::createPixmapSurface(const egl::SurfaceState &state,
                                                 NativePixmapType nativePixmap,
                                                 const egl::AttributeMap &attribs)
{
    UNIMPLEMENTED();
    return nullptr;
}

ImageImpl *DisplayAndroid::createImage(const egl::ImageState &state,
                                       EGLenum target,
                                       const egl::AttributeMap &attribs)
{
    UNIMPLEMENTED();
    return DisplayGL::createImage(state, target, attribs);
}

ContextImpl *DisplayAndroid::createContext(const gl::ContextState &state,
                                           const egl::Config *configuration,
                                           const gl::Context *shareContext,
                                           const egl::AttributeMap &attribs)
{
    return new ContextGL(state, mRenderer);
}

template <typename T>
void DisplayAndroid::getConfigAttrib(EGLConfig config, EGLint attribute, T *value) const
{
    EGLint tmp;
    EGLBoolean success = mEGL->getConfigAttrib(config, attribute, &tmp);
    ASSERT(success == EGL_TRUE);
    *value = tmp;
}

template <typename T, typename U>
void DisplayAndroid::getConfigAttribIfExtension(EGLConfig config,
                                                EGLint attribute,
                                                T *value,
                                                const char *extension,
                                                const U &defaultValue) const
{
    if (mEGL->hasExtension(extension))
    {
        getConfigAttrib(config, attribute, value);
    }
    else
    {
        *value = static_cast<T>(defaultValue);
    }
}

egl::ConfigSet DisplayAndroid::generateConfigs()
{
    egl::ConfigSet configSet;
    mConfigIds.clear();

    EGLint numConfigs;
    EGLBoolean success = mEGL->chooseConfig(mConfigAttribList.data(), nullptr, 0, &numConfigs);
    ASSERT(success == EGL_TRUE && numConfigs > 0);

    std::vector<EGLConfig> configs(numConfigs);
    EGLint numConfigs2;
    success =
        mEGL->chooseConfig(mConfigAttribList.data(), configs.data(), numConfigs, &numConfigs2);
    ASSERT(success == EGL_TRUE && numConfigs2 == numConfigs);

    for (int i = 0; i < numConfigs; i++)
    {
        egl::Config config;

        getConfigAttrib(configs[i], EGL_BUFFER_SIZE, &config.bufferSize);
        getConfigAttrib(configs[i], EGL_RED_SIZE, &config.redSize);
        getConfigAttrib(configs[i], EGL_GREEN_SIZE, &config.greenSize);
        getConfigAttrib(configs[i], EGL_BLUE_SIZE, &config.blueSize);
        getConfigAttrib(configs[i], EGL_LUMINANCE_SIZE, &config.luminanceSize);
        getConfigAttrib(configs[i], EGL_ALPHA_SIZE, &config.alphaSize);
        getConfigAttrib(configs[i], EGL_ALPHA_MASK_SIZE, &config.alphaMaskSize);
        getConfigAttrib(configs[i], EGL_BIND_TO_TEXTURE_RGB, &config.bindToTextureRGB);
        getConfigAttrib(configs[i], EGL_BIND_TO_TEXTURE_RGBA, &config.bindToTextureRGBA);
        getConfigAttrib(configs[i], EGL_COLOR_BUFFER_TYPE, &config.colorBufferType);
        getConfigAttrib(configs[i], EGL_CONFIG_CAVEAT, &config.configCaveat);
        getConfigAttrib(configs[i], EGL_CONFIG_ID, &config.configID);
        getConfigAttrib(configs[i], EGL_CONFORMANT, &config.conformant);
        getConfigAttrib(configs[i], EGL_DEPTH_SIZE, &config.depthSize);
        getConfigAttrib(configs[i], EGL_LEVEL, &config.level);
        getConfigAttrib(configs[i], EGL_MAX_PBUFFER_WIDTH, &config.maxPBufferWidth);
        getConfigAttrib(configs[i], EGL_MAX_PBUFFER_HEIGHT, &config.maxPBufferHeight);
        getConfigAttrib(configs[i], EGL_MAX_PBUFFER_PIXELS, &config.maxPBufferPixels);
        getConfigAttrib(configs[i], EGL_MAX_SWAP_INTERVAL, &config.maxSwapInterval);
        getConfigAttrib(configs[i], EGL_MIN_SWAP_INTERVAL, &config.minSwapInterval);
        getConfigAttrib(configs[i], EGL_NATIVE_RENDERABLE, &config.nativeRenderable);
        getConfigAttrib(configs[i], EGL_NATIVE_VISUAL_ID, &config.nativeVisualID);
        getConfigAttrib(configs[i], EGL_NATIVE_VISUAL_TYPE, &config.nativeVisualType);
        getConfigAttrib(configs[i], EGL_RENDERABLE_TYPE, &config.renderableType);
        getConfigAttrib(configs[i], EGL_SAMPLE_BUFFERS, &config.sampleBuffers);
        getConfigAttrib(configs[i], EGL_SAMPLES, &config.samples);
        getConfigAttrib(configs[i], EGL_STENCIL_SIZE, &config.stencilSize);
        getConfigAttrib(configs[i], EGL_SURFACE_TYPE, &config.surfaceType);
        getConfigAttrib(configs[i], EGL_TRANSPARENT_TYPE, &config.transparentType);
        getConfigAttrib(configs[i], EGL_TRANSPARENT_RED_VALUE, &config.transparentRedValue);
        getConfigAttrib(configs[i], EGL_TRANSPARENT_GREEN_VALUE, &config.transparentGreenValue);
        getConfigAttrib(configs[i], EGL_TRANSPARENT_BLUE_VALUE, &config.transparentBlueValue);
        getConfigAttribIfExtension(configs[i], EGL_COLOR_COMPONENT_TYPE_EXT,
                                   &config.colorComponentType, "EGL_EXT_pixel_format_float",
                                   EGL_COLOR_COMPONENT_TYPE_FIXED_EXT);

        if (config.colorBufferType == EGL_RGB_BUFFER)
        {
            ASSERT(config.colorComponentType == EGL_COLOR_COMPONENT_TYPE_FIXED_EXT);
            if (config.redSize == 8 && config.greenSize == 8 && config.blueSize == 8 &&
                config.alphaSize == 8)
            {
                config.renderTargetFormat = GL_RGBA8;
            }
            else if (config.redSize == 8 && config.greenSize == 8 && config.blueSize == 8 &&
                     config.alphaSize == 0)
            {
                config.renderTargetFormat = GL_RGB8;
            }
            else if (config.redSize == 5 && config.greenSize == 6 && config.blueSize == 5 &&
                     config.alphaSize == 0)
            {
                config.renderTargetFormat = GL_RGB565;
            }
            else if (config.redSize == 5 && config.greenSize == 5 && config.blueSize == 5 &&
                     config.alphaSize == 1)
            {
                config.renderTargetFormat = GL_RGB5_A1;
            }
            else if (config.redSize == 4 && config.greenSize == 4 && config.blueSize == 4 &&
                     config.alphaSize == 4)
            {
                config.renderTargetFormat = GL_RGBA4;
            }
            else
            {
                ERR() << "RGBA(" << config.redSize << "," << config.greenSize << ","
                      << config.blueSize << "," << config.alphaSize << ") not handled";
                UNREACHABLE();
            }
        }
        else
        {
            UNREACHABLE();
        }

        if (config.depthSize == 0 && config.stencilSize == 0)
        {
            config.depthStencilFormat = GL_ZERO;
        }
        else if (config.depthSize == 16 && config.stencilSize == 0)
        {
            config.depthStencilFormat = GL_DEPTH_COMPONENT16;
        }
        else if (config.depthSize == 24 && config.stencilSize == 0)
        {
            config.depthStencilFormat = GL_DEPTH_COMPONENT24;
        }
        else if (config.depthSize == 24 && config.stencilSize == 8)
        {
            config.depthStencilFormat = GL_DEPTH24_STENCIL8;
        }
        else if (config.depthSize == 0 && config.stencilSize == 8)
        {
            config.depthStencilFormat = GL_STENCIL_INDEX8;
        }
        else
        {
            UNREACHABLE();
        }

        config.matchNativePixmap = EGL_NONE;
        config.optimalOrientation = 0;

        int internalId = configSet.add(config);
        mConfigIds[internalId] = config.configID;
    }

    return configSet;
}

bool DisplayAndroid::testDeviceLost()
{
    return false;
}

egl::Error DisplayAndroid::restoreLostDevice(const egl::Display *display)
{
    UNIMPLEMENTED();
    return egl::NoError();
}

bool DisplayAndroid::isValidNativeWindow(EGLNativeWindowType window) const
{
    return ANativeWindow_getFormat(window) >= 0;
}

DeviceImpl *DisplayAndroid::createDevice()
{
    UNIMPLEMENTED();
    return nullptr;
}

egl::Error DisplayAndroid::waitClient(const gl::Context *context) const
{
    UNIMPLEMENTED();
    return egl::NoError();
}

egl::Error DisplayAndroid::waitNative(const gl::Context *context, EGLint engine) const
{
    UNIMPLEMENTED();
    return egl::NoError();
}
egl::Error DisplayAndroid::makeCurrent(egl::Surface *drawSurface,
                                       egl::Surface *readSurface,
                                       gl::Context *context)
{
    if (drawSurface)
    {
        SurfaceEGL *drawSurfaceEGL = GetImplAs<SurfaceEGL>(drawSurface);
        EGLSurface surface         = drawSurfaceEGL->getSurface();
        if (surface != mCurrentSurface)
        {
            if (mEGL->makeCurrent(surface, mContext) == EGL_FALSE)
            {
                return egl::Error(mEGL->getError(), "eglMakeCurrent failed");
            }
            mCurrentSurface = surface;
        }
    }

    return DisplayGL::makeCurrent(drawSurface, readSurface, context);
}

gl::Version DisplayAndroid::getMaxSupportedESVersion() const
{
    return mRenderer->getMaxSupportedESVersion();
}

egl::Error DisplayAndroid::makeCurrentSurfaceless(gl::Context *context)
{
    // Nothing to do because EGL always uses the same context and the previous surface can be left
    // current.
    return egl::NoError();
}

}  // namespace rx
