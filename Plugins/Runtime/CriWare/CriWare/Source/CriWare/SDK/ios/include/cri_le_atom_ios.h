/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2010-2015 CRI Middleware Co., Ltd.
 *
 * Library  : CRI Atom
 * Module   : Library User's Header for iOS
 * File     : cri_atom_ios.h
 *
 ****************************************************************************/
/*!
 *	\file		cri_atom_ios.h
 */

/* 多重定義防止					*/
/* Prevention of redefinition	*/
#ifndef	CRI_INCL_CRI_ATOM_IOS_H
#define	CRI_INCL_CRI_ATOM_IOS_H

/***************************************************************************
 *      インクルードファイル
 *      Include files
 ***************************************************************************/
#include <cri_le_error.h>
#include <cri_le_atom.h>
#include <cri_le_atom_ex.h>
#include <cri_le_atom_asr.h>

/***************************************************************************
 *      定数マクロ
 *      Macro Constants
 ***************************************************************************/
/*==========================================================================
 *      CRI Atom API
 *=========================================================================*/
/*JP
 * \brief ライブラリ初期化用コンフィグ構造体にデフォルト値をセット
 * \ingroup ATOMLIB_IOS
 * \param[out]	p_config	初期化用コンフィグ構造体へのポインタ
 * \par 説明:
 * ::criAtom_Initialize_IOS 関数に設定するコンフィグ構造体
 * （ ::CriAtomConfig_IOS ）に、デフォルトの値をセットします。<br>
 * \attention
 * 本マクロは下位レイヤ向けのAPIです。<br>
 * AtomExレイヤの機能を利用する際には、本マクロの代わりに 
 * ::criAtomEx_SetDefaultConfig_IOS マクロをご利用ください。
 * \sa CriAtomConfig_IOS
 */
#define criAtom_SetDefaultConfig_IOS(p_config)				\
{															\
	criAtom_SetDefaultConfig(&(p_config)->atom);			\
	criAtomExAsr_SetDefaultConfig(&(p_config)->asr);		\
    criAtomHcaMx_SetDefaultConfig(&(p_config)->hca_mx);		\
    (p_config)->buffering_time = 50;						\
    (p_config)->output_sampling_rate = CRIATOM_DEFAULT_OUTPUT_SAMPLING_RATE; \
}

/*JP
 * \brief AudioSession設定用コンフィグ構造体にデフォルト値をセット
 * \ingroup ATOMLIB_IOS
 * \param[out]	p_config	AudioSession設定用コンフィグ構造体へのポインタ
 * \par 説明:
 * ::criAtom_SetupAudioSession_IOS 関数に設定するコンフィグ構造体
 * （ ::CriAtomAudioSessionConfig_IOS ）に、デフォルトの値をセットします。<br>
 * \sa CriAtomAudioSessionConfig_IOS
 */
#define criAtom_SetDefaultAudioSessionConfig_IOS(p_config)				\
{															\
	(p_config)->enable_microphone = CRI_FALSE;				\
	(p_config)->enable_background_audio = CRI_TRUE;			\
}

/*==========================================================================
 *      CRI Atom Player API
 *=========================================================================*/
/*JP
 * \brief CriAtomMp3PlayerConfig_IOSへのデフォルトパラメータのセット
 * \ingroup ATOMLIB_IOS
 * \param[in]	p_config	MP3プレーヤ作成用コンフィグ構造体へのポインタ
 * \par 説明:
 * ::criAtomPlayer_CreateMp3Player_IOS 関数に設定するコンフィグ構造体
 * （ ::CriAtomMp3PlayerConfig_IOS ）に、デフォルトの値をセットします。<br>
 * \sa CriAtomMp3PlayerConfig_IOS, criAtomPlayer_CreateMp3Player_IOS
 */
#define criAtomPlayer_SetDefaultConfigForMp3Player_IOS(p_config)				\
{																				\
	(p_config)->max_channels		= CRIATOM_DEFAULT_INPUT_MAX_CHANNELS;		\
	(p_config)->max_sampling_rate	= CRIATOM_DEFAULT_INPUT_MAX_SAMPLING_RATE;	\
	(p_config)->streaming_flag		= CRI_FALSE;								\
	(p_config)->sound_renderer_type = CRIATOM_SOUND_RENDERER_DEFAULT;			\
}

