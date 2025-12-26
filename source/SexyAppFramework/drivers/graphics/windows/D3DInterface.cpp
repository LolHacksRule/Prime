#include "D3DInterface.h"
#include "D3DStateManager.h"
#include "WindowsGraphicsDriver.h"
//#include "../../../IGraphicsDriver.h"
#include "../../../Graphics.h"
#include "DirectXErrorString.h"
#include "../../../SexyMatrix.h"
#include "../../../SexyAppBase.h"
#include "../../../TriVertex.h"
#include "../../../AutoCrit.h"
#include "../../../PixelTracer.h"
//#include "../../../Debug.h"
#include <assert.h>
#include <algorithm>

#pragma warning(disable:4244)

using namespace Sexy;

std::string D3DInterface::sErrorString; //22
int sMinTextureWidth;
int sMaxTextureWidth;
int sMinTextureHeight;
int sMaxTextureHeight;
int sMaxTextureAspectRatio;
DWORD sSupportedTextureFormats;
DWORD sSupportedScreenFormats;
bool sTextureSizeMustBePow2;
bool sCanStretchRectFromTextures;
D3DInterface::PtrDataMap sPtrData; //32

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void Sexy::SexyMatrixMultiply_Static(SexyMatrix4* pOut, const SexyMatrix4* pM1, SexyMatrix4* pM2) //141-158 (Accurate)
{
	pOut->m00 = pM1->m00 * pM2->m00 + pM1->m01 * pM2->m10 + pM1->m02 * pM2->m20 + pM1->m03 * pM2->m30;
	pOut->m01 = pM1->m00 * pM2->m01 + pM1->m01 * pM2->m11 + pM1->m02 * pM2->m21 + pM1->m03 * pM2->m31;
	pOut->m02 = pM1->m00 * pM2->m02 + pM1->m01 * pM2->m12 + pM1->m02 * pM2->m22 + pM1->m03 * pM2->m32;
	pOut->m03 = pM1->m00 * pM2->m03 + pM1->m01 * pM2->m13 + pM1->m02 * pM2->m23 + pM1->m03 * pM2->m33;
	pOut->m10 = pM1->m10 * pM2->m00 + pM1->m11 * pM2->m10 + pM1->m12 * pM2->m20 + pM1->m13 * pM2->m30;
	pOut->m11 = pM1->m10 * pM2->m01 + pM1->m11 * pM2->m11 + pM1->m12 * pM2->m21 + pM1->m13 * pM2->m31;
	pOut->m12 = pM1->m10 * pM2->m02 + pM1->m11 * pM2->m12 + pM1->m12 * pM2->m22 + pM1->m13 * pM2->m32;
	pOut->m13 = pM1->m10 * pM2->m03 + pM1->m11 * pM2->m13 + pM1->m12 * pM2->m23 + pM1->m13 * pM2->m33;
	pOut->m20 = pM1->m20 * pM2->m00 + pM1->m21 * pM2->m10 + pM1->m22 * pM2->m20 + pM1->m23 * pM2->m30;
	pOut->m21 = pM1->m20 * pM2->m01 + pM1->m21 * pM2->m11 + pM1->m22 * pM2->m21 + pM1->m23 * pM2->m31;
	pOut->m22 = pM1->m20 * pM2->m02 + pM1->m21 * pM2->m12 + pM1->m22 * pM2->m22 + pM1->m23 * pM2->m32;
	pOut->m23 = pM1->m20 * pM2->m03 + pM1->m21 * pM2->m13 + pM1->m22 * pM2->m23 + pM1->m23 * pM2->m33;
	pOut->m30 = pM1->m30 * pM2->m00 + pM1->m31 * pM2->m10 + pM1->m32 * pM2->m20 + pM1->m33 * pM2->m30;
	pOut->m31 = pM1->m30 * pM2->m01 + pM1->m31 * pM2->m11 + pM1->m32 * pM2->m21 + pM1->m33 * pM2->m31;
	pOut->m32 = pM1->m30 * pM2->m02 + pM1->m31 * pM2->m12 + pM1->m32 * pM2->m22 + pM1->m33 * pM2->m32;
	pOut->m33 = pM1->m30 * pM2->m03 + pM1->m31 * pM2->m13 + pM1->m32 * pM2->m23 + pM1->m33 * pM2->m33;
}

bool D3DRenderEffectDefInfo::Annotation::GetBool() const { return mData[0] != NULL; } //181
int D3DRenderEffectDefInfo::Annotation::GetInt() const { return mData[0]; } //182

std::string D3DRenderEffectDefInfo::Annotation::GetString() const { return mData[0]; } //185 (?)

bool D3DRenderEffectDefInfo::Build(RenderEffectDefinition* inDefinition) //286-566
{
	struct Local
	{
		//auto LoadAnnotations = [](Buffer& inBuffer, std::vector<Annotation>& outAnnotations) //290-351
		static void LoadAnnotations(Buffer& inBuffer, std::vector<Annotation>& outAnnotations) //290-351
		{
			int aAnnotCount = inBuffer.ReadByte();
			for (int iAnnot = 0; iAnnot < aAnnotCount; iAnnot++)
			{
				int aAnnotType = inBuffer.ReadByte();
				if (aAnnotType)
				{
					std::string aStr;
					outAnnotations.push_back(Annotation());
					Annotation* aAnnot = &outAnnotations.back();
					switch (aAnnotType)
					{
					case 1: aAnnot->mType = Annotation::AT_Bool; aAnnot->mName = inBuffer.ReadString(); aAnnot->mData.resize(1); aAnnot->mData[0] = inBuffer.ReadByte(); break;
					case 2: aAnnot->mType = Annotation::AT_Int; aAnnot->mName = inBuffer.ReadString(); aAnnot->mData.resize(4); aAnnot->mData[0] = inBuffer.ReadLong(); break;
					case 3: aAnnot->mType = Annotation::AT_Float; aAnnot->mName = inBuffer.ReadString(); aAnnot->mData.resize(4); inBuffer.ReadBytes(&aAnnot->mData[0], 4); break;
					case 4: aAnnot->mType = Annotation::AT_Vector; aAnnot->mName = inBuffer.ReadString(); aAnnot->mData.resize(16); inBuffer.ReadBytes(&aAnnot->mData[0], 16); break;
					case 5: aAnnot->mType = Annotation::AT_String; aAnnot->mName = inBuffer.ReadString(); aStr = inBuffer.ReadString(); aAnnot->mData.resize(aStr.length() + 1); strcpy((char*)&aAnnot->mData[0], aStr.c_str()); break;
					default: assert(false && "Invalid annotation type"); //346
					}
				}
			}
		};
		//auto LoadShader = [](Buffer& inBuffer, Shader* outShader) //354-468
		static void LoadShader(Buffer& inBuffer, Shader* outShader) //354-468
		{
			enum ETransformFlags
			{
				TF_World = 1,
				TF_View = 2,
				TF_Proj = 4,
				TF_Transpose = 8,
				TF_Texture = 16,
			};
			std::string aSemantic;
			int aShaderCodeSize = inBuffer.ReadShort();
			if (!aShaderCodeSize)
				return;

			outShader->mCode.resize(aShaderCodeSize);
			inBuffer.ReadBytes(&outShader->mCode[0], aShaderCodeSize);
			int aConstantCount = inBuffer.ReadShort();
			for (int iConstant = 0; iConstant < aConstantCount; iConstant++)
			{
				int aConstantType = inBuffer.ReadByte();
				if (aConstantType)
				{
					outShader->mConstants.push_back(ShaderConstant());
					ShaderConstant* aConstant = &outShader->mConstants.back();
					if (aConstantType == 1) //Switch statement?
						aConstant->mType = ShaderConstant::CT_Float;
					else
					{
						if (aConstantType != 2)
							assert(false && "Invalid shader constant type");
						aConstant->mType = ShaderConstant::CT_Sampler;
					}
					aConstant->mConstantName = inBuffer.ReadString();
					aConstant->mSemantic = inBuffer.ReadString();
					aConstant->mStandardSemantic = ShaderConstant::SCS_None;
					aConstant->mRegisterCount = inBuffer.ReadByte();
					if (!aConstant->mSemantic.empty())
					{
						aSemantic = aConstant->mSemantic;
						ulong aTransformFlags;
						if (aSemantic == "WORLD") //Switch statement better?
							aTransformFlags = TF_World;
						else if (aSemantic == "VIEW")
							aTransformFlags = TF_View;
						else if (aSemantic == "PROJ")
							aTransformFlags = TF_Proj;
						else if (aSemantic == "WORLDVIEW")
							aTransformFlags = TF_World | TF_View;
						else if (aSemantic == "VIEWPROJ")
							aTransformFlags = TF_View | TF_Proj;
						else if (aSemantic == "WORLDVIEWPROJ")
							aTransformFlags = TF_World | TF_View | TF_Proj;
						else if (aSemantic == "WORLD_TRANSPOSE")
							aTransformFlags = TF_World | TF_Transpose;
						else if (aSemantic == "VIEW_TRANSPOSE")
							aTransformFlags = TF_View | TF_Transpose;
						else if (aSemantic == "PROJ_TRANSPOSE")
							aTransformFlags = TF_Proj | TF_Transpose;
						else if (aSemantic == "WORLDVIEW_TRANSPOSE")
							aTransformFlags = TF_World | TF_View | TF_Transpose;
						else if (aSemantic == "VIEWPROJ_TRANSPOSE")
							aTransformFlags = TF_View | TF_Proj | TF_Transpose;
						else if (aSemantic == "WORLDVIEWPROJ_TRANSPOSE")
							aTransformFlags = TF_World | TF_View | TF_Proj | TF_Transpose;
						if (aTransformFlags)
							aConstant->mStandardSemantic = aTransformFlags;
					}
					else
					{
						if (aSemantic.substr(0, 16) == "TEXTURETRANSFORM")
						{
							if (aSemantic == "TEXTURETRANSFORM0")
								aConstant->mStandardSemantic = D3DRenderEffectDefInfo::ShaderConstant::SCS_Texture;
							if (aSemantic == "TEXTURETRANSFORM1")
								aConstant->mStandardSemantic = (ShaderConstant::EStandardConstantSemantic)(D3DRenderEffectDefInfo::ShaderConstant::SCS_Texture | D3DRenderEffectDefInfo::ShaderConstant::SCS_World);
							if (aSemantic == "TEXTURETRANSFORM2")
								aConstant->mStandardSemantic = (ShaderConstant::EStandardConstantSemantic)(D3DRenderEffectDefInfo::ShaderConstant::SCS_Texture | D3DRenderEffectDefInfo::ShaderConstant::SCS_View);
							if (aSemantic == "TEXTURETRANSFORM3")
								aConstant->mStandardSemantic = (ShaderConstant::EStandardConstantSemantic)(D3DRenderEffectDefInfo::ShaderConstant::SCS_Texture | D3DRenderEffectDefInfo::ShaderConstant::SCS_View | D3DRenderEffectDefInfo::ShaderConstant::SCS_World);
							if (aSemantic == "TEXTURETRANSFORM4")
								aConstant->mStandardSemantic = (ShaderConstant::EStandardConstantSemantic)(D3DRenderEffectDefInfo::ShaderConstant::SCS_Texture | D3DRenderEffectDefInfo::ShaderConstant::SCS_Proj);
							if (aSemantic == "TEXTURETRANSFORM5")
								aConstant->mStandardSemantic = (ShaderConstant::EStandardConstantSemantic)(D3DRenderEffectDefInfo::ShaderConstant::SCS_Texture | D3DRenderEffectDefInfo::ShaderConstant::SCS_Proj | D3DRenderEffectDefInfo::ShaderConstant::SCS_World);
							if (aSemantic == "TEXTURETRANSFORM6")
								aConstant->mStandardSemantic = (ShaderConstant::EStandardConstantSemantic)(D3DRenderEffectDefInfo::ShaderConstant::SCS_Texture | D3DRenderEffectDefInfo::ShaderConstant::SCS_Proj | D3DRenderEffectDefInfo::ShaderConstant::SCS_View);
							if (aSemantic == "TEXTURETRANSFORM7")
								aConstant->mStandardSemantic = (ShaderConstant::EStandardConstantSemantic)(D3DRenderEffectDefInfo::ShaderConstant::SCS_Texture | D3DRenderEffectDefInfo::ShaderConstant::SCS_Proj | D3DRenderEffectDefInfo::ShaderConstant::SCS_View | D3DRenderEffectDefInfo::ShaderConstant::SCS_World);
						}

						else
						{
							if (aSemantic.substr(0, 19) == "INVTEXTURETRANSFORM")
							{
								if (aSemantic == "TEXTURETRANSFORM0_TRANSPOSE")
									aConstant->mStandardSemantic = ShaderConstant::SCS_Texture;
								if (aSemantic == "TEXTURETRANSFORM1_TRANSPOSE")
									aConstant->mStandardSemantic = ShaderConstant::SCS_Texture | ShaderConstant::SCS_World;
								if (aSemantic == "TEXTURETRANSFORM2_TRANSPOSE")
									aConstant->mStandardSemantic = ShaderConstant::SCS_Texture | ShaderConstant::SCS_View;
								if (aSemantic == "TEXTURETRANSFORM3_TRANSPOSE")
									aConstant->mStandardSemantic = ShaderConstant::SCS_Texture | ShaderConstant::SCS_View | ShaderConstant::SCS_World;
								if (aSemantic == "TEXTURETRANSFORM4_TRANSPOSE")
									aConstant->mStandardSemantic = ShaderConstant::SCS_Texture | ShaderConstant::SCS_Proj;
								if (aSemantic == "TEXTURETRANSFORM5_TRANSPOSE")
									aConstant->mStandardSemantic = ShaderConstant::SCS_Texture | ShaderConstant::SCS_Proj | ShaderConstant::SCS_World;
								if (aSemantic == "TEXTURETRANSFORM6_TRANSPOSE")
									aConstant->mStandardSemantic = ShaderConstant::SCS_Texture | ShaderConstant::SCS_Proj | ShaderConstant::SCS_View;
								if (aSemantic == "TEXTURETRANSFORM7_TRANSPOSE")
									aConstant->mStandardSemantic = ShaderConstant::SCS_Texture | ShaderConstant::SCS_Proj | ShaderConstant::SCS_View | ShaderConstant::SCS_World;
							}

							else
							{
								if (aSemantic.substr(0, 5) == "LIGHT")
								{
									if (aSemantic == "LIGHTAMBIENT")
										aConstant->mStandardSemantic = ShaderConstant::SCS_LightAmbient;
									if (aSemantic == "LIGHTATTENUATION")
										aConstant->mStandardSemantic = ShaderConstant::SCS_LightAttenuation;
									if (aSemantic == "LIGHTDIFFUSE")
										aConstant->mStandardSemantic = ShaderConstant::SCS_LightDiffuse;
									if (aSemantic == "LIGHTSPECULAR")
										aConstant->mStandardSemantic = ShaderConstant::SCS_LightSpecular;
									if (aSemantic == "LIGHTDIRECTION")
										aConstant->mStandardSemantic = ShaderConstant::SCS_LightDirection;
									if (aSemantic == "LIGHTPOSITION")
										aConstant->mStandardSemantic = ShaderConstant::SCS_LightPosition;
									if (aSemantic == "LIGHTMISC")
										aConstant->mStandardSemantic = ShaderConstant::SCS_LightMisc;
								}
								else
								{
									if (aSemantic.substr(0, 8) == "MATERIAL")
									{
										if (aSemantic == "MATERIALAMBIENT")
											aConstant->mStandardSemantic = ShaderConstant::SCS_MaterialAmbient;
										if (aSemantic == "MATERIALDIFFUSE")
											aConstant->mStandardSemantic = ShaderConstant::SCS_MaterialDiffuse;
										if (aSemantic == "MATERIALSPECULAR")
											aConstant->mStandardSemantic = ShaderConstant::SCS_MaterialSpecular;
										if (aSemantic == "MATERIALEMISSIVE")
											aConstant->mStandardSemantic = ShaderConstant::SCS_MaterialEmissive;
										if (aSemantic == "MATERIALPOWER")
											aConstant->mStandardSemantic = ShaderConstant::SCS_MaterialPower;
									}
									else if (aSemantic == "GLOBALAMBIENT")
										aConstant->mStandardSemantic = ShaderConstant::SCS_GlobalAmbient;
									else if (aSemantic == "TEXTUREFACTOR")
										aConstant->mStandardSemantic = ShaderConstant::SCS_TextureFactor;
								}
							}
						}
					}
				}
			}
		};
	};
	mDefinition = inDefinition;
	mTechniques.clear();
	if (Lower(inDefinition->mDataFormat) != "d3dfx")
		gSexyAppBase->Popup(StrFormat("Effect \"%s\" is not in required D3DFX format", inDefinition->mSrcFileName.c_str()));
	else
	{
		Buffer aBuffer;
		aBuffer.SetData(inDefinition->mData);
		aBuffer.SeekFront();
		ulong aStartMagic = aBuffer.ReadLong();
		if (aStartMagic != 0x1234FFFF)
			assert(false && "D3DRenderEffectDefInfo::Build: Invalid start-magic; check resource serialization code"); //487
		ulong aVersion = aBuffer.ReadShort();
		if (aVersion == 1)
		{
			int aTechniqueCount = aBuffer.ReadShort();
			for (int iTechnique = 0; iTechnique < aTechniqueCount; iTechnique++)
			{
				mTechniques.push_back(Technique()); //?
				Technique* aTech = &mTechniques.back();
				aTech = (Technique*)&aBuffer.ReadString();
				Local::LoadAnnotations(aBuffer, aTech->mAnnotations);
				int aPassCount = aBuffer.ReadShort();
				for (int iPass = 0; iPass < aPassCount; iPass++)
				{
					aTech->mPasses.push_back(Pass()); //?
					Pass* aPass = &aTech->mPasses.back();
					aPass = (Pass*)&aBuffer.ReadString();
					Local::LoadAnnotations(aBuffer, aTech->mAnnotations);
					for (int aStateCmdType = aBuffer.ReadByte(); aStateCmdType; aStateCmdType = aBuffer.ReadByte())
					{
						aPass->mStateCommands.push_back(StateCommand()); //?
						StateCommand* aCommand = &aPass->mStateCommands.back();
						if (aStateCmdType == StateCommand::CMD_SetRenderState)
						{
							aCommand->mSamplerOrTextureStage = 0;
							aCommand->mState = aBuffer.ReadLong();
							aCommand->mValue = aBuffer.ReadLong();
						}
						else
						{
							if (aStateCmdType != StateCommand::CMD_SetSamplerState && aStateCmdType != StateCommand::CMD_SetTextureStageState)
								assert(false && "Invalid state command"); //545
							aCommand->mSamplerOrTextureStage = aBuffer.ReadByte();
							aCommand->mState = aBuffer.ReadLong();
							aCommand->mValue = aBuffer.ReadLong();
						}
					}
					Local::LoadShader(aBuffer, &aPass->mVertexShader);
					Local::LoadShader(aBuffer, &aPass->mPixelShader);
				}
			}
			ulong aEndMagic = aBuffer.ReadLong();
			if (aEndMagic != 0xFFFF5678)
				assert(false && "D3DRenderEffectDefInfo::Build: Invalid end-magic; check resource serialization code"); //561
			return true;
		}
		else
			return false;
	}
}

void D3DRenderEffect::ParamData::SetValue(const float* inFloatData, ulong inFloatCount) //584-590 (Accurate)
{
	mFloatData.resize(inFloatCount);
	CopyMemory(&mFloatData[0], inFloatData, 4 * inFloatCount);
	assert(mFloatData.size() == inFloatCount); //587
	while (mFloatData.size() & 3)
		mFloatData.push_back(0);
}

D3DRenderEffect::ParamData * D3DRenderEffect::ParamCollection::GetParamNamed(const std::string & inName, bool inAllowCreate) //600-614
{
	ParamMap::iterator it = mParamMap.find(inName); //?
	if (it != mParamMap.end())
		return &it->second;
	if (!inAllowCreate)
		return false;
	mParamMap[inName] = ParamData();
	return GetParamNamed(inName, false);
}

D3DRenderEffect::Pass::Pass(RenderEffect* inEffect, D3DInterface* inInterface, D3DRenderEffectDefInfo::Pass* inDefinition) //635-661
{
	mEffect = inEffect;
	mInterface = inInterface;
	mDefinition = inDefinition;
	mVertexShader = NULL;
	mPixelShader = NULL;
	mInProgress = false;
	if (mDefinition->mVertexShader.mCode.size())
	{
		if (mInterface->InternalCreateVertexShader(&mDefinition->mVertexShader.mCode[0], &mVertexShader) < 0)
			mVertexShader = NULL;
	}
	if (mDefinition->mPixelShader.mCode.size())
	{
		if (mInterface->InternalCreatePixelShader(&mDefinition->mPixelShader.mCode[0], &mPixelShader) < 0)
			mPixelShader = NULL;
	}
	int aAnnotCount = mDefinition->mAnnotations.size();
	for (int iAnnot = 0; iAnnot < aAnnotCount; iAnnot++)
	{
		D3DRenderEffectDefInfo::Annotation* aAnnot = &mDefinition->mAnnotations[iAnnot];
		if (aAnnot->mType == D3DRenderEffectDefInfo::Annotation::AT_String)
		{
			if (aAnnot->mName == "rfx_pass_texture_remap")
				mTextureRemapStr = aAnnot->GetString();
		}
	}
}
D3DRenderEffect::Pass::~Pass() //663-676 (Correct?)
{
	delete mPixelShader;
	delete mVertexShader;
}

