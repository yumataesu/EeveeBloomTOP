#pragma once

class OP_Inputs;
class OP_ParameterManager;

#pragma region ParNames and ParLabels

// Names of the parameters
constexpr static char ThresholdName[] = "Threshold";
constexpr static char ThresholdLabel[] = "Threshold";

constexpr static char KneeName[] = "Knee";
constexpr static char KneeLabel[] = "Knee";

constexpr static char RadiusName[] = "Radius";
constexpr static char RadiusLabel[] = "Radius";

constexpr static char BloomColorName[] = "Bloomcolor";
constexpr static char BloomColorLabel[] = "BloomColor";

constexpr static char IntensityName[] = "Intensity";
constexpr static char IntensityLabel[] = "Intensity";

constexpr static char ClampName[] = "Clamp";
constexpr static char ClampLabel[] = "Clamp";

#pragma endregion


#pragma region Parameters
class Parameters
{
public:
	static void	setup(OP_ParameterManager*);

	static float evalThreshold(const OP_Inputs* input);
	static float evalKnee(const OP_Inputs* input);
	static float evalRadius(const OP_Inputs* input);
	static Color evalBloomColor(const OP_Inputs* input);
	static float evalIntensity(const OP_Inputs* input);
	static float evalClamp(const OP_Inputs* input);

	static bool bNeedUpdate(const OP_Inputs* input);
	static void lateUpdate(const OP_Inputs* input);

protected:
	static float preThreshold;
	static float preKnee;
	static float preRadius;
	static Color preBloomColor;
	static float preIntensity;
	static float preClamp;
};
#pragma endregion