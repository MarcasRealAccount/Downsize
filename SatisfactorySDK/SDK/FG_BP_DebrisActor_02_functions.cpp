// Satisfactory SDK

#ifdef _MSC_VER
	#pragma pack(push, 0x8)
#endif

#include "FG_BP_DebrisActor_02_parameters.hpp"

namespace SDK
{
//---------------------------------------------------------------------------
//Functions
//---------------------------------------------------------------------------

// Function BP_DebrisActor_02.BP_DebrisActor_02_C.UserConstructionScript
// ()

void ABP_DebrisActor_02_C::UserConstructionScript()
{
	static auto fn = UObject::FindObject<UFunction>("Function BP_DebrisActor_02.BP_DebrisActor_02_C.UserConstructionScript");

	ABP_DebrisActor_02_C_UserConstructionScript_Params params;

	auto flags = fn->FunctionFlags;

	UObject::ProcessEvent(fn, &params);

	fn->FunctionFlags = flags;
}


}

#ifdef _MSC_VER
	#pragma pack(pop)
#endif
