/* Shared Use License: This file is owned by Derivative Inc. (Derivative)
* and can only be used, and/or modified for use, in conjunction with
* Derivative's TouchDesigner software, and only if you are a licensee who has
* accepted Derivative's TouchDesigner license or assignment agreement
* (which also govern the use of this file). You may share or redistribute
* a modified version of this file provided the following conditions are met:
*
* 1. The shared file or redistribution must retain the information set out
* above and this list of conditions.
* 2. Derivative's name (Derivative Inc.) or its trademarks may not be used
* to endorse or promote products derived from this file without specific
* prior written permission from Derivative.
*/

#include "EeveeBloomTOP.h"

// These functions are basic C function, which the DLL loader can find
// much easier than finding a C++ Class.
// The DLLEXPORT prefix is needed so the compile exports these functions from the .dll
// you are creating
extern "C"
{
DLLEXPORT
void FillTOPPluginInfo(TOP_PluginInfo *info)
{
	// This must always be set to this constant
	info->apiVersion = TOPCPlusPlusAPIVersion;

	// Change this to change the executeMode behavior of this plugin.
	info->executeMode = TOP_ExecuteMode::OpenGL_FBO;

	// The opType is the unique name for this TOP. It must start with a 
	// capital A-Z character, and all the following characters must lower case
	// or numbers (a-z, 0-9)
	info->customOPInfo.opType->setString("Eeveebloom");

	// The opLabel is the text that will show up in the OP Create Dialog
	info->customOPInfo.opLabel->setString("Eevee Bloom");

	// Will be turned into a 3 letter icon on the nodes
	info->customOPInfo.opIcon->setString("EBL");

	// Information about the author of this OP
	info->customOPInfo.authorName->setString("YumaTaesu");
	info->customOPInfo.authorEmail->setString("yuma.taesu@gmail.com");

	// This TOP works with 0 inputs
	info->customOPInfo.minInputs = 0;
	info->customOPInfo.maxInputs = 1;

}

DLLEXPORT
TOP_CPlusPlusBase* CreateTOPInstance(const OP_NodeInfo* info, TOP_Context *context)
{
	// Return a new instance of your class every time this is called.
	// It will be called once per TOP that is using the .dll

    // Note we can't do any OpenGL work during instantiation

	return new EeveeBloomTOP(info, context);
}

DLLEXPORT
void DestroyTOPInstance(TOP_CPlusPlusBase* instance, TOP_Context *context)
{
	// Delete the instance here, this will be called when
	// Touch is shutting down, when the TOP using that instance is deleted, or
	// if the TOP loads a different DLL

    // We do some OpenGL teardown on destruction, so ask the TOP_Context
    // to set up our OpenGL context
    context->beginGLCommands();

	delete (EeveeBloomTOP*)instance;

    context->endGLCommands();
}

};


EeveeBloomTOP::EeveeBloomTOP(const OP_NodeInfo* info, TOP_Context *context)
	: node_info_(info)
	, error_text_(nullptr)
	, is_setuped_(false)
	, pre_input_w_(2)
	, pre_input_h_(2)
	, downsample_first_shader_()
	, downsample_shader_()
	, upsample_shader_()
	, blit_shader_()
	, resolve_shader_()
	, blit_fbo_(nullptr)
	, bloom_iteration_len(0)
	, bloom_sample_scale(0.f)
{
	// GLEW is global static function pointers, only needs to be inited once,
	// and only on Windows.
	context->beginGLCommands();
	// Setup all our GL extensions using GLEW
	glewInit();
	context->endGLCommands();

}

EeveeBloomTOP::~EeveeBloomTOP()
{}

void EeveeBloomTOP::getGeneralInfo(TOP_GeneralInfo* ginfo, const OP_Inputs *inputs, void* reserved1) 
{
	// Setting cookEveryFrame to true causes the TOP to cook every frame even
	// if none of its inputs/parameters are changing. Set it to false if it
    // only needs to cook when inputs/parameters change.
	ginfo->cookEveryFrame = false;
}

bool EeveeBloomTOP::getOutputFormat(TOP_OutputFormat* format, const OP_Inputs *inputs, void* reserved1)
{
	// In this function we could assign variable values to 'format' to specify
	// the pixel format/resolution etc that we want to output to.
	// If we did that, we'd want to return true to tell the TOP to use the settings we've
	// specified.
	// In this example we'll return false and use the TOP's settings
	return false;
}

