/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2011-2015 CRI Middleware Co., Ltd.
 *
 * Library  : CRI Atom
 * Module   : Library User's Header for Android
 * File     : cri_atom_android.h
 *
 ****************************************************************************/
/*!
 *	\file		cri_atom_android.h
 */

/* 多重定義防止					*/
/* Prevention of redefinition	*/
#ifndef	CRI_INCL_CRI_ATOM_ANDROID_H
#define	CRI_INCL_CRI_ATOM_ANDROID_H

/***************************************************************************
 *      インクルードファイル
 *      Include files
 ***************************************************************************/
#include <jni.h>
#include <cri_le_xpt.h>
#include <cri_le_error.h>
#include <cri_le_atom.h>
#include <cri_le_atom_ex.h>
#include <cri_le_atom_asr.h>

/***************************************************************************
 *      定数マクロ
 *      Macro Constants
 ***************************************************************************/
 /*JP
 * \brief AAudio設定用の無効値
 * \ingroup ATOMLIB_Android
 * AAudio関連の設定を行う際に使用する無効値<br>
 */
#define CRIATOMANDROID_AAUDIO_OPTION_NONE (-1)

/*JP
 * \brief Stream Type ID
 * \ingroup ATOMLIB_Android
 * 音声出力先のストリームタイプを設定する値です。<br>
 */
typedef enum {
    CRIATOMANDROID_STREAM_TYPE_MUSIC,
    CRIATOMANDROID_STREAM_TYPE_ALARM,
    CRIATOMANDROID_STREAM_TYPE_DTMF ,
    CRIATOMANDROID_STREAM_TYPE_NOTIFICATION,
    CRIATOMANDROID_STREAM_TYPE_RING,
    CRIATOMANDROID_STREAM_TYPE_SYSTEM,
    CRIATOMANDROID_STREAM_TYPE_VOICE_CALL,
	CRIATOMANDROID_STREAM_TYPE_ENUM_SIZE_IS_4BYTES = 0x7FFFFFFF
} CriAtomAndroidStreamType;

/*==========================================================================
 *      CRI Atom API
 *=========================================================================*/
/*JP
 * \brief ライブラリ初期化用コンフィグ構造体にデフォルト値をセット
 * \ingroup ATOMLIB_Android
 * \param[out]	p_config	初期化用コンフィグ構造体へのポインタ
 * \par 説明:
 * ::criAtom_Initialize_ANDROID 関数に設定するコンフィグ構造体
 * （ ::CriAtomConfig_ANDROID ）に、デフォルトの値をセットします。<br>
 * \attention
 * 本マクロは下位レイヤ向けのAPIです。<br>
 * AtomExレイヤの機能を利用する際には、本マクロの代わりに
 * ::criAtomEx_SetDefaultConfig_ANDROID マクロをご利用ください。
 * \sa CriAtomConfig_ANDROID
 */
#define criAtom_SetDefaultConfig_ANDROID(p_config)			\
{															\
	(p_config)->initialize_hca_mx = CRI_TRUE;				\
	criAtom_SetDefaultConfig(&(p_config)->atom);			\
	criAtomHcaMx_SetDefaultConfig(&(p_config)->hca_mx);		\
}

/*JP
 * \brief スペーシャルオーディオ出力用サウンドレンダラ
 * \ingroup ATOMLIB_Android
 * \par 説明:
 * Androidのサウンドレンダラ識別子です。<br>
 * CRIATOM_SOUND_RENDERER_SPATIAL_AUDIO
 * を指定して作成されたASRラックに音声を出力した場合、
 * Dolby Atmos for Gamingをサポートしている端末では最大5.1.2chで出力されます。<br>
 */
#define CRIATOM_SOUND_RENDERER_SPATIAL_AUDIO	\
	(CRIATOM_SOUND_RENDERER_OBJECT)

/*==========================================================================
 *      CRI AtomEx API
 *=========================================================================*/
/*JP
 * \brief ライブラリ初期化用コンフィグ構造体にデフォルト値をセット
 * \ingroup ATOMLIB_Android
 * \param[out]	p_config	初期化用コンフィグ構造体へのポインタ
 * \par 説明:
 * ::criAtomEx_Initialize_ANDROID 関数に設定するコンフィグ構造体
 * （ ::CriAtomExConfig_ANDROID ）に、デフォルトの値をセットします。<br>
 * \sa CriAtomExConfig_ANDROID
 */
#define criAtomEx_SetDefaultConfig_ANDROID(p_config)		\
{															\
	(p_config)->initialize_hca_mx = CRI_TRUE;				\
	criAtomEx_SetDefaultConfig(&(p_config)->atom_ex);		\
	(p_config)->atom_ex.thread_model = CRIATOMEX_THREAD_MODEL_MULTI_WITH_SONICSYNC; \
	criAtomExAsr_SetDefaultConfig(&(p_config)->asr);		\
	criAtomExHcaMx_SetDefaultConfig(&(p_config)->hca_mx);	\
}

/*==========================================================================
 *      CRI AtomExAsr API
 *=========================================================================*/
/*JP
 * \brief ASRラック作成用Android固有パラメーター構造体にデフォルト値をセット
 * \ingroup ATOMLIB_Android
 * \param[out]	p_config	初期化用コンフィグ構造体へのポインタ
 * \par 説明:
 * ::criAtomExAsrRack_Create 関数の引数である::CriAtomExAsrRackConfig のcontextメンバに設定するANDROID固有パラメーター構造体（ ::CriAtomExAsrRackConfig_ANDROID ）に、
 * デフォルトの値をセットします。<br>
 * \sa
 * CriAtomExAsrRackConfig_ANDROID
 */
