// GENERATED FILE - DO NOT EDIT.
// Generated by generate_entry_points.py using data from gl.xml.
//
// Copyright 2020 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// validationGL3_autogen.h:
//   Validation functions for the OpenGL Desktop GL 3.x entry points.

#ifndef LIBANGLE_VALIDATION_GL3_AUTOGEN_H_
#define LIBANGLE_VALIDATION_GL3_AUTOGEN_H_

#include "common/PackedEnums.h"

namespace gl
{
class Context;

// GL 3.0
bool ValidateBeginConditionalRender(const Context *context, GLuint id, GLenum mode);
bool ValidateBindFragDataLocation(const Context *context,
                                  ShaderProgramID programPacked,
                                  GLuint color,
                                  const GLchar *name);
bool ValidateClampColor(const Context *context, GLenum target, GLenum clamp);
bool ValidateEndConditionalRender(const Context *context);
bool ValidateFramebufferTexture1D(const Context *context,
                                  GLenum target,
                                  GLenum attachment,
                                  TextureTarget textargetPacked,
                                  TextureID texturePacked,
                                  GLint level);
bool ValidateFramebufferTexture3D(const Context *context,
                                  GLenum target,
                                  GLenum attachment,
                                  TextureTarget textargetPacked,
                                  TextureID texturePacked,
                                  GLint level,
                                  GLint zoffset);
bool ValidateVertexAttribI1i(const Context *context, GLuint index, GLint x);
bool ValidateVertexAttribI1iv(const Context *context, GLuint index, const GLint *v);
bool ValidateVertexAttribI1ui(const Context *context, GLuint index, GLuint x);
bool ValidateVertexAttribI1uiv(const Context *context, GLuint index, const GLuint *v);
bool ValidateVertexAttribI2i(const Context *context, GLuint index, GLint x, GLint y);
bool ValidateVertexAttribI2iv(const Context *context, GLuint index, const GLint *v);
bool ValidateVertexAttribI2ui(const Context *context, GLuint index, GLuint x, GLuint y);
bool ValidateVertexAttribI2uiv(const Context *context, GLuint index, const GLuint *v);
bool ValidateVertexAttribI3i(const Context *context, GLuint index, GLint x, GLint y, GLint z);
bool ValidateVertexAttribI3iv(const Context *context, GLuint index, const GLint *v);
bool ValidateVertexAttribI3ui(const Context *context, GLuint index, GLuint x, GLuint y, GLuint z);
bool ValidateVertexAttribI3uiv(const Context *context, GLuint index, const GLuint *v);
bool ValidateVertexAttribI4bv(const Context *context, GLuint index, const GLbyte *v);
bool ValidateVertexAttribI4sv(const Context *context, GLuint index, const GLshort *v);
bool ValidateVertexAttribI4ubv(const Context *context, GLuint index, const GLubyte *v);
bool ValidateVertexAttribI4usv(const Context *context, GLuint index, const GLushort *v);

// GL 3.1
bool ValidateGetActiveUniformName(const Context *context,
                                  ShaderProgramID programPacked,
                                  GLuint uniformIndex,
                                  GLsizei bufSize,
                                  const GLsizei *length,
                                  const GLchar *uniformName);
bool ValidatePrimitiveRestartIndex(const Context *context, GLuint index);

// GL 3.2
bool ValidateMultiDrawElementsBaseVertex(const Context *context,
                                         PrimitiveMode modePacked,
                                         const GLsizei *count,
                                         DrawElementsType typePacked,
                                         const void *const *indices,
                                         GLsizei drawcount,
                                         const GLint *basevertex);
bool ValidateProvokingVertex(const Context *context, ProvokingVertexConvention modePacked);
bool ValidateTexImage2DMultisample(const Context *context,
                                   GLenum target,
                                   GLsizei samples,
                                   GLenum internalformat,
                                   GLsizei width,
                                   GLsizei height,
                                   GLboolean fixedsamplelocations);
bool ValidateTexImage3DMultisample(const Context *context,
                                   GLenum target,
                                   GLsizei samples,
                                   GLenum internalformat,
                                   GLsizei width,
                                   GLsizei height,
                                   GLsizei depth,
                                   GLboolean fixedsamplelocations);

// GL 3.3
bool ValidateBindFragDataLocationIndexed(const Context *context,
                                         ShaderProgramID programPacked,
                                         GLuint colorNumber,
                                         GLuint index,
                                         const GLchar *name);
bool ValidateColorP3ui(const Context *context, GLenum type, GLuint color);
bool ValidateColorP3uiv(const Context *context, GLenum type, const GLuint *color);
bool ValidateColorP4ui(const Context *context, GLenum type, GLuint color);
bool ValidateColorP4uiv(const Context *context, GLenum type, const GLuint *color);
bool ValidateGetFragDataIndex(const Context *context,
                              ShaderProgramID programPacked,
                              const GLchar *name);
bool ValidateGetQueryObjecti64v(const Context *context,
                                QueryID idPacked,
                                GLenum pname,
                                const GLint64 *params);
bool ValidateGetQueryObjectui64v(const Context *context,
                                 QueryID idPacked,
                                 GLenum pname,
                                 const GLuint64 *params);
bool ValidateMultiTexCoordP1ui(const Context *context, GLenum texture, GLenum type, GLuint coords);
bool ValidateMultiTexCoordP1uiv(const Context *context,
                                GLenum texture,
                                GLenum type,
                                const GLuint *coords);
bool ValidateMultiTexCoordP2ui(const Context *context, GLenum texture, GLenum type, GLuint coords);
bool ValidateMultiTexCoordP2uiv(const Context *context,
                                GLenum texture,
                                GLenum type,
                                const GLuint *coords);
bool ValidateMultiTexCoordP3ui(const Context *context, GLenum texture, GLenum type, GLuint coords);
bool ValidateMultiTexCoordP3uiv(const Context *context,
                                GLenum texture,
                                GLenum type,
                                const GLuint *coords);
bool ValidateMultiTexCoordP4ui(const Context *context, GLenum texture, GLenum type, GLuint coords);
bool ValidateMultiTexCoordP4uiv(const Context *context,
                                GLenum texture,
                                GLenum type,
                                const GLuint *coords);
bool ValidateNormalP3ui(const Context *context, GLenum type, GLuint coords);
bool ValidateNormalP3uiv(const Context *context, GLenum type, const GLuint *coords);
bool ValidateQueryCounter(const Context *context, QueryID idPacked, QueryType targetPacked);
bool ValidateSecondaryColorP3ui(const Context *context, GLenum type, GLuint color);
bool ValidateSecondaryColorP3uiv(const Context *context, GLenum type, const GLuint *color);
bool ValidateTexCoordP1ui(const Context *context, GLenum type, GLuint coords);
bool ValidateTexCoordP1uiv(const Context *context, GLenum type, const GLuint *coords);
bool ValidateTexCoordP2ui(const Context *context, GLenum type, GLuint coords);
bool ValidateTexCoordP2uiv(const Context *context, GLenum type, const GLuint *coords);
bool ValidateTexCoordP3ui(const Context *context, GLenum type, GLuint coords);
bool ValidateTexCoordP3uiv(const Context *context, GLenum type, const GLuint *coords);
bool ValidateTexCoordP4ui(const Context *context, GLenum type, GLuint coords);
bool ValidateTexCoordP4uiv(const Context *context, GLenum type, const GLuint *coords);
bool ValidateVertexAttribP1ui(const Context *context,
                              GLuint index,
                              GLenum type,
                              GLboolean normalized,
                              GLuint value);
bool ValidateVertexAttribP1uiv(const Context *context,
                               GLuint index,
                               GLenum type,
                               GLboolean normalized,
                               const GLuint *value);
bool ValidateVertexAttribP2ui(const Context *context,
                              GLuint index,
                              GLenum type,
                              GLboolean normalized,
                              GLuint value);
bool ValidateVertexAttribP2uiv(const Context *context,
                               GLuint index,
                               GLenum type,
                               GLboolean normalized,
                               const GLuint *value);
bool ValidateVertexAttribP3ui(const Context *context,
                              GLuint index,
                              GLenum type,
                              GLboolean normalized,
                              GLuint value);
bool ValidateVertexAttribP3uiv(const Context *context,
                               GLuint index,
                               GLenum type,
                               GLboolean normalized,
                               const GLuint *value);
bool ValidateVertexAttribP4ui(const Context *context,
                              GLuint index,
                              GLenum type,
                              GLboolean normalized,
                              GLuint value);
bool ValidateVertexAttribP4uiv(const Context *context,
                               GLuint index,
                               GLenum type,
                               GLboolean normalized,
                               const GLuint *value);
bool ValidateVertexP2ui(const Context *context, GLenum type, GLuint value);
bool ValidateVertexP2uiv(const Context *context, GLenum type, const GLuint *value);
bool ValidateVertexP3ui(const Context *context, GLenum type, GLuint value);
bool ValidateVertexP3uiv(const Context *context, GLenum type, const GLuint *value);
bool ValidateVertexP4ui(const Context *context, GLenum type, GLuint value);
bool ValidateVertexP4uiv(const Context *context, GLenum type, const GLuint *value);
}  // namespace gl

#endif  // LIBANGLE_VALIDATION_GL3_AUTOGEN_H_