bool EeveeBloomTOP::getInfoDATSize(OP_InfoDATSize* infoSize, void* reserved1)
{
	infoSize->rows = 1;
	infoSize->cols = 1;
	//// Setting this to false means we'll be assigning values to the table
	//// one row at a time. True means we'll do it one column at a time.
	//infoSize->byColumn = false;
	return true;
}


void EeveeBloomTOP::getInfoDATEntries(int32_t index, int32_t nEntries, OP_InfoDATEntries* entries, void* reserved1)
{
	if (index == 0)
	{
		entries->values[0]->setString(error_text_);
	}
}

void EeveeBloomTOP::execute(TOP_OutputFormatSpecs* outputFormat, const OP_Inputs* inputs, TOP_Context* context, void* reserved1)
{

	if (!is_setuped_)
	{
		context->beginGLCommands();
		setupGL();
		context->endGLCommands();
	}

	if (inputs->getNumInputs() != 1)
		return;

	//// These functions must be called before
	//// beginGLCommands()/endGLCommands() block
	GLuint inputTextureIndex = inputs->getInputTOP(0)->textureIndex;
	const int w = inputs->getInputTOP(0)->width;
	const int h = inputs->getInputTOP(0)->height;

	if (parameters_.bNeedUpdate(inputs) || pre_input_w_ != w || pre_input_h_ != h)
	{
		resetupBloom(inputs, context);
	}
	pre_input_w_ = w;
	pre_input_h_ = h;
    context->beginGLCommands();
    {

		blit_fbo_->bind();
		glViewport(0, 0, w, h);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
			glUseProgram(blit_shader_.getName());
			
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, inputTextureIndex);
			glUniform1i(blit_shader_sourceBuffer, 0);
			glActiveTexture(GL_TEXTURE0);
			
			quad_.draw();

			glUseProgram(0);
		blit_fbo_->unbind();


		downsample_fbos_[0]->bind();
		glViewport(0, 0, downsample_fbos_[0]->getWidth(), downsample_fbos_[0]->getHeight());
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
			glUseProgram(downsample_first_shader_.getName());

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, blit_fbo_->getColorTextureId());
			glUniform1i(downsample_first_shader_sourceBuffer, 0);
			glActiveTexture(GL_TEXTURE0);
			
			quad_.draw();
			glUseProgram(0);
		downsample_fbos_[0]->unbind();

		std::shared_ptr<uma::FrameBuffer> last;
		last = downsample_fbos_[0];
		for (int i = 1; i < bloom_iteration_len; i++) 
		{
			downsample_fbos_[i]->bind();
			glViewport(0, 0, downsample_fbos_[i]->getWidth(), downsample_fbos_[i]->getHeight());
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			glUseProgram(downsample_shader_.getName());

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, last->getColorTextureId());
			glUniform1i(glGetUniformLocation(downsample_shader_.getName(), "sourceBuffer"), 0);
			glActiveTexture(GL_TEXTURE0);

			glUniform2f(downsample_shader_sourceBufferTexelSize, downsamp_texel_size[i - 1][0], downsamp_texel_size[i - 1][1]);
			
			quad_.draw();
			glUseProgram(0);

			last = downsample_fbos_[i];
			downsample_fbos_[i]->unbind();
		}


		for (int i = bloom_iteration_len - 2; i >= 0; i--) 
		{
			accum_fbos_[i]->bind();
			glViewport(0, 0, accum_fbos_[i]->getWidth(), accum_fbos_[i]->getHeight());
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glUseProgram(upsample_shader_.getName());

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, last->getColorTextureId());
			glUniform1i(glGetUniformLocation(upsample_shader_.getName(), "sourceBuffer"), 0);
			glActiveTexture(GL_TEXTURE0);

			glUniform2f(upsample_shader_sourceBufferTexelSize, downsamp_texel_size[i][0], downsamp_texel_size[i][1]);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, downsample_fbos_[i]->getColorTextureId());
			glUniform1i(glGetUniformLocation(upsample_shader_.getName(), "baseBuffer"), 1);
			glActiveTexture(GL_TEXTURE0);

			glUniform1f(upsample_shader_sampleScale, bloom_sample_scale);

			quad_.draw();
			glUseProgram(0);

			last = accum_fbos_[i];
			accum_fbos_[i]->unbind();
		}



		glBindFramebuffer(GL_FRAMEBUFFER, context->getFBOIndex()); //bind touchdesigner's final output fbo
		glViewport(0, 0, w, h);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(resolve_shader_.getName());

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, last->getColorTextureId());
		glUniform1i(glGetUniformLocation(resolve_shader_.getName(), "sourceBuffer"), 0);
		glActiveTexture(GL_TEXTURE0);

		glUniform2f(glGetUniformLocation(resolve_shader_.getName(), "sourceBufferTexelSize"), downsamp_texel_size[0][0], downsamp_texel_size[0][1]);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, inputTextureIndex);
		glUniform1i(glGetUniformLocation(resolve_shader_.getName(), "baseBuffer"), 1);
		glActiveTexture(GL_TEXTURE0);

		quad_.draw();
		glUseProgram(0);
    }
    context->endGLCommands();

	parameters_.lateUpdate(inputs);
}

