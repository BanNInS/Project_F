/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2011 CRI Middleware Co., Ltd.
 *
 * Library  : CRI File System
 * Module   : Library User's Header for Android
 * File     : cri_file_system_android.h
 *
 ****************************************************************************/
/*!
 *	\file		cri_file_system_android.h
 */

/* 多重定義防止					*/
/* Prevention of redefinition	*/
#ifndef	CRI_FILE_SYSTEM_ANDROID_H_INCLUDED
#define	CRI_FILE_SYSTEM_ANDROID_H_INCLUDED

/***************************************************************************
 *      インクルードファイル
 *      Include files
 ***************************************************************************/
#include "cri_le_xpt.h"
#include "cri_le_error.h"
#include "cri_le_file_system.h"
#include <jni.h>

/***************************************************************************
 *      定数マクロ
 *      Macro Constants
 ***************************************************************************/

/***************************************************************************
 *      処理マクロ
 *      Macro Functions
 ***************************************************************************/

/***************************************************************************
 *      データ型宣言
 *      Data Type Declarations
 ***************************************************************************/

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








/* 
 * 本関数は旧仕様の関数で、互換性のため残してあります。
 * criFs_EnableAssetsAccess_ANDROIDの方をお使い下さい。
 */

/*JP
 * \brief Androidプロジェクト内のassetsフォルダーに対するアクセスの有効化
 * \ingroup FSLIB_CRIFS_ANDROID
 * \param[in]	vm		JavaVMオブジェクトへの参照
 * \param[in]	jobj	android.content.Contextオブジェクト
 * \return	CriError	エラーコード
 * \par 説明:
 * assetフォルダーへのアクセスを有効化します。<br>
 * アクセスが終了した場合、 ::criFs_DisableAssetsAccess_ANDROID関数 を呼び出してください。
 * \attention:
 * 本関数を呼び出す前に、CRI File Systemライブラリの初期化が完了済みである事を確認して下さい。<br>
 */
CriError CRIAPI criFs_EnableAssetsAccess_ANDROID(JavaVM* vm, jobject jobj);

/*JP
 * \brief Androidプロジェクト内のassetsフォルダーに対するアクセスの無効化
 * \ingroup FSLIB_CRIFS_ANDROID
 * \return	CriError	エラーコード
 * \par 説明:
 * assetフォルダーへのアクセスを無効化します。<br>
 */
CriError CRIAPI criFs_DisableAssetsAccess_ANDROID();


#ifdef __cplusplus
}
#endif

#endif	/* CRI_FILE_SYSTEM_ANDROID_H_INCLUDED */

/***************************************************************************
 *      旧バージョンとの互換用
 *      For compatibility with old version
 ***************************************************************************/








/* --- end of file --- */