/*==========================================================================
 *      CRI AtomEx API
 *=========================================================================*/
/*JP
 * \brief ライブラリ初期化用コンフィグ構造体にデフォルト値をセット
 * \ingroup ATOMLIB_IOS
 * \param[out]	p_config	初期化用コンフィグ構造体へのポインタ
 * \par 説明:
 * ::criAtomEx_Initialize_IOS 関数に設定するコンフィグ構造体
 * （ ::CriAtomExConfig_IOS ）に、デフォルトの値をセットします。<br>
 * \sa CriAtomExConfig_IOS
 */
#define criAtomEx_SetDefaultConfig_IOS(p_config)			\
{															\
	criAtomEx_SetDefaultConfig(&(p_config)->atom_ex);		\
    (p_config)->atom_ex.thread_model = CRIATOMEX_THREAD_MODEL_MULTI_WITH_SONICSYNC; \
	criAtomExAsr_SetDefaultConfig(&(p_config)->asr);		\
    criAtomExHcaMx_SetDefaultConfig(&(p_config)->hca_mx);	\
    (p_config)->buffering_time = 50;						\
    (p_config)->output_sampling_rate = CRIATOM_DEFAULT_OUTPUT_SAMPLING_RATE; \
    (p_config)->use_handling_os_notifications = CRI_TRUE;			\
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
 * \ingroup ATOMLIB_IOS
 * CRI Atomライブラリの動作仕様を指定するための構造体です。<br>
 * ::criAtom_Initialize_IOS 関数の引数に指定します。<br>
 * \attention
 * 本構造体は下位レイヤ向けのAPIです。<br>
 * AtomExレイヤの機能を利用する際には、本構造体の代わりに 
 * ::CriAtomExConfig_IOS 構造体をご利用ください。
 * \sa criAtom_Initialize_IOS, criAtom_SetDefaultConfig_IOS
 */
typedef struct {
	CriAtomConfig			atom;		/* Atom初期化用コンフィグ構造体		*/
	CriAtomAsrConfig		asr;		/* ASR初期化用コンフィグ			*/
	CriAtomHcaMxConfig		hca_mx;		/* HCA-MX初期化用コンフィグ構造体		*/
	CriUint32				buffering_time;	/* 出力バッファリング時間(単位:msec)	*/
	CriSint32				output_sampling_rate;	/* 出力サンプリング周波数	*/
} CriAtomConfig_IOS;

/*JP
 * \brief AudioSession設定用コンフィグ構造体
 * \ingroup ATOMLIB_IOS
 * AudioSession設定を行うための設定情報を示す構造体です。<br>
 * ::criAtom_SetupAudioSession_IOS 関数の引数に指定します。<br>
 * \sa criAtom_SetupAudioSession_IOS
 */
typedef struct {
	CriBool enable_microphone;			/* マイクデバイスを使用するか		*/
	CriBool enable_background_audio;	/* バックグラウンド再生を許可するか	*/
} CriAtomAudioSessionConfig_IOS;

/*==========================================================================
 *      CRI Atom Player API
 *=========================================================================*/
/*JP
 * MP3プレーヤ作成用コンフィグ構造体
 * \ingroup ATOMLIB_IOS
 * \par 説明:
 * MP3が再生可能なプレーヤを作成する際に、動作仕様を指定するための構造体です。<br>
 * ::criAtomPlayer_CreateMp3Player_IOS 関数の引数に指定します。<br>
 * <br>
 * 作成されるプレーヤは、ハンドル作成時に本構造体で指定された設定に応じて、
 * 内部リソースを必要なだけ確保します。<br>
 * プレーヤが必要とするワーク領域のサイズは、本構造体で指定されたパラメータに応じて変化します。
 * \attention
 * 将来的にメンバが増える可能性があるため、
 * ::criAtomPlayer_SetDefaultConfigForMp3Player_IOS マクロで必ず構造体を初期化してください。<br>
 * （構造体のメンバに不定値が入らないようご注意ください。）
 * \sa criAtomPlayer_CreateMp3Player_IOS
 */
typedef struct {
	CriSint32 max_channels;
	CriSint32 max_sampling_rate;
	CriBool streaming_flag;
	CriAtomSoundRendererType sound_renderer_type;
} CriAtomMp3PlayerConfig_IOS;

/*==========================================================================
 *      CRI AtomEx API
 *=========================================================================*/
/*JP
 * \brief Atomライブラリ初期化用コンフィグ構造体
 * \ingroup ATOMLIB_IOS
 * CRI Atomライブラリの動作仕様を指定するための構造体です。<br>
 * ::criAtomEx_Initialize_IOS 関数の引数に指定します。<br>
 * \sa criAtomEx_Initialize_IOS, criAtomEx_SetDefaultConfig_IOS
 */
typedef struct {
	CriAtomExConfig			atom_ex;	/* AtomEx初期化用コンフィグ構造体	*/
	CriAtomExAsrConfig		asr;		/* ASR初期化用コンフィグ		*/
	CriAtomExHcaMxConfig	hca_mx;		/* HCA-MX初期化用コンフィグ構造体	*/
	CriUint32				buffering_time;	/* 出力バッファリング時間(単位:msec)	*/
	CriSint32				output_sampling_rate;	/* 出力サンプリング周波数	*/
    CriBool					use_handling_os_notifications;   /* OSからの通知対応処理を行う */
} CriAtomExConfig_IOS;

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
 * \ingroup ATOMLIB_IOS
 * \param[in]	config		初期化用コンフィグ構造体
 * \return		CriSint32	ワーク領域サイズ
 * \par 説明:
 * ライブラリを使用するために必要な、ワーク領域のサイズを取得します。<br>
 * \par 備考:
 * ライブラリが必要とするワーク領域のサイズは、ライブラリ初期化用コンフィグ
 * 構造体（ ::CriAtomConfig_IOS ）の内容によって変化します。<br>
 * <br>
 * 引数 config の情報は、関数内でのみ参照されます。<br>
 * 関数を抜けた後は参照されませんので、関数実行後に config の領域を解放しても
 * 問題ありません。
 * \attention
 * 本関数は下位レイヤ向けのAPIです。<br>
 * AtomExレイヤの機能を利用する際には、本関数の代わりに 
 * ::criAtomEx_CalculateWorkSize_IOS 関数をご利用ください。
 * \sa CriAtomConfig_IOS, criAtom_Initialize_IOS
 */
CriSint32 CRIAPI criAtom_CalculateWorkSize_IOS(const CriAtomConfig_IOS *config);

/*JP
 * \brief ライブラリの初期化
 * \ingroup ATOMLIB_IOS
 * \param[in]	config		初期化用コンフィグ構造体
 * \param[in]	work		ワーク領域
 * \param[in]	work_size	ワーク領域サイズ
 * \par 説明:
 * ライブラリを初期化します。<br>
 * ライブラリの機能を利用するには、必ずこの関数を実行する必要があります。<br>
 * （ライブラリの機能は、本関数を実行後、 ::criAtom_Finalize_IOS 関数を実行するまでの間、
 * 利用可能です。）<br>
 * <br>
 * ライブラリを初期化する際には、ライブラリが内部で利用するためのメモリ領域（ワーク領域）
 * を確保する必要があります。<br>
 * ライブラリが必要とするワーク領域のサイズは、初期化用コンフィグ構造体の内容に応じて
 * 変化します。<br>
 * ワーク領域サイズの計算には、 ::criAtom_CalculateWorkSize_IOS 
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
 * 本関数は内部的に以下の関数を実行します。<br>
 * 	- ::criAtom_Initialize
 * 	- ::criAtomAsr_Initialize
 * 	- ::criAtomHcaMx_Initialize
 * 本関数を実行する場合、上記関数を実行しないでください。<br>
 * <br>
 * 本関数を実行後、必ず対になる ::criAtom_Finalize_IOS 関数を実行してください。<br>
 * また、 ::criAtom_Finalize_IOS 関数を実行するまでは、本関数を再度実行しないでください。<br>
 * <br>
 * 本関数は下位レイヤ向けのAPIです。<br>
 * AtomExレイヤの機能を利用する際には、本関数の代わりに 
 * ::criAtomEx_Initialize_IOS 関数をご利用ください。
 * \sa CriAtomConfig_IOS, criAtom_Finalize_IOS,
 * criAtom_SetUserAllocator, criAtom_CalculateWorkSize_IOS
 */
void CRIAPI criAtom_Initialize_IOS(
	const CriAtomConfig_IOS *config, void *work, CriSint32 work_size);

/*JP
 * \brief ライブラリの終了
 * \ingroup ATOMLIB_IOS
 * \par 説明:
 * ライブラリを終了します。<br>
 * \attention
 * 本関数は内部的に以下の関数を実行します。<br>
 * 	- ::criAtom_Finalize
 * 	- ::criAtomAsr_Finalize
 * 	- ::criAtomHcaMx_Finalize
 * 本関数を実行する場合、上記関数を実行しないでください。<br>
 * <br>
 * ::criAtom_Initialize_IOS 関数実行前に本関数を実行することはできません。<br>
 * <br>
 * 本関数は下位レイヤ向けのAPIです。<br>
 * AtomExレイヤの機能を利用する際には、本関数の代わりに 
 * ::criAtomEx_Finalize_IOS 関数をご利用ください。
 * \sa criAtom_Initialize_IOS
 */
void CRIAPI criAtom_Finalize_IOS(void);

/*JP
 * \brief サーバスレッドプライオリティの設定
 * \ingroup ATOMLIB_IOS
 * \param[in]	prio	スレッドのプライオリティ
 * \par 説明:
 * CRIサーバスレッドのプライオリティを設定します。<br>
 * 引数 prio は pthread のプライオリティ設定値として使用します。<br>
 * プライオリティ設定値はメインスレッドからの相対値になります。<br>
 * アプリケーションのメインスレッド(0)よりも高いプライオリティを指定してください。<br>
 * プライオリティのデフォルト値は16です。<br>
 * \attention
 * ::criAtom_Initialize_IOS 関数実行前に本関数を実行することはできません。<br>
 * <br>
 * サーバ処理スレッドは、CRI File Systemライブラリでも利用されています。<br>
 * すでにCRI File SystemライブラリのAPIでサーバ処理スレッドの設定を変更している場合
 * 本関数により設定が上書きされますのでご注意ください。<br>
 */
void CRIAPI criAtom_SetServerThreadPriority_IOS(int prio);

/*JP
 * \brief サウンド処理の再開
 * \ingroup ATOMLIB_IOS
 * \par 説明:
 * AudioSessionのInterruption Callbak関数から呼び出すための関数です。<br>
 * サウンド処理を再開します。<br>
 * 本関数を呼び出す前に、AudioSessionのパメラータ設定とアクティベイトを行ってください。<br>
 * <br>
 * 本関数は下位レイヤ向けのAPIです。<br>
 * AtomExレイヤの機能を利用する際には、本関数の代わりに 
 * ::criAtomEx_StartSound_IOS 関数をご利用ください。
 * \attention
 * ::criAtom_Initialize_IOS 関数実行前に本関数を実行することはできません。<br>
 * \sa criAtom_StopSound_IOS
 */
void CRIAPI criAtom_StartSound_IOS(void);

/*JP
 * \brief サウンド処理の停止
 * \ingroup ATOMLIB_IOS
 * \par 説明:
 * AudioSessionのInterruption Callbak関数から呼び出すための関数です。<br>
 * サウンド処理を停止します。<br>
 * <br>
 * 本関数は下位レイヤ向けのAPIです。<br>
 * AtomExレイヤの機能を利用する際には、本関数の代わりに 
 * ::criAtomEx_StopSound_IOS 関数をご利用ください。
 * \attention
 * ::criAtom_Initialize_IOS 関数実行前に本関数を実行することはできません。<br>
 * \sa criAtom_StartSound_IOS
 */
void CRIAPI criAtom_StopSound_IOS(void);

/*JP
 * \brief サウンドの復旧
 * \ingroup ATOMLIB_IOS
 * \par 説明:
 * AudioSessionAddPropertyListener Callback関数から呼び出すための関数です。<br>
 * ライブラリ内部のボイスを復旧します。<br>
 * iOSのデーモンであるmediaserverdが死亡した際には、ライブラリ内のボイスが無効なボイスになり、
 * 再生成が必要になります。<br>
 * このように、ボイスの復旧が必要な際に呼び出してください。<br>
 * <br>
 * 本関数は下位レイヤ向けのAPIです。<br>
 * AtomExレイヤの機能を利用する際には、本関数の代わりに 
 * ::criAtomEx_RecoverSound_IOS 関数をご利用ください。
 */
void CRIAPI criAtom_RecoverSound_IOS(void);

 /*JP
  * \brief サウンドの初期化に成功したか否か
  * \ingroup ATOMLIB_IOS
  * \par 説明:
  * サウンドライブラリの初期化に成功したか否かを返す関数です。<br>
  * iOSでは、アプリがバックグラウンドにある状態でサウンドライブラリの初期化を行った場合に
  * 内部的にAudioSessionの初期化等に失敗している場合があります。<br>
  * 本関数で初期化が失敗していることを確認した場合は、アプリがフォアグラウンドにある状態で
  * 再度ライブラリの初期化を行うか、 ::criAtom_RecoverSound_IOS を用いて
  * サウンドの復旧を行う必要があります。<br>
  * <br>
  * 本関数は下位レイヤ向けのAPIです。<br>
  * AtomExレイヤの機能を利用する際には、本関数の代わりに
  * ::criAtomEx_IsInitializationSucceeded_IOS 関数をご利用ください。
  */
CriBool CRIAPI criAtom_IsInitializationSucceeded_IOS(void);

/*JP
 * \brief AudioSessionの設定
 * \ingroup ATOMLIB_IOS
 * \param[in]	config		コンフィグ構造体
 * \par 説明:
 * コンフィグに従ってAudioSessionの設定を行います。<br>
 * より詳細な設定を行いたい場合はこの関数を呼び出さず、AudioSessionの各種APIを用いて設定してください。<br>
 */
void CRIAPI criAtom_SetupAudioSession_IOS(CriAtomAudioSessionConfig_IOS* config);

/*==========================================================================
 *      CRI Atom Player API
 *=========================================================================*/
/*JP
 * \brief MP3プレーヤ作成用ワーク領域サイズの計算
 * \ingroup ATOMLIB_IOS
 * \param[in]	config		MP3プレーヤ作成用コンフィグ構造体
 * \return		CriSint32	ワーク領域サイズ
 * \par 説明:
 * MP3再生用プレーヤを作成するために必要な、ワーク領域のサイズを取得します。<br>
 * \par 備考:
 * プレーヤの作成に必要なワークメモリのサイズは、プレーヤ作成用コンフィグ
 * 構造体（ ::CriAtomMp3PlayerConfig_IOS ）の内容によって変化します。<br>
 * <br>
 * 引数にNULLを指定した場合、デフォルト設定
 * （ ::criAtomPlayer_SetDefaultConfigForMp3Player_IOS 適用時と同じパラメータ）で
 * ワーク領域サイズを計算します。
 * <br>
 * 引数 config の情報は、関数内でのみ参照されます。<br>
 * 関数を抜けた後は参照されませんので、関数実行後に config の領域を解放しても
 * 問題ありません。
 * \sa CriAtomMp3PlayerConfig_IOS, criAtomPlayer_CreateMp3Player_IOS
 */
CriSint32 CRIAPI criAtomPlayer_CalculateWorkSizeForMp3Player_IOS(
	const CriAtomMp3PlayerConfig_IOS *config);

/*JP
 * \brief MP3プレーヤの作成
 * \ingroup ATOMLIB_IOS
 * \param[in]	config		MP3プレーヤ作成用コンフィグ構造体
 * \param[in]	work		ワーク領域
 * \param[in]	work_size	ワーク領域サイズ
 * \return		CriAtomPlayerHn	Atomプレーヤハンドル
 * \par 説明:
 * MP3が再生可能なプレーヤを作成します。<br>
 * <br>
 * 本関数は完了復帰型の関数です。<br>
 * ゲームループ等の画面更新が必要なタイミングで本関数を実行するとミリ秒単位で
 * 処理がブロックされ、フレーム落ちが発生する恐れがあります。<br>
 * MP3プレーヤの作成／破棄は、シーンの切り替わり等、負荷変動を許容できる
 * タイミングで行うようお願いいたします。<br>
 * \sa CriAtomMp3PlayerConfig_IOS, criAtomPlayer_CalculateWorkSizeForMp3Player_IOS,
 * criAtomPlayer_Destroy
 */
CriAtomPlayerHn CRIAPI criAtomPlayer_CreateMp3Player_IOS(
	const CriAtomMp3PlayerConfig_IOS *config, void *work, CriSint32 work_size);

/*==========================================================================
 *      CRI AtomEx API
 *=========================================================================*/
/*JP
 * \brief ライブラリ初期化用ワーク領域サイズの計算
 * \ingroup ATOMLIB_IOS
 * \param[in]	config		初期化用コンフィグ構造体
 * \return		CriSint32	ワーク領域サイズ
 * \par 説明:
 * ライブラリを使用するために必要な、ワーク領域のサイズを取得します。<br>
 * \par 備考:
 * ライブラリが必要とするワーク領域のサイズは、ライブラリ初期化用コンフィグ
 * 構造体（ ::CriAtomExConfig_IOS ）の内容によって変化します。<br>
 * <br>
 * 引数 config の情報は、関数内でのみ参照されます。<br>
 * 関数を抜けた後は参照されませんので、関数実行後に config の領域を解放しても
 * 問題ありません。
 * \attention
 * ::CriAtomExConfig_IOS 構造体のacf_infoメンバに値を設定している場合、本関数は失敗し-1を返します。<br>
 * 初期化処理内でACFデータの登録を行う場合は、本関数値を使用したメモリ確保ではなくADXシステムによる
 * メモリアロケータを使用したメモリ確保処理が必要になります。
 * \sa CriAtomExConfig_IOS, criAtomEx_Initialize_IOS
 */
CriSint32 CRIAPI criAtomEx_CalculateWorkSize_IOS(const CriAtomExConfig_IOS *config);

/*JP
 * \brief ライブラリの初期化
 * \ingroup ATOMLIB_IOS
 * \param[in]	config		初期化用コンフィグ構造体
 * \param[in]	work		ワーク領域
 * \param[in]	work_size	ワーク領域サイズ
 * \par 説明:
 * ライブラリを初期化します。<br>
 * ライブラリの機能を利用するには、必ずこの関数を実行する必要があります。<br>
 * （ライブラリの機能は、本関数を実行後、 ::criAtomEx_Finalize_IOS 関数を実行するまでの間、
 * 利用可能です。）<br>
 * <br>
 * ライブラリを初期化する際には、ライブラリが内部で利用するためのメモリ領域（ワーク領域）
 * を確保する必要があります。<br>
 * ライブラリが必要とするワーク領域のサイズは、初期化用コンフィグ構造体の内容に応じて
 * 変化します。<br>
 * ワーク領域サイズの計算には、 ::criAtomEx_CalculateWorkSize_IOS 
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
 * 	.
 * 本関数を実行する場合、上記関数を実行しないでください。<br>
 * <br>
 * 本関数を実行後、必ず対になる ::criAtomEx_Finalize_IOS 関数を実行してください。<br>
 * また、 ::criAtomEx_Finalize_IOS 関数を実行するまでは、本関数を再度実行しないでください。<br>
 * \sa CriAtomExConfig_IOS, criAtomEx_Finalize_IOS,
 * criAtomEx_SetUserAllocator, criAtomEx_CalculateWorkSize_IOS
 */
void CRIAPI criAtomEx_Initialize_IOS(
	const CriAtomExConfig_IOS *config, void *work, CriSint32 work_size);

/*JP
 * \brief ライブラリの終了
 * \ingroup ATOMLIB_IOS
 * \par 説明:
 * ライブラリを終了します。<br>
 * \attention
 * 本関数は内部的に以下の関数を実行します。<br>
 * 	- ::criAtomEx_Finalize
 * 	- ::criAtomExAsr_Finalize
 * 	- ::criAtomExHcaMx_Finalize
 * 	.
 * 本関数を実行する場合、上記関数を実行しないでください。<br>
 * <br>
 * ::criAtomEx_Initialize_IOS 関数実行前に本関数を実行することはできません。<br>
 * \sa criAtomEx_Initialize_IOS
 */
void CRIAPI criAtomEx_Finalize_IOS(void);

/*JP
 * \brief サーバスレッドプライオリティの設定
 * \ingroup ATOMLIB_IOS
 * \param[in]	prio	スレッドのプライオリティ
 * \par 説明:
 * CRIサーバスレッドのプライオリティを設定します。<br>
 * 引数 prio は pthread のプライオリティ設定値として使用します。<br>
 * プライオリティ設定値はメインスレッドからの相対値になります。<br>
 * アプリケーションのメインスレッド(0)よりも高いプライオリティを指定してください。<br>
 * プライオリティのデフォルト値は16です。<br>
 * \attention
 * ::criAtomEx_Initialize_IOS 関数実行前に本関数を実行することはできません。<br>
 * <br>
 * サーバ処理スレッドは、CRI File Systemライブラリでも利用されています。<br>
 * すでにCRI File SystemライブラリのAPIでサーバ処理スレッドの設定を変更している場合
 * 本関数により設定が上書きされますのでご注意ください。<br>
 */
void CRIAPI criAtomEx_SetServerThreadPriority_IOS(int prio);

/*JP
 * \brief サウンド処理の再開
 * \ingroup ATOMLIB_IOS
 * \par 説明:
 * AudioSessionのInterruption Callbak関数から呼び出すための関数です。<br>
 * サウンド処理を再開します。<br>
 * 本関数を呼び出す前に、AudioSessionのパメラータ設定とアクティベイトを行ってください。<br>
 * \par 例:
 * \code
 * // AudioSession Interruption Callbak
 * static void interruptionListenerCallback(void *inUserData, UInt32 interruptionState)
 * {
 * 	if (interruptionState == kAudioSessionBeginInterruption) {
 * 		// オーディオ処理の停止
 * 		criAtomEx_StopSound_IOS();
 * 	}
 * 	if (interruptionState == kAudioSessionEndInterruption) {
 * 		// AudioSessionのプロパティ設定とアクティベイト
 * 		setupAudioSession();
 * 		// オーディオ処理の開始
 * 		criAtomEx_StartSound_IOS();
 * 	}
 * }
 * // AudioSessionのプロパティ設定とアクティベイト
 * static void setupAudioSession(void)
 * {
 * 		:
 * }
 * \endcode
 * \attention
 * ::criAtomEx_Initialize_IOS 関数実行前に本関数を実行することはできません。<br>
 * \sa criAtomEx_StopSound_IOS
 */
void CRIAPI criAtomEx_StartSound_IOS(void);

/*JP
 * \brief サウンド処理の停止
 * \ingroup ATOMLIB_IOS
 * \par 説明:
 * AudioSessionのInterruption Callbak関数から呼び出すための関数です。<br>
 * サウンド処理を停止します。<br>
 * \attention
 * ::criAtomEx_Initialize_IOS 関数実行前に本関数を実行することはできません。<br>
 * \sa criAtomEx_StartSound_IOS
 */
void CRIAPI criAtomEx_StopSound_IOS(void);

/*JP
 * \brief サウンドの復旧
 * \ingroup ATOMLIB_IOS
 * \par 説明:
 * AudioSessionAddPropertyListener Callback関数から呼び出すための関数です。<br>
 * ライブラリ内部のボイスを復旧します。<br>
 * iOSのデーモンであるmediaserverdが死亡した際には、ライブラリ内のボイスが無効なボイスになり、
 * 再生成が必要になります。<br>
 * このように、ボイスの復旧が必要な際に呼び出してください。<br>
 */
void CRIAPI criAtomEx_RecoverSound_IOS(void);

/*JP
 * \brief サウンドの初期化に成功したか否か
 * \ingroup ATOMLIB_IOS
 * \par 説明:
 * サウンドライブラリの初期化に成功したか否かを返す関数です。<br>
 * iOSでは、アプリがバックグラウンドにある状態でサウンドライブラリの初期化を行った場合に
 * 内部的にAudioSessionの初期化等に失敗している場合があります。<br>
 * 本関数で初期化が失敗していることを確認した場合は、アプリがフォアグラウンドにある状態で
 * 再度ライブラリの初期化を行うか、 ::criAtomEx_RecoverSound_IOS を用いて
 * サウンドの復旧を行う必要があります。<br>
 */
CriBool CRIAPI criAtomEx_IsInitializationSucceeded_IOS(void);

#ifdef __cplusplus
}
#endif