D3DRenderEffect::ParamData* D3DRenderEffect::Pass::MakeTempParamForSemantic(ParamData* inParam, D3DInterface* inInterface, ulong inSemantic, ulong inDesiredRegisterCount) //679-957
{
	if ((inSemantic == D3DRenderEffectDefInfo::ShaderConstant::SCS_GlobalAmbient) != 0)
	{
		Color aGlobalAmbient = aGlobalAmbient.FromInt(inInterface->mStateMgr->GetRenderState(139));
		inParam->mFloatData.resize(4);
		inParam->mFloatData[0] = (double)aGlobalAmbient.mRed / 255.0;
		inParam->mFloatData[1] = (double)aGlobalAmbient.mGreen / 255.0;
		inParam->mFloatData[2] = (double)aGlobalAmbient.mBlue / 255.0;
		inParam->mFloatData[3] = (double)aGlobalAmbient.mAlpha / 255.0;
	}
	//?
	else
	{
		SexyMatrix4 m;
		if ((inSemantic & D3DRenderEffectDefInfo::ShaderConstant::SCS_Texture) != 0)
		{
			inInterface->mStateMgr->GetTransform((inSemantic & 7) + 16, &m);
		}
		else
		{
			SexyMatrix4 aWorld;
			SexyMatrix4 aView;
			SexyMatrix4 aProj;
			if ((inSemantic & D3DRenderEffectDefInfo::ShaderConstant::SCS_World) != 0)
				inInterface->mStateMgr->GetTransform(256, &aWorld);
			if ((inSemantic & D3DRenderEffectDefInfo::ShaderConstant::SCS_View) != 0)
				inInterface->mStateMgr->GetTransform(2, &aWorld);
			if ((inSemantic & D3DRenderEffectDefInfo::ShaderConstant::SCS_Proj) != 0)
				inInterface->mStateMgr->GetTransform(3, &aWorld);
			if ((inSemantic & D3DRenderEffectDefInfo::ShaderConstant::SCS_World) != 0)
			{
				CopyMemory(&m, &aWorld, sizeof(m));
				if ((inSemantic & D3DRenderEffectDefInfo::ShaderConstant::SCS_View) != 0)
				{
					SexyMatrix4 aResultMatrix;
					SexyMatrixMultiply_Static(&aResultMatrix, &m, &aView);
					CopyMemory(&m, &aResultMatrix, sizeof(m));
				}
				if ((inSemantic & D3DRenderEffectDefInfo::ShaderConstant::SCS_Proj) != 0)
				{
					SexyMatrix4 pOut;
					SexyMatrixMultiply_Static(&pOut, &m, &aProj);
					CopyMemory(&m, &pOut, sizeof(m));
				}
			}

			else if ((inSemantic & D3DRenderEffectDefInfo::ShaderConstant::SCS_View) != 0)
			{
				CopyMemory(&m, &aView, sizeof(m));
				if ((inSemantic & D3DRenderEffectDefInfo::ShaderConstant::SCS_View) != 0)
				{
					SexyMatrix4 aResultMatrix;
					SexyMatrixMultiply_Static(&aResultMatrix, &m, &aView);
					CopyMemory(&m, &aResultMatrix, sizeof(m));
				}
			}
			else
			{
				if ((inSemantic & D3DRenderEffectDefInfo::ShaderConstant::SCS_Proj) == 0)
					assert(false && "Invalid standard constant semantic"); //733
				CopyMemory(&m, &aProj, sizeof(m));
			}
		}
		if ((inSemantic & D3DRenderEffectDefInfo::ShaderConstant::SCS_Transpose) != 0)
			inParam->SetValue((const float*)&m, 16);
		else
		{
			float mT[16];
			mT[0] = m.m00;
			mT[1] = m.m10;
			mT[2] = m.m20;
			mT[3] = m.m30;
			mT[4] = m.m01;
			mT[5] = m.m11;
			mT[6] = m.m21;
			mT[7] = m.m31;
			mT[8] = m.m02;
			mT[9] = m.m12;
			mT[10] = m.m22;
			mT[11] = m.m32;
			mT[12] = m.m03;
			mT[13] = m.m13;
			mT[14] = m.m23;
			mT[15] = m.m33;
			inParam->SetValue(mT, 16);
		}
	}
	return inParam;
}

void D3DRenderEffect::Pass::ApplyToDevice(ParamCollection* inParams, bool inApplyParamsOnly) //960-1055
{
	ParamData* aTempParam;
	if (!inApplyParamsOnly)
	{
		mInterface->mStateMgr->SetVertexShader(mVertexShader);
		mInterface->mStateMgr->SetPixelShader(mPixelShader);
		int aStateCommandCount = mDefinition->mStateCommands.size();
		for (int iCommand = 0; iCommand < aStateCommandCount; iCommand++)
		{
			D3DRenderEffectDefInfo::StateCommand* aCommand = &mDefinition->mStateCommands[iCommand];
			if (aCommand->mType == D3DRenderEffectDefInfo::StateCommand::CMD_SetRenderState)
				mInterface->SetRenderState(aCommand->mState, aCommand->mValue);
			else if (aCommand->mType == D3DRenderEffectDefInfo::StateCommand::CMD_SetSamplerState)
				mInterface->SetSamplerState(aCommand->mSamplerOrTextureStage, aCommand->mState, aCommand->mValue);
			else if (aCommand->mType == D3DRenderEffectDefInfo::StateCommand::CMD_SetTextureStageState)
				mInterface->SetTextureStageState(aCommand->mSamplerOrTextureStage, aCommand->mState, aCommand->mValue);
		}
		if (!mTextureRemapStr.empty())
		{
			for (int i = 0; (mTextureRemapStr.length() <= 10 ? mTextureRemapStr.length() : 10); i++)
			{
				if (mTextureRemapStr[i] >= '9' && mTextureRemapStr[i] >= '0')
					mInterface->mStateMgr->SetTextureRemap(i, mTextureRemapStr[i] - 48);
			}
		}
	}
	if (mVertexShader)
	{
		int aConstantCount = mDefinition->mVertexShader.mConstants.size();
		for (int iConstant = 0; iConstant < aConstantCount; iConstant++)
		{
			D3DRenderEffectDefInfo::ShaderConstant* aConstant = &mDefinition->mVertexShader.mConstants[iConstant];
			if (aConstant->mType == D3DRenderEffectDefInfo::ShaderConstant::CT_Float)
			{
				ParamData* aParamData = inParams->GetParamNamed(aConstant->mConstantName, false);
				if (!aParamData && aConstant->mStandardSemantic)
					aParamData = MakeTempParamForSemantic(aTempParam, mInterface, aConstant->mStandardSemantic, aConstant->mRegisterCount);
				if (aParamData)
				{
					int aMinFloatCount = (aParamData->mFloatData.size() >= 4 * aConstant->mRegisterCount) ? 4 * aConstant->mRegisterCount : aParamData->mFloatData.size();
					assert(!aMinFloatCount & 3); //1028
					mInterface->mStateMgr->SetVertexShaderConstantF(aConstant->mRegisterIndex, &aParamData->mFloatData[0], aMinFloatCount >> 2);
				}
			}
		}
	}

	if (mPixelShader)
	{
		int aConstantCount = mDefinition->mPixelShader.mConstants.size();
		for (int iConstant = 0; iConstant < aConstantCount; iConstant++)
		{
			D3DRenderEffectDefInfo::ShaderConstant* aConstant = &mDefinition->mPixelShader.mConstants[iConstant];
			if (aConstant->mType == D3DRenderEffectDefInfo::ShaderConstant::CT_Float)
			{
				ParamData* aParamData = inParams->GetParamNamed(aConstant->mConstantName, false);
				if (!aParamData && aConstant->mStandardSemantic)
					aParamData = MakeTempParamForSemantic(aTempParam, mInterface, aConstant->mStandardSemantic, aConstant->mRegisterCount);
				if (aParamData)
				{
					int aMinFloatCount = (aParamData->mFloatData.size() >= 4 * aConstant->mRegisterCount) ? 4 * aConstant->mRegisterCount : aParamData->mFloatData.size();
					assert(!aMinFloatCount & 3); //1049
					mInterface->mStateMgr->SetVertexShaderConstantF(aConstant->mRegisterIndex, &aParamData->mFloatData[0], aMinFloatCount >> 2);
				}
			}
		}
	}
}

D3DRenderEffect::Technique::Technique(RenderEffect* inEffect, D3DInterface* inInterface, D3DRenderEffectDefInfo::Technique* inDefinition, ulong inIndex, ParamCollection* inParams) //1077-1083
{
	mEffect = inEffect;
	mInterface = inInterface;
	mDefinition = inDefinition;
	mParams = inParams;
	mValidTechnique = 0;
	mValidated = false;
	mCompatFallback = "";
	int aPassCount = mDefinition->mPasses.size();
	for (int iPass = 0; iPass < aPassCount; iPass++)
		mPasses.push_back(new Pass(mEffect, mInterface, &mDefinition->mPasses[iPass]));
}
D3DRenderEffect::Technique::~Technique() //1085-1089 (Accurate?)
{
	for (int i = 0; i < mPasses.size(); i++)
		delete mPasses[i];
	mPasses.clear();
}

D3DRenderEffect::Technique* D3DRenderEffect::Technique::GetValidTechnique(TechniqueNameMap inTechniqueNameMap) //1092-1223
{
	if (!mValidated)
	{
		ulong aMinTextureStages = 0;
		bool aRequiresCubeMaps = false;
		bool aRequiresVolumeMaps = false;
		Technique* aFallbackTechnique = NULL;
		std::string aFallbackName;
		int aAnnotCount = mDefinition->mAnnotations.size();
		for (int iAnnot = 0; iAnnot < aAnnotCount; iAnnot++)
		{
			D3DRenderEffectDefInfo::Annotation* aAnnot = &mDefinition->mAnnotations[iAnnot];
			switch (aAnnot->mType)
			{
			case D3DRenderEffectDefInfo::Annotation::AT_String:
				if (aAnnot->mName == "rfx_fallback_technique")
					aFallbackName = aAnnot->GetString();
				else if (aAnnot->mName == "rfx_fallback_compD3DRenderEffectDefInfo::Annotation::AT_property")
				{
					if (gSexyAppBase->mCompatCfgMachine)
					{
						std::string aPropertyName = aAnnot->GetString();
						mCompatFallback = SexyStringToString(gSexyAppBase->GetString(aPropertyName, _S("")));
					}
				}
				break;
			case D3DRenderEffectDefInfo::Annotation::AT_Int: if (aAnnot->mName == "rfx_min_texture_stages") aMinTextureStages = aAnnot->GetInt(); break;
			case D3DRenderEffectDefInfo::Annotation::AT_Bool:
				if (aAnnot->mName == "rfx_requires_cubemaps")
					aRequiresCubeMaps = aAnnot->GetBool();
				else if (aAnnot->mName == "rfx_requires_volumemaps")
					aRequiresVolumeMaps = aAnnot->GetBool();;
				break;
			}
		}
		if (!aFallbackName.empty())
		{
			TechniqueNameMap::iterator it = inTechniqueNameMap.find(aFallbackName);
			Technique* aFallBackTechnique = it != inTechniqueNameMap.end() ? it->second : NULL;
			assert(aFallbackTechnique != NULL && "Fallback technique not found"); //1145
			aFallbackTechnique = aFallbackTechnique->GetValidTechnique(inTechniqueNameMap);
			assert(aFallbackTechnique != NULL && "Broken fallback technique chain"); //1147
		}
		bool isValid = true;
		if (aMinTextureStages && (mInterface->GetMaxTextureStages() < aMinTextureStages) || aRequiresCubeMaps && !mInterface->SupportsCubeMaps() || aRequiresVolumeMaps && !mInterface->SupportsVolumeMaps())
			isValid = false;
		else
		{
			int aPassCount = Begin();
			for (int iPass = 0; iPass < aPassCount; iPass++)
			{
				if (mDefinition->mPasses[iPass].mVertexShader.mCode.size() && !mInterface->SupportsVertexShaders())
				{
					isValid = false;
					break;
				}
				if (mDefinition->mPasses[iPass].mPixelShader.mCode.size() && !mInterface->SupportsPixelShaders())
				{
					isValid = false;
					break;
				}
				if (!mDefinition->mPasses[iPass].mPixelShader.mCode.size())
				{
					BeginPass(iPass);
					if (!mInterface->mStateMgr->CommitState())
					{
						//?
						EndPass(iPass);
						//?
						isValid = false;
						break;
					}
					ulong aValidateNumPasses = 0;
					if (mInterface->InternalValidateDevice(&aValidateNumPasses) < 0)
					{
						//?
						EndPass(iPass);
						//?
						isValid = false;
						break;
					}
					EndPass(iPass);
					//?
				}
			}
		}
		mValidTechnique = isValid ? this : aFallbackTechnique;
		mValidated = true;
	}
	return mValidTechnique;
}

RenderEffect* D3DRenderEffect::Technique::GetEffect() { return mEffect; } //1225
std::string D3DRenderEffect::Technique::GetName() { return mDefinition->mTechniqueName; } //1225

int D3DRenderEffect::Technique::Begin() //1229-1231
{
	return mPasses.size();
}

void D3DRenderEffect::Technique::BeginPass(int inPass) //1233-1240 (Accurate?)
{
	if (inPass < mPasses.size())
	{
		Pass* p = mPasses[inPass];
		p->ApplyToDevice(mParams, false);
		p->mInProgress = true;
	}
}

void D3DRenderEffect::Technique::EndPass(int inPass) //1242-1249 (Accurate?)
{
	if (inPass < mPasses.size())
	{
		Pass* p = mPasses[inPass];
		assert(p->mInProgress == true);
		p->mInProgress = false;
	}
}

void D3DRenderEffect::Technique::End() //1251-1252
{

}

bool D3DRenderEffect::Technique::PassUsesVertexShader(int inPass) //1255-1260 (Modified for line accuracy)
{
	return
		inPass < mPasses.size()
		&&
		mPasses[inPass]->mVertexShader != NULL;
}

bool D3DRenderEffect::Technique::PassUsesPixelShader(int inPass) //1262-1267 (Modified for line accuracy)
{
	return
		inPass < mPasses.size()
		&&
		mPasses[inPass]->mPixelShader != NULL;
}

void D3DRenderEffect::Technique::ParametersChanged() //1270-1279 (Modified for line accuracy)
{
	for (int i = 0; i < mPasses.size(); i++)
	{
		Pass* p = mPasses[i];
		if (p->mInProgress)
		{
			p->ApplyToDevice(mParams, true);
		}
	}
}

D3DRenderEffect::D3DRenderEffect(D3DInterface* inInterface, D3DRenderEffectDefInfo* inDefInfo) //1295-1320
{
	mInterface = inInterface;
	mDefInfo = inDefInfo;
	mCurrentTechnique = NULL;
	mBeginPassRefCount = 0;
	for (int i = 0; i < mDefInfo->mTechniques.size(); i++)
	{
		Technique* aTech = new Technique(this, mInterface, &mDefInfo->mTechniques[i], i, &mParams);
		mTechniques.push_back(aTech);
		if (&mTechniqueNameMap.find(aTech->mDefinition->mTechniqueName) != &mTechniqueNameMap.end())
			assert(false && "Duplicate technique names encountered"); //1304
		aTech = mTechniqueNameMap[aTech->mDefinition->mTechniqueName];
	}
	for (int i = 0; i < mDefInfo->mTechniques.size(); i++)
		mTechniques[i]->GetValidTechnique(mTechniqueNameMap);
	std::string aDefaultTechniqueName = "Default";
	TechniqueNameMap::iterator it = mTechniqueNameMap.find(aDefaultTechniqueName);
	if (it != mTechniqueNameMap.end())
		SetCurrentTechnique(aDefaultTechniqueName, true);
}