#define criAtomExAsrRack_SetDefaultConfig_ANDROID(p_config)	\
{\
	(p_config)->stream_type = CRIATOMANDROID_STREAM_TYPE_MUSIC;\
	(p_config)->enable_spatial_audio = CRI_FALSE;\
	(p_config)->aaudio.content_type = CRIATOMANDROID_AAUDIO_OPTION_NONE;\
	(p_config)->aaudio.usage = CRIATOMANDROID_AAUDIO_OPTION_NONE;\
	(p_config)->aaudio.allowed_capture_policy = CRIATOMANDROID_AAUDIO_OPTION_NONE;\
}
/***************************************************************************
 *      処理マクロ
 *      Macro Functions
 ***************************************************************************/

/***************************************************************************
 *      データ型宣言
 *      Data Type Declarations
 ***************************************************************************/
/*==========================================================================
 *      CRI Atom API
 *=========================================================================*/
/*JP
 * \brief Atomライブラリ初期化用コンフィグ構造体
 * \ingroup ATOMLIB_Android
 * CRI Atomライブラリの動作仕様を指定するための構造体です。<br>
 * ::criAtom_Initialize_ANDROID 関数の引数に指定します。<br>
 * \attention
 * 本構造体は下位レイヤ向けのAPIです。<br>
 * AtomExレイヤの機能を利用する際には、本構造体の代わりに
 * ::CriAtomExConfig_ANDROID 構造体をご利用ください。
 * \sa criAtom_Initialize_ANDROID, criAtom_SetDefaultConfig_ANDROID
 */
typedef struct {
	CriBool					initialize_hca_mx;	/*JP< HCA-MXを初期化するかどうか		*/
	CriAtomConfig			atom;				/*JP< Atom初期化用コンフィグ構造体		*/
	CriAtomAsrConfig		asr;				/*JP< ASR初期化用コンフィグ				*/
	CriAtomHcaMxConfig		hca_mx;				/*JP< HCA-MX初期化用コンフィグ構造体	*/
} CriAtomConfig_ANDROID;

/*==========================================================================
 *      CRI AtomEx API
 *=========================================================================*/
/*JP
 * \brief Atomライブラリ初期化用コンフィグ構造体
 * \ingroup ATOMLIB_Android
 * CRI Atomライブラリの動作仕様を指定するための構造体です。<br>
 * ::criAtomEx_Initialize_ANDROID 関数の引数に指定します。<br>
 * \sa criAtomEx_Initialize_ANDROID, criAtomEx_SetDefaultConfig_ANDROID
 */
typedef struct {
	CriBool					initialize_hca_mx;	/*JP< HCA-MXを初期化するかどうか		*/
	CriAtomExConfig			atom_ex;			/*JP< AtomEx初期化用コンフィグ構造体	*/
	CriAtomExAsrConfig		asr;				/*JP< ASR初期化用コンフィグ				*/
	CriAtomExHcaMxConfig	hca_mx;				/*JP< HCA-MX初期化用コンフィグ構造体	*/
} CriAtomExConfig_ANDROID;

/*JP
 * \brief 遅延推測器 状態列挙型
 * \ingroup ATOMLIB_Android
 * 遅延推測器の状態を表す列挙型です。 <br>
 */
typedef enum CriAtomExLatencyEstimatorStatus_Tag {
	CRIATOM_LATENCYESTIMATOR_STATUS_STOP,			/*JP< 初期状態/停止状態	(実行中スレッドなし)		*/
	CRIATOM_LATENCYESTIMATOR_STATUS_PROCESSING,		/*JP< 遅延時間を推測中		(実行中スレッドあり)	*/
	CRIATOM_LATENCYESTIMATOR_STATUS_DONE,			/*JP< 遅延時間の推測完了	(実行中スレッドなし)	*/
	CRIATOM_LATENCYESTIMATOR_STATUS_ERROR,			/*JP< エラー				(実行中スレッドなし)	*/
	CRIATOM_LATENCYESTIMATOR_STATUS_ENUM_SIZE_IS_4BYTES = 0x7FFFFFFF
} CriAtomLatencyEstimatorStatus;

/*JP
 * \brief 遅延推測器 情報構造体
 * \ingroup ATOMLIB_Android
 * 遅延推測器の情報を表す構造体です。<br>
 * 遅延推測器の状態と、推測遅延時間(ミリ秒)を持ちます。
 */
typedef struct CriAtomExLatencyEstimatorInfo_Tag {
	CriAtomLatencyEstimatorStatus	status;
	CriUint32						latency_msec; // 異常値は0
} CriAtomLatencyEstimatorInfo;

/*==========================================================================
 *      CRI AtomExAsr API
 *=========================================================================*/