void EeveeBloomTOP::getErrorString(OP_String *error, void* reserved1)
{
	error->setString(error_text_);
}

void EeveeBloomTOP::setupParameters(OP_ParameterManager* manager, void* reserved1)
{
	parameters_.setup(manager);
}


void EeveeBloomTOP::setupGL()
{
	std::string bloom_fs(shader::bloom_fs);

	std::string blit = "#version 410\n#define STEP_BLIT\n#define HIGH_QUALITY\n" + bloom_fs;
	blit_shader_.build(shader::passthrough_vs.c_str(), blit.c_str());

	std::string downsample_first = "#version 410\n#define STEP_DOWNSAMPLE\n#define HIGH_QUALITY\n" + bloom_fs;
	downsample_first_shader_.build(shader::passthrough_vs.c_str(), downsample_first.c_str());

	std::string downsample = "#version 410\n#define STEP_DOWNSAMPLE\n" + bloom_fs;
	downsample_shader_.build(shader::passthrough_vs.c_str(), downsample.c_str());

	std::string upsample = "#version 410\n#define STEP_UPSAMPLE\n#define HIGH_QUALITY\n" + bloom_fs;
	upsample_shader_.build(shader::passthrough_vs.c_str(), upsample.c_str());

	std::string resolve = "#version 410\n#define STEP_RESOLVE\n#define HIGH_QUALITY\n" + bloom_fs;
	resolve_shader_.build(shader::passthrough_vs.c_str(), resolve.c_str());

	blit_fbo_ = std::make_unique<uma::FrameBuffer>();

	quad_.create();

	is_setuped_ = true;
}