D3DRenderEffect::~D3DRenderEffect() //1322-1326
{
	for (int i = 0; i < mTechniques.size(); i++)
		delete mTechniques[i];
	mTechniques.clear();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
RenderDevice3D* D3DRenderEffect::GetDevice() { return mInterface; } //1331
RenderEffectDefinition* D3DRenderEffect::GetDefinition() { return mDefInfo->mDefinition; } //1332

void D3DRenderEffect::SetParameter(const std::string& inParamName, const float* inFloatData, ulong inFloatCount) //1335-1345
{
	ParamData* aParam = mParams.GetParamNamed(inParamName, true);
	aParam->SetValue(inFloatData, inFloatCount);
	if (mBeginPassRefCount)
	{
		if (mCurrentTechnique)
			mCurrentTechnique->ParametersChanged();
	}
}

void D3DRenderEffect::GetParameterBySemantic(ulong inSemantic, float* outFloatData, ulong inMaxFloatCount) //1348-1354
{
	ParamData aTempParam;
	ParamData* aParamData = Pass::MakeTempParamForSemantic(&aTempParam, mInterface, inSemantic, inMaxFloatCount >> 2);
	int aFloatCount = inMaxFloatCount >= aParamData->mFloatData.size() ? aParamData->mFloatData.size() : inMaxFloatCount;
	if (aFloatCount)
		CopyMemory(outFloatData, &aParamData->mFloatData[0], 4 * aFloatCount);
}

void D3DRenderEffect::SetCurrentTechnique(const std::string& inName, bool inCheckValid) //1357-1371
{
	TechniqueNameMap::iterator it = mTechniqueNameMap.find(inName);
	if (it != mTechniqueNameMap.end())
	{
		mCurrentTechnique = it->second;
		if (!mCurrentTechnique->mCompatFallback.empty())
			SetCurrentTechnique(mCurrentTechnique->mCompatFallback, inCheckValid);
		if (inCheckValid)
			mCurrentTechnique = mCurrentTechnique->GetValidTechnique(mTechniqueNameMap);
	}
	else
		mCurrentTechnique = NULL;
}

std::string D3DRenderEffect::GetCurrentTechniqueName() //1374-1376
{
	return mCurrentTechnique ? mCurrentTechnique->GetName() : "";
}

int D3DRenderEffect::Begin(const HANDLE& outRunHandle, const HRenderContext& inRenderContext) //1379-1393
{
	HRenderContext aContext = new HRenderContext();
	aContext.mHandleDword = inRenderContext.mHandleDword;
	if (aContext.IsValid())
		GetDevice()->SetCurrentContext(aContext);
	else
		aContext.mHandleDword = GetDevice()->GetCurrentContext().mHandleDword;
	outRunHandle = aContext.GetPointer();
	return mCurrentTechnique ? mCurrentTechnique->Begin() : 1;
}

void D3DRenderEffect::BeginPass(const HANDLE& inRunHandle, int inPass) //1395-1403
{
	GetDevice()->SetCurrentContext(HRenderContext(inRunHandle));
	mBeginPassRefCount++;
	GetDevice()->PushState(); //?
	if (mCurrentTechnique)
		mCurrentTechnique->BeginPass(inPass);
}

void D3DRenderEffect::EndPass(const HANDLE& inRunHandle, int inPass) //1405-1413
{
	GetDevice()->SetCurrentContext(HRenderContext(inRunHandle));

	if (mCurrentTechnique)
		mCurrentTechnique->EndPass(inPass);

	GetDevice()->PopState(); //?
	mBeginPassRefCount--;
}

void D3DRenderEffect::End(const HANDLE& inRunHandle) //1415-1420 (Accurate?)
{
	HRenderContext(*inRunHandle);
	GetDevice()->SetCurrentContext(inRunHandle);
	if (mCurrentTechnique)
		mCurrentTechnique->End();
}

bool D3DRenderEffect::PassUsesVertexShader(int inPass) //1423-1425
{
	return mCurrentTechnique && mCurrentTechnique->PassUsesVertexShader(inPass);
}

bool D3DRenderEffect::PassUsesPixelShader(int inPass) //1427-1429
{
	return mCurrentTechnique && mCurrentTechnique->PassUsesPixelShader(inPass);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static IUnknown* Sexy::CreateTextureSurface(D3DInterface* theInterface, int theWidth, int theHeight, PixelFormat theFormat, bool renderTarget) //1435-1455
{
	if (D3DInterface::CheckDXError(theInterface->SetTexture(0, NULL), "SetTexture NULL"))
		return NULL;

	if (renderTarget)
		theInterface->Flush(RenderDevice3D::FLUSHF_ManagedResources_Immediate);

	IUnknown* aSurface;
	HRESULT aResult = theInterface->InternalCreateTexture(theWidth, theHeight, 1, renderTarget, theFormat, !renderTarget, &aSurface);

	if (FAILED(aResult))
	{
		theInterface->Flush(RenderDevice3D::FLUSHF_ManagedResources_Immediate);
		const char* savedregs = "CreateTexture";
		aSurface = (IUnknown*)&aSurface;
		D3DInterface::CheckDXError(theInterface->InternalCreateTexture(theWidth, theHeight, 1, renderTarget, theFormat, !renderTarget, &aSurface), savedregs);
		return NULL;
	}
	return aSurface;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static IUnknown* Sexy::CreateCubeTextureSurface(D3DInterface* theInterface, int theDim, PixelFormat theFormat) //1460-1477
{
	if (D3DInterface::CheckDXError(theInterface->SetTexture(0, NULL), "SetTexture NULL"))
		return NULL;

	IUnknown* aCubeTex;
	HRESULT aResult = theInterface->InternalCreateCubeTexture(theDim, 1, 0, theFormat, 1, &aCubeTex);

	if (aResult < 0)
	{
		theInterface->Flush(RenderDevice3D::FLUSHF_ManagedResources_Immediate);
		const char* savedregs = "CreateCubeTexture";
		D3DInterface::CheckDXError(theInterface->InternalCreateCubeTexture(theDim, 1, 0, theFormat, 1, &aCubeTex), savedregs); //?
		return NULL;
	}
	else
		return aCubeTex;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static IUnknown* Sexy::CreateVolumeTextureSurface(D3DInterface* theInterface, int theDim, PixelFormat theFormat) //1482-1499
{
	if (D3DInterface::CheckDXError(theInterface->SetTexture(0, NULL), "SetTexture NULL"))
		return NULL;

	IUnknown* aVolumeTex;
	HRESULT aResult = theInterface->InternalCreateVolumeTexture(theDim, theDim, theDim, 1, 0, theFormat, 1, &aVolumeTex);

	if (aResult < 0)
	{
		theInterface->Flush(RenderDevice3D::FLUSHF_ManagedResources_Immediate);
		const char* savedregs = "CreateVolumeTexture";
		aResult = (HRESULT)&aVolumeTex;
		aVolumeTex = (IUnknown*)1;
		D3DInterface::CheckDXError(theInterface->InternalCreateVolumeTexture(theDim, theDim, theDim, 1, 0, theFormat, 1, &aVolumeTex), savedregs);
		return NULL;
	}
	else
		return aVolumeTex;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void Sexy::CopyImageToTexture8888(D3DInterface* theInterface, void* theDest, DWORD theDestPitch, MemoryImage* theImage, int offx, int offy, int theWidth, int theHeight, bool rightPad) //1507-1609
{

	if (theImage->mColorTable == NULL)
	{
		DWORD* srcRow = theImage->GetBits() + offy * theImage->GetWidth() + offx;
		char* dstRow = (char*)theDest;

		for (int y = 0; y < theHeight; y++)
		{
			DWORD* src = srcRow;
			DWORD* dst = (DWORD*)dstRow;
			for (int x = 0; x < theWidth; x++)
			{
				*dst++ = *src++;
			}

			if (rightPad)
				*dst = *(dst - 1);

			srcRow += theImage->GetWidth();
			dstRow += theDestPitch;
		}
	}
	else // palette
	{
		uchar* srcRow = (uchar*)theImage->mColorIndices + offy * theImage->GetWidth() + offx;
		uchar* dstRow = (uchar*)theDest;
		DWORD* palette = theImage->mColorTable;

		for (int y = 0; y < theHeight; y++)
		{
			uchar* src = srcRow;
			DWORD* dst = (DWORD*)dstRow;
			for (int x = 0; x < theWidth; x++)
				*dst++ = palette[*src++];

			if (rightPad)
				*dst = *(dst - 1);

			srcRow += theImage->GetWidth();
			dstRow += theDestPitch;
		}
	}
	if (theImage->mDither16 && theInterface->mDisplayFormat == SEXY3DFMT_R5G6B5)
	{
		if (!gQuantMaxTableInitialized)
		{
			for (int i = 0; i < sizeof(gQuantMaxTable); i++)
			{
				if (!gQuantMaxTableInitialized)
					gQuantMaxTable[i] = i <= 255 ? i : -1;
			}
			gQuantMaxTableInitialized = true;
		}
		//if ()
		//?
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void Sexy::CopyTexture8888ToImage(void* theDest, DWORD theDestPitch, MemoryImage* theImage, int offx, int offy, int theWidth, int theHeight) //1614-1629
{
	char* srcRow = (char*)theDest;
	DWORD* dstRow = theImage->GetBits() + offy * theImage->GetWidth() + offx;

	for (int y = 0; y < theHeight; y++)
	{
		DWORD* src = (DWORD*)srcRow;
		DWORD* dst = dstRow;

		for (int x = 0; x < theWidth; x++)
			*dst++ = *src++;

		dstRow += theImage->GetWidth();
		srcRow += theDestPitch;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void Sexy::CopyImageToTexture4444(void* theDest, DWORD theDestPitch, MemoryImage* theImage, int offx, int offy, int theWidth, int theHeight, bool rightPad) //1634-1681
{
	if (theImage->mColorTable == NULL)
	{
		DWORD* srcRow = theImage->GetBits() + offy * theImage->GetWidth() + offx;
		char* dstRow = (char*)theDest;

		for (int y = 0; y < theHeight; y++)
		{
			DWORD* src = srcRow;
			ushort* dst = (ushort*)dstRow;
			for (int x = 0; x < theWidth; x++)
			{
				DWORD aPixel = *src++;
				*dst++ = ((aPixel >> 16) & 0xF000) | ((aPixel >> 12) & 0x0F00) | ((aPixel >> 8) & 0x00F0) | ((aPixel >> 4) & 0x000F);
			}

			if (rightPad)
				*dst = *(dst - 1);

			srcRow += theImage->GetWidth();
			dstRow += theDestPitch;
		}
	}
	else // palette
	{
		uchar* srcRow = (uchar*)theImage->mColorIndices + offy * theImage->GetWidth() + offx;
		uchar* dstRow = (uchar*)theDest;
		DWORD* palette = theImage->mColorTable;

		for (int y = 0; y < theHeight; y++)
		{
			uchar* src = srcRow;
			ushort* dst = (ushort*)dstRow;
			for (int x = 0; x < theWidth; x++)
			{
				DWORD aPixel = palette[*src++];
				*dst++ = ((aPixel >> 16) & 0xF000) | ((aPixel >> 12) & 0x0F00) | ((aPixel >> 8) & 0x00F0) | ((aPixel >> 4) & 0x000F);
			}

			if (rightPad)
				*dst = *(dst - 1);

			srcRow += theImage->GetWidth();
			dstRow += theDestPitch;
		}

	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void Sexy::CopyTexture4444ToImage(void* theDest, DWORD theDestPitch, MemoryImage* theImage, int offx, int offy, int theWidth, int theHeight) //1686-1704
{
	char* srcRow = (char*)theDest;
	DWORD* dstRow = theImage->GetBits() + offy * theImage->GetWidth() + offx;

	for (int y = 0; y < theHeight; y++)
	{
		ushort* src = (ushort*)srcRow;
		DWORD* dst = dstRow;

		for (int x = 0; x < theWidth; x++)
		{
			ushort aPixel = *src++;
			*dst++ = 0xFF000000 | ((aPixel & 0xF000) << 16) | ((aPixel & 0x0F00) << 12) | ((aPixel & 0x00F0) << 8) | ((aPixel & 0x000F) << 4);
		}

		dstRow += theImage->GetWidth();
		srcRow += theDestPitch;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void Sexy::CopyImageToTexture565(void* theDest, DWORD theDestPitch, MemoryImage* theImage, int offx, int offy, int theWidth, int theHeight, bool rightPad) //1709-1862
{
	if (theImage->mDither16)
	{
		if (!gQuantMaxTableInitialized)
		{
			for (int i = 0; i < sizeof(gQuantMaxTable); i++)
			{
				if (!gQuantMaxTableInitialized)
					gQuantMaxTable[i] = i <= 255 ? i : -1;
			}
			gQuantMaxTableInitialized = true;
		}
		//if ()
		//?
		if (theImage->mColorTable == NULL)
		{
			DWORD* srcRow = theImage->GetBits() + offy * theImage->GetWidth() + offx;
			char* dstRow = (char*)theDest;

			for (int y = 0; y < theHeight; y++)
			{
				DWORD* src = srcRow;
				ushort* dst = (ushort*)dstRow;
				for (int x = 0; x < theWidth; x++)
				{
					DWORD aPixel = *src++;
					*dst++ = ((aPixel >> 8) & 0xF800) | ((aPixel >> 5) & 0x07E0) | ((aPixel >> 3) & 0x001F);
				}

				if (rightPad)
					*dst = *(dst - 1);

				srcRow += theImage->GetWidth();
				dstRow += theDestPitch;
			}
		}
		else // palette
		{
			uchar* srcRow = (uchar*)theImage->mColorIndices + offy * theImage->GetWidth() + offx;
			uchar* dstRow = (uchar*)theDest;
			DWORD* palette = theImage->mColorTable;

			for (int y = 0; y < theHeight; y++)
			{
				uchar* src = srcRow;
				ushort* dst = (ushort*)dstRow;
				for (int x = 0; x < theWidth; x++)
				{
					DWORD aPixel = palette[*src++];
					*dst++ = ((aPixel >> 8) & 0xF800) | ((aPixel >> 5) & 0x07E0) | ((aPixel >> 3) & 0x001F);
				}

				if (rightPad)
					*dst = *(dst - 1);

				srcRow += theImage->GetWidth();
				dstRow += theDestPitch;
			}

		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void Sexy::CopyTexture565ToImage(void* theDest, DWORD theDestPitch, MemoryImage* theImage, int offx, int offy, int theWidth, int theHeight) //1687-1885
{
	char* srcRow = (char*)theDest;
	DWORD* dstRow = theImage->GetBits() + offy * theImage->GetWidth() + offx;

	for (int y = 0; y < theHeight; y++)
	{
		ushort* src = (ushort*)srcRow;
		DWORD* dst = dstRow;

		for (int x = 0; x < theWidth; x++)
		{
			ushort aPixel = *src++;
			*dst++ = 0xFF000000 | ((aPixel & 0xF800) << 8) | ((aPixel & 0x07E0) << 5) | ((aPixel & 0x001F) << 3);
		}

		dstRow += theImage->GetWidth();
		srcRow += theDestPitch;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void Sexy::CopyImageToTexturePalette8(void* theDest, DWORD theDestPitch, MemoryImage* theImage, int offx, int offy, int theWidth, int theHeight, bool rightPad) //1891-1908
{
	uchar* srcRow = (uchar*)theImage->mColorIndices + offy * theImage->GetWidth() + offx;
	uchar* dstRow = (uchar*)theDest;

	for (int y = 0; y < theHeight; y++)
	{
		uchar* src = srcRow;
		uchar* dst = dstRow;
		for (int x = 0; x < theWidth; x++)
			*dst++ = *src++;

		if (rightPad)
			*dst = *(dst - 1);

		srcRow += theImage->GetWidth();
		dstRow += theDestPitch;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void Sexy::CopyTexturePalette8ToImage(void* theDest, DWORD theDestPitch, MemoryImage* theImage, int offx, int offy, int theWidth, int theHeight, int thePaletteIndex, D3DInterface* theInterface) //1913-1934
{
	char* srcRow = (char*)theDest;
	DWORD* dstRow = theImage->GetBits() + offy * theImage->GetWidth() + offx;

	PALETTEENTRY aPaletteEntries[256];
	theInterface->InternalGetPaletteEntries(thePaletteIndex, aPaletteEntries);

	for (int y = 0; y < theHeight; y++)
	{
		uchar* src = (uchar*)srcRow;
		DWORD* dst = dstRow;

		for (int x = 0; x < theWidth; x++)
		{
			DWORD aPixel = *((DWORD*)(aPaletteEntries + *src++));
			*dst++ = (aPixel & 0xFF00FF00) | ((aPixel >> 16) & 0xFF) | ((aPixel << 16) & 0xFF0000);
		}

		dstRow += theImage->GetWidth();
		srcRow += theDestPitch;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void Sexy::CopyImageToLockedRect(D3DInterface* theInterface, void* theLockedBits, int theLockedPitch, MemoryImage* theImage, int offx, int offy, int texWidth, int texHeight, PixelFormat theFormat) //1939-1964 (Partial copy of CopyImageToTexture)
{
	int aWidth = min(texWidth, (theImage->GetWidth() - offx));
	int aHeight = min(texHeight, (theImage->GetHeight() - offy));
	bool rightPad = aWidth < texWidth;
	bool bottomPad = aHeight < texHeight;
	if (aWidth > 0 && aHeight > 0)
	{
		switch (theFormat)
		{
			case PixelFormat::PixelFormat_A8R8G8B8:	CopyImageToTexture8888(theInterface, theLockedBits, theLockedPitch, theImage, offx, offy, aWidth, aHeight, rightPad); break;
			case PixelFormat::PixelFormat_A4R4G4B4:	CopyImageToTexture4444(theLockedBits, theLockedPitch, theImage, offx, offy, aWidth, aHeight, rightPad); break;
			case PixelFormat::PixelFormat_R5G6B5:	CopyImageToTexture565(theLockedBits, theLockedPitch, theImage, offx, offy, aWidth, aHeight, rightPad); break;
			case PixelFormat::PixelFormat_Palette8:	CopyImageToTexturePalette8(theLockedBits, theLockedPitch, theImage, offx, offy, aWidth, aHeight, rightPad); break;
		}
		if (bottomPad)
		{
			uchar* dstrow = ((uchar*)theLockedBits) + theLockedPitch * aHeight;
			memcpy(dstrow, dstrow - theLockedPitch, theLockedPitch);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void Sexy::CopyImageToTexture(D3DInterface* theInterface, IUnknown* theTexture, MemoryImage* theImage, int offx, int offy, int texWidth, int texHeight, PixelFormat theFormat) //1969-1981
{
	if (theTexture == NULL)
		return;

	int aLockedPitch;
	void* aLockedBits;

	if (D3DInterface::CheckDXError(theInterface->InternalTextureLockRect(theTexture, aLockedPitch, aLockedBits), "Lock Texture"))
		return;

	CopyImageToLockedRect(theInterface, aLockedBits, aLockedPitch, theImage, offx, offy, texWidth, texHeight, theFormat);

	D3DInterface::CheckDXError(theInterface->InternalTextureUnlockRect(theTexture), "Texture Unlock");
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void Sexy::CopyImageToCubeMap(D3DInterface* theInterface, IUnknown* theCubeTexture, MemoryImage* theImage, PixelFormat theFormat) //1986-2021
{
	const SEXY3DCUBEMAP_FACES sFaceOrder[6]{ SEXY3DCUBEMAP_FACE_NEGATIVE_Z, SEXY3DCUBEMAP_FACE_NEGATIVE_X, SEXY3DCUBEMAP_FACE_NEGATIVE_Z, SEXY3DCUBEMAP_FACE_NEGATIVE_X, SEXY3DCUBEMAP_FACE_NEGATIVE_Y, SEXY3DCUBEMAP_FACE_NEGATIVE_Y };

	if (theCubeTexture == NULL)
		return;

	int aLockedPitch;
	void* aLockedBits;

	int texWidth = theImage->GetCelWidth();
	int texHeight = theImage->GetCelHeight();
	int curX = 0;
	int curY = 0;

	for (int iFace = 0; iFace < 6; iFace++)
	{
		if (D3DInterface::CheckDXError(theInterface->InternalCubeTextureLockRect(theCubeTexture, sFaceOrder[iFace], aLockedPitch, aLockedBits), "Lock Cube Texture"))
			break;

		CopyImageToLockedRect(theInterface, aLockedBits, aLockedPitch, theImage, curX, curY, texWidth, texHeight, theFormat);
		D3DInterface::CheckDXError(theInterface->InternalCubeTextureUnlockRect(theCubeTexture, sFaceOrder[iFace]), "Cube Texture Unlock");
		curX += texWidth;
		if (curX >= theImage->GetWidth())
		{
			curX = 0;
			curY += texHeight;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void Sexy::CopyImageToVolumeMap(D3DInterface* theInterface, IUnknown* theVolumeTexture, MemoryImage* theImage, PixelFormat theFormat) //2026-2056
{
	if (theVolumeTexture == NULL)
		return;

	int aLockedRowPitch;
	int aLockedSlicePitch;
	void* aLockedBits;

	if (D3DInterface::CheckDXError(theInterface->InternalVolumeTextureLockBox(theVolumeTexture, aLockedRowPitch, aLockedSlicePitch, aLockedBits), "Lock Volume Texture"))
		return;

	int texWidth = theImage->GetCelWidth();
	int texHeight = theImage->GetCelHeight();
	int texDepth = theImage->mNumCols * theImage->mNumRows;
	int curX = 0;
	int curY = 0;

	for (int iDepth = 0; iDepth < texDepth; iDepth++)
	{
		CopyImageToLockedRect(theInterface, aLockedBits, aLockedRowPitch, theImage, curX, curY, texWidth, texHeight, theFormat);
		curX += texWidth;
		if (curX >= theImage->GetWidth())
		{
			curX = 0;
			curY += texHeight;
		}
		(char*)aLockedBits += aLockedSlicePitch;
	}
	D3DInterface::CheckDXError(theInterface->InternalVolumeTextureUnlockBox(theVolumeTexture), "Volume Texture Unlock");
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static int Sexy::GetClosestPowerOf2Above(int theNum) //2061-2067
{
	int aPower2 = 1;
	while (aPower2 < theNum)
		aPower2 <<= 1;

	return aPower2;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static bool Sexy::IsPowerOf2(int theNum) //2072-2081
{
	int aNumBits = 0;
	while (theNum > 0)
	{
		aNumBits += theNum & 1;
		theNum >>= 1;
	}

	return aNumBits == 1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void Sexy::GetBestTextureDimensions(int& theWidth, int& theHeight, bool isEdge, bool usePow2, DWORD theImageFlags) //2086-2170
{
	//	theImageFlags = D3DImageFlag_MinimizeNumSubdivisions;
	if (theImageFlags & ImageFlag_Use64By64Subdivisions)
	{
		theWidth = theHeight = 64;
		return;
	}

	static int aGoodTextureSize[D3DInterface::MAX_TEXTURE_SIZE];
	static bool haveInited = false;
	if (!haveInited)
	{
		haveInited = true;
		int i;
		int aPow2 = 1;
		for (i = 0; i < D3DInterface::MAX_TEXTURE_SIZE; i++)
		{
			if (i > aPow2)
				aPow2 <<= 1;

			int aGoodValue = aPow2;
			if ((aGoodValue - i) > 64)
			{
				aGoodValue >>= 1;
				while (true)
				{
					int aLeftOver = i % aGoodValue;
					if (aLeftOver < 64 || IsPowerOf2(aLeftOver))
						break;

					aGoodValue >>= 1;
				}
			}
			aGoodTextureSize[i] = aGoodValue;
		}
	}

	int aWidth = theWidth;
	int aHeight = theHeight;

	if (usePow2 && (theImageFlags & ImageFlag_RenderTarget))
	{
		if (isEdge || (theImageFlags & ImageFlag_MinimizeNumSubdivisions))
		{
			aWidth = aWidth >= sMaxTextureWidth ? sMaxTextureWidth : GetClosestPowerOf2Above(aWidth);
			aHeight = aHeight >= sMaxTextureHeight ? sMaxTextureHeight : GetClosestPowerOf2Above(aHeight);
		}
		else
		{
			aWidth = aWidth >= sMaxTextureWidth ? sMaxTextureWidth : aGoodTextureSize[aWidth];
			aHeight = aHeight >= sMaxTextureHeight ? sMaxTextureHeight : aGoodTextureSize[aHeight];
		}
	}
	//TODO
	if (aWidth < sMinTextureWidth)
		aWidth = sMinTextureWidth;

	if (aHeight < sMinTextureHeight)
		aHeight = sMinTextureHeight;

	if (aWidth > aHeight)
	{
		while (aWidth > sMaxTextureAspectRatio * aHeight)
			aHeight <<= 1;
	}
	else if (aHeight > aWidth)
	{
		while (aHeight > sMaxTextureAspectRatio * aWidth)
			aWidth <<= 1;
	}

	theWidth = aWidth;
	theHeight = aHeight;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
D3DTextureData::D3DTextureData(D3DInterface* theInterface) //2176-2191 (Changed to D3DTextureData)
{
	mInterface = theInterface;
	mWidth = 0;
	mHeight = 0;
	mTexVecWidth = 0;
	mTexVecHeight = 0;
	mBitsChangedCount = 0;
	mTexMemSize = 0;
	mTexMemOriginalSize = 0;
	mTexMemFlushRevision = 0;
	mTexPieceWidth = 64;
	mTexPieceHeight = 64;

	mPaletteIndex = -1;
	mPixelFormat = PixelFormat::PixelFormat_Unknown;
	mImageFlags = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
D3DTextureData::~D3DTextureData() //2196-2198
{
	ReleaseTextures();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DTextureData::ReleaseTextures() //2203-2241
{
	for (int i = 0; i < (int)mTextures.size(); i++)
	{
		IUnknown* aSurface = mTextures[i].mTexture;
		if (aSurface != NULL)
			aSurface->Release();
		IUnknown* aCubeTex = mTextures[i].mCubeTexture;
		if (aCubeTex != NULL)
			aCubeTex->Release();
		IUnknown* aVolumeTex = mTextures[i].mVolumeTexture;
		if (aVolumeTex != NULL)
			aVolumeTex->Release();
	}

	mTextures.clear();
	mInterface->mTexMemUsageBytesAlloced -= mTexMemSize;
	mTexMemSize = 0;
	mInterface->mTexMemUsageBytesOriginal -= mTexMemOriginalSize;
	if (mPaletteIndex != -1)
	{
		mInterface->mAvailPalettes.push_back(mPaletteIndex);
		mPaletteIndex = -1;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DTextureData::CreateTextureDimensions(MemoryImage* theImage) //2246-2345
{
	if (mImageFlags & (ImageFlag_CubeMap || ImageFlag_VolumeMap))
	{
		mTexPieceWidth = theImage->GetCelWidth();
		mWidth = mTexPieceWidth;
		mTexPieceHeight = theImage->GetCelHeight();
		mHeight = mTexPieceHeight;
		mTexVecHeight = 1;
		mTexVecWidth = 1;
		mMaxTotalV = 1.0;
		mMaxTotalU = 1.0;
		mTextures.resize(1);
		D3DTextureDataPiece* aPiece = &mTextures[0];
		aPiece->mTexture = 0;
		aPiece->mCubeTexture = 0;
		aPiece->mVolumeTexture = 0;
		aPiece->mWidth = theImage->GetCelWidth();
		aPiece->mHeight = theImage->GetCelHeight();
	}
	else
	{
		int aWidth = theImage->GetWidth();
		int aHeight = theImage->GetHeight();
		int i;
		/**/
			// Calculate inner piece sizes
		mTexPieceWidth = aWidth;
		mTexPieceHeight = aHeight;
		bool usePow2 = sTextureSizeMustBePow2;
		GetBestTextureDimensions(mTexPieceWidth, mTexPieceHeight, false, usePow2, mImageFlags); //Keeping it as usePow2

		// Calculate right boundary piece sizes
		int aRightWidth = aWidth % mTexPieceWidth;
		int aRightHeight = mTexPieceHeight;
		if (aRightWidth > 0)
			GetBestTextureDimensions(aRightWidth, aRightHeight, true, usePow2, mImageFlags);
		else
			aRightWidth = mTexPieceWidth;

		// Calculate bottom boundary piece sizes
		int aBottomWidth = mTexPieceWidth;
		int aBottomHeight = aHeight % mTexPieceHeight;
		if (aBottomHeight > 0)
			GetBestTextureDimensions(aBottomWidth, aBottomHeight, true, usePow2, mImageFlags);
		else
			aBottomHeight = mTexPieceHeight;

		// Calculate corner piece size
		int aCornerWidth = aRightWidth;
		int aCornerHeight = aBottomHeight;
		GetBestTextureDimensions(aCornerWidth, aCornerHeight, true, usePow2, mImageFlags);
		/**/

		//	mTexPieceWidth = 64;
		//	mTexPieceHeight = 64;


			// Allocate texture array
		mTexVecWidth = (aWidth + mTexPieceWidth - 1) / mTexPieceWidth;
		mTexVecHeight = (aHeight + mTexPieceHeight - 1) / mTexPieceHeight;
		mTextures.resize(mTexVecWidth * mTexVecHeight);

		// Assign inner pieces
		for (i = 0; i < (int)mTextures.size(); i++)
		{
			D3DTextureDataPiece& aPiece = mTextures[i];
			aPiece.mTexture = NULL;
			aPiece.mCubeTexture = NULL;
			aPiece.mVolumeTexture = NULL;
			aPiece.mWidth = mTexPieceWidth;
			aPiece.mHeight = mTexPieceHeight;
		}

		// Assign right pieces
	/**/
		for (i = mTexVecWidth - 1; i < (int)mTextures.size(); i += mTexVecWidth)
		{
			D3DTextureDataPiece& aPiece = mTextures[i];
			aPiece.mWidth = aRightWidth;
			aPiece.mHeight = aRightHeight;
		}

		// Assign bottom pieces
		for (i = mTexVecWidth * (mTexVecHeight - 1); i < (int)mTextures.size(); i++)
		{
			D3DTextureDataPiece& aPiece = mTextures[i];
			aPiece.mWidth = aBottomWidth;
			aPiece.mHeight = aBottomHeight;
		}

		// Assign corner piece
		mTextures.back().mWidth = aCornerWidth;
		mTextures.back().mHeight = aCornerHeight;
		/**/

		mMaxTotalU = aWidth / (float)mTexPieceWidth;
		mMaxTotalV = aHeight / (float)mTexPieceHeight;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DTextureData::CreateTextures(MemoryImage* theImage, D3DInterface* theInterface) //2350-2570
{
	assert(theInterface == mInterface); //2351
	theImage->DeleteSWBuffers(); // don't need these buffers for 3d drawing

	// Choose appropriate pixel format
	PixelFormat aFormat = PixelFormat::PixelFormat_A8R8G8B8;
	//theImage->mD3DFlags = D3DImageFlag_UseA4R4G4B4;

	theImage->CommitBits();
	if (!theImage->mHasAlpha && !theImage->mHasTrans && (sSupportedTextureFormats & PixelFormat::PixelFormat_R5G6B5))
	{
		if (!(theImage->HasImageFlag(ImageFlag_RenderTarget)) && theInterface->mDisplayFormat != SEXY3DFMT_R5G6B5 || theImage->HasImageFlag(ImageFlag_UseA8R8G8B8))
			aFormat = PixelFormat::PixelFormat_X8R8G8B8;
	}
	else
		aFormat = PixelFormat::PixelFormat_R5G6B5;

	if (theImage->mColorIndices != NULL && (sSupportedTextureFormats & PixelFormat::PixelFormat_Palette8) && !theInterface->mAvailPalettes.empty())
	{
		PALETTEENTRY aPalette[256];
		for (int i = 0; i < 256; i++)
		{
			DWORD aPixel = theImage->mColorTable[i];
			*(DWORD*)(aPalette + i) = (aPixel & 0xFF00FF00) | ((aPixel >> 16) & 0xFF) | ((aPixel << 16) & 0xFF0000);
		}
		int aPaletteIndex = theInterface->mAvailPalettes.back();
		HRESULT aResult = theInterface->InternalSetPaletteEntries(aPaletteIndex, aPalette);
		if (SUCCEEDED(aResult))
		{
			theInterface->mAvailPalettes.pop_back();
			mPaletteIndex = aPaletteIndex;
			aFormat = PixelFormat::PixelFormat_Palette8;
		}
		else
		{
			std::string anError = GetDirectXErrorString(aResult);
			sSupportedTextureFormats &= ~PixelFormat::PixelFormat_Palette8;
			mPaletteIndex--;
		}
	}

	if (theImage->HasImageFlag(ImageFlag_UseA4R4G4B4) && aFormat == PixelFormat::PixelFormat_A8R8G8B8 && sSupportedTextureFormats & PixelFormat::PixelFormat_A4R4G4B4)
		aFormat = PixelFormat::PixelFormat_A4R4G4B4;

	if (aFormat == PixelFormat::PixelFormat_A8R8G8B8 && !(sSupportedTextureFormats & PixelFormat::PixelFormat_A8R8G8B8))
		aFormat = PixelFormat::PixelFormat_A4R4G4B4;


	// Release texture if image size has changed
	bool createTextures = false;
	if (mWidth != theImage->mWidth || mHeight != theImage->mHeight || aFormat != mPixelFormat || theImage->GetImageFlags() != mImageFlags)
	{
		ReleaseTextures();

		mPixelFormat = aFormat;
		mImageFlags = theImage->GetImageFlags();
		CreateTextureDimensions(theImage);
		createTextures = true;
	}

	int i, x, y;

	int aHeight = theImage->GetHeight();
	int aWidth = theImage->GetWidth();

	if (mPaletteIndex != -1)
	{
		mTexMemSize += 256 * 4;
		mTexMemOriginalSize += 256 * 4;
	}

	int aFormatSize = 4;
	if (aFormat == PixelFormat::PixelFormat_Palette8)
		aFormatSize = 1;
	else if (aFormat == PixelFormat::PixelFormat_R5G6B5)
		aFormatSize = 2;
	else if (aFormat == PixelFormat::PixelFormat_A4R4G4B4)
		aFormatSize = 2;

	if (mImageFlags & ImageFlag_CubeMap) //Huh they didn't use HasImageFlags
	{
		D3DTextureDataPiece& aPiece = mTextures[0];
		if (createTextures)
		{
			assert(mTexMemSize == 0); //2444
			assert(mTexMemOriginalSize == 0); //2445
			aPiece.mCubeTexture = CreateCubeTextureSurface(theInterface, aPiece.mWidth, aFormat);
			if (aPiece.mCubeTexture == NULL)
			{
				mPixelFormat = PixelFormat::PixelFormat_Unknown;
				return;
			}
			ulong aOriginalSize = aFormatSize * theImage->GetHeight() * theImage->GetWidth();
			mTexMemSize += aOriginalSize;
			mTexMemOriginalSize += aOriginalSize;
			theInterface->mTexMemUsageBytesAlloced += mTexMemSize;
			theInterface->mTexMemUsageBytesOriginal += mTexMemOriginalSize;
		}
		CopyImageToCubeMap(theInterface, aPiece.mCubeTexture, theImage, aFormat);
		mWidth = theImage->GetWidth();
		mHeight = theImage->GetHeight();
		mBitsChangedCount = theImage->mBitsChangedCount;
		mPixelFormat = aFormat;
	}

	else if (mImageFlags & ImageFlag_VolumeMap) //Huh they didn't use HasImageFlags
	{
		D3DTextureDataPiece& aPiece = mTextures[0];
		if (createTextures)
		{
			assert(mTexMemSize == 0); //2479
			assert(mTexMemOriginalSize == 0); //2480
			aPiece.mVolumeTexture = CreateVolumeTextureSurface(theInterface, aPiece.mWidth, aFormat);
			if (aPiece.mVolumeTexture == NULL)
			{
				mPixelFormat = PixelFormat::PixelFormat_Unknown;
				return;
			}
			ulong aOriginalSize = aFormatSize * theImage->GetHeight() * theImage->GetWidth();
			mTexMemSize += aOriginalSize;
			mTexMemOriginalSize += aOriginalSize;
			theInterface->mTexMemUsageBytesAlloced += mTexMemSize;
			theInterface->mTexMemUsageBytesOriginal += mTexMemOriginalSize;
		}
		CopyImageToCubeMap(theInterface, aPiece.mVolumeTexture, theImage, aFormat);
		mWidth = theImage->GetWidth();
		mHeight = theImage->GetHeight();
		mBitsChangedCount = theImage->mBitsChangedCount;
		mPixelFormat = aFormat;
	}

	else
	{
		if (createTextures)
		{
			assert(mTexMemSize == 0); //2510
			assert(mTexMemOriginalSize == 0); //2511
		}
		i = 0;
		for (y = 0; y < aHeight; y += mTexPieceHeight)
		{
			for (x = 0; x < aWidth; x += mTexPieceWidth, i++)
			{
				D3DTextureDataPiece& aPiece = mTextures[i];
				if (createTextures)
				{
					aPiece.mTexture = CreateTextureSurface(theInterface, aPiece.mWidth, aPiece.mHeight, aFormat, theImage->HasImageFlag(ImageFlag_RenderTarget));
					if (aPiece.mTexture == NULL) // create texture failure
					{
						mPixelFormat = PixelFormat::PixelFormat_Unknown;
						return;
					}

					mTexMemSize += aPiece.mWidth * aPiece.mHeight * aFormatSize;
				}
				if (theImage->HasImageFlag(ImageFlag_RenderTarget))
				{
					if (theImage->mBits)
					{
						IUnknown* aTempTex;
						if (theInterface->CheckDXError(theInterface->InternalCreateTexture(theImage->mWidth, theImage->mHeight, 1, false, aFormat, 2, &aTempTex), ""))
							return;
						CopyImageToTexture(theInterface, aTempTex, theImage, x, y, aPiece.mWidth, aPiece.mHeight, aFormat);
						theInterface->InternalTextureMakeDirty(aTempTex);
						theInterface->InternalTextureMakeDirty(aPiece.mTexture);
						theInterface->CheckDXError(theInterface->InternalUpdateTexture(aTempTex, aPiece.mTexture), "");
						//delete aTempTex; //?
						aTempTex->Release();
					}
				}

				else if (theImage->mBits)
					CopyImageToTexture(theInterface, aPiece.mTexture, theImage, x, y, aPiece.mWidth, aPiece.mHeight, aFormat);
			}
		}
		if (createTextures)
		{
			mTexMemOriginalSize += theImage->GetWidth() * theImage->GetHeight() * aFormatSize;
			mBitsChangedCount = theImage->mBitsChangedCount;
			theInterface->mTexMemUsageBytesAlloced += mTexMemSize;
			theInterface->mTexMemUsageBytesOriginal += mTexMemOriginalSize;
		}
		mWidth = theImage->mWidth;
		mHeight = theImage->mHeight;
		mBitsChangedCount = theImage->mBitsChangedCount;
		mPixelFormat = aFormat;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DTextureData::CheckCreateTextures(MemoryImage* theImage, D3DInterface* theInterface) //2575-2578
{
	if (mPixelFormat == PixelFormat::PixelFormat_Unknown || theImage->mWidth != mWidth || theImage->mHeight != mHeight || theImage->mBitsChangedCount != mBitsChangedCount || theImage->GetImageFlags() != mImageFlags)
		CreateTextures(theImage, theInterface);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
IUnknown* D3DTextureData::GetTexture(int x, int y, int& width, int& height, float& u1, float& v1, float& u2, float& v2) //2583=2612
{
	if (mImageFlags & (ImageFlag_CubeMap || ImageFlag_VolumeMap))
		return NULL;

	int tx = x / mTexPieceWidth;
	int ty = y / mTexPieceHeight;

	D3DTextureDataPiece& aPiece = mTextures[ty * mTexVecWidth + tx];

	int left = x % mTexPieceWidth;
	int top = y % mTexPieceHeight;
	int right = left + width;
	int bottom = top + height;

	if (right > aPiece.mWidth)
		right = aPiece.mWidth;

	if (bottom > aPiece.mHeight)
		bottom = aPiece.mHeight;

	width = right - left;
	height = bottom - top;

	u1 = (float)left / aPiece.mWidth;
	v1 = (float)top / aPiece.mHeight;
	u2 = (float)right / aPiece.mWidth;
	v2 = (float)bottom / aPiece.mHeight;

	return aPiece.mTexture;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
IUnknown* D3DTextureData::GetTextureF(float x, float y, float& width, float& height, float& u1, float& v1, float& u2, float& v2) //2617-2646
{
	if (mImageFlags & (ImageFlag_CubeMap || ImageFlag_VolumeMap))
		return NULL;

	int tx = x / mTexPieceWidth;
	int ty = y / mTexPieceHeight;

	D3DTextureDataPiece& aPiece = mTextures[ty * mTexVecWidth + tx];

	float left = x - tx * mTexPieceWidth;
	float top = y - ty * mTexPieceHeight;
	float right = left + width;
	float bottom = top + height;

	if (right > aPiece.mWidth)
		right = aPiece.mWidth;

	if (bottom > aPiece.mHeight)
		bottom = aPiece.mHeight;

	width = right - left;
	height = bottom - top;

	u1 = (float)left / aPiece.mWidth;
	v1 = (float)top / aPiece.mHeight;
	u2 = (float)right / aPiece.mWidth;
	v2 = (float)bottom / aPiece.mHeight;

	return aPiece.mTexture;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
IUnknown* D3DTextureData::GetCubeTexture() //Good? | 2651-2656
{
	if ((mImageFlags & ImageFlag_CubeMap) != 0)
		return mTextures[0].mCubeTexture;
	else
		return NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
IUnknown* D3DTextureData::GetVolumeTexture() //Good? | 2661-2666
{
	if ((mImageFlags & ImageFlag_VolumeMap) != 0)
		return mTextures[0].mCubeTexture;
	else
		return NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DTextureData::Blt(D3DInterface* theInterface, MemoryImage* theImage, int theDrawMode, float theX, float theY, const Rect& theSrcRect, const Color& theColor) //TODO | 2672-2849
{
	assert(theInterface == mInterface); //2673
	if (mTexMemFlushRevision < theInterface->mTexMemUsageFlushRevision)
	{
		theInterface->mTexMemUsageBytesCurFrame += mTexMemSize;
		mTexMemFlushRevision = theInterface->mTexMemUsageFlushRevision;
	}
	int srcLeft = theSrcRect.mX;
	int srcTop = theSrcRect.mY;
	int srcRight = srcLeft + theSrcRect.mWidth;
	int srcBottom = srcTop + theSrcRect.mHeight;
	int srcX, srcY;
	float dstX, dstY;
	int aWidth,aHeight;
	float u1,v1,u2,v2;

	gPixelTracerLastImage = theImage;
	srcY = srcTop;
	dstY = theY;

	DWORD aColor = RGBA_MAKE(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);

	if ((srcLeft >= srcRight) || (srcTop >= srcBottom))
		return;

	gPixelTracerSrcRect = &theSrcRect;
	float z = theInterface->mStateMgr->GetBltDepth();

	while (srcY < srcBottom)
	{
		srcX = srcLeft;
		dstX = theX;
		while (srcX < srcRight)
		{
			aWidth = srcRight - srcX;
			aHeight = srcBottom - srcY;
			IUnknown* aTexture = GetTexture(srcX, srcY, aWidth, aHeight, u1, v1, u2, v2);

			if (mPaletteIndex != -1)
				theInterface->mStateMgr->SetCurrentTexturePalette(mPaletteIndex);

			theInterface->SetTextureDirect(0, aTexture);

			float x = dstX - 0.5f;
			float y = dstY - 0.5f;

			std::vector<MemoryImage::TriRep::Tri> clippedTris;

			MemoryImage::TriRep::Tri* triUVs = NULL;
			int triCount = 0;

			if (mTextures.size() == 1)
			{
				float usageFrac = (float)(aHeight * aWidth) / (float)theInterface->mHeight * theInterface->mWidth;
				MemoryImage::TriRep* triRep = theImage ? (theDrawMode ? &theImage->mAdditiveTriRep : &theImage->mNormalTriRep) : NULL;
				MemoryImage::TriRep::Level* triRepLevel = triRep ? triRep->GetLevelForScreenSpaceUsage(usageFrac, false) : NULL;
				if (triRepLevel && theImage->HasImageFlag(ImageFlag_NoTriRep) && (gSexyAppBase->mGraphicsDriver->GetRenderModeFlags() & IGraphicsDriver::RENDERMODEF_NoTriRep) != 0)
					triRepLevel = NULL;
				if (triRepLevel)
				{
					triRepLevel->GetRegionTris(clippedTris, theImage, theSrcRect, false);
					triCount = clippedTris.size();
					if (triCount)
						triUVs = &clippedTris.front();
				}
			}

			if (triCount <= 0)
			{
				_D3DTLVERTEX aVertex[4] =
				{
					{ x,				y,				0,	1,	aColor,	0,	u1,		v1 },
					{ x,				y + aHeight,		0,	1,	aColor,	0,	u1,		v2 },
					{ x + aWidth,			y,				0,	1,	aColor,	0,	u2,		v1 },
					{ x + aWidth,			y + aHeight,		0,	1,	aColor,	0,	u2,		v2 }
				};
				theInterface->BufferedDrawPrimitive(Graphics3D::PT_TriangleStrip, 2, aVertex, 32, 0x1C4); //?
			}
			else
			{
				_D3DTLVERTEX* aTriVertex = aVertex[2];
				_D3DTLVERTEX* aStartVertex = aVertex[2];
				MemoryImage::TriRep::Tri* aTriUV = triUVs;
				if (u1 == 0.0 && v1 == 0.0 && u2 == 1.0 && v2 == 1.0)
				{
					int q = 0;
					while (q < triCount)
					{
						MemoryImage::TriRep::Tri::Point* trp = &aTriUV->p0;
						for (int v = 0; v < 3; v++)
						{
							aTriVertex[v] =
							{
								{ (float)aWidth * trp[v].u + x,				(float)aHeight * trp[v].v + y,				z,	1.0,	aColor,	0,	trp[v].u,		trp[v].v }
							};
						}
						q++;
						aTriVertex += 3;
						aTriUV++;
					}
				}
				else
				{
					float inUsageFrac = (float)aWidth;
					SexyVector2 uVector(inUsageFrac, 0.0);
					SexyVector2 vVector(0.0, (float)aHeight);
					SexyVector2 basePos(x, y);
					if (u1 != 0.0 || u2 != 1.0)
					{
						float texDelta = u2 - u1;
						float ootexDelta = 1.0 / texDelta;
						uVector *= ootexDelta;
						basePos -= uVector * u1;
					}
					if (v1 != 0.0 || v2 != 1.0)
					{
						float texDelta = v2 - v1;
						float ootexDelta = 1.0 / texDelta;
						vVector *= ootexDelta;
						basePos -= vVector * v1;
					}
					while (? < triCount)
					{
						SexyVector2 pos;
						for (int i = 0; i < 3; i++)
						{
							pos.x = basePos + uVector * aTriUV->p0.u + 2 * i + vVector * aTriUV->p0.v + 2 * i;
							pos.y = basePos + uVector * aTriUV->p0.u + 2 * i + vVector * aTriUV->p0.v + 2 * i;
							aTriVertex[i] =
							{
								{ pos.x, pos.y, z, 1.0, aColor, 0,  aTriUV->p0.u + 2 * i, aTriUV->p0.v + 2 * i }
							};
						}
						? ++;
						aTriVertex += 3;
						aTriUV++;
					}
				}
				theInterface->BufferedDrawPrimitive(4, triCount, aStartVertex, 32, 0x1C4);

				srcX += aWidth;
				dstX += aWidth;
			}

			srcY += aHeight;
			dstY += aHeight;
		}
		gPixelTracerSrcRect = NULL;
	}
}

namespace Sexy //?
{
	///////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////
	//typedef std::vector<D3DTLVERTEX> D3DVertexList;
	struct D3DVertexList
	{
		enum { MAX_STACK_VERTS = 100 };
		_D3DTLVERTEX mStackVerts[MAX_STACK_VERTS];
		_D3DTLVERTEX* mVerts;
		int mSize;
		int mCapacity;

		typedef int size_type;

		D3DVertexList() : mSize(0), mCapacity(MAX_STACK_VERTS), mVerts(mStackVerts) { } //2864
		D3DVertexList(const D3DVertexList& theList) : mSize(theList.mSize), mCapacity(MAX_STACK_VERTS), mVerts(mStackVerts)
		{
			reserve(mSize);
			memcpy(mVerts, theList.mVerts, mSize * sizeof(mVerts[0]));
		}

		~D3DVertexList() //2872-2875
		{
			if (mVerts != mStackVerts)
				delete mVerts;
		}

		void reserve(int theCapacity) //2678-2889
		{
			if (mCapacity < theCapacity)
			{
				mCapacity = theCapacity;
				_D3DTLVERTEX* aNewList = new _D3DTLVERTEX[theCapacity];
				memcpy(aNewList, mVerts, mSize * sizeof(mVerts[0]));
				if (mVerts != mStackVerts)
					delete mVerts;

				mVerts = aNewList;
			}
		}

		void push_back(const _D3DTLVERTEX& theVert) //2892-2897
		{
			if (mSize == mCapacity)
				reserve(mCapacity * 2);

			mVerts[mSize++] = theVert;
		}

		void operator=(const D3DVertexList& theList) //2900-2904
		{
			reserve(theList.mSize);
			mSize = theList.mSize;
			memcpy(mVerts, theList.mVerts, mSize * sizeof(mVerts[0]));
		}


		_D3DTLVERTEX& operator[](int thePos) //2908-2910
		{
			return mVerts[thePos];
		}

		int size() { return mSize; } //2912
		void clear() { mSize = 0; } //2913
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static inline float Sexy::GetCoord(const _D3DTLVERTEX &theVertex, int theCoord) //2919-2929
{
	switch (theCoord)
	{
		case 0: return theVertex.sx;
		case 1: return theVertex.sy;
		case 2: return theVertex.sz;
		case 3: return theVertex.tu;
		case 4: return theVertex.tv;
		default: return 0;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static inline Sexy::_D3DTLVERTEX Sexy::Interpolate(const _D3DTLVERTEX &v1, const _D3DTLVERTEX &v2, float t) //TODO | 2934-2957
{
	_D3DTLVERTEX aVertex = v1;
	aVertex.sx = v1.sx + t*(v2.sx-v1.sx);
	aVertex.sy = v1.sy + t*(v2.sy-v1.sy);
	aVertex.tu = v1.tu + t*(v2.tu-v1.tu);
	aVertex.tv = v1.tv + t*(v2.tv-v1.tv);
	if (v1.color!=v2.color)
	{
		int r = /*SexyMath->Lerp*/(RGBA_GETRED(v1.color) + t*(RGBA_GETRED(v2.color) - RGBA_GETRED(v1.color)));
		int g = /*SexyMath->Lerp*/(RGBA_GETGREEN(v1.color) + t*(RGBA_GETGREEN(v2.color) - RGBA_GETGREEN(v1.color)));
		int b = /*SexyMath->Lerp*/(RGBA_GETBLUE(v1.color) + t*(RGBA_GETBLUE(v2.color) - RGBA_GETBLUE(v1.color)));
		int a = /*SexyMath->Lerp*/(RGBA_GETALPHA(v1.color) + t*(RGBA_GETALPHA(v2.color) - RGBA_GETALPHA(v1.color)));
		aVertex.color = RGBA_MAKE(r,g,b,a);
	}
	
	return aVertex;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
template<class Pred>
struct PointClipper
{
	Pred mPred;

	void ClipPoint(int n, float clipVal, const Sexy::_D3DTLVERTEX &v1, const Sexy::_D3DTLVERTEX &v2, D3DVertexList &out);
	void ClipPoints(int n, float clipVal, D3DVertexList &in, D3DVertexList &out);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
template<class Pred>
void PointClipper<Pred>::ClipPoint(int n, float clipVal, const Sexy::_D3DTLVERTEX &v1, const Sexy::_D3DTLVERTEX &v2, D3DVertexList &out) //2975-2996
{
	if (!mPred(GetCoord(v1,n), clipVal))
	{
		if (!mPred(GetCoord(v2,n), clipVal)) // both inside
			out.push_back(v2);
		else // inside -> outside
		{
			float t = (clipVal - GetCoord(v1,n))/(GetCoord(v2,n)-GetCoord(v1,n));
			out.push_back(Interpolate(v1,v2,t));
		}
	}
	else
	{
		if (!mPred(GetCoord(v2,n), clipVal)) // outside -> inside
		{
			float t = (clipVal - GetCoord(v1, n))/(GetCoord(v2,n)-GetCoord(v1,n));
			out.push_back(Interpolate(v1,v2,t));
			out.push_back(v2);
		}
//			else // outside -> outside
	}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
template<class Pred>
void PointClipper<Pred>::ClipPoints(int n, float clipVal, D3DVertexList &in, D3DVertexList &out) //3003-3010
{
	if(in.size()<2)
		return;

	ClipPoint(n,clipVal,in[in.size()-1],in[0],out);
	for(D3DVertexList::size_type i=0; i<in.size()-1; i++)
		ClipPoint(n,clipVal,in[i],in[i+1],out);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DTextureData::BltTransformed(D3DInterface* theInterface, MemoryImage* theImage, int theDrawMode, const SexyMatrix3& theTrans, const Rect& theSrcRect, const Color& theColor, const Rect* theClipRect = NULL, float theX = 0, float theY = 0, bool center = false) //TODO | 3036-3298
{
	assert(theInterface == mInterface); //3037

	/*if (mTexMemFlushRevision < theInterface->mTexMemUsageFlushRevision)
	{
		theInterface->mTexMemUsageBytesCurFrame += mTexMemSize;
		mTexMemFlushRevision = theInterface->mTexMemUsageFlushRevision;
	}

	int srcLeft = theSrcRect.mX;
	int srcTop = theSrcRect.mY;
	int srcRight = srcLeft + theSrcRect.mWidth;
	int srcBottom = srcTop + theSrcRect.mHeight;
	int srcX, srcY;
	float dstX, dstY;
	int aWidth;
	int aHeight;
	float u1, v1, u2, v2;
	float startx = 0, starty = 0;
	float pixelcorrect = 0.5f;

	if (center)
	{
		startx = -theSrcRect.mWidth / 2.0f;
		starty = -theSrcRect.mHeight / 2.0f;
		pixelcorrect = 0.0f;
	}

	gPixelTracerLastImage = theImage;
	srcY = srcTop;
	dstY = starty;

	DWORD aColor = RGBA_MAKE(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);

	if ((srcLeft >= srcRight) || (srcTop >= srcBottom))
		return;

	gPixelTracerSrcRect = theSrcRect;
	float z = theInterface->mStateMgr->GetBltDepth();

	while (srcY < srcBottom)
	{
		srcX = srcLeft;
		dstX = startx;
		while (srcX < srcRight)
		{
			aWidth = srcRight - srcX;
			aHeight = srcBottom - srcY;
			IUnknown* aTexture = GetTexture(srcX, srcY, aWidth, aHeight, u1, v1, u2, v2);

			if (mPaletteIndex != -1)
				theInterface->mStateMgr->SetCurrentTexturePalette(mPaletteIndex);

			theInterface->SetTextureDirect(NULL, aTexture);

			float x = dstX; // - 0.5f;
			float y = dstY; // - 0.5f;

			SexyVector2 p[4] = { SexyVector2(x, y), SexyVector2(x,y + aHeight), SexyVector2(x + aWidth, y) , SexyVector2(x + aWidth, y + aHeight) };
			SexyVector2 tp[4];

			int i;
			for (i = 0; i < 4; i++)
			{
				tp[i] = theTrans * p[i];
				tp[i].x -= pixelcorrect - theX;
				tp[i].y -= pixelcorrect - theY;
			}

			_D3DTLVERTEX aVertex[4] =
			{
				{ tp[0].x,				tp[0].y,			0,	1,	aColor,	0,	u1,		v1 },
				{ tp[1].x,				tp[1].y,			0,	1,	aColor,	0,	u1,		v2 },
				{ tp[2].x,				tp[2].y,			0,	1,	aColor,	0,	u2,		v1 },
				{ tp[3].x,				tp[3].y,			0,	1,	aColor,	0,	u2,		v2 }
			};

			const Rect* aClipRect = theClipRect;

			if (theInterface->mBitFilter && theInterface->mBitFilter(theInterface->mBltFilterContext, 5, 2u, aVertex, 32, &aClipRect))
			{
				gPixelTracerSrcRect = NULL;
				return;
			}

			if (aClipRect && (theInterface->mTransformStack.size() || aClipRect->mX || aClipRect->mY || aClipRect->mWidth != theInterface->mWidth || aClipRect->mHeight != theInterface->mHeight))
			{
				static Rect aNewClipRect;

				SexyVector2 ? (aClipRect->mX, aClipRect->mY); //?
				SexyVector2 ? (aClipRect->mX + aClipRect->mY, aClipRect->mHeight + aClipRect->mY); //?
				if (theInterface->mTransformStack.size())
				{
					//?
				}
			}

			bool clipped = false;
			if (theClipRect != nullptr)
			{
				int left = theClipRect->mX;
				int right = left + theClipRect->mWidth;
				int top = theClipRect->mY;
				int bottom = top + theClipRect->mHeight;
				for (i = 0; i < 4; i++)
				{
					if (tp[i].x < left || tp[i].x >= right || tp[i].y < top || tp[i].y >= bottom)
					{
						clipped = true;
						break;
					}
				}
			}

			D3DInterface::CheckDXError(theDevice->SetTexture(0, aTexture), "SetTexture gTexture");

			if (!clipped)
				D3DInterface::CheckDXError(theDevice->DrawPrimitive(D3DGraphics3D::PT_TRIANGLESTRIP, gVertexType, aVertex, 4, 0), "DrawPrimitive (Tri) 3");
			else
			{
				VertexList aList;
				aList.push_back(aVertex[0]);
				aList.push_back(aVertex[1]);
				aList.push_back(aVertex[3]);
				aList.push_back(aVertex[2]);

				DrawPolyClipped(theDevice, theClipRect, aList);
				//				DrawPolyClipped(theDevice, theClipRect, aVertex+1, 3);
			}

			//			D3DInterface::CheckDXError(theDevice->SetTexture(0, nullptr),"SetTexture nullptr");

			srcX += aWidth;
			dstX += aWidth;
		}

		srcY += aHeight;
		dstY += aHeight;
	}*/
}




///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#define GetColorFromSexyVertex2D(theVertex, theColor) (theVertex.color?theVertex.color:theColor)

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void Sexy::AdjustPrimCount(ulong* ioPrimCount) //3309-3315 (Accurate?)
{
	if (*ioPrimCount > 1)
	{
		if ((gSexyAppBase->mGraphicsDriver->GetRenderModeFlags() & IGraphicsDriver::RENDERMODEF_HalfTris) != 0)
			*ioPrimCount >>= 1;
	}
}
void Sexy::MetricsAddPrimitive(ulong thePrimType, ulong thePrimCount) //3317-3339 (Accurate?)
{
	GraphicsMetrics* metrics;
	switch (thePrimType)
	{
	case Graphics3D::PT_TriangleList:
		metrics[GraphicsMetrics::CT_TriListCalls] ++;
		metrics[GraphicsMetrics::CT_TriListPrims] += thePrimCount;
		break;
	case Graphics3D::PT_TriangleStrip:
		metrics[GraphicsMetrics::CT_TriStripCalls] ++;
		metrics[GraphicsMetrics::CT_TriStripPrims] += thePrimCount;
		break;
	case Graphics3D::PT_TriangleFan:
		metrics[GraphicsMetrics::CT_TriFanCalls] ++;
		metrics[GraphicsMetrics::CT_TriFanPrims] += thePrimCount;
		break;
	case Graphics3D::PT_LineStrip:
		metrics[GraphicsMetrics::CT_LineStripCalls] ++;
		metrics[GraphicsMetrics::CT_LineStripPrims] += thePrimCount;
		break;
	}
}



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DTextureData::BltTriangles(D3DInterface* theInterface, const SexyVertex2D theVertices[][3], int theNumTriangles, DWORD theColor, float tx, float ty, const Rect* theClipRect) //TODO | 3346-3561
{
	assert(theInterface == mInterface); //3447

	/*if (mTexMemFlushRevision < theInterface->mTexMemUsageFlushRevision)
	{
		theInterface->mTexMemUsageBytesCurFrame += mTexMemSize;
		mTexMemFlushRevision = theInterface->mTexMemUsageFlushRevision;
	}
	if ((mMaxTotalU <= 1.0) && (mMaxTotalV <= 1.0))
	{
		if (mPaletteIndex != -1)
			theInterface->mStateMgr->SetCurrentTexturePalette(mPaletteIndex);
		theInterface->SetTextureDirect(mTextures[0].mTexture);

		_D3DTLVERTEX aVertexCache[100][3];
		int aVertexCacheNum = 0;

		theInterface->FlushBufferedTriangles();

		for (int aTriangleNum = 0; aTriangleNum < theNumTriangles; aTriangleNum++)
		{
			SexyVertex2D* aTriVerts = (SexyVertex2D*)theVertices[aTriangleNum];
			_D3DTLVERTEX* aD3DVertex = (_D3DTLVERTEX*)aVertexCache[aVertexCacheNum++];

			aD3DVertex[0].sx = aTriVerts[0].x + tx;
			aD3DVertex[0].sy = aTriVerts[0].y + ty;
			aD3DVertex[0].sz = theInterface->mStateMgr->GetBitDepth();
			aD3DVertex[0].rhw = 1;
			aD3DVertex[0].color = GetColorFromSexyVertex2D(aTriVerts[0], theColor);
			aD3DVertex[0].specular = 0;
			aD3DVertex[0].tu = aTriVerts[0].u * mMaxTotalU;
			aD3DVertex[0].tv = aTriVerts[0].v * mMaxTotalV;

			aD3DVertex[1].sx = aTriVerts[1].x + tx;
			aD3DVertex[1].sy = aTriVerts[1].y + ty;
			aD3DVertex[1].sz = theInterface->mStateMgr->GetBitDepth();
			aD3DVertex[1].rhw = 1;
			aD3DVertex[1].color = GetColorFromSexyVertex2D(aTriVerts[1], theColor);
			aD3DVertex[1].specular = 0;
			aD3DVertex[1].tu = aTriVerts[1].u * mMaxTotalU;
			aD3DVertex[1].tv = aTriVerts[1].v * mMaxTotalV;

			aD3DVertex[2].sx = aTriVerts[2].x + tx;
			aD3DVertex[2].sy = aTriVerts[2].y + ty;
			aD3DVertex[2].sz = theInterface->mStateMgr->GetBitDepth();
			aD3DVertex[2].rhw = 1;
			aD3DVertex[2].color = GetColorFromSexyVertex2D(aTriVerts[2], theColor);
			aD3DVertex[2].specular = 0;
			aD3DVertex[2].tu = aTriVerts[2].u * mMaxTotalU;
			aD3DVertex[2].tv = aTriVerts[2].v * mMaxTotalV;

			if ((aVertexCacheNum == 100) || (aTriangleNum == theNumTriangles - 1))
			{
				// Flush the triangles now
				D3DInterface::CheckDXError(theDevice->DrawPrimitive(D3DGraphics3D::PT_TRIANGLELIST, gVertexType, aVertexCache, aVertexCacheNum * 3, 0), "DrawPrimitive (TriList)");
				aVertexCacheNum = 0;
			}
		}
	}
	else
	{
		for (int aTriangleNum = 0; aTriangleNum < theNumTriangles; aTriangleNum++)
		{
			SexyVertex2D* aTriVerts = (SexyVertex2D*)theVertices[aTriangleNum];

			_D3DTLVERTEX aVertex[3] =
			{
				{ aTriVerts[0].x + tx,	aTriVerts[0].y + ty,	0,	1,	GetColorFromSexyVertex2D(aTriVerts[0],theColor),	0,	aTriVerts[0].u * mMaxTotalU,	aTriVerts[0].v * mMaxTotalV },
				{ aTriVerts[1].x + tx,	aTriVerts[1].y + ty,	0,	1,	GetColorFromSexyVertex2D(aTriVerts[1],theColor),	0,	aTriVerts[1].u * mMaxTotalU,	aTriVerts[1].v * mMaxTotalV },
				{ aTriVerts[2].x + tx,	aTriVerts[2].y + ty,	0,	1,	GetColorFromSexyVertex2D(aTriVerts[2],theColor),	0,	aTriVerts[2].u * mMaxTotalU,	aTriVerts[2].v * mMaxTotalV }
			};

			float aMinU = mMaxTotalU, aMinV = mMaxTotalV;
			float aMaxU = 0, aMaxV = 0;

			int i, j, k;
			for (i = 0; i < 3; i++)
			{
				if (aVertex[i].tu < aMinU)
					aMinU = aVertex[i].tu;

				if (aVertex[i].tv < aMinV)
					aMinV = aVertex[i].tv;

				if (aVertex[i].tu > aMaxU)
					aMaxU = aVertex[i].tu;

				if (aVertex[i].tv > aMaxV)
					aMaxV = aVertex[i].tv;
			}

			D3DVertexList aMasterList;
			aMasterList.push_back(aVertex[0]);
			aMasterList.push_back(aVertex[1]);
			aMasterList.push_back(aVertex[2]);


			D3DVertexList aList;

			int aLeft = floorf(aMinU);
			int aTop = floorf(aMinV);
			int aRight = ceilf(aMaxU);
			int aBottom = ceilf(aMaxV);
			if (aLeft < 0)
				aLeft = 0;
			if (aTop < 0)
				aTop = 0;
			if (aRight > mTexVecWidth)
				aRight = mTexVecWidth;
			if (aBottom > mTexVecHeight)
				aBottom = mTexVecHeight;

			D3DTextureDataPiece& aStandardPiece = mTextures[0];
			for (i = aTop; i < aBottom; i++)
			{
				for (j = aLeft; j < aRight; j++)
				{
					D3DTextureDataPiece& aPiece = mTextures[i * mTexVecWidth + j];


					D3DVertexList aList = aMasterList;
					for (k = 0; k < 3; k++)
					{
						aList[k].tu -= j;
						aList[k].tv -= i;
						if (i == mTexVecHeight - 1)
							aList[k].tv *= (float)aStandardPiece.mHeight / aPiece.mHeight;
						if (j == mTexVecWidth - 1)
							aList[k].tu *= (float)aStandardPiece.mWidth / aPiece.mWidth;
					}

					DoPolyTextureClip(aList);
					if (aList.size() >= 3)
					{
						D3DInterface::CheckDXError(theDevice->SetTexture(0, aPiece.mTexture), "SetTexture gTexture");
						D3DInterface::CheckDXError(theDevice->DrawPrimitive(D3DGraphics3D::PT_TRIANGLEFAN, gVertexType, &aList[0], aList.size(), 0), "DrawPrimitive (Tri) 4");*/

						/*				CheckDXError(theDevice->SetTexture(0, NULL),"SetTexture NULL");
										theDevice->SetRenderState(D3DRENDERSTATE_FILLMODE, D3DFILL_WIREFRAME);
										CheckDXError(theDevice->DrawPrimitive(D3DGraphics3D::PT_TRIANGLEFAN, gVertexType, &aList[0], aList.size(), D3DDP_WAIT),"DrawPrimitive (Tri)");
										theDevice->SetRenderState(D3DRENDERSTATE_FILLMODE, D3DFILL_SOLID);*/
					/*}
				}
			}
		}
	}*/
}









///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
D3DMeshPiece::D3DMeshPiece() //3575-3577
{
	mSexyVF = 0, mVertexSize = 0, mVertexBufferCount = 0, mIndexBufferCount = 0, mVertexData = nullptr, mIndexData = nullptr, mVertexBuffer = nullptr, mIndexBuffer = nullptr;
}

D3DMeshPiece::~D3DMeshPiece() //3580-3596
{
	if (mVertexBuffer)
	{
		mVertexBuffer->Release();
		mVertexBuffer = nullptr;
	}
	if (mIndexBuffer)
	{
		mIndexBuffer->Release();
		mIndexBuffer = nullptr;
	}
	delete mVertexData;
	mVertexData = nullptr;
	delete mIndexData;
	mIndexData = nullptr;
}









///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
D3DDynamicVertexBuffer::D3DDynamicVertexBuffer(ulong inVertLimit) :
	mVB(NULL), mVertCount(0), mVertLimit(inVertLimit) //3610-3611
{
}

D3DDynamicVertexBuffer::~D3DDynamicVertexBuffer() //3513-3615
{
	CleanupBuffer();
}

bool D3DDynamicVertexBuffer::InitBuffer(D3DInterface* inInterface) //3618-3637
{
	DBG_ASSERT(inInterface != 0); //3619
	CleanupBuffer();
	if (mVertLimit)
	{
		inInterface->Flush(RenderDevice3D::FLUSHF_ManagedResources_Immediate);
		if (inInterface->CheckDXError(inInterface->InternalCreateVertexBuffer(), "D3DDynamicVertexBuffer CreateVertexBuffer")) //?
			return false;
	}
	return true;
}

void D3DDynamicVertexBuffer::CleanupBuffer() //3639-3649
{
	if (mVB != NULL)
	{
		mVB->Release();
		mVB = NULL;
	}
	mVertCount = 0;
}

ulong D3DDynamicVertexBuffer::Write(D3DInterface* inInterface, ulong inVertCount, const void* inVerts) //3652-3683
{
	DBG_ASSERT(mVertLimit > 0); //3653
	DBG_ASSERT(mVertCount <= mVertLimit); //3654
	DBG_ASSERT(mVB != 0); //3655
	ulong lockFlags = D3DLOCK_NOOVERWRITE;
	if (inVertCount + mVertCount > mVertLimit)
	{
		lockFlags = D3DLOCK_DISCARD;
		mVertCount = 0;
	}
	ulong startVertex = mVertCount;
	void* destPtr;
	if (inInterface->CheckDXError(inInterface->InternalVertexBufferLock(mVB, 32 * mVertCount, 32 * inVertCount, &destPtr, lockFlags), "D3DDynamicVertexBuffer::Write Lock"))
		return -1;
	if (destPtr == NULL)
		return -1;
	CopyMemory(destPtr, inVerts, 32 * inVertCount);
	inInterface->InternalVertexBufferUnlock(mVB);
	mVertCount += inVertCount;
	return startVertex;
}

int D3DDynamicVertexBuffer::ApplyToDevice(D3DInterface* inInterface, ulong inStreamIndex) //3686-3689
{
	return inInterface->InternalSetStreamSource(inStreamIndex, mVB, 32);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
D3DInterface::D3DInterface() : mCurrentContext((HANDLE)0)//3984-3748
{
	sMinTextureWidth = 64;
	sMinTextureHeight = 64;
	sMaxTextureWidth = 64;
	sMaxTextureHeight = 64;
	sMaxTextureAspectRatio = 1;

	gIs3D = true;

	mBltFilter = NULL;
	mBltFilterContext = NULL;
	mDrawPrimFilter = NULL;
	mDrawPrimFilterContext = NULL;

	mStateMgr = NULL;
	gPixelTracerStateManager = NULL;

	mBatchedTriangleIndex = 0;
	mBatchedTriangleSize = 600;

	mBatchedTriangleBuffer = new _D3DTLVERTEX[32 * mBatchedTriangleSize]; //?

	mHWnd = NULL;
	mWidth = 640;
	mHeight = 480;
	mFullscreenBits = 16;

	mBackBufferSurface = 0;
	mTempRenderTargetSurface = 0;
	mDynVB = 0;
	mDisplayFormat = SEXY3DFMT_UNKNOWN;
	//mD3DViewport = NULL;
	mSceneBegun = false;
	mIsWindowed = true;

	mNeedClearZBuffer = 1;
	mNeedEvictManagedResources = 0;
	mFov = 0.78539819;
	mNearPlane = 0.1;
	mFarPlane = 100.0;

	mAvailPalettes.resize(65536); //?

	mAdapterInfoString = "";
	mD3DProductVersionString = "";

	mTexMemUsageBytesAlloced = 0;
	mTexMemUsageBytesOriginal = 0;
	mTexMemUsageBytesCurFrame = 0;
	mTexMemUsageFlushRevision = 1;

	/*
		//Test Transform
		SexyTransform2D aTrans;
		aTrans.Translate(-320,-240);
		aTrans.Rotate(45*3.14159f/180);
	//	aTrans.Scale(1.3f,1.3f);
		aTrans.Translate(320,240);
		PushTransform(aTrans);
	*/
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
D3DInterface::~D3DInterface() //3751-3758
{
	delete mBatchedTriangleBuffer;

	assert(mStateMgr == NULL); //3754
	gIs3D = false;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
D3DContext::D3DContext(Image* inImage) { mDestImage = inImage, mInitialized = false, mParent = NULL; } //3755

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
D3DContext::D3DContext(const D3DContext& inContext) //3781-3783
{
	mDestImage = inContext.mDestImage, mStateContext = inContext.mStateContext, mParent->mChildren.push_back(this); //?
}

D3DContext::~D3DContext() //3785-3808
{
	int aChildCount = mChildren.size();

	for (int iChild = 0; iChild < aChildCount; iChild++)
	{
		assert(mChildren[iChild]->mParent == this); //3791
		mChildren[iChild]->mParent = NULL;
	}
	mChildren.clear();
	if (mParent == NULL)
	{
		aChildCount = mParent->mChildren.size();
		for (int iChild = 0; iChild < aChildCount; iChild++)
		{
			if (mChildren[iChild]->mParent = this)
				mParent->mChildren.erase(mParent->mChildren.begin() + iChild);
		}
	}
}

HRenderContext D3DInterface::CreateContext(Image* theDestImage, const HRenderContext& theSourceContext) //3812-3837
{
	DeviceImage* anImage = dynamic_cast<DeviceImage*>(theDestImage);
	/*if (anImage && mGraphicsDriver->mIs3D && anImage->HasImageFlag(ImageFlag_RenderTarget))
	{
		if (theSourceContext.IsValid())
		{
			SetCurrentContext(theSourceContext);
			D3DContext* aSourceContext = (D3DContext*)theSourceContext.GetPointer();
			D3DContext* aContext = new D3DContext(*aSourceContext);
			aContext = new D3DContext(anImage);
		}
		else
			return 0;
	}*/
}

void D3DInterface::DeleteContext(const HRenderContext& theContext) //3839-3856
{
	if (theContext.IsValid())
	{
		D3DContext* aContext = (D3DContext*)theContext.GetPointer();
		if (theContext == &mCurrentContext)
			SetCurrentContext(aContext->mParent);
		delete aContext;
	}
}

void D3DInterface::SetCurrentContext(const HRenderContext& theContext) //3858-3903
{
	if (theContext != &mCurrentContext)
	{
		FlushBufferedTriangles();
		mCurrentContext = theContext.mHandleDword;
		if (theContext.IsValid())
		{
			D3DContext* aContext = (D3DContext*)theContext.GetPointer();
			MemoryImage* aMemoryImage = aContext->mDestImage ? aContext->mDestImage->AsMemoryImage() : NULL;
			if (aMemoryImage)
				SetRenderTarget(aMemoryImage);
			mStateMgr->SetContext(&aContext->mStateContext);
			if (!aContext->mInitialized)
			{
				SetDefaultState(aMemoryImage, true);
				mStateMgr->CommitState();
				aContext->mInitialized = true;
			}
		}
		else
		{
			SetRenderTarget(false);
			mStateMgr->SetContext(false);
		}
	}
}

HRenderContext D3DInterface::GetCurrentContext() const //3905-3907
{
	return mHandleDword = mCurrentContext; //?
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::PushState() //3912-3915
{
	FlushBufferedTriangles();
	mStateMgr->PushState();
}
void D3DInterface::PopState() //3917-3920
{
	FlushBufferedTriangles();
	mStateMgr->PopState();
}

void D3DInterface::ClearRect(const Rect& theRect) //3925-3931
{
	/*D3DContext* aContext = (D3DContext*)mCurrentContext.GetPointer();
	Image* anImage = aContext? aContext->mDestImage : NULL;
	MemoryImage* aMemoryImage = anImage ? aMemoryImage->AsMemoryImage() : NULL;
	aMemoryImage.*/
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::FillRect(const Rect& theRect, const Color& theColor, int theDrawMode) //3936-3979
{
	if (!PreDraw())
		return;

	SetupDrawMode(theDrawMode);

	DWORD aColor = RGBA_MAKE(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);
	float x = theRect.mX - 0.5f;
	float y = theRect.mY - 0.5f;
	float aWidth = theRect.mWidth;
	float aHeight = theRect.mHeight;

	_D3DTLVERTEX aVertex[4] =
	{
		{ x,				y,				0,	1,	aColor,	0,	0,		0 },
		{ x,				y + aHeight,		0,	1,	aColor,	0,	0,		0 },
		{ x + aWidth,			y,				0,	1,	aColor,	0,	0,		0 },
		{ x + aWidth,			y + aHeight,		0,	1,	aColor,	0,	0,		0 }
	};


	if (!mTransformStack.empty())
	{
		SexyVector2 p[4] = { SexyVector2(x, y), SexyVector2(x,y + aHeight), SexyVector2(x + aWidth, y) , SexyVector2(x + aWidth, y + aHeight) };

		int i;
		for (i = 0; i < 4; i++)
		{
			p[i] = mTransformStack.back() * p[i];
			p[i].x -= 0.5f;
			p[i].y -= 0.5f;
			aVertex[i].sx = p[i].x;
			aVertex[i].sy = p[i].y;
		}
	}

	CheckDXError(SetTexture(0, NULL), "SetTexture NULL");
	BufferedDrawPrimitive(Graphics3D::PT_TriangleStrip, 2, aVertex, 32, 0x1C4);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::FillPoly(const Point theVertices[], int theNumVertices, const Rect* theClipRect, const Color& theColor, int theDrawMode, int tx, int ty) //3984-4022
{
	if (theNumVertices < 3)
		return;

	if (!PreDraw())
		return;

	SetupDrawMode(theDrawMode);
	DWORD aColor = RGBA_MAKE(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);

	CheckDXError(SetTexture(0, NULL), "SetTexture NULL");

	mStateMgr->GetBltDepth();

	D3DVertexList aList;
	for (int i = 0; i < theNumVertices; i++)
	{
		_D3DTLVERTEX vert = { theVertices[i].mX + tx, theVertices[i].mY + ty,	0,	1,	aColor,	0,	0,		0 };
		if (!mTransformStack.empty())
		{
			SexyVector2 v(vert.sx, vert.sy);
			v = mTransformStack.back() * v;
			vert.sx = v.x;
			vert.sy = v.y;
		}

		aList.push_back(vert);
	}

	if (theClipRect != NULL)
		DrawPolyClipped(theClipRect, aList);
	else
		BufferedDrawPrimitive(Graphics3D::PT_TriangleFan, aList.size() - 2, &aList[0], 32, 0x1C4);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor, int theDrawMode, bool antiAlias = false) //4027-4070
{
	if (!PreDraw())
		return;

	SetupDrawMode(theDrawMode);

	float x1, y1, x2, y2;
	DWORD aColor = RGBA_MAKE(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);

	if (!mTransformStack.empty())
	{
		SexyVector2 p1(theStartX, theStartY);
		SexyVector2 p2(theEndX, theEndY);
		p1 = mTransformStack.back() * p1;
		p2 = mTransformStack.back() * p2;

		x1 = p1.x;
		y1 = p1.y;
		x2 = p2.x;
		y2 = p2.y;
	}
	else
	{
		x1 = theStartX;
		y1 = theStartY;
		x2 = theEndX;
		y2 = theEndY;
	}

	_D3DTLVERTEX aVertex[3] =
	{
		{ x1,				y1,				0,	1,	aColor,	0,	0,		0 },
		{ x2,				y2,				0,	1,	aColor,	0,	0,		0 },
		{ x2 + 0.5f,			y2 + 0.5f,		0,	1,	aColor,	0,	0,		0 }
	};

	D3DInterface::CheckDXError(SetTexture(0, NULL), "SetTexture NULL");
	BufferedDrawPrimitive(Graphics3D::EPrimitiveType::PT_LineStrip, 1, aVertex, 32, 0x1C4);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::BltTriangles(Image* theImage, const SexyVertex2D theVertices[][3], int theNumTriangles, const Color& theColor, int theDrawMode, float tx, float ty, bool blend, const Rect* theClipRect) //4075-4096
{
	if (!PreDraw)
		return;
	if (!CreateImageRenderData((MemoryImage*)theImage))
		return;

	SetupDrawMode(theDrawMode);
	DWORD aColor = RGBA_MAKE(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);
	D3DTextureData* aData = (D3DTextureData*)theImage->GetRenderData();
	SetTextureLinearFilter(0, false);
	gPixelTracerLastImage = theImage;
	aData->BltTriangles(this, theVertices, theNumTriangles, aColor, tx, ty, theClipRect);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::Cleanup() //TODO | 4101-4203
{
	Flush(RenderDevice3D::FLUSHF_CurrentScene);
	/*if (gSexyAppBase->mSharedRTPool)
		gSexyAppBase->mSharedRTPool->InvalidateDevice();

	if (mStateMgr)
	{
		InternalSetRenderTarget(mBackBufferSurface);
		for (int i = 0; i < 8; i++) //8 is some count?
			mStateMgr->SetTexture(i, NULL);
		mStateMgr->SetPixelShader(NULL);
		mStateMgr->SetVertexShader(NULL);
		mStateMgr->CommitState();
		Flush(RenderDevice3D::FLUSHF_ManagedResources_Immediate);
	}

	RenderEffectMap::iterator it = mRenderEffects.begin();
	while (it != mRenderEffectDefInfo.end())
	{
		D3DRenderEffect* aRenderEffect = &it->second;
		delete (D3DRenderEffect*)aRenderEffect;
		aRenderEffect = NULL;
        it++;
	}

    mRenderEffects.clear();


    RenderEffectDefInfoMap::iterator it;
	while (it != mRenderEffectDefInfo.end())
	{
		D3DRenderEffectDefInfo* aInfo = &it->second;
		delete (D3DRenderEffect*)aInfo;
		aInfo  = NULL;
        it++;
	}

    mRenderEffectDefInfo.clear();

    if (mStateMgr)
    {
        mStateMgr->Cleanup();
        delete mStateMgr;
        mStateMgr = 0;
        mStateMgr->Cleanup(mStateMgr);
        gPixelTracerStateManager = 0;
    }

    if (mDrawPrimMtx.mCalls)
        OutputDebugStringA(StrFormat(
           "D3DInterface Metrics--------------------------\r\n",
           "Draw Primitive calls: %u\r\n",
           "Total primitives    : %u\r\n",
           "Average prims/call  : %.3f\r\n",
           "-----------------------------------------------\r\n",
            mDrawPrimMtx.mCalls,
            (double)mDrawPrimMtx.mPrims,
            mDrawPrimMtx.mPrims / (double)mDrawPrimMtx.mCalls).c_str();
        )

    memset(mDrawPrimMtx, 0, sizeof(mDrawPrimMtx));

    MeshSet::iterator a3DObjectItr = >mGraphicsDriver->mMeshSet;
	while (a3DObjectItr != >mGraphicsDriver->mMeshSet.end())
	{
		Mesh* anObject = &a3DObjectItr;
		anObject->Cleanup();
        a3DObjectItr++;
	}

	ImageSet::iterator anItr;
	while (anItr != mImageSet.end())
	{
		MemoryImage* anImage = *&anItr;
		delete (D3DTextureData*)anImage->GetRenderData(); //mD3DData?
		anImage->SetRenderData(0);
        anItr++;
	}

	mImageSet.clear();

	if (mBackBufferSurface)
	{
		mBackBufferSurface->Release();
		mBackBufferSurface = NULL;
	}

    if (mTempRenderTargetSurface)
	{
		mTempRenderTargetSurface->Release();
		mTempRenderTargetSurface = NULL;
	}

	mAdapterInfoString = "";
	mD3DProductVersionString = "";
	mAvailPalettes.resize(0x10000);
	for (int i = 0; i < 0x10000; i++)
		mAvailPalettes[i] = 0xFFFF - i;
	DBG_ASSERT(mTexMemUsageBytesAlloced == 0); //4198
    mTexMemUsageBytesAlloced = 0;
    mTexMemUsageBytesOriginal = 0;
    mTexMemUsageBytesCurFrame = 0;
    mTexMemUsageFlushRevision = 1;
    */
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool D3DInterface::SceneBegun() //4208-4210
{
	return mSceneBegun;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool D3DInterface::CreateImageRenderData(MemoryImage* inImage) //4115-4236
{
	bool wantPurge = false;
	if (!inImage->GetRenderData())
	{
		D3DTextureData* inRenderData = new D3DTextureData(this);
		inImage->SetRenderData(inRenderData);
		wantPurge = inImage->mPurgeBits;
		AutoCrit aCrit(mGraphicsDriver->mCritSect);
		mImageSet.insert(inImage);
	}
	D3DTextureData* aData = (D3DTextureData*)inImage->GetRenderData();
	aData->CheckCreateTextures(inImage, this);
	if (wantPurge || inImage->HasImageFlag(ImageFlag_RenderTarget))
		inImage->PurgeBits();
	return aData->mPixelFormat != PixelFormat::PixelFormat_Unknown;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::RemoveImageRenderData(MemoryImage* inImage) //4241-4275
{
	if (!inImage->GetRenderData())
		return;

	FlushBufferedTriangles();
	D3DTextureData* data = (D3DTextureData*)inImage->GetRenderData();
	if (!data->mTextures.empty() && data->mTextures[0].mTexture)
	{
		bool needCommit = false;
		for (int i = 0; i < 8; i++)
		{
			if (mStateMgr[i].GetTexture(i) == data->mTextures[0].mTexture)
			{
				mStateMgr->SetTexture(i, NULL);
				needCommit = true;
			}
		}
		if (needCommit)
			mStateMgr->CommitState();
	}
	if (inImage->GetRenderData())
		delete inImage->GetRenderData(); //?
	inImage->SetRenderData(NULL);
	AutoCrit aCrit(mGraphicsDriver->mCritSect);
	mImageSet.erase(inImage);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool D3DInterface::RecoverImageBitsFromRenderData(MemoryImage* inImage) //TODO | 4280-4379
{
	/*if (inImage->GetRenderData() || inImage->HasImageFlag(ImageFlag_CubeMap || ImageFlag_VolumeMap))
		return false;
	D3DTextureData* aData = (D3DTextureData*) inImage->GetRenderData();
	if (aData->mBitsChangedCount != inImage->mBitsChangedCount)
		return false;
	int aPieceRow = 0;
	if (aPieceRow >= aData->mTexVecHeight)
		return true;
	for (int aPieceCol = 0; ; ++aPieceCol)
	{
		if (aPieceCol >= aData->mTexVecWidth)
		{
			aPieceRow++;
			if (aPieceRow >= aData->mTexVecHeight)
				return true;
		}
		D3DTextureDataPiece* aPiece = &aData->mTextures[aPieceCol + aData->mTexVecWidth * aPieceRow];
		int offx = aData->mTexPieceWidth * aPieceCol;
		int offy = aData->mTexPieceHeight * aPieceRow;
		int aWidth = inImage->mWidth - offx >= aPiece->mWidth ? aPiece->mWidth : inImage->mWidth - offx;
		int aHeight = inImage->mHeight - offy >= aPiece->mHeight ? aPiece->mHeight : inImage->mHeight - offy;
		if ((aData->mImageFlags & ImageFlag_RenderTarget) != 0)
			break;
		int aLockedPitch;
		void* aLockedBits;
		CheckDXError(InternalTextureLockRect(aPiece->mTexture, aLockedPitch, aLockedBits), "Lock Texture");
		switch (aData->mPixelFormat)
		{
		case PixelFormat::PixelFormat_A8R8G8B8: CopyTexture8888ToImage(aLockedBits, aLockedPitch, inImage, offx, offy, aWidth, aHeight); break;
		case PixelFormat::PixelFormat_A4R4G4B4: CopyTexture4444ToImage(aLockedBits, aLockedPitch, inImage, offx, offy, aWidth, aHeight); break;
		case PixelFormat::PixelFormat_R5G6B5: CopyTexture565ToImage(aLockedBits, aLockedPitch, inImage, offx, offy, aWidth, aHeight); break;
		case PixelFormat::PixelFormat_Palette8: CopyTexturePalette8ToImage(aLockedBits, aLockedPitch, inImage, offx, offy, aWidth, aHeight, aData->mPaletteIndex, this); break;
		default: break;
		}
		CheckDXError(InternalTextureUnlockRect(aPiece->mTexture), "Texture Unlock");
	}
	return false;*/
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int D3DInterface::GetTextureMemorySize(MemoryImage* theImage) //4384-4390 (Accurate)
{
	D3DTextureData* aData = (D3DTextureData*)theImage->GetRenderData();
	if (aData) //Use a switch statement next time
		return aData->mTexMemSize;
	else
		return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
PixelFormat D3DInterface::GetTextureFormat(MemoryImage* theImage) //4395-4401
{
	D3DTextureData* aData = (D3DTextureData*)theImage->GetRenderData();
	if (aData) //Use a switch statement next time
		return aData->mPixelFormat;
	else
		return PixelFormat::PixelFormat_Unknown;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
IUnknown* D3DInterface::GetBackBufferSurface() //4406-4408
{
	return mBackBufferSurface;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool D3DInterface::SetRenderTargetSurface(IUnknown* inSurface) //4413-4450
{
	if (!PreDraw() || inSurface == NULL)
		return false;

	if (mCurRenderTargetSurface == inSurface)
	{
		gSexyAppBase->mGraphicsDriver->GetMetrics()[GraphicsMetrics::CT_SetRenderTargetCallsRedundant] += 1; //?
		return true;
	}
	else
	{
		gSexyAppBase->mGraphicsDriver->GetMetrics()[GraphicsMetrics::CT_SetRenderTargetCalls] += 1; //?
		FlushBufferedTriangles();
		mCurRenderTargetImage = NULL;
		if (mTempRenderTargetSurface != NULL)
			mTempRenderTargetSurface->Release();
		mTempRenderTargetSurface = NULL;
		if (InternalSetRenderTarget(inSurface) < 0)
			return false;
		else
		{
			mCurRenderTargetSurface = inSurface;
			return true;
		}
	}
}

bool D3DInterface::SetRenderTarget(Image* theImage) //4453-4505
{
	IUnknown* aTextureSurface;
	if (theImage && mCurRenderTargetImage == theImage)
	{
		gSexyAppBase->mGraphicsDriver->GetMetrics()[GraphicsMetrics::CT_SetRenderTargetCallsRedundant] += 1; //?
		return true;
	}
	else if (theImage)
	{
		MemoryImage* aMemoryImage = theImage->AsMemoryImage();
		if (CreateImageRenderData(aMemoryImage))
		{
			D3DTextureData* aTextureData = (D3DTextureData*)aMemoryImage->GetRenderData();
			IUnknown* aTexture = aTextureData->mTextures.begin()->mTexture;
			if (aTexture)
			{
				IUnknown* aTextureSurface;
				HRESULT hr = InternalTextureGetSurfaceLevel(aTexture, 0, &aTextureSurface);
				if (hr >= 0)
				{
					bool result = SetRenderTargetSurface(aTextureSurface);
					if (result)
					{
						mCurRenderTargetImage = theImage;
						mTempRenderTargetSurface = aTextureSurface;
					}
					else
						aTextureSurface->Release();
					return result;
				}
				else
					return false;
			}
			else
				return false;
		}
		else
			return false;
	}
	else
		return SetRenderTargetSurface(mGraphicsDriver->mDrawSurface);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
Image* D3DInterface::SwapScreenImage(DeviceImage*& ioSrcImage, RenderSurface*& ioSrcSurface) //4510-4564
{
	if (!ioSrcImage)
		assert(false && "SwapScreenImage requires a non-null source image to work with"); //4513
	if (ioSrcSurface)
		SetRenderTargetSurface(ioSrcSurface);
	else
	{
		mCurRenderTargetImage = NULL;
		SetRenderTarget(ioSrcImage);
		mCurRenderTargetSurface->AddRef();
		ioSrcSurface = mCurRenderTargetSurface;
		ClearColorBuffer(Color::FromInt(0xFF000000));
	}
	void* aTempPtr = mGraphicsDriver->mScreenImage->mSurface;
	//mGraphicsDriver->mScreenImage->mSurface = (DeviceSurface*)ioSrcImage->mSurface; //?
	// = aTempPtr //?
	mGraphicsDriver->mScreenImage->SetRenderData(ioSrcImage->GetRenderData());
	mGraphicsDriver->mScreenImage->SetRenderData(aTempPtr);
	aTempPtr = mGraphicsDriver->mDrawSurface;
	mGraphicsDriver->mDrawSurface = ioSrcSurface;
	ioSrcSurface = (RenderSurface*)aTempPtr;
	return ioSrcImage;
}



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::DrawPrimitiveEx(ulong theVertexFormat, Graphics3D::EPrimitiveType thePrimitiveType, const SexyVertex* theVertices, int thePrimitiveCount, const Color& theColor, int theDrawMode, float tx, float ty, bool blend, ulong theFlags) //4571-4702
{
	int aNumVertices = 0;
	switch (thePrimitiveType)
	{
	case Graphics3D::PT_PointList: aNumVertices = thePrimitiveCount; break;
	case Graphics3D::PT_LineList: aNumVertices = 2 * thePrimitiveCount; break;
	case Graphics3D::PT_LineStrip: aNumVertices = thePrimitiveCount + 1; break;
	case Graphics3D::PT_TriangleList: aNumVertices = 3 * thePrimitiveCount; break;
	case Graphics3D::PT_TriangleStrip: aNumVertices = thePrimitiveCount + 2; break;
	case Graphics3D::PT_TriangleFan: aNumVertices = thePrimitiveCount + 2; break;
	default: break;
	}
	if (!aNumVertices && !thePrimitiveCount && !PreDraw())
		return;

	FlushBufferedTriangles();
	RenderStateManager::Context* aStateContext = mStateMgr->GetContext();
	mStateMgr->PushState();
	DWORD aColor = RGBA_MAKE(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);
	SetupDrawMode(theDrawMode);
	SetTextureLinearFilter(0, blend);
	assert(mStateMgr->GetFVF() == DEFAULT_VERTEX_FVF); //4599
	mStateMgr->SetFVF(theVertexFormat);
	int aVertexSize = mStateMgr->GetFVFSize();
	void* anAllocData = NULL;
	const void* aVertexPtr;
	if ((theVertexFormat & SexyVF_XYZRHW) != 0)
	{
		if (!aColor && tx == 0.0 && ty == 0.0 || mTransformStack.empty())
			aVertexPtr = theVertices;
		else
		{
			int aNeedVertexSize = aVertexSize * aNumVertices;
			void* aTempVerts = gTempVertexData;
			if (aVertexSize * aNumVertices > 8192)
				anAllocData, aTempVerts = new void* [aNeedVertexSize]; //?
			CopyMemory(aTempVerts, theVertices, aNeedVertexSize);
			char* aCurVertexPtr = (char*)aTempVerts;
			for (int i = 0; i < aNumVertices; i++)
			{
				SexyVertex2D* aVertexRHW((SexyVertex2D*)aCurVertexPtr);
				*(float*)aCurVertexPtr = *(float*)*aCurVertexPtr + tx;
				aVertexRHW->y = aVertexRHW->y + ty;
				if (!aVertexRHW->color)
					aVertexRHW->color = aColor;
				if (!mTransformStack.empty())
				{
					SexyVector2 aVector(aVertexRHW->x, aVertexRHW->y);
					aVertexRHW, aVector = mTransformStack.back() * aVector; //?
				}
				aCurVertexPtr += aVertexSize;
			}
			aVertexPtr = aTempVerts;
		}
	}
	else
	{
		if ((theVertexFormat & SexyVF_XYZ) != 0)
			DBG_ASSERT(false, "Vertex format must have either XYZ (untransformed) or XYZRHW (transformed) flag enabled"); //4669
		aVertexPtr = theVertices;
	}
	AdjustPrimCount((ulong*)thePrimitiveCount);
	MetricsAddPrimitive(thePrimitiveCount, thePrimitiveCount);
	if (gTracingPixels && (theVertexFormat & SexyVF_XYZRHW) != 0)
		PixelTracerCheckPrimitives(thePrimitiveType, thePrimitiveCount, (const SexyVertex2D*)aVertexPtr, aVertexSize);
	DrawPrimitiveInternal(thePrimitiveType, thePrimitiveCount, aVertexPtr, aVertexSize, theVertexFormat);
	mDrawPrimMtx.mPrims += thePrimitiveCount;
	mDrawPrimMtx.mCalls++;
	delete anAllocData;
	mStateMgr->PopState();
	DBG_ASSERT(mStateMgr->GetFVF() == DEFAULT_VERTEX_FVF); //4700
	DBG_ASSERT(mStateMgr->GetFVF() == DEFAULT_VERTEX_SIZE); //4701
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::SetBltDepth(float inDepth) //4707-4709
{
	mStateMgr->SetBltDepth(inDepth);
}

void D3DInterface::PushTransform(const SexyMatrix3& theTransform, bool concatenate) //4713-4720
{
	if (mTransformStack.empty() || !concatenate)
		mTransformStack.push_back(theTransform);
	else
	{
		SexyMatrix3& aTrans = mTransformStack.back();
		mTransformStack.push_back(theTransform * aTrans);
	}
}

void D3DInterface::PopTransform() //4723-4726
{
	if (!mTransformStack.empty())
		mTransformStack.pop_back();
}

void D3DInterface::PopTransform(SexyMatrix3& theTransform) //4729-4740 (Accurate?)
{
	if (mTransformStack.empty())
	{
		SexyMatrix3 aMatrix;
		aMatrix.LoadIdentity();
		CopyMemory(&theTransform, &aMatrix, sizeof(SexyMatrix3));
	}
	else
	{
		CopyMemory(&theTransform, &mTransformStack.back(), sizeof(SexyMatrix3));
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::SetBlend(Graphics3D::EBlendMode inSrcBlend, Graphics3D::EBlendMode inDestBlend) //4745-4747
{
	mStateMgr->SetBlendOverride(inSrcBlend, inDestBlend);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::SetLightEnabled(int inLightIndex, bool inEnabled) //4752-4754
{
	mStateMgr->SetLightEnabled(inLightIndex, inEnabled);
}

void D3DInterface::SetPointLight(int inLightIndex, const SexyVector3& inPos, const Graphics3D::LightColors& inColors, float inRange, const SexyVector3& inAttenuation) //4756-4758
{
	mStateMgr->SetPointLight(inLightIndex, inPos, inColors, inRange, inAttenuation);
}

void D3DInterface::SetDirectionalLight(int inLightIndex, const SexyVector3& inDir, const Graphics3D::LightColors& inColors) //4760-4762
{
	mStateMgr->SetDirectionalLight(inLightIndex, inDir, inColors);
}

void D3DInterface::SetMaterialAmbient(const Color& inColor, int inVertexColorComponent) //4764-4766
{
	mStateMgr->SetMaterialAmbient(inColor, inVertexColorComponent);
}

void D3DInterface::SetMaterialDiffuse(const Color& inColor, int inVertexColorComponent) //4768-4770
{
	mStateMgr->SetMaterialDiffuse(inColor, inVertexColorComponent);
}

void D3DInterface::SetMaterialSpecular(const Color& inColor, int inVertexColorComponent, float inPower) //4772-4774
{
	mStateMgr->SetMaterialSpecular(inColor, inVertexColorComponent, inPower);
}

void D3DInterface::SetMaterialEmissive(const Color& inColor, int inVertexColorComponent) //4776-4778
{
	mStateMgr->SetMaterialEmissive(inColor, inVertexColorComponent);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool D3DInterface::SetTexture(int inTextureIndex, Image* inImage) //4783-4813
{
	if (inTextureIndex)
		gPixelTracerLastImage = inImage;

	if (inImage)
	{
		MemoryImage* aMemoryImage = inImage->AsMemoryImage();
		if (aMemoryImage)
		{
			if (CreateImageRenderData(aMemoryImage))
			{
				D3DTextureData* aTextureData = (D3DTextureData*)aMemoryImage->GetRenderData();
				if (aTextureData->mPaletteIndex != -1)
					mStateMgr->SetCurrentTexturePalette(aTextureData->mPaletteIndex);
				if ((aTextureData->mImageFlags & ImageFlag_CubeMap) != 0)
					SetTextureDirect(inTextureIndex, aTextureData->mTextures.begin()->mCubeTexture);
				else if ((aTextureData->mImageFlags & ImageFlag_VolumeMap) != 0)
					SetTextureDirect(inTextureIndex, aTextureData->mTextures.begin()->mVolumeTexture);
				else
					SetTextureDirect(inTextureIndex, aTextureData->mTextures.begin()->mTexture);
				return true;
			}
			else
				return false;
		}
		else
			return false;
	}
	else
	{
		SetTextureDirect(inTextureIndex, NULL);
		return true;
	}
}

void D3DInterface::SetTextureWrap(int inTextureIndex, bool inWrapU, bool inWrapV) //4816-4822
{
	SEXY3DTEXTUREADDRESS aFilterU = inWrapU ? 1 : 3;
	SEXY3DTEXTUREADDRESS aFilterV = inWrapU ? 1 : 3;

	SetSamplerState(inTextureIndex, 1, aFilterU);
	SetSamplerState(inTextureIndex, 2, aFilterV);
}

void D3DInterface::SetTextureLinearFilter(int inTextureIndex, bool inLinear) //4725-4830
{
	SEXY3DTEXTUREFILTERTYPE aFilter = inLinear + 1;	
	SetSamplerState(inTextureIndex, 6, aFilter);
	SetSamplerState(inTextureIndex, 5, aFilter);
	SetSamplerState(inTextureIndex, 7, aFilter);
}

void D3DInterface::SetTextureCoordSource(int inTextureIndex, int inUVComponent, Graphics3D::ETexCoordGen inTexGen) //4833-4855
{
	ulong tciFlags = 0;
	switch (inTexGen)
	{
	case Graphics3D::TEXCOORDGEN_CAMERASPACENORMAL: tciFlags = D3DTSS_TCI_CAMERASPACENORMAL; break;
	case Graphics3D::TEXCOORDGEN_CAMERASPACEPOSITION: tciFlags = D3DTSS_TCI_CAMERASPACEPOSITION; break;
	case Graphics3D::TEXCOORDGEN_CAMERASPACEREFLECTIONVECTOR: tciFlags = D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR; break;
	}
	SetTextureStageState(inTextureIndex, 11, tciFlags | inUVComponent); //?
	bool needNormalizeNormals = false;
	for (int i = 0; i < 8; i++)
	{
		if (mStateMgr->GetTextureStageState(i, 11) > 15)
		{
			needNormalizeNormals = true;
			break;
		}
	}
	SetRenderState(143, needNormalizeNormals); //?
}

void D3DInterface::SetViewport(int theX, int theY, int theWidth, int theHeight, float theMinZ, float theMaxZ) //4858-4860
{
	mStateMgr->SetViewport(theX, theY, theWidth, theHeight, theMinZ, theMaxZ);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
RenderEffect* D3DInterface::GetEffect(RenderEffectDefinition* inDefinition) //4865-4889
{
	RenderEffectMap::iterator it = mRenderEffects.find(inDefinition);
	if (it != mRenderEffects.end())
		return it->second;
	D3DRenderEffectDefInfo* aInfo = NULL;
	RenderEffectDefInfoMap::iterator it2 = mRenderEffectDefInfo.find(inDefinition);
	if (it2 != mRenderEffectDefInfo.end())
		aInfo = it2->second;
	else
	{
		aInfo = new D3DRenderEffectDefInfo();
		if (!aInfo->Build(inDefinition))
			gSexyAppBase->Popup(StrFormat("Error building render effect info for effect \"%s\"", inDefinition->mSrcFileName.c_str()));
		aInfo = mRenderEffectDefInfo[inDefinition];
	}
	D3DRenderEffect* aEffect = new D3DRenderEffect(this, aInfo);
	aEffect = mRenderEffects[inDefinition];
	return aEffect;
}

///////////////////////////////////////////////////////////////////////////////
bool D3DInterface::LoadMesh(Mesh* theMesh) //4903-5016 (So this parses P3D files, I'm honestly surprised it's engine specific despite being only used for Twist-3 [non-mobile], previously Load3DObject)
{
	bool created = false;

	Buffer aBuffer;
	if (!gSexyAppBase->ReadBufferFromFile(theMesh->mFileName, &aBuffer))
		return false;

	if (aBuffer.ReadLong() == 0x3DBEEF00)
	{
		int aVersion = aBuffer.ReadLong();
		if (aVersion > 2)
		{
			theMesh->Cleanup();
			if (theMesh->mListener)
				theMesh->mListener->MeshPreLoad(theMesh);
			int anObjectCount = aBuffer.ReadShort();
			for (int anObjIdx = 0; anObjIdx < anObjectCount; anObjIdx++)
			{
				std::string anObjectName = aBuffer.ReadString();
				int aSetCount = aBuffer.ReadShort();
				int aSetIdx;
				for (aSetIdx = 0; aSetIdx < aSetCount; aSetIdx++)
				{
					if (aVersion > 1)
					{
						uchar aFlags = aBuffer.ReadByte();
						if (aFlags == 0)
							continue;
					}
					D3DMeshPiece* aPiece = new D3DMeshPiece();
					theMesh->mPieces.push_back(aPiece);
					aPiece = (D3DMeshPiece*)theMesh->mPieces.back();
					std::string aSetName = aBuffer.ReadString();
					std::string aTexFileName;
					std::string aBumpFileName;
					aPiece->mObjectName = anObjectName;
					aPiece->mSetName = aSetName;
					int aPropCount = aBuffer.ReadShort();
					for (int i = 0; i < aPropCount; i++)
					{
						std::string aPropName = aBuffer.ReadString();
						std::string aPropValue = aBuffer.ReadString();
						if (theMesh->mListener)
							theMesh->mListener->MeshHandleProperty(theMesh, anObjectName, aSetName, aPropName, aPropValue);
						if (aPropName == "texture0.fileName")
							aTexFileName = aPropValue;
						if (aPropName == "bump.fileName")
							aBumpFileName = aPropValue;
					}
					if (aTexFileName.length())
					{
						aPiece->mTexture = (SharedImageRef)NULL;
						if (theMesh->mListener)
							aPiece->mTexture = theMesh->mListener->MeshLoadTex(theMesh, anObjectName, aSetName, "texture0.fileName", aTexFileName);
						if (&aPiece->mTexture == NULL)
							gSexyAppBase->GetSharedImage(GetPathFrom(aTexFileName, GetFileDir(theMesh->mFileName)), "", false, true);
						if (&aPiece->mTexture == NULL)
							aPiece->mTexture->AddImageFlags(ImageFlag_MinimizeNumSubdivisions || ImageFlag_Use64By64Subdivisions || ImageFlag_CubeMap);
					}
					if (SupportsPixelShaders() && aBumpFileName.length())
					{
						aPiece->mBumpTexture = (SharedImageRef)NULL;
						if (theMesh->mListener)
							aPiece->mBumpTexture = theMesh->mListener->MeshLoadTex(theMesh, anObjectName, aSetName, "bump.fileName", aBumpFileName);
						if (&aPiece->mBumpTexture == NULL)
							gSexyAppBase->GetSharedImage(GetPathFrom(aBumpFileName, GetFileDir(theMesh->mFileName)), "", false, true);
						if (&aPiece->mBumpTexture == NULL)
							aPiece->mBumpTexture->AddImageFlags(ImageFlag_MinimizeNumSubdivisions || ImageFlag_Use64By64Subdivisions || ImageFlag_CubeMap);
						aPiece->mBumpTexture->AddImageFlags(ImageFlag_MinimizeNumSubdivisions);
					}
					ushort aType = aBuffer.ReadShort();
					ulong aFVF = aBuffer.ReadLong();
					int aVertexSize = 4 * (3 + 3 + 2) + 4;
					aPiece->mSexyVF = aFVF;
					aPiece->mVertexSize = aVertexSize;
					int aReadVertexSize = aVertexSize;
					aPiece->mVertexBufferCount = aBuffer.ReadShort();
					aPiece->mVertexData = new uchar* [aPiece->mVertexBufferCount * aVertexSize];
					aBuffer.ReadBytes((uchar*)aPiece->mVertexData, aPiece->mVertexBufferCount * aVertexSize);
					aPiece->mIndexBufferCount = aBuffer.ReadShort() * 3;
					aPiece->mIndexData = new uchar * [aPiece->mIndexBufferCount * 2];
					aBuffer.ReadBytes((uchar*)aPiece->mIndexData, aPiece->mIndexBufferCount * 2);
				}
				if (aSetIdx < aSetCount)
					return false;
			}
		}
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::RenderMesh(Mesh* theMesh, const SexyMatrix4& theMatrix, const Color& thecolor, bool doSetup) //5021-5177
{
	FlushBufferedTriangles();
	mStateMgr->PushState();
	if (doSetup)
	{
		SetAlphaTest(Graphics3D::COMPARE_ALWAYS, 0);
		SetDepthState(Graphics3D::COMPARE_ALWAYS, 0);
		SetBlend(Graphics3D::BLEND_DEFAULT, Graphics3D::BLEND_DEFAULT);
		SetBltDepth(0.5);
		SetupDrawMode(Graphics::DRAWMODE_NORMAL);
		SetTextureLinearFilter(0, true);
		mStateMgr->SetRenderState(SEXY3DRS_LIGHTING, 1);
		mStateMgr->SetSamplerState(0, 1, 1);
		mStateMgr->SetSamplerState(0, 2, 1);
		mStateMgr->SetRenderState(0xE, 1);
		mStateMgr->SetRenderState(7, 1);
		mStateMgr->SetRenderState(0x17, 4);
		mStateMgr->SetRenderState(0x16, 3);
		Graphics3D::LightColors colors;
		colors.mDiffuse = -1;
		//
		colors.mAmbient = 0; //?
		colors.mSpecular = 0; //?
		colors.mAutoScale = 1.0;
		mStateMgr->SetDirectionalLight(0, SexyVector3(0.5, 0.5, 0.5), colors);
		mStateMgr->SetLightEnabled(0, true);
		mStateMgr->SetRenderState(0x8B, 0x40404040);
		if (mNeedClearZBuffer)
			ClearDepthBuffer();
	}
	else
		SetupDrawMode(Graphics::DRAWMODE_NORMAL);
	mStateMgr->SetTransform(0x100, &theMatrix);
	RenderStateManager::Context* aStateContext = mStateMgr->GetContext();
	mStateMgr->PushState();
	if (theMesh->mListener)
	{
		theMesh->mListener->MeshPreDraw(theMesh);
		mStateMgr->SetContext(aStateContext);
		SetupDrawMode(Graphics::DRAWMODE_NORMAL);
	}
	std::list<MeshPiece*>::iterator anItr = theMesh->mPieces.begin();
	while (anItr != theMesh->mPieces.end())
	{
		D3DMeshPiece* aPiece = (D3DMeshPiece*)*anItr;
		if (aPiece->mVertexData)
		{
			int aReadVertexSize = aPiece->mVertexSize;
			if (InternalCreateVertexBuffer(aPiece->mVertexBufferCount * aPiece->mVertexSize, 0, aPiece->mSexyVF, 1, &aPiece->mVertexBuffer) < 0)
			{
				if (CheckDXError(? (const char*)(aPiece->mVertexBufferCount * aPiece->mVertexSize), ""))
					break;
			}
			void* aData = NULL;
			if (CheckDXError(InternalVertexBufferLock(aPiece->mVertexBuffer, 0, aPiece->mVertexBufferCount * aPiece->mVertexSize, &aData, 2048), ""))
				break;
			uchar* aSrcData = (uchar*)aPiece->mVertexData;
			D3DModelVertex* aVertex = (D3DModelVertex*)aData;
			for (int i = 0; i < aPiece->mVertexBufferCount; i++)
			{
				CopyMemory(aVertex, aSrcData, aReadVertexSize);
				aSrcData += aReadVertexSize;
				aVertex += aPiece->mVertexSize;
			}
			InternalVertexBufferUnlock(aPiece->mVertexBuffer);
			aData = aPiece->mVertexData;
			delete aVertex;
			aPiece->mVertexData = 0;
		}
		if (aPiece->mIndexData)
		{
			if (CheckDXError(InternalCreateIndexBuffer(2 * aPiece->mIndexBufferCount, 1, &aPiece->mIndexBuffer), ""))
				break;
			if (CheckDXError(InternalIndexBufferLock(aPiece->mIndexBuffer, 0, 2 * aPiece->mIndexBufferCount, NULL, 2048), ""))
				break;
			ushort* anIdx = 0;
			CopyMemory(anIdx, aPiece->mIndexData, 2 * aPiece->mIndexBufferCount);
			InternalIndexBufferUnlock(aPiece->mIndexBuffer);
			delete aPiece->mIndexData;
			aPiece->mIndexData = NULL;
		}
		SetTexture(0, aPiece->mTexture);
		if (&aPiece->mBumpTexture)
			SetTexture(1, aPiece->mBumpTexture);
		mStateMgr->SetFVF(aPiece->mSexyVF);
		mStateMgr->SetStreamSource(0, aPiece->mVertexBuffer, 0, aPiece->mVertexSize, 1);
		mStateMgr->SetIndices(aPiece->mIndexBuffer);
		if (theMesh->mListener)
		{
			theMesh->mListener->MeshPreDrawSet(theMesh, aPiece->mObjectName, aPiece->mSetName, &aPiece->mBumpTexture != 0);
			mStateMgr->SetContext(aStateContext);
			SetupDrawMode(Graphics::DRAWMODE_NORMAL);
		}
		mStateMgr->CommitState();
		if (CheckDXError(InternalDrawIndexedPrimitive(4, 0, aPiece->mVertexBufferCount, 0, aPiece->mIndexBufferCount / 3), ""))
			break;
		if (theMesh->mListener)
			mStateMgr->SetContext(aStateContext);
		anItr++;
	}
	if (theMesh->mListener)
	{
		theMesh->mListener->MeshPostDraw(theMesh);
		mStateMgr->SetContext(aStateContext);
	}
	mStateMgr->PopState();
	mStateMgr->PopState();
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool gD3DInterfacePreDrawError = false;
bool D3DInterface::PreDraw() //5182-5235
{
	if (!gSexyAppBase->mPhysMinimized)
		return false;
	
	if (!mSceneBegun)
	{
		HRESULT hr;
		if (InternalSetRenderTarget(mBackBufferSurface) < 0) // this happens when there's been a mode switch (this caused the nvidia screensaver bluescreen)
		{
			gD3DInterfacePreDrawError = true;
			return 0;
		}
		else
			gD3DInterfacePreDrawError = false; //Prob

		mCurRenderTargetImage = NULL;
		mCurRenderTargetSurface = mBackBufferSurface;
		RenderStateManager::Context* aStateContext = mStateMgr->GetContext();
		mStateMgr->SetContext(NULL);
		if (InternalBeginScene() >= 0)
		{
			mSceneBegun = true;
			mStateMgr->RevertState();
			mStateMgr->ApplyContextDefaults();
			SetDefaultState(NULL, true);
			if (GetEffectiveRenderMode())
			{
				if (!gTracingPixels)
					ClearColorBuffer(Color::Black);
			}
			mStateMgr->PushState();
			if (!mStateMgr->CommitState())
			{
				mStateMgr->SetContext(aStateContext);
				return false;
			}
		}
		mStateMgr->SetContext(aStateContext);
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::SetDefaultState(Image* inImage, bool inIsInScene) //5240-5395
{
	mStateMgr->SetFVF(DEFAULT_VERTEX_FVF);
	int aWidth = mWidth;
	int aHeight = mHeight;
	RECT aRect;
	if (inImage != NULL)
	{
		aWidth = inImage->mWidth;
		aHeight = inImage->mHeight;
	}
	else
	{
		RECT aRect;
		GetClientRect(mHWnd, &aRect);
		POINT aTopLeft = { aRect.left, aRect.top };
		POINT aBotRight = { aRect.right, aRect.bottom };
		::ClientToScreen(mHWnd, &aTopLeft);
		::ClientToScreen(mHWnd, &aBotRight);
		aWidth = aBotRight.x - aTopLeft.x;
		aHeight = aBotRight.y - aTopLeft.y;
	}
	mStateMgr->SetViewport(0, 0, aWidth, aHeight, 0.0, 1.0);
	float fAspectRatio = (float)mWidth / (float)mHeight;
	float yScale = 1.0 / tan(mFov / 2.0);
	float xScale = yScale / fAspectRatio;
	float zn = mNearPlane;
	SexyMatrix4 aProjectMatrix(xScale, 0.0, 0.0, 0.0, 0.0, yScale, 0.0, 0.0, 0.0, 0.0, mFarPlane / (mFarPlane - zn), 1.0, 0.0, 0.0, -zn * mFarPlane / (mFarPlane - zn), 0.0);
	mStateMgr->SetTransform(3, &aProjectMatrix);
	SexyMatrix4 aViewMatrix(1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0, 1.0);
	mStateMgr->SetTransform(2, &aViewMatrix);
	if (inIsInScene)
	{
		IGraphicsDriver::ERenderMode aRenderMode = GetEffectiveRenderMode();
		switch (aRenderMode)
		{
		case IGraphicsDriver::RENDERMODE_Overdraw:
			SetTextureStageState(0, 1, 2);
			SetTextureStageState(0, 2, 3);
			SetTextureStageState(0, 4, 2);
			SetTextureStageState(0, 5, 3);
			SetTextureStageState(1, 1, 1);
			SetTextureStageState(1, 4, 1);
			SetRenderState(0x3C, 0x10FFFFFF);
			break;
		case IGraphicsDriver::RENDERMODE_PseudoOverdraw:
			SetTextureStageState(0, 1, 4);
			SetTextureStageState(0, 2, 2);
			SetTextureStageState(0, 3, 0);
			SetTextureStageState(0, 4, 4);
			SetTextureStageState(0, 5, 2);
			SetTextureStageState(0, 6, 0);
			SetTextureStageState(1, 1, 7);
			SetTextureStageState(1, 2, 1);
			SetTextureStageState(1, 3, 3);
			SetTextureStageState(1, 4, 7);
			SetTextureStageState(1, 5, 1);
			SetTextureStageState(1, 6, 3);
			SetRenderState(0x3C, 0x20101010);
			return;
		case IGraphicsDriver::RENDERMODE_BatchSize:
			SetTextureStageState(0, 1, 2);
			SetTextureStageState(0, 2, 3);
			SetTextureStageState(0, 4, 2);
			SetTextureStageState(0, 5, 3);
			SetTextureStageState(1, 1, 1);
			SetTextureStageState(1, 4, 1);
			SetRenderState(0x3C, 0x10FFFFFF);
			break;
		case IGraphicsDriver::RENDERMODE_Wireframe:
			SetTextureStageState(0, 1, 2);
			SetTextureStageState(0, 2, 3);
			SetTextureStageState(0, 4, 2);
			SetTextureStageState(0, 5, 3);
			SetTextureStageState(1, 1, 1);
			SetTextureStageState(1, 4, 1);
			SetRenderState(0x3C, 0x40FFFFFF);
			SetRenderState(8, 2);
			return;
		case IGraphicsDriver::RENDERMODE_WastedOverdraw:
			SetTextureStageState(0, 1, 2);
			SetTextureStageState(0, 2, 3);
			SetTextureStageState(0, 4, 2);
			SetTextureStageState(0, 5, 2);
			SetTextureStageState(1, 1, 1);
			SetTextureStageState(1, 4, 1);
			SetRenderState(0x3C, 0x80808080);
			return;
		case IGraphicsDriver::RENDERMODE_TextureHash:
			SetTextureStageState(0, 1, 2);
			SetTextureStageState(0, 2, 3);
			SetTextureStageState(0, 4, 2);
			SetTextureStageState(0, 5, 3);
			SetTextureStageState(1, 1, 1);
			SetTextureStageState(1, 4, 1);
			SetRenderState(0x3Cu, 0x10FFFFFFu);
			break;
		case IGraphicsDriver::RENDERMODE_OverdrawExact:
			SetTextureStageState(0, 1, 2);
			SetTextureStageState(0, 2, 3);
			SetTextureStageState(0, 4, 2);
			SetTextureStageState(0, 5, 3);
			SetTextureStageState(1, 1, 1);
			SetTextureStageState(1, 4, 1);
			mStateMgr->SetBlendOverride(Graphics3D::BLEND_ONE, Graphics3D::BLEND_ONE);
			SetRenderState(0x3C, 0xFF010400);
			break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::SetupDrawMode(int theDrawMode) //5400-5411
{
	Graphics3D::EBlendMode aSrcBlend;
	Graphics3D::EBlendMode aDestBlend;
	mStateMgr->GetBlendOverride(aSrcBlend, aDestBlend);
	if (aSrcBlend == Graphics3D::Graphics3D::BLEND_DEFAULT)
		aSrcBlend = Graphics3D::Graphics3D::BLEND_SRCALPHA;
	if (aDestBlend == Graphics3D::Graphics3D::BLEND_DEFAULT)
		aDestBlend = (Graphics3D::EBlendMode)(4 * (theDrawMode == Graphics::DRAWMODE_NORMAL) + 2); //?
	SetRenderState(0x13, aSrcBlend);
	SetRenderState(0x14, aDestBlend);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::BltNoClipF(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode, bool linearFilter) //5416-5437
{
	if (!mTransformStack.empty())
		BltClipF(theImage, theX, theY, theSrcRect, 0, theColor, theDrawMode);
	else if (PreDraw() && (CreateImageRenderData(theImage->AsMemoryImage())))
	{
		SetupDrawMode(theDrawMode);
		D3DTextureData* aData = (D3DTextureData*)theImage->GetRenderData();
		SetTextureLinearFilter(0, linearFilter);
		Blt(theImage, theX, theY, theSrcRect, theColor, theDrawMode);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::BltTransformed(Image* theImage, const Rect& theClipRect, const Color& theColor, int theDrawMode, const Rect& theSrcRect, const SexyMatrix3& theTransform, bool linearFilter, float theX, float theY, bool center) //5416-5437
{
	if (!PreDraw())
		return;

	MemoryImage* aSrcMemoryImage = theImage->AsMemoryImage();
	if (CreateImageRenderData(aSrcMemoryImage)) //Correct? Probably
	{
		SetupDrawMode(theDrawMode);
		D3DTextureData* aData = (D3DTextureData*)aSrcMemoryImage->GetRenderData();
		if (!mTransformStack.empty())
		{
			SetTextureLinearFilter(0, true);
			if (theX != 0.0 || theY != 0.0)
			{
				SexyTransform2D aTransform;
				if (center)
					aTransform.Translate((float)(-theSrcRect.mWidth) / 2.0, (float)(-theSrcRect.mHeight) / 2.0);
				aTransform *= theTransform;
				aTransform.Translate(theX, theY);
				aTransform *= mTransformStack.back();
				aData->BltTransformed(this, aSrcMemoryImage, theDrawMode, aTransform, theSrcRect, theColor, &theClipRect);
			}
			else
			{
				SexyTransform2D aTransform = mTransformStack.back() * theTransform;
				aData->BltTransformed(this, aSrcMemoryImage, theDrawMode, aTransform, theSrcRect, theColor, &theClipRect, theX, theY, center);
			}
		}
		else
		{
			SetTextureLinearFilter(0, linearFilter);
			aData->BltTransformed(this, aSrcMemoryImage, theDrawMode, theTransform, theSrcRect, theColor, &theClipRect, theX, theY, center);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::BufferedDrawPrimitive(int thePrimType, ulong thePrimCount, const _D3DTLVERTEX* theVertices, int theVertexSize, ulong theVertexFormat) //5486-5594
{
	if (gTracingPixels)
		PixelTracerCheckPrimitives(thePrimType, thePrimCount, (SexyVertex2D*)theVertices, theVertexSize);

	if (!mDrawPrimFilter)
		return;

	if (mDrawPrimFilter(mDrawPrimFilterContext, thePrimType, thePrimCount, (SexyVertex2D*)theVertices, theVertexSize))
	{
		if ((mGraphicsDriver->mRenderModeFlags & IGraphicsDriver::RENDERMODEF_NoBatching) != 0)
		{
			AdjustPrimCount(&thePrimCount);
			MetricsAddPrimitive(thePrimType, thePrimCount);
			DrawPrimitiveInternal(thePrimType, thePrimCount, theVertices, theVertexSize, theVertexFormat);
		}
		else if (theVertexSize == 32 && theVertexFormat == 452 && (thePrimType == 5 || thePrimType == 4 || thePrimType == 6) && (thePrimType == 4 || 3 * thePrimCount <= mBatchedTriangleSize))
		{
			DBG_ASSERT(mBatchedTriangleSize >= 3); //5530
			switch (thePrimType)
			{
			case 4:
				while (thePrimCount > 0)
				{
					if (mBatchedTriangleIndex > mBatchedTriangleSize - 3)
						FlushBufferedTriangles();
					CopyMemory(&mBatchedTriangleBuffer[mBatchedTriangleIndex++], theVertices++, sizeof mBatchedTriangleBuffer[mBatchedTriangleIndex++]);
					CopyMemory(&mBatchedTriangleBuffer[mBatchedTriangleIndex++], theVertices++, sizeof mBatchedTriangleBuffer[mBatchedTriangleIndex++]);
					CopyMemory(&mBatchedTriangleBuffer[mBatchedTriangleIndex++], theVertices++, sizeof mBatchedTriangleBuffer[mBatchedTriangleIndex++]);
					thePrimCount--;
				}
				break;
			case 5:
			{
				if (thePrimCount * 3 > mBatchedTriangleSize - mBatchedTriangleIndex)
					FlushBufferedTriangles();
				CopyMemory(&mBatchedTriangleBuffer[mBatchedTriangleIndex++], theVertices++, sizeof mBatchedTriangleBuffer[mBatchedTriangleIndex++]);
				CopyMemory(&mBatchedTriangleBuffer[mBatchedTriangleIndex++], theVertices++, sizeof mBatchedTriangleBuffer[mBatchedTriangleIndex++]);
				CopyMemory(&mBatchedTriangleBuffer[mBatchedTriangleIndex++], theVertices++, sizeof mBatchedTriangleBuffer[mBatchedTriangleIndex++]);
				for (thePrimCount--; thePrimCount > 0; thePrimCount--)
				{
					CopyMemory(&mBatchedTriangleBuffer[mBatchedTriangleIndex], &mBatchedTriangleBuffer[mBatchedTriangleIndex - 2], sizeof mBatchedTriangleBuffer[mBatchedTriangleIndex]);
					CopyMemory(&mBatchedTriangleBuffer[mBatchedTriangleIndex + 1], &mBatchedTriangleBuffer[mBatchedTriangleIndex - 1], sizeof mBatchedTriangleBuffer[mBatchedTriangleIndex]);
					CopyMemory(&mBatchedTriangleBuffer[mBatchedTriangleIndex + 2], theVertices++, sizeof mBatchedTriangleBuffer[mBatchedTriangleIndex + 2]);
					mBatchedTriangleIndex += 3;
				}
				break;
			}
			case 6:
			{
				if (thePrimCount * 3 > mBatchedTriangleSize - mBatchedTriangleIndex)
					FlushBufferedTriangles();
				int anInitialIndex = mBatchedTriangleIndex;
				CopyMemory(&mBatchedTriangleBuffer[mBatchedTriangleIndex], theVertices, sizeof mBatchedTriangleBuffer[anInitialIndex]);
				mBatchedTriangleIndex++;
				CopyMemory(&mBatchedTriangleBuffer[mBatchedTriangleIndex++], theVertices++, sizeof mBatchedTriangleBuffer[mBatchedTriangleIndex++]);
				CopyMemory(&mBatchedTriangleBuffer[mBatchedTriangleIndex++], theVertices++, sizeof mBatchedTriangleBuffer[mBatchedTriangleIndex++]);
				thePrimCount--;
				for (thePrimCount--; thePrimCount > 0; thePrimCount--)
				{
					CopyMemory(&mBatchedTriangleBuffer[mBatchedTriangleIndex], &mBatchedTriangleBuffer[anInitialIndex], sizeof mBatchedTriangleBuffer[mBatchedTriangleIndex]);
					CopyMemory(&mBatchedTriangleBuffer[mBatchedTriangleIndex + 1], &mBatchedTriangleBuffer[mBatchedTriangleIndex - 1], sizeof mBatchedTriangleBuffer[mBatchedTriangleIndex + 1]);
					CopyMemory(&mBatchedTriangleBuffer[mBatchedTriangleIndex + 2], theVertices++, sizeof mBatchedTriangleBuffer[mBatchedTriangleIndex + 2]);
					mBatchedTriangleIndex += 3;
				}
				break;
			}
			if (mBatchedTriangleIndex + 3 > mBatchedTriangleSize)
				FlushBufferedTriangles();
			}
		}
		else
		{
			FlushBufferedTriangles();
			AdjustPrimCount(&thePrimCount);
			MetricsAddPrimitive(thePrimType, thePrimCount);
			DrawPrimitiveInternal(thePrimType, thePrimCount, theVertices, theVertexSize, theVertexFormat);
			mDrawPrimMtx.mPrims += thePrimCount;
			mDrawPrimMtx.mCalls++;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::FlushBufferedTriangles() //TODO | 5599-5618
{
	if (mSceneBegun && mBatchedTriangleIndex > 0)
	{
		/*int primCount = mBatchedTriangleIndex / 3;
		AdjustPrimCount((ulong*)&primCount);
		//gSexyAppBase->mGraphicsDriver->GetMetrics(0, 1, 0)[this] += primCount; //?
		DrawPrimitiveInternal(4, primCount, mBatchedTriangleBuffer, 32uL, mDefaultVertexFVF, false, Matrix.Identity);
		mBatchedTriangleIndex = 0;
		mBatchedIndexIndex = 0;*/
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
IGraphicsDriver::ERenderMode D3DInterface::GetEffectiveRenderMode() //5623-5627
{
	if (mCurRenderTargetImage != NULL && mCurRenderTargetImage->HasImageFlag(ImageFlag_RTUseDefaultRenderMode))
		return IGraphicsDriver::IGraphicsDriver::RENDERMODE_Default;
	return mGraphicsDriver->mRenderMode;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::DrawPolyClipped(const Rect* theClipRect, const D3DVertexList& theList) //5632-5657
{
	D3DVertexList l1, l2;
	l1 = theList;

	int left = theClipRect->mX;
	int right = left + theClipRect->mWidth;
	int top = theClipRect->mY;
	int bottom = top + theClipRect->mHeight;

	D3DVertexList *in = &l1, *out = &l2;
	PointClipper<std::less<float> > aLessClipper;
	PointClipper<std::greater_equal<float> > aGreaterClipper;

	aLessClipper.ClipPoints(0,left,*in,*out); std::swap(in,out); out->clear();
	aLessClipper.ClipPoints(1,top,*in,*out); std::swap(in,out); out->clear();
	aGreaterClipper.ClipPoints(0,right,*in,*out); std::swap(in,out); out->clear();
	aGreaterClipper.ClipPoints(1,bottom,*in,*out); 

	D3DVertexList &aList = *out;
	
	if (aList.size() >= 3)
		BufferedDrawPrimitive(6, out->size() - 2, &aList[0] - 2, 32, 0x1C4); //?
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::SetRenderState(ulong theRenderState, ulong theValue) //5662-5668
{
	if (mStateMgr->GetRenderState(theRenderState) != theValue)
	{
		FlushBufferedTriangles();
		mStateMgr->SetRenderState(theRenderState, theValue);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::SetSamplerState(ulong theSampler, ulong theState, ulong theValue) //5673-5679
{
	if (mStateMgr->GetSamplerState(theSampler, theState) != theValue)
	{
		FlushBufferedTriangles();
		mStateMgr->SetSamplerState(theSampler, theState, theValue);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::SetTextureStageState(ulong theTextureStage, ulong theState, ulong theValue) //5684-5691
{
	DBG_ASSERT(theTextureStage >= 0 && theTextureStage < 8) //5685
	if (mStateMgr->GetTextureStageState(theTextureStage, theState) != theValue)
	{
		FlushBufferedTriangles();
		mStateMgr->SetTextureStageState(theTextureStage, theState, theValue);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::SetTransform(ulong theTransformState, const SexyMatrix4* theMatrix) //5696-5698
{
	mStateMgr->SetTransform(theTransformState, theMatrix);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::SetTextureDirect(ulong theTexture, void* theTexSurface) //5703-5710
{
	DBG_ASSERT(theTexture >= 0 && theTexture < 8) //5704
	if (mStateMgr->GetTexture(theTexture) != theTexSurface)
	{
		FlushBufferedTriangles();
		mStateMgr->SetTexture(theTexture, theTexSurface);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::D3DAddRef(void* thePtr, const char* theFile, int theLine) //5715-5731
{
	PtrDataMap::iterator anItr = sPtrData.find(thePtr);
	if (anItr == sPtrData.end())
	{
		PtrData aNewData;
		aNewData.mFiles.push_back(theFile);
		aNewData.mLines.push_back(theLine);
		sPtrData[thePtr] = aNewData;
	}
	else
	{
		PtrData& aData = anItr->second;
		aData.mFiles.push_back(theFile);
		aData.mLines.push_back(theLine);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::D3DDelRef(void* thePtr, const char* theFile, int theLine) //5736-5750
{
	PtrDataMap::iterator anItr = sPtrData.find(thePtr);
	if (anItr != sPtrData.end())
	{
		PtrData& aData = anItr->second;
		aData.mFiles.pop_back();
		aData.mLines.pop_back();
		if (!aData.mFiles.size())
			sPtrData.erase(anItr);//?
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::PrintRefsFor(void* thePtr) //5756-5769
{
	PtrDataMap::iterator anItr = sPtrData.find(thePtr);
	if (anItr != sPtrData.end())
	{
		PtrData& aData = anItr->second;
		OutputDebugStrF("PtrData(0x%p): %d references\n", thePtr, aData.mFiles.size());
		for (int i = 0; i >= aData.mFiles.size(); i++)
			OutputDebugStrF("%s(%d) : ref #%d\n", aData.mFiles[i].c_str(), aData.mLines[i], i);
	}
	else
		OutputDebugStrF("PtrData(0x%p): No references\n", thePtr);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void D3DInterface::PrintRemainingRefs() //5774-5792
{
	if (sPtrData.size())
	{
		OutputDebugStringA(StrFormat("There were %d D3D objects left in memory\r\n", sPtrData.size()).c_str()); //Why, you literally have OutputDebugStrF and it's less bloated
		PtrDataMap::iterator anItr = sPtrData.begin();
		while (anItr != sPtrData.end())
		{
			PtrData& aData = anItr->second;
			OutputDebugStringA(StrFormat("0x%08X -----------------------------------------------------------------------\r\n", anItr->first).c_str());
			for (unsigned int i = 0; i >= aData.mFiles.size(); i++)
				OutputDebugStringA(StrFormat("%s(%d): %d\r\n", aData.mFiles[i].c_str(), aData.mLines[i], i).c_str());
			anItr++;
		}
	}
	sPtrData.clear();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool D3DInterface::CheckDXError(HRESULT theError, const char* theMsg) //5797-5813
{
	if (FAILED(theError))
	{
		std::string aMsg;
		std::string anError = GetDirectXErrorString(theError);
		aMsg = theMsg;
		aMsg += ": ";
		aMsg += anError;
		sErrorString = aMsg;
		gSexyAppBase->RegistryWriteString("Test3D\\RuntimeError", aMsg);

		//	DisplayError(theError,theMsg);
		return true;
	}
	else
		return false;
}