/***************************************************************************
 *      暫定API（使用はお控え下さい）
 *      Tentative API (Please don't use it)
 ***************************************************************************/
/* 以下に宣言されている関数はAtomの内部処理用、ゲームエンジンとの連携等にて
 * 暫定的に使用されているAPIです。
 * 今後のアップデートにて予告なく変更・削除が行われるため、使用はお控え下さい。
 */

/*JP
 * \brief バックグラウンド再生の開始
 * \ingroup ATOMLIB_IOS
 * \par 説明:
 * バックグラウンド再生の開始をAtomライブラリに通知します。
 */
void CRIAPI criAtomEx_EnableBackgroundPlayback_IOS(void);

/*JP
 * \brief バックグラウンド再生の終了
 * \ingroup ATOMLIB_IOS
 * \par 説明:
 * バックグラウンド再生の終了をAtomライブラリに通知します。
 */
void CRIAPI criAtomEx_DisableBackgroundPlayback_IOS(void);

/*JP
 * \brief 割り込みフラグの取得
 * \ingroup ATOMLIB_IOS
 * \par 説明:
 * 外部要因によって音声の割り込みが発生し、Atomライブラリの音声が停止している場合CRI_TRUEが返ります。<br>
 * CriAtomExConfig_IOS::use_handling_os_notificationsがCRI_TRUEで初期化されている場合、
 * Atomライブラリの音声を自動復帰します。自動復帰が完了するとCRI_FALSEが返ります。<br>
 * ::criAtomEx_EnableBackgroundPlayback_IOS でバックグラウンド再生を有効にしている場合、
 * 自動復帰は無効になります。音声を復帰させる場合は::criAtomEx_ResumeAudio_IOSを呼び出して音声を再開してください。<br>
 */
