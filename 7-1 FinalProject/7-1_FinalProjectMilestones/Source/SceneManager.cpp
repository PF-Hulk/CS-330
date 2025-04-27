///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ================
// This file contains the implementation of the `SceneManager` class, which is 
// responsible for managing the preparation and rendering of 3D scenes. It 
// handles textures, materials, lighting configurations, and object rendering.
//
// AUTHOR: Brian Battersby
// INSTITUTION: Southern New Hampshire University (SNHU)
// COURSE: CS-330 Computational Graphics and Visualization
//
// INITIAL VERSION: November 1, 2023
// LAST REVISED: December 1, 2024
//
// RESPONSIBILITIES:
// - Load, bind, and manage textures in OpenGL.
// - Define materials and lighting properties for 3D objects.
// - Manage transformations and shader configurations.
// - Render complex 3D scenes using basic meshes.
//
// NOTE: This implementation leverages external libraries like `stb_image` for 
// texture loading and GLM for matrix and vector operations.
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/
void SceneManager::LoadSceneTextures()
{
	// Load Textures
	// Texture for ESD Mat
	CreateGLTexture("textures/esdmat.png", "esdmat");
	// Texture for L901 body
	CreateGLTexture("textures/powdercoated.jpg", "powdercoated");
	// Texture for L901 copper
	CreateGLTexture("textures/brushedcopper.jpg", "brushedcopper");
	// Texture for c234 body
	CreateGLTexture("textures/brushedmetal.jpg", "brushedmetal");
	// Texture for c234 top
	CreateGLTexture("textures/brushedmetaltop.jpg", "brushedmetaltop");
	// Texture for Black Plastic
	CreateGLTexture("textures/casing.jpg", "casing");
	// Texture for Copper
	CreateGLTexture("textures/copper.png", "copper");
	// Texture for PCB
	CreateGLTexture("textures/pcba.png", "pcba");
	// Texture for Sodler Fillets
	CreateGLTexture("textures/solder.png", "solder");
	// Texture for U902 Black Plastic
	CreateGLTexture("textures/casingu902.jpg", "casingu902");
	// Texture for Leads and Pads
	CreateGLTexture("textures/aluminum.png", "aluminum");
	// After the texture image data is loaded, bind the loaded textures to texture slots.
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for the objects in the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	// FR4: Composite (matte, low reflectivity)
	{
		OBJECT_MATERIAL fr4Material;
		fr4Material.diffuseColor = glm::vec3(0.35f, 0.45f, 0.30f);
		fr4Material.specularColor = glm::vec3(0.05f, 0.05f, 0.05f);
		fr4Material.shininess = 2.0f;
		fr4Material.tag = "fr4Material";
		m_objectMaterials.push_back(fr4Material);
	}

	// Solder: A shiny, silvery metal
	{
		OBJECT_MATERIAL solderMat;
		solderMat.diffuseColor = glm::vec3(0.70f, 0.70f, 0.70f);
		solderMat.specularColor = glm::vec3(0.90f, 0.90f, 0.90f);
		solderMat.shininess = 16.0f;
		solderMat.tag = "solderMaterial";
		m_objectMaterials.push_back(solderMat);
	}

	// Copper: Warm, reddish‑brown metallic
	{
		OBJECT_MATERIAL copperMat;
		copperMat.diffuseColor = glm::vec3(0.70f, 0.40f, 0.30f);
		copperMat.specularColor = glm::vec3(0.80f, 0.50f, 0.40f);
		copperMat.shininess = 12.0f;
		copperMat.tag = "copperMaterial";
		m_objectMaterials.push_back(copperMat);
	}

	// Aluminum: re‑tuned for a crisper metallic highlight
	{
		OBJECT_MATERIAL aluminumMat;
		aluminumMat.diffuseColor = glm::vec3(0.65f, 0.65f, 0.65f);
		aluminumMat.specularColor = glm::vec3(0.90f, 0.90f, 0.90f);
		aluminumMat.shininess = 32.0f;
		aluminumMat.tag = "aluminumMaterial";
		m_objectMaterials.push_back(aluminumMat);
	}

	// ESD Mat: Matte but slightly reflective
	{
		OBJECT_MATERIAL planeMat;
		planeMat.diffuseColor = glm::vec3(0.20f, 0.30f, 0.35f);
		planeMat.specularColor = glm::vec3(0.20f, 0.20f, 0.20f);
		planeMat.shininess = 8.0f;
		planeMat.tag = "planeMaterial";
		m_objectMaterials.push_back(planeMat);
	}

	// Injection‑Molded Plastic: Dull matte finish
	{
		OBJECT_MATERIAL injectionPlastic;
		injectionPlastic.diffuseColor = glm::vec3(0.35f, 0.35f, 0.35f);
		injectionPlastic.specularColor = glm::vec3(0.10f, 0.10f, 0.10f);
		injectionPlastic.shininess = 4.0f;
		injectionPlastic.tag = "injectionPlasticMaterial";
		m_objectMaterials.push_back(injectionPlastic);
	}
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method sets up at least 2 lights so the entire scene
 *  is illuminated. One directional, one point with subtle color.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Enable custom lighting in the shader
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Directional Light
	// This defines the direction the light is coming from.
	m_pShaderManager->setVec3Value("directionalLight.direction", 0.0f, -0.707f, -0.707f);
	// Set the ambient component of the directional light.
	// Ambient light simulates the general, non-directional light.
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.4f, 0.4f, 0.4f);
	// Set the diffuse component of the directional light.
	// Diffuse lighting gives the surfaces a soft, directional brightness based on the light direction.
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 1.0f, 1.0f, 1.0f);
	// Set the specular component of the directional light.
	// Specular lighting creates the shiny, mirror-like highlights on surfaces.
	m_pShaderManager->setVec3Value("directionalLight.specular", 1.0f, 1.0f, 1.0f);
	// Activate the directional light so it is used in the scene.
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// Point Light (slightly warm tint)
	// Set the position of the first point light in the scene.
	// The position determines where the point light is located.
	m_pShaderManager->setVec3Value("pointLights[0].position", 2.0f, 3.0f, 2.0f);
	// Set the ambient component of the point light.
	// This defines the low-level, omnidirectional light emitted by the point light.
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.1f, 0.05f, 0.05f);
	// Set the diffuse component of the point light.
	// This gives the main color and brightness to the surfaces affected by this light.
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.8f, 0.4f, 0.3f);
	// Set the specular component of the point light.
	// This controls the intensity of the shiny highlights on reflective surfaces.
	m_pShaderManager->setVec3Value("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
	// Activate the point light so that it is used in the scene lighting.
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);
}
/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the textures for the 3D scene
	LoadSceneTextures();
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	// Define materials so objects can reflect light
	DefineObjectMaterials();

	// Setup lighting 
	SetupSceneLights();

	// load the shapes
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadCylinderMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// Floor Plane
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign a plane material(reflects light)
	SetShaderMaterial("planeMaterial");
	// Bind ESD Mat texture to mesh
	SetShaderTexture("esdmat");
	SetTextureUVScale(5.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/******************************************************************/

	/******************************************************************/
	/********************** Circuit Board Model ***********************/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(18.0f, 0.16f, 18.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.05f, 0.1f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// Assign the FR4 composite material (matte, low reflectivity)
	SetShaderMaterial("fr4Material");
	// Bind PCB texture to mesh
	SetShaderTexture("pcba");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	/************************* L903 Model *****************************/
	/******************************************************************/
	// L903 Box Mesh (1/4)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.985f, 0.75f, 2.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.3f, 0.3f, -2.15f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the injection plastic material (moderate shine)
	SetShaderMaterial("injectionPlasticMaterial");
	// Bind Black Plastic texture to mesh
	SetShaderTexture("casing");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// L903 Box Mesh (2/4)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.985f, 0.75f, 2.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.3f, 0.3f, -2.15f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the injection plastic material (moderate shine)
	SetShaderMaterial("injectionPlasticMaterial");
	// Bind Black Plastic texture to mesh
	SetShaderTexture("casing");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// L903 Box Mesh (3/4)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.985f, 0.75f, 2.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.3f, 0.3f, -2.15f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the injection plastic material (moderate shine)
	SetShaderMaterial("injectionPlasticMaterial");
	// Bind Black Plastic texture to mesh
	SetShaderTexture("casing");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// L903 Box Mesh (4/4)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.985f, 0.75f, 2.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 135.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.3f, 0.3f, -2.15f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the injection plastic material (moderate shine)
	SetShaderMaterial("injectionPlasticMaterial");
	// Bind Black Plastic texture to mesh
	SetShaderTexture("casing");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// L903 Pad 1 Mesh (Pin 1 Solder Pad)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.8f, 0.3f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.3f, 0.19f, -3.4f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// L903 Fillet 1 Mesh (Pin/Pad 1 Solder Fillet)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.38f, 0.125f, 0.125f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.3f, 0.2f, -3.35f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("solder");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/******************************************************************/

	/******************************************************************/
	// L903 Lead 1 Mesh (Pin 1 Lead)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.69f, 0.3f, 0.075f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.3f, 0.33f, -3.35f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// L903 Copper Wire to Lead 1
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.1f, 0.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.575f, 0.55f, -3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the copper material (reddish metal)
	SetShaderMaterial("copperMaterial");
	// Bind Copper texture to mesh
	SetShaderTexture("copper");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/******************************************************************/

	/******************************************************************/
	// L903 Pad 2 Mesh (Pin 2 Solder Pad)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.8f, 0.3f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.3f, 0.19f, -0.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// L903 Lead 2 Mesh (Pin 2 Lead)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.69f, 0.3f, 0.075f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.3f, 0.33f, -0.95f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// L903 Fillet 2 Mesh (Pin/Pad 2 Solder Fillet)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.38f, 0.125f, 0.125f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(1.3f, 0.2f, -0.95f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("solder");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/******************************************************************/

	/******************************************************************/
	// L903 Copper Wire to Lead 2
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.1f, 0.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 0.55f, -1.275f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the copper material (reddish metal)
	SetShaderMaterial("copperMaterial");
	// Bind Copper texture to mesh
	SetShaderTexture("copper");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/******************************************************************/

	/******************************************************************/
	/************************* C234 Model *****************************/
	/******************************************************************/
	/******************************************************************/
	// C234 Box Mesh Base Polarity side (1/3)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.025f, 0.25f, 2.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-7.0f, 0.24f, 4.625f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the injection plastic material (moderate shine)
	SetShaderMaterial("injectionPlasticMaterial");
	// Bind Black Plastic texture to mesh
	SetShaderTexture("casing");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// C234 Box Mesh Base Polarity side (2/3)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.72f, 0.25f, 0.75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-6.5f, 0.24f, 5.1315f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the injection plastic material (moderate shine)
	SetShaderMaterial("injectionPlasticMaterial");
	// Bind Black Plastic texture to mesh
	SetShaderTexture("casing");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// C234 Box Mesh Base Polarity side (3/3)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.72f, 0.25f, 0.75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 135.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-7.5f, 0.24f, 5.1315f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the injection plastic material (moderate shine)
	SetShaderMaterial("injectionPlasticMaterial");
	// Bind Black Plastic texture to mesh
	SetShaderTexture("casing");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// C234 Box Mesh Base non-polarity side
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.041f, 0.25f, 1.535f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-7.0f, 0.24f, 4.375f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the injection plastic material (moderate shine)
	SetShaderMaterial("injectionPlasticMaterial");
	// Bind Black Plastic texture to mesh
	SetShaderTexture("casing");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// C234 Box Mesh pin 1 Solder Pad
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5, 0.05f, 0.35);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-7.0f, 0.19f, 5.825f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Black Plastic texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// C234 Box Mesh pin 1 Lead
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.175, 0.1f, 0.25);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-7.0f, 0.24f, 5.75f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Black Plastic texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// C234 Box Mesh Pin/Pad 1 Fillet 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2, 0.075f, 0.35);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-7.0f, 0.2f, 5.625f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Black Plastic texture to mesh
	SetShaderTexture("solder");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/******************************************************************/


	/******************************************************************/
	// C234 Box Mesh pin 2 Solder Pad
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5, 0.05f, 0.35);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-7.0f, 0.19f, 3.425f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Black Plastic texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// C234 Box Mesh pin 2 Lead
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.175, 0.1f, 0.25);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-7.0f, 0.24f, 3.525f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Black Plastic texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// C234 Box Mesh Pin/Pad 2 Fillet 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.2, 0.075f, 0.35);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-7.0f, 0.2f, 3.625f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Black Plastic texture to mesh
	SetShaderTexture("solder");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/******************************************************************/

	/******************************************************************/
	// C234 Electrolytic Capacitor Cylinder Body
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 1.3f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-7.0f, 0.19f, 4.625f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the aluminum material (so it looks like a soft silver metal)
	SetShaderMaterial("aluminumMaterial");
	// Bind brushedmetal texture to mesh
	SetShaderTexture("brushedmetal");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/******************************************************************/
	// C234 Electrolytic Capacitor Cylinder Top Curve
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.77f, 0.77f, 0.77f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-7.0f, 1.49f, 4.625f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the aluminum material (so it looks like a soft silver metal)
	SetShaderMaterial("aluminumMaterial");
	// Bind brushedmetal texture to mesh
	SetShaderTexture("brushedmetal");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/******************************************************************/

	/******************************************************************/
	// C234 Electrolytic Capacitor Cylinder Top
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.78f, 0.1f, 0.78f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-7.0f, 1.619f, 4.625f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the aluminum material (so it looks like a soft silver metal)
	SetShaderMaterial("aluminumMaterial");
	// Bind brushedmetal texture to mesh
	SetShaderTexture("brushedmetaltop");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/******************************************************************/
	/************************* L901 Model *****************************/
	/************** Bottom terminated (No solder Fillet) **************/
	/******************************************************************/
	// L901 Box Mesh (1/2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.4f, 1.0f, 2.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.05f, 0.3f, -1.3f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the injection plastic material (moderate shine)
	SetShaderMaterial("injectionPlasticMaterial");
	// Bind Black Plastic texture to mesh
	SetShaderTexture("casing");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// L901 Box Mesh (2/2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.25f, 0.25f, 2.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.05f, 0.75f, -1.3f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the injection plastic material (moderate shine)
	SetShaderMaterial("injectionPlasticMaterial");
	// Bind Black Plastic texture to mesh
	SetShaderTexture("powdercoated");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// L901 Copper strand (1/2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.15f, 1.25f, 0.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.65f, 0.65f, -0.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the injection plastic material (moderate shine)
	SetShaderMaterial("copperMaterial");
	// Bind Black Plastic texture to mesh
	SetShaderTexture("brushedcopper");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/******************************************************************/
	// L901 Copper strand (2/2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.15f, 1.25f, 0.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.65f, 0.65f, -2.4f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the injection plastic material (moderate shine)
	SetShaderMaterial("copperMaterial");
	// Bind Black Plastic texture to mesh
	SetShaderTexture("brushedcopper");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/******************************************************************/
	// L901 Pad 1 Mesh (Pin 1 Solder Pad)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.025f, 1.75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.85f, 0.19f, -1.3f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// L901 Pad 2 Mesh (Pin 2 Solder Pad)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.025f, 1.75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.25f, 0.19f, -1.3f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	/************************* U902 Model *****************************/
	/********************** IC with 8 leads ***************************/
	/******************************************************************/
	// U902 Box Mesh Top
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.6f, 0.01f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.75f, 0.42f, 3.75f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the injection plastic material (moderate shine)
	SetShaderMaterial("injectionPlasticMaterial");
	// Bind Black Plastic texture to mesh
	SetShaderTexture("casingu902");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Box Mesh Bottom
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.6f, 0.24f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.75f, 0.3f, 3.75f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the injection plastic material (moderate shine)
	SetShaderMaterial("injectionPlasticMaterial");
	// Bind Black Plastic texture to mesh
	SetShaderTexture("casing");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pin 1  Mesh (Pin 1 Lead, 1/2) 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.015f, 0.35f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 315.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.267f, 0.15f, 3.36f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pin 2  Mesh (Pin 2 Lead, 1/2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.015f, 0.35f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 315.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.267f, 0.15f, 3.61f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pin 3  Mesh (Pin 3 Lead, 1/2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.015f, 0.35f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 315.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.267f, 0.15f, 3.87f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pin 4  Mesh (Pin 4 Lead, 1/2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.015f, 0.35f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 315.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.267f, 0.15f, 4.13f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pin 5  Mesh (Pin 5 Lead, 1/2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.015f, 0.35f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = 45.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.225f, 0.15f, 3.36f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pin 6  Mesh (Pin 6 Lead, 1/2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.015f, 0.35f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = 45.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.225f, 0.15f, 3.61f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pin 7  Mesh (Pin 7 Lead, 1/2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.015f, 0.35f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = 45.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.225f, 0.15f, 3.87f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pin 8  Mesh (Pin 8 Lead, 1/2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.015f, 0.35f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = 45.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.225f, 0.15f, 4.13f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pin 1  Mesh (Pin 1 Lead, 2/2) 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.015f, 0.15f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 270.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.357f, 0.21f, 3.36f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pin 2  Mesh (Pin 2 Lead, 2/2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.015f, 0.15f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 270.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.357f, 0.21f, 3.61f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pin 3  Mesh (Pin 3 Lead, 2/2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.015f, 0.15f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 270.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.357f, 0.21f, 3.87f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pin 4  Mesh (Pin 4 Lead, 2/2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.015f, 0.15f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 270.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.357f, 0.21f, 4.13f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pin 5  Mesh (Pin 5 Lead, 2/2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.015f, 0.15f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.135f, 0.21f, 3.36f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pin 6  Mesh (Pin 6 Lead, 2/2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.015f, 0.15f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.135f, 0.21f, 3.61f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pin 7  Mesh (Pin 7 Lead, 2/2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.015f, 0.15f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.135f, 0.21f, 3.87f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pin 8  Mesh (Pin 8 Lead, 2/2)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.015f, 0.15f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.135f, 0.21f, 4.13f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pin 1  Mesh (Pin 1 Fillet)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.025f, 0.17f, 0.06f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 270.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.2795f, 0.2f, 3.36f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("solder");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pin 2  Mesh (Pin 2 Fillet)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.025f, 0.17f, 0.06f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 270.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.2795f, 0.2f, 3.61f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("solder");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pin 3  Mesh (Pin 3 Fillet)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.025f, 0.17f, 0.06f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 270.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.2795f, 0.20f, 3.87f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("solder");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/******************************************************************/

	/******************************************************************/
// U902 Pin 4  Mesh (Pin 4 Fillet)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.025f, 0.17f, 0.06f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 270.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.2795f, 0.20f, 4.13f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("solder");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pin 5  Mesh (Pin 5 Fillet)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.025f, 0.17f, 0.06f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.2125f, 0.20f, 3.36f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("solder");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pin 6  Mesh (Pin 6 Fillet)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.025f, 0.17f, 0.06f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.2125f, 0.20f, 3.61f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("solder");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pin 7  Mesh (Pin 7 Fillet)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.025f, 0.17f, 0.06f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.2125f, 0.20f, 3.87f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("solder");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pin 8  Mesh (Pin 8 Fillet)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.025f, 0.17f, 0.06f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.2125f, 0.20f, 4.13f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("solder");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	/******************************************************************/






	/******************************************************************/
	// U902 Pad 1  Mesh (Pin 1 Solder Pad)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.35f, 0.025f, 0.125f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.275f, 0.19f, 3.36f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/
	
	/******************************************************************/
	// U902 Pad 2 Mesh (Pin 2 Solder Pad)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.35f, 0.025f, 0.125f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.275f, 0.19f, 3.61f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pad 3 Mesh (Pin 3 Solder Pad)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.35f, 0.025f, 0.125f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.275f, 0.19f, 3.87f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pad 4 Mesh (Pin 4 Solder Pad)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.35f, 0.025f, 0.125f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.275f, 0.19f, 4.13f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pad 5 Mesh (Pin 5 Solder Pad)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.35f, 0.025f, 0.125f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.217f, 0.19f, 3.36f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pad 6 Mesh (Pin 6 Solder Pad)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.35f, 0.025f, 0.125f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.217f, 0.19f, 3.61f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pad 7 Mesh (Pin 7 Solder Pad)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.35f, 0.025f, 0.125f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.217f, 0.19f, 3.87f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// U902 Pad 8 Mesh (Pin 8 Solder Pad)
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.35f, 0.025f, 0.125f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.217f, 0.19f, 4.13f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Assign the solder material (shiny silver)
	SetShaderMaterial("solderMaterial");
	// Bind Solder texture to mesh
	SetShaderTexture("aluminum");
	SetTextureUVScale(1.0, 1.0);
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/
}