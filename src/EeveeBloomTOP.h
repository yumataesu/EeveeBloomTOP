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

#include <assert.h>
#include <iostream>
#include <memory>
#include <array>
#include <string>
#include <sstream>
#include <cstdio>

#include "touchdesigner/TOP_CPlusPlusBase.h"
#include "Parameters.h"
#include "uma/inc.h"

#include "shader/Bloom.h"

#define MAX_BLOOM_STEP 16

class EeveeBloomTOP : public TOP_CPlusPlusBase
{
public:
	EeveeBloomTOP(const OP_NodeInfo *info, TOP_Context *context);
	virtual ~EeveeBloomTOP();

	virtual void getGeneralInfo(TOP_GeneralInfo*, const OP_Inputs*, void* reserved1) override;
	virtual bool getOutputFormat(TOP_OutputFormat*, const OP_Inputs*, void* reserved1) override;
	virtual bool getInfoDATSize(OP_InfoDATSize* infoSize, void* reserved1) override;
    virtual void getErrorString(OP_String *error, void* reserved1) override;
	virtual void setupParameters(OP_ParameterManager *manager, void* reserved1) override;
	virtual void getInfoDATEntries(int32_t index, int32_t nEntries, OP_InfoDATEntries* entries, void* reserved1);
	virtual void execute(TOP_OutputFormatSpecs*, const OP_Inputs*, TOP_Context *context, void* reserved1) override;


protected:
	virtual void setupGL();
	virtual void resetupBloom(const OP_Inputs* inputs, TOP_Context* context);
	// We don't need to store this pointer, but we do for the example.
	// The OP_NodeInfo class store information about the node that's using
	// this instance of the class (like its name).
	const OP_NodeInfo*	node_info_;

	// In this example this value will be incremented each time the execute()
	// function is called, then passes back to the TOP 
    const char* error_text_;
    bool is_setuped_;
	Parameters parameters_;
	int pre_input_w_, pre_input_h_;

	uma::Quad quad_;
	uma::Program blit_shader_;
	uma::Program downsample_first_shader_;
	uma::Program downsample_shader_;
	uma::Program upsample_shader_;
	uma::Program resolve_shader_;

	std::unique_ptr<uma::FrameBuffer> blit_fbo_;
	std::array<std::shared_ptr<uma::FrameBuffer>, MAX_BLOOM_STEP> downsample_fbos_;
	std::array<std::shared_ptr<uma::FrameBuffer>, MAX_BLOOM_STEP-1> accum_fbos_;

	int bloom_iteration_len;
	float bloom_sample_scale;
	float downsamp_texel_size[MAX_BLOOM_STEP][2];
	
	GLuint blit_shader_sourceBuffer;
	GLuint downsample_first_shader_sourceBuffer;
	GLuint downsample_shader_sourceBufferTexelSize;
	GLuint upsample_shader_sourceBufferTexelSize;
	GLuint upsample_shader_sampleScale;
};
