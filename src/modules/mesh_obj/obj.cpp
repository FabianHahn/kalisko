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

extern "C" {
#include <glib.h>
}
#include <fstream>
#include <sstream>
#include <string>
#include <limits>
#include <vector>
#include "dll.h"
#include "api.h"
#include "obj.h"

/**
 * Reads a mesh from an obj file
 *
 * @param filename			the obj file to read from
 * @result					the parsed mesh of NULL on failure
 */
API Mesh *readMeshFileObj(const char *filename)
{
	std::ifstream file(filename);

	if(!file) {
		LOG_ERROR("Failed to open mesh obj file '%s'", filename);
		return NULL;
	}

	std::vector<float> positions;
	std::vector<MeshVertex> vertices;
	std::vector<MeshTriangle> triangles;

	while(file) {
		std::string type;
		file >> type;

		if(type == std::string("v")) {
			MeshVertex vertex;
			for(int i = 0; i < 3; i++) {
				file >> vertex.position[i];
			}

			for(int j = 0; j < 4; j++) { // obj meshes don't have vertex colors, so just set them all to red
				vertex.color[j] = j == 0 ? 1.0f : 0.0f;
			}

			vertices.push_back(vertex);
		} else if(type == std::string("f")) {
			MeshTriangle triangle;
			for(int i = 0; i < 3; i++) {
				std::string indices;
				file >> indices;

				char **parts = g_strsplit(indices.c_str(), "/", 0);
				int index = atoi(parts[0]);
				g_strfreev(parts);

				triangle.indices[i] = index;
			}
			triangles.push_back(triangle);
		}

		// Jump to next line
		file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}

	Mesh *mesh = $(Mesh *, mesh, createMesh)(vertices.size(), triangles.size());

	for(unsigned int i = 0; i < vertices.size(); i++) {
		mesh->vertices[i] = vertices[i];
	}

	for(unsigned int i = 0; i < triangles.size(); i++) {
		mesh->triangles[i] = triangles[i];
	}

	$(void, mesh, generateMeshNormals)(mesh);

	return mesh;
}