/*JP
* \brief ASRラック作成用Android固有パラメーター構造体
* \ingroup ATOMLIB_Android
* ASR（Atom Sound Renderer）の動作仕様を指定するための構造体です。<br>
* ::criAtomExAsrRack_Create 関数の引数である::CriAtomExAsrRackConfig のcontextメンバに設定します。<br>
* \attention
* 将来的にメンバが増える可能性があるため、 設定を行う前に::criAtomExAsrRack_SetDefaultConfig_ANDROID
* マクロを使用して初期化してください。<br>
* （構造体のメンバに不定値が入らないようご注意ください。）
*/
typedef struct {
	/*JP
	 * \brief 出力音声ストリームタイプ
	 * \par 説明:
	 * Android OSにおける出力音声ストリームタイプを指定します。
	 * \attention
	 * ASRラックの出力先サウンドレンダラがCRIATOM_SOUND_RENDERER_NATIVE（デフォルト値）の場合のみ有効です。
	 */
	CriAtomAndroidStreamType stream_type;

	/*JP
	 * \brief 空間オーディオ有効フラグ
	 * \par 説明:
	 * 空間オーディオの使用を指定します。
	 * \attention
	 * デバイスが空間オーディオに対応していない場合、ダウンミックスした結果が出力されます。
	 */
	CriBool enable_spatial_audio;

	/*JP
 	 * \brief AAudio向け設定
 	 * \par 説明:
 	 * AAudioの再生リソース作成時の設定を指定します。
 	 * \attention
 	 * criAtom_EnableAAudio_ANDROID関数を使用してAAudioの機能を有効にする必要があります。
 	 */
	struct {
		/*JP
		 * \brief 出力オーディオデータの種別
		 * \par 説明:
		 * AAudioの再生リソース作成時に
		 * AAudioStreamBuilder_setContentType関数に渡す値を指定します。
		 * CRIATOMANDROID_AAUDIO_OPTION_NONEが指定されている場合、設定を行いません。
		 * \attention
		 * API Level 28以降で有効です。
		 */
		CriSint32 content_type;

		/*JP
		 * \brief 出力オーディオデータの使用目的
		 * \par 説明:
		 * AAudioの再生リソース作成時に
		 * AAudioStreamBuilder_setUsage関数に渡す値を指定します。
		 * CRIATOMANDROID_AAUDIO_OPTION_NONEが指定されている場合、設定を行いません。
		 * \attention
		 * API Level 28以降で有効です。
		 */
		CriSint32 usage;

		/*JP
		 * \brief 出力オーディオデータの録音ポリシー
		 * \par 説明:
		 * AAudioの再生リソース作成時に
		 * AAudioStreamBuilder_setAllowedCapturePolicy関数に渡す値を指定します。
		 * CRIATOMANDROID_AAUDIO_OPTION_NONEが指定されている場合、設定を行いません。
		 * \attention
		 * API Level 29以降で有効です。
		 */
		CriSint32 allowed_capture_policy;
	} aaudio;
} CriAtomExAsrRackConfig_ANDROID;

/***************************************************************************
 *      変数宣言
 *      Prototype Variables
 ***************************************************************************/

/***************************************************************************
 *      関数宣言
 *      Prototype Functions
 ***************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

/*==========================================================================
 *      CRI Atom API
 *=========================================================================*/
/*JP
 * \brief ライブラリ初期化用ワーク領域サイズの計算
 * \ingroup ATOMLIB_Android
 * \param[in]	config		初期化用コンフィグ構造体
 * \return		CriSint32	ワーク領域サイズ
 * \par 説明:
 * ライブラリを使用するために必要な、ワーク領域のサイズを取得します。<br>
 * \par 備考:
 * ライブラリが必要とするワーク領域のサイズは、ライブラリ初期化用コンフィグ
 * 構造体（ ::CriAtomConfig_ANDROID ）の内容によって変化します。<br>
 * <br>
 * 引数 config の情報は、関数内でのみ参照されます。<br>
 * 関数を抜けた後は参照されませんので、関数実行後に config の領域を解放しても
 * 問題ありません。
 * \attention
 * 本関数は下位レイヤ向けのAPIです。<br>
 * AtomExレイヤの機能を利用する際には、本関数の代わりに
 * ::criAtomEx_CalculateWorkSize_ANDROID 関数をご利用ください。
 * \sa CriAtomConfig_ANDROID, criAtom_Initialize_ANDROID
 */
CriSint32 CRIAPI criAtom_CalculateWorkSize_ANDROID(const CriAtomConfig_ANDROID *config);

/*JP
 * \brief ライブラリの初期化
 * \ingroup ATOMLIB_Android
 * \param[in]	config		初期化用コンフィグ構造体
 * \param[in]	work		ワーク領域
 * \param[in]	work_size	ワーク領域サイズ
 * \par 説明:
 * ライブラリを初期化します。<br>
 * ライブラリの機能を利用するには、必ずこの関数を実行する必要があります。<br>
 * （ライブラリの機能は、本関数を実行後、 ::criAtom_Finalize_ANDROID 関数を実行するまでの間、
 * 利用可能です。）<br>
 * <br>
 * ライブラリを初期化する際には、ライブラリが内部で利用するためのメモリ領域（ワーク領域）
 * を確保する必要があります。<br>
 * ライブラリが必要とするワーク領域のサイズは、初期化用コンフィグ構造体の内容に応じて
 * 変化します。<br>
 * ワーク領域サイズの計算には、 ::criAtom_CalculateWorkSize_ANDROID
 * 関数を使用してください。<br>
 * \par 備考:
 * ::criAtom_SetUserAllocator マクロを使用してアロケータを登録済みの場合、
 * 本関数にワーク領域を指定する必要はありません。<br>
 * （ work に NULL 、 work_size に 0 を指定することで、登録済みのアロケータ
 * から必要なワーク領域サイズ分のメモリが動的に確保されます。）
 * <br>
 * 引数 config の情報は、関数内でのみ参照されます。<br>
 * 関数を抜けた後は参照されませんので、関数実行後に config の領域を解放しても
 * 問題ありません。
 * \attention
 * 本関数は内部的に以下の関数を実行します。
 * - ::criAtom_Initialize
 * - ::criAtomAsr_Initialize
 * - ::criAtomHcaMx_Initialize
 * 本関数を実行する場合、上記関数を実行しないでください。<br>
 * <br>
 * 本関数を実行後、必ず対になる ::criAtom_Finalize_ANDROID 関数を実行してください。<br>
 * また、 ::criAtom_Finalize_ANDROID 関数を実行するまでは、本関数を再度実行しないでください。<br>
 * <br>
 * 本関数は下位レイヤ向けのAPIです。<br>
 * AtomExレイヤの機能を利用する際には、本関数の代わりに
 * ::criAtomEx_Initialize_ANDROID 関数をご利用ください。
 * \sa CriAtomConfig_ANDROID, criAtom_Finalize_ANDROID,
 * criAtom_SetUserAllocator, criAtom_CalculateWorkSize_ANDROID
 */
