

#include "AtomCustomVersion.h"
#include "Serialization/CustomVersion.h"

const FGuid FAtomCustomVersion::GUID(0x908F3629, 0x21CA49EA, 0x940059CD, 0xAEAFC642);

// Register the custom version with core
FCustomVersionRegistration GRegisterAtomCustomVersion(FAtomCustomVersion::GUID, FAtomCustomVersion::LatestVersion, TEXT("AtomVer"));
