#pragma once

#include <glew.h>
#include <glm/glm.hpp>
#include <DSLoaders\DDSLoader.h>

#include <fstream>

void initAABB(GLuint *vao, GLfloat side_len = 0.5f)
{

	float AABB[] = {
		//+Z
		// vertex position (x,y,z)
		-side_len, -side_len,  side_len,// v0
		 side_len, -side_len,  side_len,// v1
		 side_len,  side_len,  side_len,// v2  (MAX)

		-side_len, -side_len,  side_len,// v0
		 side_len,  side_len,  side_len,// v2  (MAX)
		-side_len,  side_len,  side_len,// v3
		//+X
		 side_len, -side_len,  side_len,// v1
		 side_len, -side_len, -side_len,// v4
		 side_len,  side_len, -side_len,// v7

		 side_len, -side_len,  side_len,// v1
		 side_len,  side_len, -side_len,// v7
		 side_len,  side_len,  side_len,// v2  (MAX)
		//+Y
		-side_len,  side_len,  side_len,// v3
		 side_len,  side_len,  side_len,// v2  (MAX)
		 side_len,  side_len, -side_len,// v7

		-side_len,  side_len,  side_len,// v3
		 side_len,  side_len, -side_len,// v7
		-side_len,  side_len, -side_len,// v6
		//-Z
		 side_len, -side_len, -side_len,// v4
		-side_len, -side_len, -side_len,// v5 (min)
		-side_len,  side_len, -side_len,// v6

		 side_len, -side_len, -side_len,// v4
		-side_len,  side_len, -side_len,// v6
		 side_len,  side_len, -side_len,// v7
		//-X
		-side_len, -side_len, -side_len,// v5 (min)
		-side_len, -side_len,  side_len,// v0
		-side_len,  side_len,  side_len,// v3

		-side_len, -side_len, -side_len,// v5 (min)
		-side_len,  side_len,  side_len,// v3
		-side_len,  side_len, -side_len,// v6
		//-Y
		-side_len, -side_len,  side_len,// v0
		-side_len, -side_len, -side_len,// v5 (min)
		 side_len, -side_len, -side_len,// v4

		-side_len, -side_len,  side_len,// v0
		 side_len, -side_len, -side_len,// v4
		 side_len, -side_len,  side_len// v1
	};
	GLuint vbo = 0;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(AABB), (void*)AABB, GL_STATIC_DRAW);

	glGenVertexArrays(1, vao);
	glBindVertexArray(*vao);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0); // unbind
}

GLuint initVol(std::string fileName) {
	GLuint tex = 0;
	DDSLoader volLoader;
	uint8_t* voldata = volLoader.loadFile(fileName);
	unsigned volWidth, volHeight, volDepth, volBytesPerVal;
	volLoader.getDimensions(volWidth, volHeight, volDepth, volBytesPerVal);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // for 8-bit volume data

	glEnable(GL_TEXTURE_3D);
	glGenTextures(1, &tex); //bind to the new texture ID 
	glBindTexture(GL_TEXTURE_3D, tex); //store the texture data for OpenGL use 
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, volWidth, volHeight, volDepth, 0, GL_RED, GL_UNSIGNED_BYTE, (const void*)voldata);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 0);

	return tex;
}

struct TFParams {
	float R, G, B, A;
	float peak[3] = { 1.5f, 3.0f, 4.5f };
	float range[3] = { 0.5f, 1.0f, 0.5f };
	float slope[3] = { 3.0f, 4.0f, 3.0f };

	void setPeak(float p1, float p2, float p3) {
		peak[0] = p1; peak[1] = p2; peak[2] = p3;
	}
	void setPeakRange(float p1, float p2, float p3) {
		range[0] = p1; range[1] = p2; range[2] = p3;
	}
	void setRange(float p1, float p2, float p3) {
		slope[0] = p1; slope[1] = p2; slope[2] = p3;
	}
	void transferFunc(float scalar) {
		A = (exp(scalar) - 1.0) / (exp(1.0) - 1.0);
		const float dx = 0; //Regions on either end of the color map
		//f is already normalized to [0, 1]
		scalar = (6 - 2 * dx) * scalar + dx; //scale f to [dx, 6 - dx]
		R = std::max(0.0f, (slope[0] - fabs(scalar - (peak[0] - range[0])) - fabs(scalar - (peak[0] + range[0]))) / 2);
		G = std::max(0.0f, (slope[1] - fabs(scalar - (peak[1] - range[1])) - fabs(scalar - (peak[1] + range[1]))) / 2);
		B = std::max(0.0f, (slope[2] - fabs(scalar - (peak[2] - range[2])) - fabs(scalar - (peak[0] - range[2]))) / 2);
	}
};

GLuint initTF(int numSamples, TFParams &tfDef) {
	
	GLuint tex = 0;
	float* LUT = (float*)malloc(numSamples * 4 * sizeof(float));

	for (int i = 0; i < numSamples; i++) {
		tfDef.transferFunc((float)(i + 1) / numSamples);
		LUT[i * 4] = tfDef.R;
		LUT[i * 4 + 1] = tfDef.G;
		LUT[i * 4 + 2] = tfDef.B;
		LUT[i * 4 + 3] = tfDef.A;
	}

	glEnable(GL_TEXTURE_1D);
	glGenTextures(1, &tex); // get texture id
	glBindTexture(GL_TEXTURE_1D, tex);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA16F, numSamples, 0, GL_RGBA, GL_FLOAT, (void*)LUT);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // linear interpolation when too small
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // linear interpolation when too big
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

	//Free allocated memory
	free(LUT);

	return tex;

}

std::string g_config;
TFParams g_userDef;
std::string g_volFile;

void parseConfig() {

	//userDef.load(g_ini);
	std::ifstream infile(g_config);
	if (infile.is_open()) {
		std::string line;
		while (getline(infile, line)) {
			line.erase(std::remove_if(line.begin(), line.end(), isspace),
				line.end());
			if (line[0] == '#' || line.empty())
				continue;
			try {
				auto delimiterPos = line.find("=");
				auto name = line.substr(0, delimiterPos);
				auto value = line.substr(delimiterPos + 1);
				
				switch (name[0]) {
				case 'p':
					g_userDef.peak[(int)name[1]] = atof(value.c_str());
					break;
				case 'r':
					g_userDef.range[(int)name[1]] = atof(value.c_str());
					break;
				case 's':
					g_userDef.slope[(int)name[1]] = atof(value.c_str());
					break;
				case 'v':
					g_volFile = value;
					break;
				default:
					break;
				}

			}
			catch (int e){
				continue;
			}
			
		}
		infile.close();
	}
	else {
		g_volFile = "C:\\Users\\Mukund\\Documents\\UW\\GPU\\GPUSciVis_Win20\\Final_Project\\Box.pvm";
	}
}