void CRIAPI criAtom_Initialize_ANDROID(
	const CriAtomConfig_ANDROID *config, void *work, CriSint32 work_size);

/*JP
 * \brief ライブラリの終了
 * \ingroup ATOMLIB_Android
 * \par 説明:
 * ライブラリを終了します。<br>
 * \attention
 * 本関数は内部的に以下の関数を実行します。<br>
 * 	- ::criAtom_Finalize
 * 	- ::criAtomAsr_Finalize
 * 	- ::criAtomHcaMx_Finalize
 * 本関数を実行する場合、上記関数を実行しないでください。<br>
 * <br>
 * ::criAtom_Initialize_ANDROID 関数実行前に本関数を実行することはできません。<br>
 * <br>
 * 本関数は下位レイヤ向けのAPIです。<br>
 * AtomExレイヤの機能を利用する際には、本関数の代わりに
 * ::criAtomEx_Finalize_ANDROID 関数をご利用ください。
 * \sa criAtom_Initialize_ANDROID
 */
void CRIAPI criAtom_Finalize_ANDROID(void);

/*JP
 * \brief サーバ処理スレッドのプライオリティ変更
 * \ingroup ATOMLIB_Android
 * \param[in]	prio	スレッドプライオリティ
 * \par 説明:
 * サーバ処理（ライブラリの内部処理）を行うスレッドのプライオリティを変更します。<br>
 * デフォルト状態（本関数を実行しない場合）では、サーバ処理スレッドのプライオリティは
 * -19(ナイス値) に設定されます。<br>
 * \attention:
 * 本関数は、ライブラリ初期化時にスレッドモデルをマルチスレッドモデル
 * （ ::CRIATOM_THREAD_MODEL_MULTI ）に設定した場合にのみ効果を発揮します。<br>
 * 他のスレッドモデルを選択した場合、本関数は何も処理を行いません。<br>
 * （エラーコールバックが発生します。）<br>
 * <br>
 * 本関数は初期化後～終了処理前の間に実行する必要があります。<br>
 * 初期化前や終了処理後に本関数を実行しても、効果はありません。<br>
 * （エラーコールバックが発生します。）<br>
 * <br>
 * サーバ処理スレッドは、CRI File Systemライブラリでも利用されています。<br>
 * すでにCRI File SystemライブラリのAPIでサーバ処理スレッドの設定を変更している場合
 * 本関数により設定が上書きされますのでご注意ください。<br>
 * \sa criAtom_Initialize_ANDROID, criAtom_GetThreadPriority_ANDROID
 */
void CRIAPI criAtom_SetThreadPriority_ANDROID(int prio);

/*JP
 * \brief サーバ処理スレッドのプライオリティ取得
 * \ingroup ATOMLIB_Android
 * \return	int		スレッドプライオリティ
 * \par 説明:
 * サーバ処理（ライブラリの内部処理）を行うスレッドのプライオリティを取得します。<br>
 * 取得に成功すると、本関数はサーバ処理を行うスレッドのプライオリティ(ナイス値)を返します。<br>
 * \attention:
 * 本関数は、ライブラリ初期化時にスレッドモデルをマルチスレッドモデル
 * （ ::CRIATOM_THREAD_MODEL_MULTI ）に設定した場合にのみ効果を発揮します。<br>
 * 他のスレッドモデルを選択した場合、本関数はエラー値を返します。<br>
 * （エラーコールバックが発生します。）<br>
 * <br>
 * 本関数は初期化後～終了処理前の間に実行する必要があります。<br>
 * 初期化前や終了処理後に本関数を実行した場合、本関数はエラー値を返します。<br>
 * （エラーコールバックが発生します。）<br>
 * \sa criAtom_Initialize_ANDROID, criAtom_SetThreadPriority_ANDROID
 */
int CRIAPI criAtom_GetThreadPriority_ANDROID(void);

/*JP
 * \brief JavaVMオブジェクトの設定
 * \ingroup ATOMLIB_Android
 * \return	JavaVM*		JavaVMオブジェクト
 * \par 説明:
 * JavaVMオブジェクトをAtomライブラリに登録します。<br>
 * Android OS 2.3より前のOSをサポートする場合は、本関数を必ず使用し、JavaVMオブジェクトを登録してください。<br>
 * Android OS 2.3以降のOSのみを対象とする場合は、本関数を呼び出さなくても動作します。<br>
 * \attention:
 * JavaVMオブジェクトを登録した場合、ライブラリ内部で作成したスレッドをJavaVMにアタッチします。<br>
 *
 * <br>
 * 本関数はライブラリ初期化前に実行する必要があります。<br>
 * \sa criAtom_Initialize_ANDROID
 */
