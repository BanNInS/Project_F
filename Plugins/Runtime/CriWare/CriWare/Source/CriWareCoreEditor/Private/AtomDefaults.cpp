/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomDefaults.cpp
 *
 ****************************************************************************/

#include "AtomDefaults.h"

FName Atom::GetNameForRackType(const UAtomRackBase* InRack)
{
	if (!ensure(InRack))
	{
		return RackName;
	}
	else if (InRack->IsA<UAtomRack>())
	{
		return RackName;
	}
	else if (InRack->IsA<UAtomSoundfieldRack>())
	{
		return SoundfieldRackName;
	}
	else if (InRack->IsA<UAtomEndpointRack>())
	{
		return EndpointRackName;
	}
	else if (InRack->IsA<UAtomSoundfieldEndpointRack>())
	{
		return SoundfieldEndpointRackName;
	}
	else
	{
		return RackName;
	}
}

FColor Atom::GetColorForRackType(const UAtomRackBase* InRack)
{
	if (!ensure(InRack))
	{
		return DefaultRackColor;
	}
	else if (InRack->IsA<UAtomRack>())
	{
		return DefaultRackColor;
	}
	else if (InRack->IsA<UAtomSoundfieldRack>())
	{
		return SoundfieldDefaultRackColor;
	}
	else if (InRack->IsA<UAtomEndpointRack>())
	{
		return EndpointDefaultRackColor;
	}
	else if (InRack->IsA<UAtomSoundfieldEndpointRack>())
	{
		return SoundfieldEndpointDefaultRackColor;
	}
	else
	{
		return DefaultRackColor;
	}
}

FColor Atom::GetColorForRackType(const FName& InRackName)
{
	if (InRackName == RackName)
	{
		return DefaultRackColor;
	}
	else if (InRackName == SoundfieldRackName)
	{
		return SoundfieldDefaultRackColor;
	}
	else if (InRackName == EndpointRackName)
	{
		return EndpointDefaultRackColor;
	}
	else if (InRackName == SoundfieldEndpointRackName)
	{
		return SoundfieldEndpointDefaultRackColor;
	}
	else
	{
		return DefaultRackColor;
	}
}

const bool Atom::IsConnectionPerformingSoundfieldConversion(const UAtomRackBase* InputRack, const UAtomRackBase* OutputRack)
{
	if (!ensure(InputRack && OutputRack))
	{
		return false;
	}
	else if (InputRack->IsA<UAtomSoundfieldRack>() || InputRack->IsA<UAtomSoundfieldEndpointRack>())
	{
		return OutputRack->IsA<UAtomRack>() || OutputRack->IsA<UAtomEndpointRack>();
	}
	else if (OutputRack->IsA<UAtomSoundfieldRack>() || OutputRack->IsA<UAtomSoundfieldEndpointRack>())
	{
		return InputRack->IsA<UAtomRack>() || InputRack->IsA<UAtomEndpointRack>();
	}
	else
	{
		return false;
	}
}