void EeveeBloomTOP::resetupBloom(const OP_Inputs* inputs, TOP_Context* context)
{
	/* Bloom */
	int blitsize[2], texsize[2];
	int w = inputs->getInputTOP(0)->width;
	int h = inputs->getInputTOP(0)->height;

	/* Blit Buffer */
	float source_texel_size[2];
	source_texel_size[0] = 1.0f / (float)w;
	source_texel_size[1] = 1.0f / (float)h;

	blitsize[0] = w;
	blitsize[1] = h;

	/* Parameters */
	const float threshold = parameters_.evalThreshold(inputs);
	const float knee = parameters_.evalKnee(inputs);
	const float intensity = parameters_.evalIntensity(inputs);
	const Color color = parameters_.evalBloomColor(inputs);
	const float radius = parameters_.evalRadius(inputs);
	const float bloom_clamp = parameters_.evalClamp(inputs);

	/* determine the iteration count */
	const float minDim = (float)std::min(blitsize[0], blitsize[1]);
	const float maxIter = (radius - 8.0f) + log(minDim) / log(2);
	const int maxIterInt =  bloom_iteration_len = (int)maxIter;

	bloom_iteration_len = std::min(std::max(bloom_iteration_len, 1), MAX_BLOOM_STEP);

	bloom_sample_scale = 0.5f + maxIter - (float)maxIterInt;

	float bloom_curve_threshold[4];
	bloom_curve_threshold[0] = threshold - knee;
	bloom_curve_threshold[1] = knee * 2.0f;
	bloom_curve_threshold[2] = 0.25f / std::max(1e-5f, knee);
	bloom_curve_threshold[3] = threshold;

	//mul_v3_v3fl(bloom_color, color, intensity);
	float bloom_color[3];
	bloom_color[0] = color.r * intensity;
	bloom_color[1] = color.g * intensity;
	bloom_color[2] = color.b * intensity;

	/* Downsample buffers */
	texsize[0] = blitsize[0];
	texsize[1] = blitsize[1];

	//copy_v2_v2_int(texsize, blitsize);

	context->beginGLCommands();

	blit_fbo_->allocateSingleColorTexture(w, h, GL_R11F_G11F_B10F, GL_RGB, GL_FLOAT);

	for (int i = 0; i < bloom_iteration_len; i++) 
	{
		texsize[0] /= 2;
		texsize[1] /= 2;

		texsize[0] = std::max(texsize[0], 2);
		texsize[1] = std::max(texsize[1], 2);

		downsamp_texel_size[i][0] = 1.0f / (float)texsize[0];
		downsamp_texel_size[i][1] = 1.0f / (float)texsize[1];

		auto fbo = std::make_unique<uma::FrameBuffer>();
		fbo->allocateSingleColorTexture(texsize[0], texsize[1], GL_R11F_G11F_B10F, GL_RGB, GL_FLOAT);
		downsample_fbos_[i] = std::move(fbo);
	}

	/* Upsample buffers */
	texsize[0] = blitsize[0];
	texsize[1] = blitsize[1];
	for (int i = 0; i < bloom_iteration_len - 1; i++) {
		texsize[0] /= 2;
		texsize[1] /= 2;

		texsize[0] = std::max(texsize[0], 2);
		texsize[1] = std::max(texsize[1], 2);

		auto fbo = std::make_unique<uma::FrameBuffer>();
		fbo->allocateSingleColorTexture(texsize[0], texsize[1], GL_R11F_G11F_B10F, GL_RGB, GL_FLOAT);
		accum_fbos_[i] = std::move(fbo);
	}

	glUseProgram(blit_shader_.getName());
	blit_shader_sourceBuffer = glGetUniformLocation(blit_shader_.getName(), "sourceBuffer");
	glUniform2f(glGetUniformLocation(blit_shader_.getName(), "sourceBufferTexelSize"), source_texel_size[0], source_texel_size[1]);
	glUniform4f(glGetUniformLocation(blit_shader_.getName(), "curveThreshold"), bloom_curve_threshold[0], bloom_curve_threshold[1], bloom_curve_threshold[2], bloom_curve_threshold[3]);
	glUniform1f(glGetUniformLocation(blit_shader_.getName(), "clampIntensity"), bloom_clamp);
	glUseProgram(0);

	glUseProgram(downsample_first_shader_.getName());
	downsample_first_shader_sourceBuffer = glGetUniformLocation(downsample_first_shader_.getName(), "sourceBuffer");
	glUniform2f(glGetUniformLocation(downsample_first_shader_.getName(), "sourceBufferTexelSize"), source_texel_size[0], source_texel_size[1]);
	glUseProgram(0);

	glUseProgram(downsample_shader_.getName());
	downsample_shader_sourceBufferTexelSize = glGetUniformLocation(downsample_shader_.getName(), "sourceBufferTexelSize");
	glUseProgram(0);

	glUseProgram(upsample_shader_.getName());
	upsample_shader_sourceBufferTexelSize = glGetUniformLocation(upsample_shader_.getName(), "sourceBufferTexelSize");
	upsample_shader_sampleScale = glGetUniformLocation(upsample_shader_.getName(), "sampleScale");
	glUseProgram(0);

	glUseProgram(resolve_shader_.getName());
	glUniform1f(glGetUniformLocation(resolve_shader_.getName(), "sampleScale"), bloom_sample_scale);
	glUniform3f(glGetUniformLocation(resolve_shader_.getName(), "bloomColor"), bloom_color[0], bloom_color[1], bloom_color[2]);
	glUniform1i(glGetUniformLocation(resolve_shader_.getName(), "bloomAddBase"), true);
	glUseProgram(0);

	context->endGLCommands();
}