void CRIAPI criAtom_SetJavaVM_ANDROID(JavaVM* vm);

/*JP
 * \brief 再生開始バッファリング時間の設定
 * \ingroup ATOMLIB_Android
 * \param[in]	time	再生開始バッファリング時間（ミリ秒単位）
 * \par 説明:
 * Atomライブラリ内で音声再生開始時にバッファリングする時間（ミリ秒単位）を指定します。<br>
 * 0 を指定した場合、デフォルトサイズでバッファリングします。<br>
 * 本関数の設定値はNSR（ネイティブサウンドレンダラ）モードでの再生開始レイテンシに影響します。<br>
 * （大きい値を設定してしまうとその分、再生開始レイテンシが大きくなることになります）<br>
 * \par 備考:
 * デフォルトでは3V相当の再生開始バッファリング時間を設定しています。
 * （サーバ周波数の設定が60[fps]：16.6[ms]の場合、3Vは約50[ms]の再生開始バッファリング時間になります）<br>
 * <br>
 * Android端末によっては再生開始バッファリング時間が少ないとノイズになってしまう場合があります。
 * その場合は本関数で調整することで改善する事があります。
 * \sa criAtom_SetSoundBufferingTime_ANDROID
 */
void CRIAPI criAtom_SetSoundStartBufferingTime_ANDROID(CriSint32 time);

/*JP
 * \brief サウンドデコードバッファ量の設定（ミリ秒単位）
 * \ingroup ATOMLIB_Android
 * \param[in]	time	サウンドデコードバッファ量（ミリ秒単位）
 * \par 説明:
 * Atomライブラリ内で音声再生で使用するデコードバッファ量（ミリ秒単位）を指定します。<br>
 * 0 を指定した場合、デフォルトサイズのサウンドデコードバッファ量を設定します。<br>
 * 本関数の設定値はASR（Atomサウンドレンダラ）モードでの再生開始レイテンシに影響します。<br>
 * （大きい値を設定してしまうとその分、再生開始レイテンシが大きくなることになります）<br>
 * \par 備考:
 * デフォルトでは4V相当のデコードバッファ量を設定します。
 * （サーバ周波数の設定が60[fps]：16.6[ms]の場合、4Vは約66[ms]のサウンドデコードバッファ量になります）<br>
 * <br>
 * Android端末によってはサウンドデコードバッファ量が少ないとノイズになってしまう場合があります。
 * その場合は本関数で調整することで改善する事があります。
 * \attention
 * 本関数は初期化よりも前に実行しておく必要があります。<br>
 * \sa criAtom_SetSoundStartBufferingTime_ANDROID
 */
void CRIAPI criAtom_SetSoundBufferingTime_ANDROID(CriSint32 time);

/*JP
 * \brief サウンドバッファサイズの設定
 * \ingroup ATOMLIB_Android
 * \param[in]	num_samples	サウンドバッファサイズ（サンプル数単位）
 * \par 説明:
 * 本関数は互換性のために残しております。
 * CRI Atom Ver.1.30.02以降はcriAtom_SetSoundBufferingTime_ANDROID関数を
 * 使用してサウンドバッファサイズを調整して下さい。
 * 現状は互換性のために旧関数として残しております。
 * <br>
 * Atomライブラリ内でボイスにキューイングするサンプル数の最大値を指定します。<br>
 * サウンドバッファのサイズはサンプル数単位で指定します。<br>
 * <br>
 * サウンドバッファサイズに 0 を指定した場合、
 * デフォルトサイズでサウンドバッファが作成されます。<br>
 * \sa criAtom_SetSoundBufferingTime_ANDROID
 */
void CRIAPI criAtom_SetSoundBufferSize_ANDROID(CriSint32 num_samples);

/*JP
 * \brief 端末固有プロパティの適用
 * \ingroup ATOMLIB_Android
 * \param[in]	vm		JavaVMオブジェクトの参照
 * \param[in]	context		android.content.Context オブジェクト
 * \return		CriBool	プロパティの適用に成功したか
 * \par 説明:
 * 出力サンプリングレートなど、端末固有のプロパティをライブラリの動作に適用します。<br>
 * CRI_TRUE を返した場合、適用に成功しています。<br>
 * CRI_FALSE を返した場合、API Levelが低いなどの理由により失敗しており、デフォルトの設定で
 * 動作します。<br>
 * \par 備考:
 * NSR（ネイティブサウンドレンダラ）モードを利用して音声出力を行う場合、端末やAndroid OSの
 * バージョンにより、音声再生の負荷が増大することがあります。<br>
 * 本関数の呼び出しにより、OpenSL ESのレイヤで端末のサンプリングレートに応じた動作を行う
 * ようになり、再生負荷を低減することが可能です。>
 * \attention
 * 本関数は初期化よりも前に実行しておく必要があります。<br>
 * \sa criAtomEx_Initialize_ANDROID
 */
CriBool CRIAPI criAtom_ApplyHardwareProperties_ANDROID(JavaVM* vm, jobject context);

/*==========================================================================
 *      CRI AtomEx API
 *=========================================================================*/
