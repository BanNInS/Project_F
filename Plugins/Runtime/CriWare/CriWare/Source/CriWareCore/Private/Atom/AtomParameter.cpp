#include "Atom/AtomParameter.h"

#include "CriWareCorePrivate.h"

#define LOCTEXT_NAMESPACE "AtomParameters"

/*
 * FAtomSelectorParam struct
 *****************************************************************************/

FAtomSelectorParam::FAtomSelectorParam()
{
#if WITH_EDITOR
	/* Localization of unreal properties metadata with LOCTEXT markups and reflection */
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

#undef LOCTEXT_NAMESPACE
