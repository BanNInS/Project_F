#ifndef _CRI_ATOM_EX_ACF_READER_H
#define _CRI_ATOM_EX_ACF_READER_H

#include "cri_le_xpt.h"
#include "cri_le_atom_ex.h"

#ifdef __cplusplus
extern "C" {
#endif

/* バージョン情報の取得 */
CriUint32 CRIAPI criAtomExAcfReader_GetVersionFromData(CriUint8* data, CriUint32 data_size, CriBool* flag);

/* モジュールサイズの取得 */
CriSint32 CRIAPI criAtomExAcfReader_CalculateWorkSize(void);

/* モジュールの初期化 */
void CRIAPI criAtomExAcfReader_Initialize(const void* config, void *work, CriSint32 work_size);

/* モジュールの終了 */
void CRIAPI criAtomExAcfReader_Finalize(void);

/* バイナリファイルのセット */
void CRIAPI criAtomExAcfReader_Register(CriUint8* data, CriUint32 data_size);

/* バイナリファイルのアンセット */
void CRIAPI criAtomExAcfReader_Unregister(void);

/* ACF情報の取得 */
CriBool CRIAPI criAtomExAcfReader_GetAcfInfo(CriAtomExAcfInfo* acf_info);

/* IDによるカテゴリテーブルインデックス取得 */
CriUint16 CRIAPI criAtomExAcfReader_GetCategoryIndexById(CriUint32 id);

/* Nameによるカテゴリテーブルインデックス取得 */
CriUint16 CRIAPI criAtomExAcfReader_GetCategoryIndexByName(const CriChar8* name);

/* カテゴリ情報の取得（インデックス指定） */
CriBool CRIAPI criAtomExAcfReader_GetCategoryInfo(CriUint16 index, CriAtomExCategoryInfo* info);

/* Dsp Setting数の取得 */
CriSint32 CRIAPI criAtomExAcfReader_GetNumberOfDspSettings(void);

/* Dsp Setting nameの取得 */
const CriChar8* CRIAPI criAtomExAcfReader_GetDspSettingNameByIndex(CriUint16 index);

/* デフォルトDSPバス設定名の取得 */
const CriChar8* CRIAPI criAtomExAcfReader_GetDefaultDspBusSettingName(void);

/* DSP設定の取得（ライブラリ内呼び出し用） */
CriBool CRIAPI criAtomExAcfReader_GetDspSettingInformation(const CriChar8* name, CriAtomExAcfDspSettingInfo* info);

/* */
CriBool CRIAPI criAtomExAcfReader_GetDspSettingSnapshotInformation(CriUint16 index, CriAtomExAcfDspSettingSnapshotInfo* info);

/* DSPバス情報の取得（ライブラリ内呼び出し用） */
CriBool CRIAPI criAtomExAcfReader_GetDspBusInformation(CriUint16 index, CriAtomExAcfDspBusInfo* info);

/* */
CriBool CRIAPI criAtomExAcfReader_GetDspBusLinkInformation(CriUint16 index, CriAtomExAcfDspBusLinkInfo* info);

/* Dsp fxの名前をインデックス指定により取得 */
const CriChar8* CRIAPI criAtomExAcfReader_GetDspFxName(CriUint16 index);

/* DSP Fxパラメーターの取得 - dsp_bypass can be NULL*/
CriBool CRIAPI criAtomExAcfReader_GetDspFxParameters(CriUint16 index, void* params, CriSint32 size, CriBool* dsp_bypass);

/* Category数の取得 */
CriUint16 CRIAPI criAtomExAcfReader_GetNumberOfCategories(void);

/* 再生毎カテゴリ参照数の取得 */
CriUint16 CRIAPI criAtomExAcfReader_GetNumberOfCategoriesPerPlayback(void);

/* Category Group数の取得 */
CriUint16 CRIAPI criAtomExAcfReader_GetNumberOfCategoryGroups(void);

/* カテゴリ情報の取得（インデックス指定） */
CriBool CRIAPI criAtomExAcfReader_GetCategoryInfo(CriUint16 index, CriAtomExCategoryInfo* info);

/* Global aisac数の取得 */
CriSint32 CRIAPI criAtomExAcfReader_GetNumberOfAisacControls(void);

/* Global aisac indexの取得 */
CriUint16 CRIAPI criAtomExAcfReader_GetGlobalAisacIndex(const CriChar8* name);

/* Global Aisac情報の取得（インデックス指定） */
CriBool CRIAPI criAtomExAcfReader_GetGlobalAisacInfo(CriUint16 index, CriAtomExGlobalAisacInfo* info);

CriBool CRIAPI criAtomExAcfReader_GetGlobalAisacDefaultControl(CriUint16 index, CriBool* default_control_flag, CriFloat32* default_control);

/* Global Aisac Nameの取得 */
const CriChar8* CRIAPI criAtomExAcfReader_GetGlobalAisacName(CriUint16 index);

CriBool CRIAPI criAtomExAcfReader_GetAisacControlInfoByNameIndex(CriUint16 index, CriAtomExAisacControlInfo* info);
CriBool CRIAPI criAtomExAcfReader_GetAisacControlInfoByIdIndex(CriUint16 index, CriAtomExAisacControlInfo* info);

CriAtomExAisacControlId CRIAPI criAtomExAcfReader_GetAisacControlId(const CriChar8* name);
const CriChar8* CRIAPI criAtomExAcfReader_GetAisacControlName(CriAtomExAisacControlId id);

CriUint16 CRIAPI criAtomExAcfReader_GetNumberOfSelectors(void);
const CriChar8* CRIAPI criAtomExAcfReader_GetSelectorName(CriUint16 index);
CriUint16 CRIAPI criAtomExAcfReader_GetNumberOfSelectorLabels(CriUint16 index);
const CriChar8* CRIAPI criAtomExAcfReader_GetSelectorLabel(CriUint16 index, CriUint16 label_index);

/* ゲーム変数の総数の取得 */
CriSint32 CRIAPI criAtomExAcfReader_GetNumGameVariables(void);
/* ゲーム変数情報の取得（インデックス指定） */
CriBool CRIAPI criAtomExAcfReader_GetGameVariableInfo(CriUint16 index, CriAtomExGameVariableInfo* info);
/* ゲーム変数情報の取得（ID指定） */
CriBool CRIAPI criAtomExAcfReader_GetGameVariableInfoById(CriUint16 id, CriAtomExGameVariableInfo* info);
/* ゲーム変数情報の取得（Name指定） */
CriBool CRIAPI criAtomExAcfReader_GetGameVariableInfoByName(const CriChar8* name, CriAtomExGameVariableInfo* info);

/* バイノーラライザータイプの取得 */
CriBool CRIAPI criAtomExAcfReader_GetBinauralizerType(CriSint32* binauralizer_type);

/* 出力ポート情報を取得 */
CriBool CRIAPI criAtomExAcfReader_GetOutputPortInfo(CriUint16 index, CriAtomExOutputPortInfo* info);
/* 出力ポートを取得 */
CriSint32 CRIAPI criAtomExAcfReader_GetNumberOfOutputPorts(void);

#ifdef __cplusplus
}
#endif

#endif /* _CRI_ATOM_EX_ACF_READER_H */