/*JP
 * \brief ライブラリ初期化用ワーク領域サイズの計算
 * \ingroup ATOMLIB_Android
 * \param[in]	config		初期化用コンフィグ構造体
 * \return		CriSint32	ワーク領域サイズ
 * \par 説明:
 * ライブラリを使用するために必要な、ワーク領域のサイズを取得します。<br>
 * \par 備考:
 * ライブラリが必要とするワーク領域のサイズは、ライブラリ初期化用コンフィグ
 * 構造体（ ::CriAtomExConfig_ANDROID ）の内容によって変化します。<br>
 * <br>
 * 引数 config の情報は、関数内でのみ参照されます。<br>
 * 関数を抜けた後は参照されませんので、関数実行後に config の領域を解放しても
 * 問題ありません。
 * \attention
 * ::CriAtomExConfig_ANDROID 構造体のacf_infoメンバに値を設定している場合、本関数は失敗し-1を返します。<br>
 * 初期化処理内でACFデータの登録を行う場合は、本関数値を使用したメモリ確保ではなくADXシステムによる
 * メモリアロケータを使用したメモリ確保処理が必要になります。
 * \sa CriAtomExConfig_ANDROID, criAtomEx_Initialize_ANDROID
 */
CriSint32 CRIAPI criAtomEx_CalculateWorkSize_ANDROID(const CriAtomExConfig_ANDROID *config);

/*JP
 * \brief ライブラリの初期化
 * \ingroup ATOMLIB_Android
 * \param[in]	config		初期化用コンフィグ構造体
 * \param[in]	work		ワーク領域
 * \param[in]	work_size	ワーク領域サイズ
 * \par 説明:
 * ライブラリを初期化します。<br>
 * ライブラリの機能を利用するには、必ずこの関数を実行する必要があります。<br>
 * （ライブラリの機能は、本関数を実行後、 ::criAtomEx_Finalize_ANDROID 関数を実行するまでの間、
 * 利用可能です。）<br>
 * <br>
 * ライブラリを初期化する際には、ライブラリが内部で利用するためのメモリ領域（ワーク領域）
 * を確保する必要があります。<br>
 * ライブラリが必要とするワーク領域のサイズは、初期化用コンフィグ構造体の内容に応じて
 * 変化します。<br>
 * ワーク領域サイズの計算には、 ::criAtomEx_CalculateWorkSize_ANDROID
 * 関数を使用してください。<br>
 * \par 備考:
 * ::criAtomEx_SetUserAllocator マクロを使用してアロケータを登録済みの場合、
 * 本関数にワーク領域を指定する必要はありません。<br>
 * （ work に NULL 、 work_size に 0 を指定することで、登録済みのアロケータ
 * から必要なワーク領域サイズ分のメモリが動的に確保されます。）
 * <br>
 * 引数 config の情報は、関数内でのみ参照されます。<br>
 * 関数を抜けた後は参照されませんので、関数実行後に config の領域を解放しても
 * 問題ありません。
 * \attention
 * 本関数は内部的に以下の関数を実行します。<br>
 * 	- ::criAtomEx_Initialize
 * 	- ::criAtomExAsr_Initialize
 * 	- ::criAtomExHcaMx_Initialize
 * 本関数を実行する場合、上記関数を実行しないでください。<br>
 * <br>
 * 本関数を実行後、必ず対になる ::criAtomEx_Finalize_ANDROID 関数を実行してください。<br>
 * また、 ::criAtomEx_Finalize_ANDROID 関数を実行するまでは、本関数を再度実行しないでください。<br>
 * \sa CriAtomExConfig_ANDROID, criAtomEx_Finalize_ANDROID,
 * criAtomEx_SetUserAllocator, criAtomEx_CalculateWorkSize_ANDROID
 */
void CRIAPI criAtomEx_Initialize_ANDROID(
	const CriAtomExConfig_ANDROID *config, void *work, CriSint32 work_size);

/*JP
 * \brief ライブラリの終了
 * \ingroup ATOMLIB_Android
 * \par 説明:
 * ライブラリを終了します。<br>
 * \attention
 * 本関数は内部的に以下の関数を実行します。<br>
 * 	- ::criAtomEx_Finalize
 * 	- ::criAtomExAsr_Finalize
 * 	- ::criAtomExHcaMx_Finalize
 * 本関数を実行する場合、上記関数を実行しないでください。<br>
 * <br>
 * ::criAtomEx_Initialize_ANDROID 関数実行前に本関数を実行することはできません。<br>
 * \sa criAtomEx_Initialize_ANDROID
 */
void CRIAPI criAtomEx_Finalize_ANDROID(void);

/*JP
 * \brief サウンド処理の再開
 * \ingroup ATOMLIB_Android
 * \par 説明:
 * 本関数内ではAtomサーバスレッドのロック状態が解除されます。<br>
 * また、ASRの停止を解除します。<br>
 * 本関数の動作保証のため、呼び出しの前には必ずcriAtom_StopSound_ANDROID関数が<br>
 * 呼ばれることを確認してください。
 * \sa criAtomEx_StartSound_ANDROID
 */
void CRIAPI criAtomEx_StartSound_ANDROID(void);

/*JP
 * \brief サウンド処理の停止
 * \ingroup ATOMLIB_Android
 * \par 説明:
 * 本関数内ではAtomサーバスレッドをロック状態にします。<br>
 * また、ASRの処理を停止します。<br>
 * 本関数の動作保証のため、呼び出しの後には必ずcriAtom_StartSound_ANDROID関数が<br>
 * 呼ばれることを確認してください。
 * \sa criAtomEx_StopSound_ANDROID
 */
void CRIAPI criAtomEx_StopSound_ANDROID(void);

