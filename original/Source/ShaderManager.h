#pragma once

#include <GL/glew.h>        // GLEW library

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class ShaderManager
{
public:
	unsigned int m_programID;
	
	GLuint LoadShaders(
		const char* vertex_file_path, 
		const char* fragment_file_path);

	// activate the shader
	// ------------------------------------------------------------------------
	inline void use()
	{
		glUseProgram(m_programID);
	}

	// utility uniform functions
	// ------------------------------------------------------------------------
	inline void setBoolValue(const std::string &name, bool value) const
	{
		glUniform1i(glGetUniformLocation(m_programID, name.c_str()), (int)value);
	}

	// ------------------------------------------------------------------------
	inline void setIntValue(const std::string &name, int value) const
	{
		glUniform1i(glGetUniformLocation(m_programID, name.c_str()), value);
	}

	// ------------------------------------------------------------------------
	inline void setFloatValue(const std::string &name, float value) const
	{
		glUniform1f(glGetUniformLocation(m_programID, name.c_str()), value);
	}

	// ------------------------------------------------------------------------
	inline void setVec2Value(const std::string &name, const glm::vec2 &value) const
	{
		glUniform2fv(glGetUniformLocation(m_programID, name.c_str()), 1, &value[0]);
	}

	inline void setVec2Value(const std::string &name, float x, float y) const
	{
		glUniform2f(glGetUniformLocation(m_programID, name.c_str()), x, y);
	}

	// ------------------------------------------------------------------------
	inline void setVec3Value(const std::string &name, const glm::vec3 &value) const
	{
		glUniform3fv(glGetUniformLocation(m_programID, name.c_str()), 1, &value[0]);
	}
	inline void setVec3Value(const std::string &name, float x, float y, float z) const
	{
		glUniform3f(glGetUniformLocation(m_programID, name.c_str()), x, y, z);
	}

	// ------------------------------------------------------------------------
	inline void setVec4Value(const std::string &name, const glm::vec4 &value) const
	{
		glUniform4fv(glGetUniformLocation(m_programID, name.c_str()), 1, &value[0]);
	}
	inline void setVec4Value(const std::string &name, float x, float y, float z, float w)
	{
		glUniform4f(glGetUniformLocation(m_programID, name.c_str()), x, y, z, w);
	}

	// ------------------------------------------------------------------------
	inline void setMat2Value(const std::string &name, const glm::mat2 &mat) const
	{
		glUniformMatrix2fv(glGetUniformLocation(m_programID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}

	// ------------------------------------------------------------------------
	inline void setMat3Value(const std::string &name, const glm::mat3 &mat) const
	{
		glUniformMatrix3fv(glGetUniformLocation(m_programID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}

	// ------------------------------------------------------------------------
	inline void setMat4Value(const std::string &name, const glm::mat4 &mat) const
	{
		glUniformMatrix4fv(glGetUniformLocation(m_programID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
	}

	// ------------------------------------------------------------------------
	inline void setSampler2DValue(const std::string& name, const int &value) const
	{
		glUniform1i(glGetUniformLocation(m_programID, name.c_str()), value);
	}
};