CriBool CRIAPI criAtomEx_IsInterruptedOtherAudio_IOS(void);

/*JP
 * \brief 音声の再開
 * \ingroup ATOMLIB_IOS
 * \par 説明:
 * バックグラウンド再生有効時に音声が停止した際、音声を復帰させます。
 * ::criAtomEx_IsInterruptedOtherAudio_IOS がCRI_TRUEを返すタイミングのみ効果があります。
 */
void CRIAPI criAtomEx_ResumeAudio_IOS(void);

/***************************************************************************
 *      旧バージョンとの互換用
 *      For compatibility with old version
 ***************************************************************************/
#define criAtom_SetDefaultConfig_iOS(p_config)		criAtom_SetDefaultConfig_IOS(p_config)
#define criAtomEx_SetDefaultConfig_iOS(p_config)	criAtomEx_SetDefaultConfig_IOS(p_config)
#define CriAtomConfig_iOS							CriAtomConfig_IOS
#define CriAtomExConfig_iOS							CriAtomExConfig_IOS
#define criAtom_CalculateWorkSize_iOS				criAtom_CalculateWorkSize_IOS
#define criAtom_Initialize_iOS						criAtom_Initialize_IOS
#define criAtom_Finalize_iOS						criAtom_Finalize_IOS
#define criAtom_SetServerThreadPriority_iOS			criAtom_SetServerThreadPriority_IOS
#define criAtom_StartSound_iOS						criAtom_StartSound_IOS
#define criAtom_StopSound_iOS						criAtom_StopSound_IOS
#define criAtomEx_CalculateWorkSize_iOS				criAtomEx_CalculateWorkSize_IOS
#define criAtomEx_Initialize_iOS					criAtomEx_Initialize_IOS
#define criAtomEx_Finalize_iOS						criAtomEx_Finalize_IOS
#define criAtomEx_SetServerThreadPriority_iOS		criAtomEx_SetServerThreadPriority_IOS
#define criAtomEx_StartSound_iOS					criAtomEx_StartSound_IOS
#define criAtomEx_StopSound_iOS						criAtomEx_StopSound_IOS


#endif	/* CRI_INCL_CRI_ATOM_IOS_H */

/* --- end of file --- */
