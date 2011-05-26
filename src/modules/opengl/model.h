/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2009, Kalisko Project Leaders
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *     @li Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *     @li Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 *       in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef OPENGL_MODEL_H
#define OPENGL_MODEL_H

#include "modules/linalg/Matrix.h"
#include "modules/linalg/Vector.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "primitive.h"

API void initOpenGLModels();
API void freeOpenGLModels();
API bool createOpenGLModel(const char *name, OpenGLPrimitive *primitive);
API bool deleteOpenGLModel(const char *name);
API OpenGLPrimitive *getOpenGLModelPrimitive(const char *name);
API bool attachOpenGLModelMaterial(const char *model_name, const char *material_name);
API bool setOpenGLModelTranslation(const char *model_name, Vector *translation);
API bool setOpenGLModelRotationX(const char *model_name, double rotation);
API bool setOpenGLModelRotationY(const char *model_name, double rotation);
API bool setOpenGLModelRotationZ(const char *model_name, double rotation);
API bool setOpenGLModelScaleX(const char *model_name, double scale);
API bool setOpenGLModelScaleY(const char *model_name, double scale);
API bool setOpenGLModelScaleZ(const char *model_name, double scale);
API void drawOpenGLModels();
API void updateOpenGLModels(double dt);

#ifdef __cplusplus
}
#endif

#endif
