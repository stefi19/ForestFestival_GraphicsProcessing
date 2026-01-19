#include "Mesh.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
namespace gps {

	// opt-in debug flag defined in main.cpp
	extern bool g_debugPrintMeshInfo;

	/* Mesh Constructor */
	Mesh::Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices, std::vector<Texture> textures, Material material) {

		this->vertices = vertices;
		this->indices = indices;
		this->textures = textures;
		this->material = material;

		this->setupMesh();
	}

	Buffers Mesh::getBuffers() {
	    return this->buffers;
	}

	/* Mesh drawing function - also applies associated textures */
	void Mesh::Draw(gps::Shader shader, int flatShading) {

		shader.useShaderProgram();

		// ensure flatShading uniform is set after the shader program is active
		GLint flatLoc = glGetUniformLocation(shader.shaderProgram, "flatShading");
		if (flatLoc != -1) {
			glUniform1i(flatLoc, flatShading);
		}

		// set material uniforms (fallback when no texture)
		GLint matDiffLoc = glGetUniformLocation(shader.shaderProgram, "materialDiffuse");
		if (matDiffLoc != -1) {
			glUniform3fv(matDiffLoc, 1, glm::value_ptr(this->material.diffuse));
		}

		// detect presence of diffuse/specular textures
		bool hasDiffuse = false;
		bool hasSpecular = false;
		for (GLuint i = 0; i < textures.size(); i++) {
			if (this->textures[i].type == "diffuseTexture") hasDiffuse = true;
			if (this->textures[i].type == "specularTexture") hasSpecular = true;
		}

		if (g_debugPrintMeshInfo) {
			std::cout << "[MeshDebug] textures=" << textures.size() << " hasDiffuse=" << hasDiffuse
				<< " materialDiffuse=(" << material.diffuse.r << "," << material.diffuse.g << "," << material.diffuse.b << ")"
				<< std::endl;
		}
		GLint hasDiffLoc = glGetUniformLocation(shader.shaderProgram, "hasDiffuseTexture");
		if (hasDiffLoc != -1) glUniform1i(hasDiffLoc, hasDiffuse ? 1 : 0);
		GLint hasSpecLoc = glGetUniformLocation(shader.shaderProgram, "hasSpecularTexture");
		if (hasSpecLoc != -1) glUniform1i(hasSpecLoc, hasSpecular ? 1 : 0);

		// bind textures
		for (GLuint i = 0; i < textures.size(); i++) {
			glActiveTexture(GL_TEXTURE0 + i);
			glUniform1i(glGetUniformLocation(shader.shaderProgram, this->textures[i].type.c_str()), i);
			glBindTexture(GL_TEXTURE_2D, this->textures[i].id);
		}

		glBindVertexArray(this->buffers.VAO);
		glDrawElements(GL_TRIANGLES, (GLsizei)this->indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

        for(GLuint i = 0; i < this->textures.size(); i++) {

            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

    }

	// Initializes all the buffer objects/arrays
	void Mesh::setupMesh() {

		// Create buffers/arrays
		glGenVertexArrays(1, &this->buffers.VAO);
		glGenBuffers(1, &this->buffers.VBO);
		glGenBuffers(1, &this->buffers.EBO);

		glBindVertexArray(this->buffers.VAO);
		// Load data into vertex buffers
		glBindBuffer(GL_ARRAY_BUFFER, this->buffers.VBO);
		glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(Vertex), &this->vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->buffers.EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(GLuint), &this->indices[0], GL_STATIC_DRAW);

		// Set the vertex attribute pointers
		// Vertex Positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
		// Vertex Normals
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, Normal));
		// Vertex Texture Coords
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, TexCoords));

		glBindVertexArray(0);
	}
}