/*JP
 * \brief AAudioの有効化
 * \ingroup ATOMLIB_Android
 * \par 説明:
 * 本関数にCRI_TRUEを指定すると
 * 音声出力APIにAndroid 8以降で使用できるAAudioを使用します。
 * AAudioが使用できない環境では、自動的にOpenSLESを使用した再生に切り替わります。
 * \attention
 * 本関数はライブラリの初期化前に呼び出してください。
 */
void CRIAPI criAtom_EnableAAudio_ANDROID(CriBool use_aaudio);

/*JP
 * \brief 音声遅延時間推測機能の有効化・無効化
 * \ingroup ATOMLIB_Android
 * \attention
 * 本関数はベータ機能です。製品には使用しないでください。<br/>
 * \par 説明:
 * Android端末の音声遅延時間推測機能の有効化・無効化を切り替えます。<br/>
 * 引数にCRI_TRUEを与える事で有効化、CRI_FALSEを与える事で無効化します。
 * 有効化すると、Atomライブラリの内部で遅延測定処理が動くようになります。<br/>
 * <br/>
 * 推測機能を有効にした状態で、 criAtom_GetSlBufferConsumptionLatency_ANDROID 関数を呼び出すと
 * 実行中のAndroid端末の音声再生遅延時間の推測値が取得できます。<br/>
 * <br/>
 * 本関数はAtom ライブラリの初期化前に呼んで下さい。
 * また、結果を取得した後は、本関数を使用して計測処理を無効化してください。
 * \sa criAtom_GetSlBufferConsumptionLatency_ANDROID
 */
void criAtom_EnableSlLatencyCheck_ANDROID(CriBool sw);

/*JP
 * \brief 音声再生遅延時間の取得
 * \ingroup ATOMLIB_Android
 * \attention
 * 本関数はベータ機能です。製品には使用しないでください。<br/>
 * \par 説明:
 * 実行中のAndroid端末上での音声再生遅延時間を取得できます。<br/>
 * <br/>
 * 計測結果はあくまで見積もりに過ぎず、実際の遅延時間と異なる場合があります。<br/>
 * 本関数で得られた結果は開発時のパラメーター調整のための目安とし、実際のアプリケーション上で使用しないでください。<br>
 * \par 注意:
 * 本関数を実行する場合、事前に criAtom_EnableSlLatencyCheck_ANDROID 関数を用いて、遅延計測処理を有効化しておく必要があります。
 * \sa criAtom_EnableSlLatencyCheck_ANDROID
 */
CriSint32 criAtom_GetSlBufferConsumptionLatency_ANDROID();	// 遅延を知りたいときに呼ぶ

/*JP
 * \brief サーバ処理スレッドのプライオリティ変更
 * \ingroup ATOMLIB_Android
 * \param[in]	prio	スレッドプライオリティ
 * \par 説明:
 * サーバ処理（ライブラリの内部処理）を行うスレッドのプライオリティを変更します。<br>
 * デフォルト状態（本関数を実行しない場合）では、サーバ処理スレッドのプライオリティは
 * -19(ナイス値) に設定されます。<br>
 * \attention:
 * 本関数は、ライブラリ初期化時にスレッドモデルをマルチスレッドモデル
 * （ ::CRIATOM_THREAD_MODEL_MULTI ）に設定した場合にのみ効果を発揮します。<br>
 * 他のスレッドモデルを選択した場合、本関数は何も処理を行いません。<br>
 * （エラーコールバックが発生します。）<br>
 * <br>
 * 本関数は初期化後～終了処理前の間に実行する必要があります。<br>
 * 初期化前や終了処理後に本関数を実行しても、効果はありません。<br>
 * （エラーコールバックが発生します。）<br>
 * <br>
 * サーバ処理スレッドは、CRI File Systemライブラリでも利用されています。<br>
 * すでにCRI File SystemライブラリのAPIでサーバ処理スレッドの設定を変更している場合
 * 本関数により設定が上書きされますのでご注意ください。<br>
 * \sa criAtomEx_Initialize_ANDROID, criAtomEx_GetThreadPriority_ANDROID
 */
#define criAtomEx_SetThreadPriority_ANDROID(prio)	\
	criAtom_SetThreadPriority_ANDROID(prio)

/*JP
 * \brief サーバ処理スレッドのプライオリティ取得
 * \ingroup ATOMLIB_Android
 * \return	int		スレッドプライオリティ
 * \par 説明:
 * サーバ処理（ライブラリの内部処理）を行うスレッドのプライオリティを取得します。<br>
 * 取得に成功すると、本関数はサーバ処理を行うスレッドのプライオリティ(ナイス値)を返します。<br>
 * \attention:
 * 本関数は、ライブラリ初期化時にスレッドモデルをマルチスレッドモデル
 * （ ::CRIATOM_THREAD_MODEL_MULTI ）に設定した場合にのみ効果を発揮します。<br>
 * 他のスレッドモデルを選択した場合、本関数はエラー値を返します。<br>
 * （エラーコールバックが発生します。）<br>
 * <br>
 * 本関数は初期化後～終了処理前の間に実行する必要があります。<br>
 * 初期化前や終了処理後に本関数を実行した場合、本関数はエラー値を返します。<br>
 * （エラーコールバックが発生します。）<br>
 * \sa criAtomEx_Initialize_ANDROID, criAtomEx_SetThreadPriority_ANDROID
 */
#define criAtomEx_GetThreadPriority_ANDROID()	\
	criAtom_GetThreadPriority_ANDROID()

