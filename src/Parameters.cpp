#include <string>
#include <array>
#include "touchdesigner/CPlusPlus_Common.h"
#include "Parameters.h"

#pragma region Evals

float Parameters::evalThreshold(const OP_Inputs* input)
{
	return input->getParDouble(ThresholdName);
}


float Parameters::evalKnee(const OP_Inputs* input)
{
	return input->getParDouble(KneeName);
}


float Parameters::evalRadius(const OP_Inputs* input)
{
	return input->getParDouble(RadiusName);
}


Color Parameters::evalBloomColor(const OP_Inputs* input)
{
	std::array<double, 3> vals;
	input->getParDouble3(BloomColorName, vals[0], vals[1], vals[2]);
	return Color((float)vals[0], (float)vals[1], (float)vals[2], 1.0f);
}


float Parameters::evalIntensity(const OP_Inputs* input)
{
	return input->getParDouble(IntensityName);
}


float Parameters::evalClamp(const OP_Inputs* input)
{
	return input->getParDouble(ClampName);
}


#pragma endregion


float Parameters::preThreshold;
float Parameters::preKnee;
float Parameters::preRadius;
Color Parameters::preBloomColor;
float Parameters::preIntensity;
float Parameters::preClamp;


void Parameters::lateUpdate(const OP_Inputs* input)
{
	preThreshold = Parameters::evalThreshold(input);
	preKnee = Parameters::evalKnee(input);
	preRadius = Parameters::evalRadius(input);
	preBloomColor = Parameters::evalBloomColor(input);
	preIntensity = Parameters::evalIntensity(input);
	preClamp = Parameters::evalClamp(input);
}


bool Parameters::bNeedUpdate(const OP_Inputs* input)
{
	return preThreshold != Parameters::evalThreshold(input) ||
		preKnee != Parameters::evalKnee(input) ||
		preRadius != Parameters::evalRadius(input) ||
		preBloomColor.r != Parameters::evalBloomColor(input).r ||
		preBloomColor.g != Parameters::evalBloomColor(input).g ||
		preBloomColor.b != Parameters::evalBloomColor(input).b ||
		preIntensity != Parameters::evalIntensity(input) ||
		preClamp != Parameters::evalClamp(input);
}


#pragma region Setup

void Parameters::setup(OP_ParameterManager* manager)
{
	{
		OP_NumericParameter p;

		p.name = ThresholdName;
		p.label = ThresholdLabel;
		p.page = "Parameter";
		p.defaultValues[0] = 0.4;
		p.minSliders[0] = 0.0;
		p.maxSliders[0] = 1.0;
		p.minValues[0] = 0.0;
		p.maxValues[0] = 10.0;
		p.clampMins[0] = true;
		p.clampMaxes[0] = false;
		OP_ParAppendResult res = manager->appendFloat(p);

		assert(res == OP_ParAppendResult::Success);
	}

	{
		OP_NumericParameter p;

		p.name = KneeName;
		p.label = KneeLabel;
		p.page = "Parameter";
		p.defaultValues[0] = 0.2;
		p.minSliders[0] = 0.0;
		p.maxSliders[0] = 1.0;
		p.minValues[0] = 0.0;
		p.maxValues[0] = 1.0;
		p.clampMins[0] = true;
		p.clampMaxes[0] = false;
		OP_ParAppendResult res = manager->appendFloat(p);

		assert(res == OP_ParAppendResult::Success);
	}

	{
		OP_NumericParameter p;

		p.name = RadiusName;
		p.label = RadiusLabel;
		p.page = "Parameter";
		p.defaultValues[0] = 2.0;
		p.minSliders[0] = 0.0;
		p.maxSliders[0] = 10.0;
		p.minValues[0] = 0.0;
		p.maxValues[0] = 10.0;
		p.clampMins[0] = true;
		p.clampMaxes[0] = false;
		OP_ParAppendResult res = manager->appendFloat(p);

		assert(res == OP_ParAppendResult::Success);
	}

	{
		OP_NumericParameter p;
		p.name = BloomColorName;
		p.label = BloomColorLabel;
		p.page = "Parameter";

		const int ArraySize = 3;

		const std::array<double, ArraySize>  DefaultValues = { 1.0, 1.0, 1.0 };
		const std::array<double, ArraySize>  MinSliders = { 0.0, 0.0, 0.0 };
		const std::array<double, ArraySize>  MaxSliders = { 1.0, 1.0, 1.0 };
		const std::array<double, ArraySize>  MinValues = { 0.0, 0.0, 0.0 };
		const std::array<double, ArraySize>  MaxValues = { 1.0, 1.0, 1.0 };
		const std::array<bool, ArraySize>  ClampMins = { true, true, true };
		const std::array<bool, ArraySize>  ClampMaxes = { true, true, true };
		for (int i = 0; i < DefaultValues.size(); ++i)
		{
			p.defaultValues[i] = DefaultValues[i];
			p.minSliders[i] = MinSliders[i];
			p.maxSliders[i] = MaxSliders[i];
			p.minValues[i] = MinValues[i];
			p.maxValues[i] = MaxValues[i];
			p.clampMins[i] = ClampMins[i];
			p.clampMaxes[i] = ClampMaxes[i];
		}
		OP_ParAppendResult res = manager->appendRGB(p);

		assert(res == OP_ParAppendResult::Success);
	}

	{
		OP_NumericParameter p;

		p.name = IntensityName;
		p.label = IntensityLabel;
		p.page = "Parameter";
		p.defaultValues[0] = 0.1;
		p.minSliders[0] = 0.0;
		p.maxSliders[0] = 1.0;
		p.minValues[0] = 0.0;
		p.maxValues[0] = 1.0;
		p.clampMins[0] = true;
		p.clampMaxes[0] = false;
		OP_ParAppendResult res = manager->appendFloat(p);

		assert(res == OP_ParAppendResult::Success);
	}

	{
		OP_NumericParameter p;

		p.name = ClampName;
		p.label = ClampLabel;
		p.page = "Parameter";
		p.defaultValues[0] = 0.1;
		p.minSliders[0] = 0.0;
		p.maxSliders[0] = 1000.0;
		p.minValues[0] = 0.0;
		p.maxValues[0] = 1000.0;
		p.clampMins[0] = true;
		p.clampMaxes[0] = false;
		OP_ParAppendResult res = manager->appendFloat(p);

		assert(res == OP_ParAppendResult::Success);
	}


}

#pragma endregion