/*JP
 * \brief JavaVMオブジェクトの設定
 * \ingroup ATOMLIB_Android
 * \return	JavaVM*		JavaVMオブジェクト
 * \par 説明:
 * JavaVMオブジェクトをAtomライブラリに登録します。<br>
 * Android OS 2.3より前のOSをサポートする場合は、本関数を必ず使用し、JavaVMオブジェクトを登録してください。<br>
 * Android OS 2.3以降のOSのみを対象とする場合は、本関数を呼び出さなくても動作します。<br>
 * \attention:
 * JavaVMオブジェクトを登録した場合、ライブラリ内部で作成したスレッドをJavaVMにアタッチします。<br>
 *
 * <br>
 * 本関数はライブラリ初期化前に実行する必要があります。<br>
 * \sa criAtomEx_Initialize_ANDROID
 */
#define criAtomEx_SetJavaVM_ANDROID(vm)		\
	criAtom_SetJavaVM_ANDROID(vm)

/*JP
 * \brief 遅延推測器の初期化
 * \ingroup ATOMLIB_Android
 * \par 説明:
 * 遅延推測器を初期化します。<br>
 * <br>
 * 遅延推測器を使用する際、本関数の呼び出しは必須です。
 * <br>
 * 本関数を呼び出すと、Atomライブラリ内部で遅延推測器が生成、起動します。
 * 起動された遅延推測器のスレッドは、他スレッドの要求等を待たず遅延推測処理を開始します。
 * 遅延推測を終了するには、::criAtomLatencyEstimator_Finalize_ANDROID 関数を呼ぶ必要があります。
 * <br>
 * また、本関数は完了復帰です。多重呼び出しを許容しますが、Atomライブラリ内では
 * 本関数の呼び出し回数をカウントしています。
 * 実際の初期化処理が実行されるのは、最初の呼び出しの時だけになります。
 * なお、この呼び出し回数カウンタは、::criAtomLatencyEstimator_Finalize_ANDROID 関数を呼び出す度に
 * デクリメントされます。
 * \attention
 * ::criAtom_Initialize_ANDROID 関数実行前に本関数を実行しないでください。<br>
 * \sa criAtomLatencyEstimator_Finalize_ANDROID
 */
void CRIAPI criAtomLatencyEstimator_Initialize_ANDROID();

/*JP
 * \brief 遅延推測器の終了
 * \ingroup ATOMLIB_Android
 * \par 説明:
 * 遅延推測器を終了します。<br>
 * <br>
 * 遅延推測器を終了する際、本関数を呼び出してください。
 * <br>
 * 起動中の遅延推測器スレッドは、本関数の呼び出しによって推測処理を終了します。
 * 推測処理を終えたスレッドは、自動的に破棄されます。
 * <br>
 * また、本関数は完了復帰です。多重呼び出しを許容します。
 * 本関数を呼び出すと、::criAtomLatencyEstimator_Initialize 関数の呼び出しカウンタが
 * デクリメントされます。本関数と::criAtomLatencyEstimator_Initialize 関数の呼び出し回数が
 * 同数になるよう注意してください。
 * \attention
 * ::criAtom_Initialize_ANDROID 関数実行前に本関数を実行しないでください。<br>
 * \sa criAtomLatencyEstimator_Initialize
 */
void CRIAPI criAtomLatencyEstimator_Finalize_ANDROID();

/*JP
 * \brief 遅延推測器の情報取得
 * \ingroup ATOMLIB_Android
 * \return	CriAtomLatencyEstimatorInfo		遅延推測器の情報
 * \par 説明:
 * 遅延推測器の現在の情報を取得します。<br>
 * 取得できる情報は「遅延推測器の状態」「推測遅延時間(ミリ秒)」の２つです。<br>
 * <br>
 * ::criAtomLatencyEstimator_Initialize 関数を呼び出した後、本関数を呼ぶことで
 * 現在の遅延推測器の情報(状態、推測遅延時間)を取得することができます。
 * 状態が ::CRIATOM_LATENCYESTIMATOR_STATUS_DONE である時、推測遅延時間は0でない数値になります。
 * それ以外の状態では、推測遅延時間は0を返します。
 * <br>
 * \attention
 * ::criAtomLatencyEstimator_Initialize 関数実行前に本関数を実行しないでください。<br>
 * ::criAtomLatencyEstimator_Finalize 関数実行後に本関数を実行しないでください。<br>
 * \sa criAtomLatencyEstimator_Initialize criAtomLatencyEstimator_Finalize
 */
CriAtomLatencyEstimatorInfo CRIAPI criAtomLatencyEstimator_GetCurrentInfo_ANDROID();

/*JP
 * \brief ライブラリ初期化状態の取得
 * \ingroup ATOMLIB_Android
 * \return	CriBool		初期化済みかどうか
 * \retval	CRI_FALSE	未初期化状態
 * \retval	CRI_TRUE	初期化済み
 * \par 説明:
 * 遅延推測器が既に初期化されているかどうかをチェックします。<br>
 * \sa criAtomLatencyEstimator_Initialize criAtomLatencyEstimator_Finalize
 */
CriBool CRIAPI criAtomLatencyEstimator_IsInitialized_ANDROID();

#ifdef __cplusplus
}
#endif

#endif	/* CRI_INCL_CRI_ATOM_ANDROID_H */


/***************************************************************************
 *      旧バージョンとの互換用
 *      For compatibility with old version
 ***************************************************************************/



















/* --- end of file --- */
